// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirValidator.cpp

#include "HrirValidator.h"
#include "../audio/ConvolverProcessor.h"
#include <cmath>

namespace virtuoso {

HrirValidationResult
HrirValidator::validateFileSize(int64_t fileSizeBytes) noexcept {
  if (fileSizeBytes <= 0)
    return HrirValidationResult::fail("File is empty");
  if (fileSizeBytes > kMaxFileSizeBytes)
    return HrirValidationResult::fail(
        "File too large: " + std::to_string(fileSizeBytes) + " bytes (max " +
        std::to_string(kMaxFileSizeBytes) + ")");
  return HrirValidationResult::ok();
}

HrirValidationResult
HrirValidator::validateChannelPair(const juce::AudioBuffer<float> &left,
                                   const juce::AudioBuffer<float> &right,
                                   double sampleRate) noexcept {
  // Sample rate bounds
  if (sampleRate < kMinSampleRate || sampleRate > kMaxSampleRate)
    return HrirValidationResult::fail("Invalid sample rate: " +
                                      std::to_string(sampleRate));

  // Channel count
  if (left.getNumChannels() != 1 || right.getNumChannels() != 1)
    return HrirValidationResult::fail(
        "HRIR pair must be mono (1 channel each)");

  // Length bounds
  if (left.getNumSamples() == 0)
    return HrirValidationResult::fail("HRIR left ear is empty");
  if (left.getNumSamples() > kMaxSamplesPerCh)
    return HrirValidationResult::fail("HRIR left ear too long: " +
                                      std::to_string(left.getNumSamples()));
  if (right.getNumSamples() == 0)
    return HrirValidationResult::fail("HRIR right ear is empty");
  if (right.getNumSamples() > kMaxSamplesPerCh)
    return HrirValidationResult::fail("HRIR right ear too long: " +
                                      std::to_string(right.getNumSamples()));

  // NaN / Inf checks (stop-ship — these would propagate to DSP output)
  if (containsNaN(left))
    return HrirValidationResult{false, "Left HRIR contains NaN", true, false,
                                false};
  if (containsInf(left))
    return HrirValidationResult{false, "Left HRIR contains Inf", false, true,
                                false};
  if (containsNaN(right))
    return HrirValidationResult{false, "Right HRIR contains NaN", true, false,
                                false};
  if (containsInf(right))
    return HrirValidationResult{false, "Right HRIR contains Inf", false, true,
                                false};

  // Silence warning (not a hard failure — some HRIRs have silent channels for
  // specific positions)
  HrirValidationResult result = HrirValidationResult::ok();
  if (isSilent(left) && isSilent(right))
    result.hasSilence = true; // Warning: HRIR pair is all zeros

  return result;
}

bool HrirValidator::containsNaN(const juce::AudioBuffer<float> &buf) noexcept {
  for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
    const float *data = buf.getReadPointer(ch);
    for (int i = 0; i < buf.getNumSamples(); ++i)
      if (std::isnan(data[i]))
        return true;
  }
  return false;
}

bool HrirValidator::containsInf(const juce::AudioBuffer<float> &buf) noexcept {
  for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
    const float *data = buf.getReadPointer(ch);
    for (int i = 0; i < buf.getNumSamples(); ++i)
      if (std::isinf(data[i]))
        return true;
  }
  return false;
}

bool HrirValidator::isSilent(const juce::AudioBuffer<float> &buf) noexcept {
  for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
    const float *data = buf.getReadPointer(ch);
    for (int i = 0; i < buf.getNumSamples(); ++i)
      if (data[i] != 0.0f)
        return false;
  }
  return true;
}

} // namespace virtuoso
