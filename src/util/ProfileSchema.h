// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ProfileSchema.h
// JSON schema and strong type definitions for Virtuoso user profiles.
// Profiles are stored encrypted via VirtuosoCrypto (ADR-004).

#pragma once
#include <juce_core/juce_core.h>
#include <optional>
#include <string>
#include <vector>


namespace virtuoso {

// ---------------------------------------------------------------------------
// EQ Band (mirrors EqBandParams but JSON-serialisable without JUCE dependency)
// ---------------------------------------------------------------------------
struct ProfileEqBand {
  std::string
      type; // "peak", "lowShelf", "highShelf", "lowPass", "highPass", "bypass"
  float freqHz{1000.0f};
  float gainDb{0.0f};
  float q{0.707f};
  bool enabled{true};
};

// ---------------------------------------------------------------------------
// Full user profile — all user-adjustable settings in one struct
// ---------------------------------------------------------------------------
struct VirtuosoProfile {
  static constexpr int kSchemaVersion = 1; // Bump on breaking changes

  int schemaVersion{kSchemaVersion};
  std::string profileId;   // UUID v4
  std::string displayName; // User-facing name (e.g. "Gaming / HeSuVi FX")
  std::string hrirSetName; // Name of loaded HRIR set
  std::string hrirSetPath; // Absolute path to HRIR file (resolved at load)

  float masterVolumeDb{0.0f};
  bool eqEnabled{true};
  std::vector<ProfileEqBand> eqBands; // Up to 8 bands

  float bassBoostDb{0.0f}; // LFE bass management extra gain (−6 to +6 dB)
  bool limitingEnabled{true};
  float limiterThresholdDb{-1.0f};

  std::string createdAt; // ISO-8601 timestamp
  std::string updatedAt;

  // ---------------------------------------------------------------------------
  // JSON serialisation (no external dependency — uses nlohmann via Crypto.cpp
  // TU)
  // ---------------------------------------------------------------------------
  juce::String toJsonString() const;
  static std::optional<VirtuosoProfile>
  fromJsonString(const juce::String &json);

  // Default "flat" profile (no EQ, MIT KEMAR HRIR)
  static VirtuosoProfile makeDefault();
};

} // namespace virtuoso
