// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// WavHrirParser.h
// Strictly safe WAV parser for HRIR files.
//
// Supported formats:
//   - Standard 8-channel WAV (FL,FR,C,LFE,BL,BR,SL,SR)
//   - HeSuVi 14-channel WAV (7 speaker pairs × 2 ears, interleaved)
//
// SECURITY: This parser is run inside HrirParserWorker (isolated subprocess).
// All arithmetic is overflow-safe. No heap allocation until after header
// validation. THERE IS NO "RIFF CHECKSUM" in the WAV format — do not add one.
//
// Fuzz target: tests/fuzz/FuzzWavParser.cpp

#pragma once
#include "../audio/ConvolverProcessor.h" // HrirSet, HrirChannelPair
#include "HrirValidator.h"
#include <expected>
#include <juce_audio_formats/juce_audio_formats.h>
#include <string>
#include <vector>


namespace virtuoso {

// Result type: either a valid HrirSet or an error message
using WavParseResult = std::expected<HrirSet, std::string>;

class WavHrirParser {
public:
  WavHrirParser() = default;

  // Parse a WAV file from disk.
  // Checks file size before reading. All validation via HrirValidator.
  // Must be called from HrirParserWorker subprocess — never from audio thread.
  WavParseResult parse(const juce::File &wavFile) const;

  // Parse from pre-loaded bytes (for fuzz testing)
  WavParseResult parseFromMemory(const void *data, size_t sizeBytes) const;

  // HeSuVi 14-channel channel order (7 speakers × 2 ears):
  // Ch: 0=FL_L, 1=FL_R, 2=FR_L, 3=FR_R, 4=C_L, 5=C_R,
  //     6=SL_L, 7=SL_R, 8=SR_L, 9=SR_R, 10=BL_L, 11=BL_R, 12=BR_L, 13=BR_R
  static constexpr int kHeSuViChannelCount = 14;

private:
  // Map a 14-channel HeSuVi buffer → HrirSet (7 channel pairs)
  static WavParseResult parseHeSuVi14Ch(const juce::AudioBuffer<float> &raw,
                                        double sampleRate);

  // Map an 8-channel WAV (standard 7.1 HRIR layout) → HrirSet
  // Assumes separate files for L/R ears or a specific vendor layout.
  static WavParseResult parse8Ch(const juce::AudioBuffer<float> &raw,
                                 double sampleRate);
};

} // namespace virtuoso
