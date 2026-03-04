// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ProfileManager.h
// Manages loading, saving, encrypting, and listing Virtuoso user profiles.
// Profiles are encrypted with VirtuosoCrypto before being written to disk.

#pragma once
#include "Crypto.h"
#include "ProfileSchema.h"
#include <functional>
#include <juce_core/juce_core.h>
#include <optional>
#include <vector>


namespace virtuoso {

class ProfileManager {
public:
  static constexpr std::string_view kContextLabel = "virtuoso.profile";
  static constexpr std::string_view kFileExtension =
      ".vprf"; // Virtuoso Profile

  ProfileManager();

  // ---------------------------------------------------------------------------
  // Profile I/O
  // ---------------------------------------------------------------------------
  bool saveProfile(const VirtuosoProfile &profile);
  std::optional<VirtuosoProfile> loadProfile(const juce::String &profileId);
  bool deleteProfile(const juce::String &profileId);

  // List all available profile IDs and display names
  std::vector<std::pair<std::string, std::string>> listProfiles() const;

  // Load default profile (or create one if none exists)
  VirtuosoProfile loadOrCreateDefault();

  // Import/Export as plain JSON (for backup — no encryption)
  bool exportProfileJson(const VirtuosoProfile &profile,
                         const juce::File &destination);
  std::optional<VirtuosoProfile> importProfileJson(const juce::File &source);

  // Callback fired on message thread after successful save
  std::function<void(const juce::String &profileId)> onProfileSaved;

  // Profile storage directory
  juce::File profileDirectory() const;

private:
  juce::File m_profileDir;

  juce::File profileFileFor(const juce::String &profileId) const;
};

} // namespace virtuoso
