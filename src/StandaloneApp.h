// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// StandaloneApp.h — Top-level JUCE application class

#pragma once
#include "../audio/AudioEngine.h"
#include "../hrir/HrirManager.h"
#include "../logging/Logger.h"
#include "../util/ProfileManager.h"
#include <juce_application/juce_application.h>
#include <memory>


namespace virtuoso {

class VirtuosoStandaloneApp : public juce::JUCEApplication {
public:
  VirtuosoStandaloneApp() = default;

  const juce::String getApplicationName() override { return "Virtuoso Audio"; }
  const juce::String getApplicationVersion() override { return "1.0.0"; }
  bool moreThanOneInstanceAllowed() override { return false; }

  void initialise(const juce::String &commandLine) override;
  void shutdown() override;
  void systemRequestedQuit() override;
  void anotherInstanceStarted(const juce::String &) override;

private:
  std::unique_ptr<AudioEngine> m_engine;
  std::unique_ptr<ProfileManager> m_profileManager;
  std::unique_ptr<HrirManager> m_hrirManager;
  // GUI created in Phase 3 initialise()
};

} // namespace virtuoso
