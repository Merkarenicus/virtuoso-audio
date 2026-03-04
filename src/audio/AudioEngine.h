// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// AudioEngine.h
// Central audio engine coordinator.
// Owns the entire real-time DSP chain:
//   VirtualDeviceCapture → ChannelLayoutMapper → UpmixProcessor →
//   [ResamplerProcessor] → ConvolverProcessor + BassManagement → EqProcessor →
//   LimiterProcessor → output device
//
// Threading model:
//   - Audio render callback: processes one block per audio device interrupt
//   - UI thread: posts AudioCommands via AudioCommandQueue (lock-free)
//   - Watchdog thread: monitors device hot-plug / sample rate changes

#pragma once

#include "AudioCommandQueue.h"
#include "BassManagement.h"
#include "ChannelLayoutMapper.h"
#include "ConvolverProcessor.h"
#include "DeviceWatchdog.h"
#include "EqProcessor.h"
#include "LimiterProcessor.h"
#include "ResamplerProcessor.h"
#include "UpmixProcessor.h"
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>


namespace virtuoso {

struct EngineMetrics {
  std::atomic<float> cpuLoadPercent{0.0f};
  std::atomic<float> latencyMs{0.0f};
  std::atomic<float> limiterGainReductionDb{0.0f};
  std::atomic<int> droppedBlocks{0};
};

class AudioEngine final : public juce::AudioIODeviceCallback {
public:
  AudioEngine();
  ~AudioEngine() override;

  // Not copyable
  AudioEngine(const AudioEngine &) = delete;
  AudioEngine &operator=(const AudioEngine &) = delete;

  // ---------------------------------------------------------------------------
  // Lifecycle
  // ---------------------------------------------------------------------------
  bool start(const juce::String &outputDeviceId = {});
  void stop();

  // Post a command from any thread (UI, watchdog, etc.)
  bool postCommand(AudioCommand cmd) noexcept {
    return m_commandQueue.postCommand(std::move(cmd));
  }

  // ---------------------------------------------------------------------------
  // juce::AudioIODeviceCallback
  // ---------------------------------------------------------------------------
  void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
  void audioDeviceStopped() override;
  void audioDeviceIOCallbackWithContext(
      const float *const *inputChannelData, int numInputChannels,
      float *const *outputChannelData, int numOutputChannels, int numSamples,
      const juce::AudioIODeviceCallbackContext &context) override;

  // ---------------------------------------------------------------------------
  // Status / Metrics
  // ---------------------------------------------------------------------------
  const EngineMetrics &metrics() const noexcept { return m_metrics; }
  bool isRunning() const noexcept {
    return m_running.load(std::memory_order_relaxed);
  }
  double getSampleRate() const noexcept { return m_sampleRate; }
  int getBlockSize() const noexcept { return m_blockSize; }

  // Passthrough mode: bypass DSP and pass audio directly (used when HRIR not
  // loaded)
  void setPassthrough(bool on) noexcept {
    m_passthrough.store(on, std::memory_order_relaxed);
  }

private:
  // DSP chain
  ConvolverProcessor m_convolver;
  UpmixProcessor m_upmixer;
  BassManagement m_bassManagement;
  EqProcessor m_eq;
  ResamplerProcessor m_resampler;
  LimiterProcessor m_limiter;
  ChannelLayoutMapper m_layoutMapper;

  // Command queue
  AudioCommandQueue m_commandQueue;

  // JUCE device manager
  juce::AudioDeviceManager m_deviceManager;

  // Working buffers (allocated in audioDeviceAboutToStart)
  juce::AudioBuffer<float> m_canonical8ch; // Post-mapping 8-channel buffer
  juce::AudioBuffer<float> m_convolverOut; // Post-convolution stereo
  juce::AudioBuffer<float> m_lfeOut;       // Bass management mono output

  // State
  double m_sampleRate{48000.0};
  int m_blockSize{256};
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_passthrough{true}; // True until HRIR loaded
  std::atomic<bool> m_enabled{true};

  // Metrics
  EngineMetrics m_metrics;

  // Watchdog
  std::unique_ptr<DeviceWatchdog> m_watchdog;

  // Process one block on the audio thread (called from audioDeviceIOCallback)
  void processBlock(const float *const *inputData, int numInputCh,
                    float *const *outputData, int numOutputCh,
                    int numSamples) noexcept;

  // Drain and apply commands from the command queue
  void handleCommands() noexcept;
};

} // namespace virtuoso
