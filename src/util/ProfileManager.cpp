// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ProfileManager.cpp

#include "ProfileManager.h"
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

namespace virtuoso {

ProfileManager::ProfileManager() {
  m_profileDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("virtuoso-audio/profiles");
  m_profileDir.createDirectory();
}

juce::File ProfileManager::profileDirectory() const { return m_profileDir; }

juce::File ProfileManager::profileFileFor(const juce::String &profileId) const {
  return m_profileDir.getChildFile(profileId +
                                   juce::String(kFileExtension.data()));
}

bool ProfileManager::saveProfile(const VirtuosoProfile &profile) {
  juce::String jsonStr = profile.toJsonString();
  const std::span<const uint8_t> plaintext{
      reinterpret_cast<const uint8_t *>(jsonStr.toRawUTF8()),
      static_cast<size_t>(jsonStr.getNumBytesAsUTF8())};

  auto encrypted = VirtuosoCrypto::encrypt(plaintext, kContextLabel);
  if (!encrypted.has_value())
    return false;

  auto outStream =
      profileFileFor(juce::String(profile.profileId)).createOutputStream();
  if (!outStream || !outStream->openedOk())
    return false;
  outStream->write(encrypted->data(), encrypted->size());

  if (onProfileSaved)
    juce::MessageManager::callAsync(
        [this, id = juce::String(profile.profileId)] { onProfileSaved(id); });

  return true;
}

std::optional<VirtuosoProfile>
ProfileManager::loadProfile(const juce::String &profileId) {
  juce::File file = profileFileFor(profileId);
  if (!file.existsAsFile())
    return std::nullopt;

  juce::MemoryBlock ciphertext;
  if (!file.loadFileAsData(ciphertext))
    return std::nullopt;

  const std::span<const uint8_t> cSpan{
      static_cast<const uint8_t *>(ciphertext.getData()), ciphertext.getSize()};
  auto decrypted = VirtuosoCrypto::decryptToString(cSpan, kContextLabel);
  if (!decrypted.has_value())
    return std::nullopt;

  return VirtuosoProfile::fromJsonString(*decrypted);
}

bool ProfileManager::deleteProfile(const juce::String &profileId) {
  return profileFileFor(profileId).deleteFile();
}

std::vector<std::pair<std::string, std::string>>
ProfileManager::listProfiles() const {
  juce::Array<juce::File> files;
  m_profileDir.findChildFiles(files, juce::File::findFiles, false,
                              "*" + juce::String(kFileExtension.data()));

  std::vector<std::pair<std::string, std::string>> result;
  for (auto &f : files) {
    juce::String id = f.getFileNameWithoutExtension();
    // Load and peek at display name without full decryption parse for perf
    // (in practice profiles are small; full load is fine)
    if (auto profile = loadProfile(id))
      result.emplace_back(id.toStdString(), profile->displayName);
  }
  return result;
}

VirtuosoProfile ProfileManager::loadOrCreateDefault() {
  auto profiles = listProfiles();
  if (!profiles.empty()) {
    if (auto p = loadProfile(juce::String(profiles[0].first)))
      return *p;
  }
  auto def = VirtuosoProfile::makeDefault();
  saveProfile(def);
  return def;
}

bool ProfileManager::exportProfileJson(const VirtuosoProfile &profile,
                                       const juce::File &dest) {
  return dest.replaceWithText(profile.toJsonString());
}

std::optional<VirtuosoProfile>
ProfileManager::importProfileJson(const juce::File &src) {
  return VirtuosoProfile::fromJsonString(src.loadFileAsString());
}

} // namespace virtuoso
