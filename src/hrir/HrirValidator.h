// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirValidator.h
// Hard-limit validation for HRIR data loaded from untrusted user files.
// All validation MUST occur before any data reaches the DSP heap.
// Run inside HrirParserWorker subprocess — never in the host audio thread.

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>
#include <string>


namespace virtuoso {

struct HrirValidationResult {
  bool valid{false};
  std::string errorMessage; // Human-readable rejection reason
  bool hasNaN{false};
  bool hasInf{false};
  bool hasSilence{false}; // Warning only — all zeros

  static HrirValidationResult ok() { return {true, {}}; }
  static HrirValidationResult fail(std::string msg) {
    HrirValidationResult r;
    r.valid = false;
    r.errorMessage = std::move(msg);
    return r;
  }
};

class HrirValidator {
public:
  // Hard limits (enforced before parsing begins)
  static constexpr int64_t kMaxFileSizeBytes = 50 * 1024 * 1024; // 50 MB
  static constexpr int kMaxChannels = 96;
  static constexpr int kMaxSamplesPerCh = 65536;
  static constexpr double kMinSampleRate = 8000.0;
  static constexpr double kMaxSampleRate = 384000.0;
  static constexpr float kPeakWarningLevel =
      6.0f; // >6.0 linera → warn (not reject)

  // Validate raw file bytes before parsing (pre-parse check)
  static HrirValidationResult validateFileSize(int64_t fileSizeBytes) noexcept;

  // Validate a parsed single-speaker HRIR pair (left + right ear)
  static HrirValidationResult
  validateChannelPair(const juce::AudioBuffer<float> &left,
                      const juce::AudioBuffer<float> &right,
                      double sampleRate) noexcept;

  // Validate a complete HrirSet (calls validateChannelPair for each speaker)
  // Returns the first failure, or ok() if all pass.
  template <typename HrirSet>
  static HrirValidationResult validateSet(const HrirSet &hrirSet) noexcept;

private:
  static bool containsNaN(const juce::AudioBuffer<float> &buf) noexcept;
  static bool containsInf(const juce::AudioBuffer<float> &buf) noexcept;
  static bool isSilent(const juce::AudioBuffer<float> &buf) noexcept;
};

} // namespace virtuoso
