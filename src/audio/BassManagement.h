// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// BassManagement.h
// LFE channel bass management: 4th-order Butterworth LPF at 120 Hz + 6 dB gain.
// Output is mono (added to both L and R by AudioEngine, bypassing HRIR
// convolution).
//
// Reference: AES standard for LFE channel processing in cinema/home theatre.

#pragma once
#include <array>
#include <juce_dsp/juce_dsp.h>


namespace virtuoso {

class BassManagement {
public:
  BassManagement() = default;

  // Must be called on audio thread before first process().
  void prepare(const juce::dsp::ProcessSpec &monoSpec);
  void reset();

  // Process one block: filter LFE input through LPF, apply +6 dB gain.
  // Both src and dst are mono (single-channel).
  // Returns the filtered, gain-boosted LFE signal ready to be added to stereo
  // output.
  void process(const float *lfeIn, float *monoOut, int numSamples) noexcept;

  // Cutoff frequency (read-only for unit tests)
  static constexpr float kCutoffHz = 120.0f;
  static constexpr float kGainLinear = 1.9953f; // +6 dB = 10^(6/20)

private:
  // 4th-order Butterworth LPF = two cascaded 2nd-order sections
  using BiquadChain = juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>,
                                                juce::dsp::IIR::Filter<float>>;

  BiquadChain m_biquadChain;
  juce::AudioBuffer<float> m_workBuf;
  double m_sampleRate{48000.0};
  int m_blockSize{512};

  void updateCoefficients();
};

} // namespace virtuoso
