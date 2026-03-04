// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// AudioEngine.cpp

#include "AudioEngine.h"
#include "../logging/Logger.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace virtuoso {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() { stop(); }

bool AudioEngine::start(const juce::String &outputDeviceId) {
  if (m_running.load())
    return true;

  VLOG_INFO("AudioEngine", "Starting audio engine");

  // Configure device manager
  juce::String error = m_deviceManager.initialise(0, 2, nullptr, true);
  if (error.isNotEmpty()) {
    VLOG_ERROR("AudioEngine", "Device init failed: " + error.toStdString());
    return false;
  }

  // Set output device if specified
  if (outputDeviceId.isNotEmpty()) {
    auto setup = m_deviceManager.getAudioDeviceSetup();
    setup.outputDeviceName = outputDeviceId;
    m_deviceManager.setAudioDeviceSetup(setup, true);
  }

  m_deviceManager.addAudioCallback(this);

  // Start watchdog
  m_watchdog = std::make_unique<DeviceWatchdog>();
  m_watchdog->start(
      [this](WatchdogEvent event, const juce::String &deviceId) {
        switch (event) {
        case WatchdogEvent::DeviceLost:
          VLOG_WARNING("AudioEngine",
                       "Output device lost: " + deviceId.toStdString());
          setPassthrough(true);
          break;
        case WatchdogEvent::DeviceRestored:
          VLOG_INFO("AudioEngine",
                    "Output device restored: " + deviceId.toStdString());
          break;
        case WatchdogEvent::DriverMissing:
          VLOG_WARNING("AudioEngine",
                       "Virtuoso virtual driver not found in device list");
          break;
        case WatchdogEvent::DriverRestored:
          VLOG_INFO("AudioEngine", "Virtuoso virtual driver restored");
          break;
        default:
          break;
        }
      },
      outputDeviceId);

  m_running.store(true, std::memory_order_release);
  VLOG_INFO("AudioEngine", "Engine started at " + std::to_string(m_sampleRate) +
                               " Hz, block=" + std::to_string(m_blockSize));
  return true;
}

void AudioEngine::stop() {
  if (!m_running.load())
    return;
  m_running.store(false, std::memory_order_release);

  if (m_watchdog)
    m_watchdog->stop();
  m_deviceManager.removeAudioCallback(this);
  m_deviceManager.closeAudioDevice();

  VLOG_INFO("AudioEngine", "Engine stopped");
}

// ---------------------------------------------------------------------------
// AudioIODeviceCallback
// ---------------------------------------------------------------------------
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  m_sampleRate = device->getCurrentSampleRate();
  m_blockSize = device->getCurrentBufferSizeSamples();
  if (m_watchdog)
    m_watchdog->setCurrentSampleRate(m_sampleRate);

  juce::dsp::ProcessSpec stereoSpec{m_sampleRate,
                                    static_cast<uint32_t>(m_blockSize), 2};
  juce::dsp::ProcessSpec monoSpec{m_sampleRate,
                                  static_cast<uint32_t>(m_blockSize), 1};

  m_convolver.prepare(stereoSpec);
  m_upmixer = {}; // stateless — no prepare needed
  m_bassManagement.prepare(monoSpec);
  m_eq.prepare(stereoSpec);
  m_limiter.prepare(stereoSpec);

  // Allocate working buffers
  m_canonical8ch.setSize(8, m_blockSize, false, true, true);
  m_convolverOut.setSize(2, m_blockSize, false, true, true);
  m_lfeOut.setSize(1, m_blockSize, false, true, true);

  VLOG_INFO("AudioEngine",
            "Device started: rate=" + std::to_string(m_sampleRate) +
                " blockSize=" + std::to_string(m_blockSize));
}

void AudioEngine::audioDeviceStopped() {
  m_convolver.reset();
  m_bassManagement.reset();
  m_eq.reset();
  m_limiter.reset();
  VLOG_INFO("AudioEngine", "Device stopped");
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float *const *inputChannelData, int numInputChannels,
    float *const *outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext &) {
  processBlock(inputChannelData, numInputChannels, outputChannelData,
               numOutputChannels, numSamples);
}

