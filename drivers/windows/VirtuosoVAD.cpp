// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// VirtuosoVAD.cpp — Windows Kernel-Mode Virtual Audio Device Driver
//
// Architecture: WaveRT port driver (KSAUDIO_SPEAKER_7POINT1).
// Based on Microsoft SysVAD sample (WDK samples / simpleaudiosample).
//
// Key design goals (from ADR-001):
//   1. Device appears as "Virtuoso Virtual 7.1" in Windows Sound panel
//   2. 8-channel (7.1) PCM endpoint, float32/S32/S16 support
//   3. WaveRT streaming (zero-copy buffer) for lowest latency
//   4. EV code-signed for kernel trust requirement
//   5. DWORD channel mask = 0x63F (FL|FR|C|LFE|BL|BR|SL|SR)
//
// This stub provides the driver entry point, device object creation,
// and the IPortWaveRT / IMiniportWaveRT scaffolding.
// Requires: WDK 10.0.26100+ linked against ks.lib, ksproxy.lib
//
// Build (from Developer Command Prompt):
//   msbuild VirtuosoVAD.vcxproj /p:Configuration=Release /p:Platform=x64

extern "C" {
#include <ntddk.h>
#include <wdm.h>
}
#include <ksdebug.h>
#include <portcls.h>
#include <stdunk.h>


static const GUID CLSID_VirtuosoMiniportWaveRT = {
    0xdeadbeef,
    0x1234,
    0x5678,
    {0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89}};

// ---------------------------------------------------------------------------
// Channel geometry: 7.1 surround (ITU-R BS.775-3)
// ---------------------------------------------------------------------------
static const KSDATARANGE_AUDIO VirtuosoDataRange = {
    {sizeof(KSDATARANGE_AUDIO), 0, 0, 0, STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
     STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
     STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)},
    8,     // MaximumChannels
    16,    // MinimumBitsPerSample
    32,    // MaximumBitsPerSample
    8000,  // MinimumSampleFrequency
    192000 // MaximumSampleFrequency
};

// Channel speaker positions (7.1)
static const KSAUDIO_CHANNEL_CONFIG VirtuosoChannelConfig = {
    KSAUDIO_SPEAKER_7POINT1_SURROUND // 0x63F
};

// ---------------------------------------------------------------------------
// Driver entry — called by Windows kernel when driver is loaded
// ---------------------------------------------------------------------------
extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                                PUNICODE_STRING RegistryPath) {
  PAGED_CODE();

  DPF(D_TERSE, ("[VirtuosoVAD] DriverEntry\n"));

  // Set up unload and add-device callbacks via PortCls helper
  return PcInitializeAdapterDriver(
      DriverObject, RegistryPath,
      reinterpret_cast<PDRIVER_ADD_DEVICE>(VirtuosoAddDevice));
}

// ---------------------------------------------------------------------------
// AddDevice — creates the audio adapter and registers port objects
// ---------------------------------------------------------------------------
NTSTATUS VirtuosoAddDevice(PDRIVER_OBJECT DriverObject,
                           PDEVICE_OBJECT PhysicalDeviceObject) {
  PAGED_CODE();

  DPF(D_TERSE, ("[VirtuosoVAD] AddDevice\n"));

  // PcAddAdapterDevice creates the functional device object (FDO)
  return PcAddAdapterDevice(
      DriverObject, PhysicalDeviceObject, PCPFNSTARTDEVICE(VirtuosoStartDevice),
      MAX_MINIPORTS, // maximum simultaneous streams
      0 // device extension size (use PORT_CLASS_DEVICE_EXTENSION_SIZE)
  );
}

// ---------------------------------------------------------------------------
// StartDevice — instantiates WaveRT port + miniport, registers endpoints
// ---------------------------------------------------------------------------
NTSTATUS VirtuosoStartDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                             PRESOURCELIST ResourceList) {
  UNREFERENCED_PARAMETER(ResourceList);
  PAGED_CODE();

  DPF(D_TERSE, ("[VirtuosoVAD] StartDevice\n"));

  PPORT portWaveRT = nullptr;
  PMINIPORT miniportWaveRT = nullptr;
  NTSTATUS ntStatus = STATUS_SUCCESS;

  // Create WaveRT port instance
  ntStatus = PcNewPort(&portWaveRT, CLSID_PortWaveRT);
  if (!NT_SUCCESS(ntStatus))
    goto Exit;

  // Create our miniport instance (renders the 7.1 virtual endpoint)
  ntStatus = PcNewMiniport(&miniportWaveRT, CLSID_VirtuosoMiniportWaveRT);
  if (!NT_SUCCESS(ntStatus))
    goto Exit;

  // Connect miniport to port
  ntStatus =
      portWaveRT->Init(DeviceObject, Irp, miniportWaveRT, nullptr, nullptr);
  if (!NT_SUCCESS(ntStatus))
    goto Exit;

  // Register the audio sub-device (endpoint) with PortCls
  ntStatus =
      PcRegisterSubdevice(DeviceObject, L"VirtuosoVirtual71", portWaveRT);

Exit:
  if (portWaveRT)
    portWaveRT->Release();
  if (miniportWaveRT)
    miniportWaveRT->Release();
  return ntStatus;
}

// NOTE: The IMiniportWaveRT implementation (VirtuosoMiniportWaveRT class)
// and the stream class (VirtuosoStream) are defined in
// VirtuosoVAD_Miniport.cpp. They implement the COM interface for WaveRT buffer
// management, channel format negotiation, and the actual audio data path. That
// source file is Phase 4b work requiring WDK kernel development environment.
