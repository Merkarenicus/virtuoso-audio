// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// CoreAudioCapture.h — macOS CoreAudio capture from Virtuoso AudioServerPlugIn

#pragma once
#include "../VirtualDeviceCapture.h"
#include <atomic>
#include <thread>

#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

#endif

namespace virtuoso {

class CoreAudioCapture final : public VirtualDeviceCapture {
public:
  CoreAudioCapture() = default;
  ~CoreAudioCapture() override { stop(); }

  bool start(const std::string &deviceName, CaptureCallback callback) override;
  void stop() override;
  bool isRunning() const noexcept override { return m_running.load(); }
  double getSampleRate() const noexcept override { return m_sampleRate; }
  int getNumChannels() const noexcept override { return m_numChannels; }

private:
#if defined(__APPLE__)
  AudioDeviceID m_deviceID{kAudioObjectUnknown};
  AudioDeviceIOProcID m_ioProcID{nullptr};
#endif

  std::atomic<bool> m_running{false};
  CaptureCallback m_callback;
  double m_sampleRate{48000.0};
  int m_numChannels{8};

#if defined(__APPLE__)
  static OSStatus audioIOProc(AudioDeviceID, const AudioTimeStamp *,
                              const AudioBufferList *inData,
                              const AudioTimeStamp *, AudioBufferList *,
                              const AudioTimeStamp *, void *refCon);
  AudioDeviceID findVirtuosoDevice(const std::string &name) const;
#endif
};

} // namespace virtuoso
