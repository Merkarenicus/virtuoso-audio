// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// PipeWireCapture.h — Linux PipeWire capture from Virtuoso virtual sink

#pragma once
#include "../VirtualDeviceCapture.h"
#include <atomic>
#include <string>
#include <thread>


#if defined(__linux__)
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#endif

namespace virtuoso {

class PipeWireCapture final : public VirtualDeviceCapture {
public:
  PipeWireCapture();
  ~PipeWireCapture() override { stop(); }

  bool start(const std::string &deviceName, CaptureCallback callback) override;
  void stop() override;
  bool isRunning() const noexcept override { return m_running.load(); }
  double getSampleRate() const noexcept override { return m_sampleRate; }
  int getNumChannels() const noexcept override { return 8; }

private:
#if defined(__linux__)
  pw_main_loop *m_loop{nullptr};
  pw_stream *m_stream{nullptr};

  static void onProcess(void *userdata);
  static const pw_stream_events kStreamEvents;
#endif

  std::atomic<bool> m_running{false};
  std::thread m_pwThread;
  CaptureCallback m_callback;
  double m_sampleRate{48000.0};
  int m_numChannels{8};
};

} // namespace virtuoso
