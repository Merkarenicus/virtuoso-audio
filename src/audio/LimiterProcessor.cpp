// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// LimiterProcessor.cpp

#include "LimiterProcessor.h"

namespace virtuoso {

LimiterProcessor::LimiterProcessor() {
  m_limiter.setThreshold(kDefaultThresholdDb);
  m_limiter.setRelease(50.0f); // 50 ms release
}

void LimiterProcessor::prepare(const juce::dsp::ProcessSpec &stereoSpec) {
  m_sampleRate = stereoSpec.sampleRate;
  m_limiter.prepare(stereoSpec);
  m_limiter.setThreshold(m_thresholdDb.load(std::memory_order_relaxed));
  m_limiter.setRelease(50.0f);
}

void LimiterProcessor::reset() {
  m_limiter.reset();
  m_lastGainReductionDb.store(0.0f, std::memory_order_relaxed);
}

void LimiterProcessor::process(juce::AudioBuffer<float> &buf,
                               int numSamples) noexcept {
  // Apply pending threshold update
  const float thresh = m_thresholdDb.load(std::memory_order_relaxed);
  m_limiter.setThreshold(thresh);

  // Measure peak before limiting for gain reduction metering
  float peakBefore = buf.getMagnitude(0, numSamples);
  float peakBeforeDb =
      peakBefore > 1e-9f ? 20.0f * std::log10(peakBefore) : -120.0f;

  // Process through JUCE limiter
  juce::dsp::AudioBlock<float> block(buf.getArrayOfWritePointers(),
                                     static_cast<size_t>(buf.getNumChannels()),
                                     static_cast<size_t>(numSamples));
  juce::dsp::ProcessContextReplacing<float> ctx(block);
  m_limiter.process(ctx);

  // Measure peak after limiting
  float peakAfter = buf.getMagnitude(0, numSamples);
  float peakAfterDb =
      peakAfter > 1e-9f ? 20.0f * std::log10(peakAfter) : -120.0f;

  float gr = peakAfterDb - peakBeforeDb;
  m_lastGainReductionDb.store(gr < 0.0f ? gr : 0.0f, std::memory_order_relaxed);
}

void LimiterProcessor::setThresholdDb(float thresholdDb) noexcept {
  m_thresholdDb.store(thresholdDb, std::memory_order_relaxed);
}

} // namespace virtuoso
