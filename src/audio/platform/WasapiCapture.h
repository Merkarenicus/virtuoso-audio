// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// WasapiCapture.h — Windows WASAPI loopback capture from Virtuoso virtual
// device

#pragma once
#include "../VirtualDeviceCapture.h"
#include <atomic>
#include <string>
#include <thread>


#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>

#endif

namespace virtuoso {

class WasapiCapture final : public VirtualDeviceCapture {
public:
  WasapiCapture() = default;
  ~WasapiCapture() override { stop(); }

  bool start(const std::string &deviceName, CaptureCallback callback) override;
  void stop() override;
  bool isRunning() const noexcept override { return m_running.load(); }
  double getSampleRate() const noexcept override { return m_sampleRate; }
  int getNumChannels() const noexcept override { return m_numChannels; }

private:
#if defined(_WIN32)
  IMMDeviceEnumerator *m_enumerator{nullptr};
  IMMDevice *m_device{nullptr};
  IAudioClient *m_audioClient{nullptr};
  IAudioCaptureClient *m_captureClient{nullptr};
#endif

  std::atomic<bool> m_running{false};
  std::thread m_captureThread;
  CaptureCallback m_callback;
  double m_sampleRate{48000.0};
  int m_numChannels{8};

  void captureLoop();
  bool findVirtuosoDevice(const std::string &preferredName);
};

} // namespace virtuoso