// ---------------------------------------------------------------------------
// Real-time processing block (audio thread only)
// ---------------------------------------------------------------------------
void AudioEngine::processBlock(const float *const *inputData, int numInputCh,
                               float *const *outputData, int numOutputCh,
                               int numSamples) noexcept {
  // Track CPU load start time
  const double callbackStart = juce::Time::getMillisecondCounterHiRes();

  // 1. Drain command queue first (lock-free)
  handleCommands();

  // 2. Passthrough mode (bypass DSP when no HRIR loaded or engine disabled)
  if (m_passthrough.load(std::memory_order_relaxed) ||
      !m_enabled.load(std::memory_order_relaxed)) {
    // Output silence (or stereo passthrough if stereo input available)
    for (int ch = 0; ch < numOutputCh; ++ch) {
      if (ch < 2 && ch < numInputCh)
        juce::FloatVectorOperations::copy(outputData[ch], inputData[ch],
                                          numSamples);
      else
        juce::FloatVectorOperations::clear(outputData[ch], numSamples);
    }
    return;
  }

  // 3. Wrap input into AudioBuffer (no copy — just pointer wrapper)
  juce::AudioBuffer<float> inputBuf(const_cast<float **>(inputData), numInputCh,
                                    numSamples);

  // 4. If input is not already 8-channel, detect format and upmix
  const int inputCh = inputBuf.getNumChannels();
  const auto format = UpmixProcessor::detectFormat(inputCh);

  if (format != UpmixProcessor::SourceFormat::Surround71) {
    m_canonical8ch.clear();
    m_upmixer.process(inputBuf, m_canonical8ch, format, numSamples);
  } else {
    // Direct copy to canonical buffer
    for (int ch = 0; ch < 8 && ch < inputCh; ++ch)
      juce::FloatVectorOperations::copy(m_canonical8ch.getWritePointer(ch),
                                        inputData[ch], numSamples);
  }

  // 5. Extract LFE (ch 3) and process through BassManagement
  m_bassManagement.process(m_canonical8ch.getReadPointer(3),
                           m_lfeOut.getWritePointer(0), numSamples);

  // 6. Convolve 7 speaker channels through HRIR
  m_convolverOut.clear();
  m_convolver.process(m_canonical8ch, m_convolverOut);

  // 7. Add LFE (bass managed, no HRIR) to both L and R
  float *outL = m_convolverOut.getWritePointer(0);
  float *outR = m_convolverOut.getWritePointer(1);
  const float *lfe = m_lfeOut.getReadPointer(0);
  juce::FloatVectorOperations::add(outL, lfe, numSamples);
  juce::FloatVectorOperations::add(outR, lfe, numSamples);

  // 8. Parametric EQ
  m_eq.process(m_convolverOut, numSamples);

  // 9. True-peak limiter
  m_limiter.process(m_convolverOut, numSamples);
  m_metrics.limiterGainReductionDb.store(m_limiter.getLastGainReductionDb(),
                                         std::memory_order_relaxed);

  // 10. Copy to output
  for (int ch = 0; ch < std::min(numOutputCh, 2); ++ch)
    juce::FloatVectorOperations::copy(
        outputData[ch], m_convolverOut.getReadPointer(ch), numSamples);
  // Clear any extra output channels
  for (int ch = 2; ch < numOutputCh; ++ch)
    juce::FloatVectorOperations::clear(outputData[ch], numSamples);

  // Update CPU load metric
  double elapsed = juce::Time::getMillisecondCounterHiRes() - callbackStart;
  double blockMs = (numSamples / m_sampleRate) * 1000.0;
  m_metrics.cpuLoadPercent.store(
      static_cast<float>((elapsed / blockMs) * 100.0),
      std::memory_order_relaxed);
  m_metrics.latencyMs.store(static_cast<float>(elapsed),
                            std::memory_order_relaxed);
}

void AudioEngine::handleCommands() noexcept {
  m_commandQueue.drainCommands([this](const AudioCommand &cmd) {
    std::visit(
        [this](auto &&c) {
          using T = std::decay_t<decltype(c)>;
          if constexpr (std::is_same_v<T, CmdSetEnabled>) {
            m_enabled.store(c.enabled, std::memory_order_relaxed);
          } else if constexpr (std::is_same_v<T, CmdLoadHrir>) {
            m_convolver.loadHrir(c.hrirSet);
            m_passthrough.store(!c.hrirSet, std::memory_order_relaxed);
          } else if constexpr (std::is_same_v<T, CmdSetEqBandGain>) {
            EqBandParams p = m_eq.getBandParams(c.bandIndex);
            p.gainDb = c.gainDb;
            m_eq.setBandParams(c.bandIndex, p);
          } else if constexpr (std::is_same_v<T, CmdSetEqEnabled>) {
            m_eq.setEnabled(c.enabled);
          }
          // Additional command handlers in Phase 3
        },
        cmd);
  });
}

} // namespace virtuoso
