// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HeSuViImporter.h — HeSuVi compliance import helper

#pragma once
#include "WavHrirParser.h"
#include <expected>
#include <string>

namespace virtuoso {

// Validates and imports a HeSuVi-formatted 14-channel WAV file.
// Applies the HeSuVi compliance gate: all 14 channels must be present,
// total IR length must be in [64, 131072] samples, silent speaker check
// warning.
class HeSuViImporter {
public:
  static constexpr int kMinIrLength = 64;
  static constexpr int kMaxIrLength = 131072;

  struct ImportResult {
    bool valid{false};
    std::string warningMessage; // Non-fatal (e.g. silent speaker)
    HrirSet hrirSet;
  };

  // Parse and gate-check a HeSuVi WAV file.
  // Uses WavHrirParser internally; this adds the compliance gate on top.
  static std::expected<ImportResult, std::string>
  importFile(const juce::File &wavFile);
};

} // namespace virtuoso
