// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// EqProcessor.h
// Parametric EQ processor — post-convolution, pre-limiter.
// Supports up to 8 bands: PEQ bell, high shelf, low shelf, high-pass, low-pass.
// Compatible with AutoEq-exported filter lists.

#pragma once
#include <array>
#include <atomic>
#include <juce_dsp/juce_dsp.h>


namespace virtuoso {

enum class EqBandType {
  PeakBell,
  LowShelf,
  HighShelf,
  LowPass,
  HighPass,
  Bypass
};

struct EqBandParams {
  EqBandType type{EqBandType::Bypass};
  float freqHz{1000.0f};
  float gainDb{0.0f};
  float q{0.707f};
  bool enabled{true};
};

class EqProcessor {
public:
  static constexpr int kMaxBands = 8;

  EqProcessor();

  // Call on audio thread before first use
  void prepare(const juce::dsp::ProcessSpec &stereoSpec);
  void reset();

  // Process stereo buffer in-place
  void process(juce::AudioBuffer<float> &stereoBuffer, int numSamples) noexcept;

  // Thread-safe band parameter update (may be called from UI thread)
  void setBandParams(int bandIndex, const EqBandParams &params);
  EqBandParams getBandParams(int bandIndex) const noexcept;

  // Global enable/disable
  void setEnabled(bool enabled) noexcept {
    m_enabled.store(enabled, std::memory_order_relaxed);
  }
  bool isEnabled() const noexcept {
    return m_enabled.load(std::memory_order_relaxed);
  }

  // Import from AutoEq export format (one filter per line: "Filter X: ON PK Fc
  // NNN Hz Gain NNN dB Q NNN")
  bool importAutoEq(const juce::String &autoEqText);

private:
  using BiquadFilter = juce::dsp::IIR::Filter<float>;
  using Chain =
      juce::dsp::ProcessorChain<BiquadFilter, BiquadFilter, BiquadFilter,
                                BiquadFilter, BiquadFilter, BiquadFilter,
                                BiquadFilter, BiquadFilter>;

  std::array<Chain, 2> m_chains; // L and R
  std::array<EqBandParams, kMaxBands> m_params;
  std::array<std::atomic<bool>, kMaxBands>
      m_dirty; // flags: params updated, need coefficient recalc

  double m_sampleRate{48000.0};
  std::atomic<bool> m_enabled{true};
  juce::SpinLock m_paramsLock;

  void updateCoefficients();
  void applyCoeffToChain(int bandIdx, double sampleRate, const EqBandParams &p,
                         BiquadFilter &filterL, BiquadFilter &filterR);
};

} // namespace virtuoso
