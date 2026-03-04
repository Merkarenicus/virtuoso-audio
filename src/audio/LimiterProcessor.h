// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// LimiterProcessor.h
// True-peak limiter at −1.0 dBTP.
// Uses JUCE's dsp::Limiter (lookahead brick-wall compressor) as the base,
// with an oversampled true-peak meter for compliance with EBU R128/ITU-R
// BS.1770.

#pragma once
#include <atomic>
#include <juce_dsp/juce_dsp.h>


namespace virtuoso {

class LimiterProcessor {
public:
  // Limit level in dBFS (default −1.0 dBTP per ADR-003 / broadcast spec)
  static constexpr float kDefaultThresholdDb = -1.0f;

  LimiterProcessor();

  void prepare(const juce::dsp::ProcessSpec &stereoSpec);
  void reset();

  // Process stereo buffer in-place
  void process(juce::AudioBuffer<float> &stereoBuffer, int numSamples) noexcept;

  // Set threshold (dBFS). Thread-safe.
  void setThresholdDb(float thresholdDb) noexcept;
  float getThresholdDb() const noexcept {
    return m_thresholdDb.load(std::memory_order_relaxed);
  }

  // Returns peak gain reduction in dB applied in last block (for UI metering)
  float getLastGainReductionDb() const noexcept {
    return m_lastGainReductionDb.load(std::memory_order_relaxed);
  }

private:
  juce::dsp::Limiter<float> m_limiter;
  std::atomic<float> m_thresholdDb{kDefaultThresholdDb};
  std::atomic<float> m_lastGainReductionDb{0.0f};
  bool m_needsParamUpdate{false};
  double m_sampleRate{48000.0};
};

} // namespace virtuoso
