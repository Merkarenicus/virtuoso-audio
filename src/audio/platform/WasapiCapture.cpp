// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// WasapiCapture.cpp — Windows WASAPI loopback capture

#include "WasapiCapture.h"

#if defined(_WIN32)
#include <Functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <combaseapi.h>

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "ole32.lib")

namespace virtuoso {

bool WasapiCapture::start(const std::string &deviceName,
                          CaptureCallback callback) {
  if (m_running.load())
    return true;
  m_callback = std::move(callback);

  CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  HRESULT hr =
      CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                       __uuidof(IMMDeviceEnumerator), (void **)&m_enumerator);
  if (FAILED(hr))
    return false;

  if (!findVirtuosoDevice(deviceName)) {
    // Fall back to default render device (loopback)
    hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    if (FAILED(hr))
      return false;
  }

  hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                          (void **)&m_audioClient);
  if (FAILED(hr))
    return false;

  WAVEFORMATEX *fmt = nullptr;
  m_audioClient->GetMixFormat(&fmt);
  if (fmt) {
    m_sampleRate = static_cast<double>(fmt->nSamplesPerSec);
    m_numChannels = 8; // We always output 8 channels from our virtual device
    CoTaskMemFree(fmt);
  }

  // Initialize for loopback capture (AUDCLNT_STREAMFLAGS_LOOPBACK)
  hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK,
                                 10000000, // 1 second buffer
                                 0, fmt, nullptr);
  if (FAILED(hr))
    return false;

  hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient),
                                 (void **)&m_captureClient);
  if (FAILED(hr))
    return false;

  m_audioClient->Start();
  m_running.store(true, std::memory_order_release);
  m_captureThread = std::thread([this] { captureLoop(); });
  return true;
}

void WasapiCapture::stop() {
  m_running.store(false, std::memory_order_release);
  if (m_captureThread.joinable())
    m_captureThread.join();

  if (m_audioClient) {
    m_audioClient->Stop();
    m_audioClient->Release();
    m_audioClient = nullptr;
  }
  if (m_captureClient) {
    m_captureClient->Release();
    m_captureClient = nullptr;
  }
  if (m_device) {
    m_device->Release();
    m_device = nullptr;
  }
  if (m_enumerator) {
    m_enumerator->Release();
    m_enumerator = nullptr;
  }
  CoUninitialize();
}

void WasapiCapture::captureLoop() {
  // Set thread priority for audio (MMCSS)
  DWORD taskIndex = 0;
  HANDLE mmcssHandle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

  const int kBlockSize = 512;
  juce::AudioBuffer<float> buf(m_numChannels, kBlockSize);

  while (m_running.load(std::memory_order_acquire)) {
    UINT32 packetSize = 0;
    if (FAILED(m_captureClient->GetNextPacketSize(&packetSize)))
      break;
    if (packetSize == 0) {
      Sleep(1);
      continue;
    }

    BYTE *data = nullptr;
    UINT32 frames = 0;
    DWORD flags = 0;
    if (FAILED(m_captureClient->GetBuffer(&data, &frames, &flags, nullptr,
                                          nullptr)))
      break;

    if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && data && frames > 0) {
      const int n = std::min(static_cast<int>(frames), kBlockSize);
      buf.clear();
      // WASAPI delivers interleaved float (assuming WAVEFORMATEXTENSIBLE float)
      const float *src = reinterpret_cast<const float *>(data);
      for (int i = 0; i < n; ++i)
        for (int ch = 0; ch < m_numChannels; ++ch)
          buf.getWritePointer(ch)[i] = src[i * m_numChannels + ch];

      if (m_callback)
        m_callback(buf, m_sampleRate);
    }

    m_captureClient->ReleaseBuffer(frames);
  }

  if (mmcssHandle)
    AvRevertMmThreadCharacteristics(mmcssHandle);
}

bool WasapiCapture::findVirtuosoDevice(const std::string &preferredName) {
  if (preferredName.empty())
    return false;

  IMMDeviceCollection *collection = nullptr;
  if (FAILED(m_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE,
                                              &collection)))
    return false;

  UINT count = 0;
  collection->GetCount(&count);

  for (UINT i = 0; i < count; ++i) {
    IMMDevice *dev = nullptr;
    if (FAILED(collection->Item(i, &dev)))
      continue;

    IPropertyStore *props = nullptr;
    if (FAILED(dev->OpenPropertyStore(STGM_READ, &props))) {
      dev->Release();
      continue;
    }

    PROPVARIANT pv;
    PropVariantInit(&pv);
    props->GetValue(PKEY_Device_FriendlyName, &pv);

    if (pv.vt == VT_LPWSTR) {
      char narrowName[256] = {};
      WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, narrowName, 255, nullptr,
                          nullptr);
      if (std::string(narrowName).find(preferredName) != std::string::npos) {
        m_device = dev;
        PropVariantClear(&pv);
        props->Release();
        collection->Release();
        return true;
      }
    }
    PropVariantClear(&pv);
    props->Release();
    dev->Release();
  }
  collection->Release();
  return false;
}

} // namespace virtuoso
#endif // _WIN32
