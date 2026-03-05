// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// VirtuosoVAD_Miniport.cpp — IMiniportWaveRT implementation
// Implements the WaveRT miniport object that renders a virtual 7.1 endpoint.
// Companion to VirtuosoVAD.cpp (port driver entry + AddDevice + StartDevice
// scaffold).
//
// Key COM interfaces implemented:
//   - IMiniportWaveRT         : required for WaveRT
//   - IMiniportAudioSignalProcessing : required for Windows 8+ APO pipeline
//
// Data flow:
//   HW loopback → WaveRT ring buffer → user-mode capture (WasapiCapture.cpp)
//
// Build: requires WDK 10.0.26100+, KMDF 1.33+
//   msbuild VirtuosoVAD.vcxproj /p:Configuration=Release /p:Platform=x64

extern "C" {
#include <ntddk.h>
}
#include <ksdebug.h>
#include <portcls.h>
#include <stdunk.h>
#include <waveformattable.h>


// ---------------------------------------------------------------------------
// Supported wave formats (IEEE Float + PCM, 8 channels, up to 192 kHz)
// ---------------------------------------------------------------------------
static const KSDATARANGE_AUDIO SupportedFormats[] = {
    // IEEE Float 32-bit, 7.1 surround
    {{sizeof(KSDATARANGE_AUDIO), 0, 0, 0, STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)},
     8,
     32,
     32,
     22050,
     192000},
    // PCM 32-bit, 7.1 surround
    {{sizeof(KSDATARANGE_AUDIO), 0, 0, 0, STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)},
     8,
     16,
     32,
     22050,
     192000}};

static const ULONG kNumFormats = ARRAYSIZE(SupportedFormats);

// ---------------------------------------------------------------------------
// VirtuosoMiniportWaveRT — implements IMiniportWaveRT
// ---------------------------------------------------------------------------
class VirtuosoMiniportWaveRT : public IMiniportWaveRT,
                               public IMiniportAudioSignalProcessing,
                               public CUnknown {
public:
  DECLARE_STD_UNKNOWN();
  VirtuosoMiniportWaveRT(PUNKNOWN outer) : CUnknown(outer) {}

  // IUnknown — handled by CUnknown macro
  DEFINE_STD_CONSTRUCTOR(VirtuosoMiniportWaveRT);
  ~VirtuosoMiniportWaveRT() override {
    DPF(D_TERSE, ("[VirtuosoMiniport] Destroyed"));
  }

  // IMiniport
  STDMETHODIMP GetDescription(PPCFILTER_DESCRIPTOR *desc) override;
  STDMETHODIMP DataRangeIntersection(ULONG pindId, PKSDATARANGE clientRange,
                                     PKSDATARANGE myRange, ULONG outputLen,
                                     PVOID resultFormat,
                                     PULONG resultLen) override;

  // IMiniportWaveRT
  STDMETHODIMP Init(PUNKNOWN unknAdapterCommon,
                    PRESOURCELIST resourceList) override;
  STDMETHODIMP NewStream(PMINIPORTWAVERTSTREAM *outStream,
                         PPORTWAVERTSTREAM portStream, ULONG pin,
                         BOOLEAN capture, PKSDATAFORMAT dataFormat) override;
  STDMETHODIMP GetDeviceDescription(PDEVICE_DESCRIPTION devDesc) override;

  // IMiniportAudioSignalProcessing
  STDMETHODIMP GetMaxInputChannelCount(PULONG count) override {
    *count = 8;
    return STATUS_SUCCESS;
  }

private:
  PPORTWAVERT m_port{nullptr};
};

// ---------------------------------------------------------------------------
// GetDescription — return the filter descriptor exposing pins and nodes
// ---------------------------------------------------------------------------
STDMETHODIMP
VirtuosoMiniportWaveRT::GetDescription(PPCFILTER_DESCRIPTOR *desc) {
  // Phase 5b: return a real PCFILTER_DESCRIPTOR with render + capture pins
  // and the required DAC/ADC nodes for WaveRT topology
  *desc = nullptr; // Stub: will be filled with actual pin descriptors
  return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP VirtuosoMiniportWaveRT::DataRangeIntersection(
    ULONG pindId, PKSDATARANGE clientRange, PKSDATARANGE myRange,
    ULONG outputLen, PVOID resultFormat, PULONG resultLen) {
  UNREFERENCED_PARAMETER(pindId);
  // Simple intersection: check if client format is in our supported list
  for (ULONG i = 0; i < kNumFormats; ++i) {
    const auto &sr = SupportedFormats[i];
    if (IsEqualGUID(clientRange->SubFormat, sr.DataRange.SubFormat)) {
      if (resultLen)
        *resultLen = sizeof(KSDATARANGE_AUDIO);
      if (resultFormat && outputLen >= sizeof(KSDATARANGE_AUDIO))
        RtlCopyMemory(resultFormat, &sr, sizeof(KSDATARANGE_AUDIO));
      return STATUS_SUCCESS;
    }
  }
  return STATUS_NO_MATCH;
}

STDMETHODIMP VirtuosoMiniportWaveRT::Init(PUNKNOWN, PRESOURCELIST) {
  DPF(D_TERSE, ("[VirtuosoMiniport] Init"));
  return STATUS_SUCCESS;
}

STDMETHODIMP VirtuosoMiniportWaveRT::NewStream(PMINIPORTWAVERTSTREAM *outStream,
                                               PPORTWAVERTSTREAM portStream,
                                               ULONG pin, BOOLEAN capture,
                                               PKSDATAFORMAT dataFormat) {
  UNREFERENCED_PARAMETER(portStream);
  UNREFERENCED_PARAMETER(pin);
  UNREFERENCED_PARAMETER(capture);
  UNREFERENCED_PARAMETER(dataFormat);

  // Phase 5b: allocate VirtuosoWaveRTStream, create ring buffer, return via
  // outStream
  *outStream = nullptr;
  return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP
VirtuosoMiniportWaveRT::GetDeviceDescription(PDEVICE_DESCRIPTION devDesc) {
  RtlZeroMemory(devDesc, sizeof(DEVICE_DESCRIPTION));
  devDesc->Version = DEVICE_DESCRIPTION_VERSION1;
  devDesc->Master = TRUE;
  devDesc->ScatterGather = TRUE;
  devDesc->Dma32BitAddresses = TRUE;
  devDesc->MaximumLength = 0xFFFFFFFF;
  return STATUS_SUCCESS;
}

// ---------------------------------------------------------------------------
// Factory function called from VirtuosoVAD.cpp via PcNewMiniport
// ---------------------------------------------------------------------------
NTSTATUS CreateVirtuosoMiniportWaveRT(OUT PUNKNOWN *unknown, IN REFCLSID,
                                      IN PUNKNOWN outerUnknown,
                                      IN POOL_FLAGS poolFlags) {
  UNREFERENCED_PARAMETER(poolFlags);
  auto *miniport =
      new (POOL_FLAG_NON_PAGED, 'troV') VirtuosoMiniportWaveRT(outerUnknown);
  if (!miniport)
    return STATUS_INSUFFICIENT_RESOURCES;
  miniport->AddRef();
  *unknown = static_cast<IMiniportWaveRT *>(miniport);
  return STATUS_SUCCESS;
}
