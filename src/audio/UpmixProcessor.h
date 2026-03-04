// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// UpmixProcessor.h
// Deterministic channel upmix: 2-ch → 7.1 and 5.1 → 7.1.
// Applies -6 dB (0.5×) headroom attenuation pre-convolution to prevent
// clipping from correlated signal summation (per ADR-003 and review consensus).

#pragma once
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>


namespace virtuoso {

// Output is always canonical 8-ch (FL,FR,C,LFE,BL,BR,SL,SR)
class UpmixProcessor {
public:
  enum class SourceFormat { Stereo, Surround51, Surround71 };

  UpmixProcessor() = default;

  // Detect source format from channel count
  static SourceFormat detectFormat(int channels) noexcept {
    if (channels >= 8)
      return SourceFormat::Surround71;
    if (channels >= 6)
      return SourceFormat::Surround51;
    return SourceFormat::Stereo;
  }

  // Upmix src → dst (8 channels). dst must have exactly 8 channels.
  // headroomGain: applied to ALL output channels (default = 0.5 = −6 dB)
  void process(const juce::AudioBuffer<float> &src,
               juce::AudioBuffer<float> &dst, SourceFormat format,
               int numSamples, float headroomGain = 0.5f) noexcept;

  // Coefficient matrices (for unit testing)
  struct Matrix71 {
    std::array<std::array<float, 2>, 8> c;
  }; // 8 output × 2 stereo inputs

  // Returns the stereo → 7.1 matrix
  static const Matrix71 &stereoMatrix() noexcept;

private:
  // Stereo → 7.1 upmix coefficients (ITU-R BS.775-3 derived)
  // Output channel × input (L, R)
  static constexpr float kStereoMatrix[8][2] = {
      {1.0f, 0.0f},     // FL  = L
      {0.0f, 1.0f},     // FR  = R
      {0.707f, 0.707f}, // C  = 0.707*(L+R)
      {0.0f, 0.0f},     // LFE = 0 (no bass management from stereo)
      {0.707f, 0.0f},   // BL  = 0.707*L
      {0.0f, 0.707f},   // BR  = 0.707*R
      {0.5f, 0.0f},     // SL  = 0.5*L
      {0.0f, 0.5f},     // SR  = 0.5*R
  };

  // 5.1 → 7.1 upmix coefficients (ITU-R BS.775-3)
  // Input channels: FL=0,FR=1,C=2,LFE=3,BL=4,BR=5
  static constexpr float kSurround51Matrix[8][6] = {
      {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},   // FL
      {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},   // FR
      {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},   // C
      {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},   // LFE (pass-through)
      {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},   // BL  = BL
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},   // BR  = BR
      {0.0f, 0.0f, 0.0f, 0.0f, 0.707f, 0.0f}, // SL = 0.707*BL
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.707f}, // SR = 0.707*BR
  };
};

} // namespace virtuoso
