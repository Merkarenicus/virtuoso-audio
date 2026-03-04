// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ProfileSchema.cpp — JSON serialisation / deserialisation for VirtuosoProfile

#include "ProfileSchema.h"
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>
#include <sstream>


using json = nlohmann::json;

namespace virtuoso {

static const char *bandTypeToStr(const std::string &type) {
  return type.c_str();
}

juce::String VirtuosoProfile::toJsonString() const {
  json j;
  j["schemaVersion"] = schemaVersion;
  j["profileId"] = profileId;
  j["displayName"] = displayName;
  j["hrirSetName"] = hrirSetName;
  j["hrirSetPath"] = hrirSetPath;
  j["masterVolumeDb"] = masterVolumeDb;
  j["eqEnabled"] = eqEnabled;
  j["bassBoostDb"] = bassBoostDb;
  j["limitingEnabled"] = limitingEnabled;
  j["limiterThresholdDb"] = limiterThresholdDb;
  j["createdAt"] = createdAt;
  j["updatedAt"] = updatedAt;

  j["eqBands"] = json::array();
  for (const auto &b : eqBands)
    j["eqBands"].push_back({{"type", b.type},
                            {"freqHz", b.freqHz},
                            {"gainDb", b.gainDb},
                            {"q", b.q},
                            {"enabled", b.enabled}});

  return juce::String(j.dump(2));
}

std::optional<VirtuosoProfile>
VirtuosoProfile::fromJsonString(const juce::String &jsonStr) {
  try {
    auto j = json::parse(jsonStr.toStdString());
    VirtuosoProfile p;
    p.schemaVersion = j.value("schemaVersion", kSchemaVersion);
    p.profileId = j.value("profileId", "");
    p.displayName = j.value("displayName", "Unnamed");
    p.hrirSetName = j.value("hrirSetName", "");
    p.hrirSetPath = j.value("hrirSetPath", "");
    p.masterVolumeDb = j.value("masterVolumeDb", 0.0f);
    p.eqEnabled = j.value("eqEnabled", true);
    p.bassBoostDb = j.value("bassBoostDb", 0.0f);
    p.limitingEnabled = j.value("limitingEnabled", true);
    p.limiterThresholdDb = j.value("limiterThresholdDb", -1.0f);
    p.createdAt = j.value("createdAt", "");
    p.updatedAt = j.value("updatedAt", "");
    if (j.contains("eqBands") && j["eqBands"].is_array()) {
      for (const auto &b : j["eqBands"])
        p.eqBands.push_back({b.value("type", "bypass"),
                             b.value("freqHz", 1000.0f),
                             b.value("gainDb", 0.0f), b.value("q", 0.707f),
                             b.value("enabled", true)});
    }
    return p;
  } catch (...) {
    return std::nullopt;
  }
}

VirtuosoProfile VirtuosoProfile::makeDefault() {
  VirtuosoProfile p;
  p.profileId = juce::Uuid().toDashedString().toStdString();
  p.displayName = "Default";
  p.hrirSetName = "MIT KEMAR";
  p.createdAt = juce::Time::getCurrentTime().toISO8601(true).toStdString();
  p.updatedAt = p.createdAt;
  return p;
}

} // namespace virtuoso
