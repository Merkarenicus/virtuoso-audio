// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// VirtualDeviceCapture.h
// Platform-agnostic interface for capturing audio from the Virtuoso virtual
// audio device. Concrete implementations: WasapiCapture (Win), CoreAudioCapture
// (Mac), PipeWireCapture (Linux).

#pragma once
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <string>


namespace virtuoso {

// Callback invoked on the capture thread for every block from the virtual
// device
using CaptureCallback = std::function<void(const juce::AudioBuffer<float> &,
                                           double /*sampleRate*/)>;

class VirtualDeviceCapture {
public:
  virtual ~VirtualDeviceCapture() = default;

  // Start capturing from the virtual device with the given name (empty =
  // auto-detect Virtuoso device)
  virtual bool start(const std::string &deviceName,
                     CaptureCallback callback) = 0;
  virtual void stop() = 0;
  virtual bool isRunning() const noexcept = 0;

  // Returns the actual sample rate the virtual device is operating at
  virtual double getSampleRate() const noexcept = 0;
  virtual int getNumChannels() const noexcept = 0; // Should be 8 for 7.1

  // Factory: create the appropriate platform capture object
  static std::unique_ptr<VirtualDeviceCapture> create();
};

} // namespace virtuoso
