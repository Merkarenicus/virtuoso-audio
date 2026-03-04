// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// SofaHrirParser.h
// SOFA (Spatially Oriented Format for Acoustics) file parser.
// Uses libmysofa to load .sofa files and extract HRIR data for 7.1 speaker
// positions. Runs ONLY inside the HrirParserWorker subprocess — never in the
// main process.

#pragma once
#include "HrirValidator.h"
#include "WavHrirParser.h" // For HrirSet definition
#include <array>
#include <expected>
#include <juce_core/juce_core.h>
#include <mysofa.h>
#include <string>


namespace virtuoso {

// Speaker azimuth/elevation positions to extract from SOFA (ITU-R BS.775-3)
struct SpeakerPosition {
  float azimuthDeg;   // 0=front, 90=left, -90=right
  float elevationDeg; // 0=horizontal
  const char *label;
};

class SofaHrirParser {
public:
  // Default speaker positions for 7.1 extraction from SOFA file
  static constexpr std::array<SpeakerPosition, 7> kDefaultPositions{
      {{30.0f, 0.0f, "FL"},
       {-30.0f, 0.0f, "FR"},
       {0.0f, 0.0f, "C"},
       {90.0f, 0.0f, "SL"},
       {-90.0f, 0.0f, "SR"},
       {135.0f, 0.0f, "BL"},
       {-135.0f, 0.0f, "BR"}}};

  SofaHrirParser() = default;

  // Parse a .sofa file and extract HRIRs for the 7 speaker positions.
  // Uses nearest-neighbour search in the SOFA dataset.
  std::expected<HrirSet, std::string> parse(const juce::File &sofaFile) const;

  // Custom speaker positions (overrides kDefaultPositions)
  void setCustomPositions(std::array<SpeakerPosition, 7> positions) {
    m_positions = positions;
  }

private:
  std::array<SpeakerPosition, 7> m_positions{kDefaultPositions};

  struct SofaHandle {
    MYSOFA_EASY *handle{nullptr};
    int err{0};
    ~SofaHandle() {
      if (handle)
        mysofa_close(handle);
    }
  };

  static std::expected<SofaHandle, std::string>
  openSofa(const juce::File &file, double targetSampleRate);

  static std::expected<HrirChannelPair, std::string>
  extractHrirForPosition(MYSOFA_EASY *sofa, float azimuthDeg,
                         float elevationDeg, double targetSampleRate,
                         const char *label);
};

} // namespace virtuoso
