// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// VirtuosoWaveRTStream.cpp — IMiniportWaveRTStream + ring buffer for
// VirtuosoVAD
//
// This file implements the WaveRT stream object returned by
// VirtuosoMiniportWaveRT::NewStream(). The ring buffer is shared between:
//   - The HAL (kernel): writes audio data from applications playing to
//   "Virtuoso Virtual 7.1"
//   - WasapiCapture.cpp (user mode): reads via WaveRT loopback
//
// Architecture:
//   AllocatePagesForMdl → MmMapLockedPagesSpecifyCache (WC) → ring buffer
//   Applications write → HAL copies into ring → our user-mode capture reads out
//
// Channel format: 8-channel IEEE Float32, 48 kHz, block-aligned to system page
// size

extern "C" {
#include <ntddk.h>
}
#include <ksdebug.h>
#include <portcls.h>
#include <stdunk.h>


static constexpr ULONG kRingBufferSizeBytes =
    65536; // 64 KB — ~170 ms at 48 kHz Float32 8ch
static constexpr ULONG kChannelCount = 8;
static constexpr ULONG kSampleRate = 48000;
static constexpr ULONG kBytesPerSample = 4; // float32
static constexpr ULONG kFrameSizeBytes = kChannelCount * kBytesPerSample;

class VirtuosoWaveRTStream : public IMiniportWaveRTStream, public CUnknown {
public:
  DECLARE_STD_UNKNOWN();
  DEFINE_STD_CONSTRUCTOR(VirtuosoWaveRTStream);

  ~VirtuosoWaveRTStream() override {
    FreeRingBuffer();
    DPF(D_TERSE, ("[VirtuosoStream] Destroyed"));
  }

  // ---------------------------------------------------------------------------
  // IMiniportWaveRTStream interface
  // ---------------------------------------------------------------------------

  // AllocateAudioBuffer — kernel allocates the ring buffer and maps it to
  // user-mode
  STDMETHODIMP AllocateAudioBuffer(ULONG requestedSize, PMDL *mdl, ULONG *size,
                                   ULONG *offset,
                                   MEMORY_CACHING_TYPE *cachingType) override;

  // FreeAudioBuffer — kernel releases the ring buffer
  STDMETHODIMP_(void) FreeAudioBuffer(PMDL mdl, ULONG size) override;

  // GetHWLatency — report hardware latency contribution
  STDMETHODIMP GetHWLatency(KSRTAUDIO_HWLATENCY *latency) override;

  // GetPositionRegister — expose atomic position register to user-mode
  STDMETHODIMP GetPositionRegister(KSRTAUDIO_POSITIONREGISTER *reg) override;

  // GetClockRegister — expose hardware clock register to user-mode (optional)
  STDMETHODIMP GetClockRegister(KSRTAUDIO_CLOCKREGISTER *reg) override;

  // SetFormat — validate and accept the requested wave format
  STDMETHODIMP SetFormat(PKSDATAFORMAT format) override;

  // SetState — start / pause / stop the stream
  STDMETHODIMP SetState(KSSTATE state) override;

  // GetPosition — return current write position in ring buffer
  STDMETHODIMP_(NTSTATUS) GetPosition(KSAUDIO_POSITION *position) override;

private:
  PMDL m_mdl{nullptr};         // MDL for ring buffer memory
  PVOID m_ringBuffer{nullptr}; // Kernel VA of ring buffer
  PVOID m_userVA{nullptr};     // User-mode VA mapped by the port driver
  ULONG m_bufferSize{0};
  KSSTATE m_state{KSSTATE_STOP};

  // Position tracking (atomic for safe user-mode access)
  ULONG volatile m_writePos{0}; // Bytes written by HAL

  void FreeRingBuffer();
};

