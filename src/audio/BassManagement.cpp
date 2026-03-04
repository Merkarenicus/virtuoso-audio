// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// BassManagement.cpp

#include "BassManagement.h"

namespace virtuoso {

void BassManagement::prepare(const juce::dsp::ProcessSpec &monoSpec) {
  m_sampleRate = monoSpec.sampleRate;
  m_blockSize = static_cast<int>(monoSpec.maximumBlockSize);
  m_workBuf.setSize(1, m_blockSize, false, true, true);
  m_biquadChain.prepare(monoSpec);
  updateCoefficients();
}

void BassManagement::reset() { m_biquadChain.reset(); }

void BassManagement::process(const float *lfeIn, float *monoOut,
                             int numSamples) noexcept {
  jassert(numSamples <= m_blockSize);

  // Copy LFE input into work buffer
  float *work = m_workBuf.getWritePointer(0);
  juce::FloatVectorOperations::copy(work, lfeIn, numSamples);

  // Apply two-stage biquad LPF chain in-situ
  juce::dsp::AudioBlock<float> block(m_workBuf);
  auto trimmed = block.getSubBlock(0, static_cast<size_t>(numSamples));
  juce::dsp::ProcessContextReplacing<float> ctx(trimmed);
  m_biquadChain.process(ctx);

  // Apply +6 dB gain and write to output
  juce::FloatVectorOperations::copyWithMultiply(monoOut, work, kGainLinear,
                                                numSamples);
}

void BassManagement::updateCoefficients() {
  // 4th-order Butterworth LPF = product of two 2nd-order Butterworth sections
  // Pole angles for normalised Butterworth: π/4, 3π/4 (rotated to be in the
  // left half-plane)
  const double cutoff = static_cast<double>(kCutoffHz);

  // Section 1: Q = 1/(2*sin(π/4)) = 0.7071
  auto coeff1 = juce::dsp::IIR::Coefficients<float>::makeLowPass(
      m_sampleRate, cutoff, 0.5411f);
  // Section 2: Q = 1/(2*sin(3π/4)) = 1.3066
  auto coeff2 = juce::dsp::IIR::Coefficients<float>::makeLowPass(
      m_sampleRate, cutoff, 1.3066f);

  *m_biquadChain.get<0>().coefficients = *coeff1;
  *m_biquadChain.get<1>().coefficients = *coeff2;
}

} // namespace virtuoso
