// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirResampler.h — Dedicated IR-length resampler (thin wrapper over
// ResamplerProcessor)

#pragma once
#include "../audio/ResamplerProcessor.h"
#include <expected>
#include <juce_audio_basics/juce_audio_basics.h>
#include <string>


namespace virtuoso {

class HrirResampler {
public:
  // Resample a mono IR buffer from sourceSr to targetSr.
  // Returns resampled buffer or error string.
  static std::expected<juce::AudioBuffer<float>, std::string>
  resample(const juce::AudioBuffer<float> &ir, double sourceSr,
           double targetSr);
};

} // namespace virtuoso