// ---------------------------------------------------------------------------
// AllocateAudioBuffer — allocates contiguous physical pages for WaveRT ring
// ---------------------------------------------------------------------------
STDMETHODIMP
VirtuosoWaveRTStream::AllocateAudioBuffer(ULONG requestedSize, PMDL *outMdl,
                                          ULONG *outSize, ULONG *outOffset,
                                          MEMORY_CACHING_TYPE *outCachingType) {
  PAGED_CODE();

  // Align up to system page size
  const ULONG pageSize = PAGE_SIZE;
  const ULONG alignedSz =
      ((kRingBufferSizeBytes + pageSize - 1) / pageSize) * pageSize;

  m_mdl =
      MmAllocatePagesForMdl({0LL}, {(LONGLONG)0x7FFFFFFF}, {0LL}, alignedSz);

  if (!m_mdl) {
    DPF(D_ERROR, ("[VirtuosoStream] MmAllocatePagesForMdl failed"));
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  m_ringBuffer = MmMapLockedPagesSpecifyCache(
      m_mdl, KernelMode, MmWriteCombined, nullptr, FALSE, NormalPagePriority);

  if (!m_ringBuffer) {
    MmFreePagesFromMdl(m_mdl);
    ExFreePool(m_mdl);
    m_mdl = nullptr;
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(m_ringBuffer, alignedSz);
  m_bufferSize = alignedSz;

  *outMdl = m_mdl;
  *outSize = alignedSz;
  *outOffset = 0;
  *outCachingType = MmWriteCombined;

  DPF(D_TERSE,
      ("[VirtuosoStream] Ring buffer allocated: %lu bytes", alignedSz));
  return STATUS_SUCCESS;
}

STDMETHODIMP_(void)
VirtuosoWaveRTStream::FreeAudioBuffer(PMDL mdl, ULONG size) {
  PAGED_CODE();
  UNREFERENCED_PARAMETER(size);
  if (m_ringBuffer) {
    MmUnmapLockedPages(m_ringBuffer, mdl);
    m_ringBuffer = nullptr;
  }
  if (mdl) {
    MmFreePagesFromMdl(mdl);
    ExFreePool(mdl);
  }
  if (m_mdl == mdl)
    m_mdl = nullptr;
  m_bufferSize = 0;
}

void VirtuosoWaveRTStream::FreeRingBuffer() {
  if (m_mdl)
    FreeAudioBuffer(m_mdl, m_bufferSize);
}

STDMETHODIMP VirtuosoWaveRTStream::GetHWLatency(KSRTAUDIO_HWLATENCY *latency) {
  PAGED_CODE();
  if (!latency)
    return STATUS_INVALID_PARAMETER;
  latency->FifoSize = kFrameSizeBytes; // One frame FIFO
  latency->ChipsetDelay = 0;
  latency->CodecDelay = 0;
  return STATUS_SUCCESS;
}

STDMETHODIMP
VirtuosoWaveRTStream::GetPositionRegister(KSRTAUDIO_POSITIONREGISTER *reg) {
  PAGED_CODE();
  if (!reg)
    return STATUS_INVALID_PARAMETER;
  // Expose the write-position variable as an MMIO register visible to user-mode
  reg->Register = const_cast<ULONG *>(&m_writePos);
  reg->Numerator = 1;
  reg->Denominator = 1;
  return STATUS_SUCCESS;
}

STDMETHODIMP
VirtuosoWaveRTStream::GetClockRegister(KSRTAUDIO_CLOCKREGISTER *reg) {
  return STATUS_NOT_IMPLEMENTED; // No hardware clock — use QPC
}

STDMETHODIMP VirtuosoWaveRTStream::SetFormat(PKSDATAFORMAT format) {
  PAGED_CODE();
  if (!format)
    return STATUS_INVALID_PARAMETER;

  // Accept 8-channel Float32 or PCM32 at 8–192 kHz
  auto *wfex = reinterpret_cast<PKSDATAFORMAT_WAVEFORMATEX>(format);
  const WAVEFORMATEX &wf = wfex->WaveFormatEx;

  if (wf.nChannels == kChannelCount && (wf.wBitsPerSample == 32) &&
      (wf.nSamplesPerSec >= 8000 && wf.nSamplesPerSec <= 192000))
    return STATUS_SUCCESS;

  return STATUS_NOT_SUPPORTED;
}

STDMETHODIMP VirtuosoWaveRTStream::SetState(KSSTATE state) {
  PAGED_CODE();
  DPF(D_TERSE, ("[VirtuosoStream] SetState: %d", (int)state));
  m_state = state;
  if (state == KSSTATE_STOP) {
    InterlockedExchange(reinterpret_cast<volatile LONG *>(&m_writePos), 0);
    if (m_ringBuffer)
      RtlZeroMemory(m_ringBuffer, m_bufferSize);
  }
  return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtuosoWaveRTStream::GetPosition(KSAUDIO_POSITION *position) {
  if (!position)
    return STATUS_INVALID_PARAMETER;
  position->PlayOffset = m_writePos; // For render: bytes played by hardware
  position->WriteOffset = m_writePos;
  return STATUS_SUCCESS;
}
