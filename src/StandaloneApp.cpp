// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// StandaloneApp.cpp

#include "StandaloneApp.h"
#include "hrir/HrirParserWorker.h"

namespace virtuoso {

void VirtuosoStandaloneApp::initialise(const juce::String &commandLine) {
  // Subprocess mode: this binary doubles as the HRIR parser worker
  if (commandLine.contains("--hrir-parser-worker")) {
    HrirParserWorker::workerMain();
    quit();
    return;
  }

  // Normal startup
  Logger::instance().initialise(
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("virtuoso-audio/logs"),
      1024 * 1024, // 1 MB per file
      10           // Keep 10 files
  );

  VLOG_INFO("App", "Virtuoso Audio " + getApplicationVersion().toStdString() +
                       " starting");

  m_profileManager = std::make_unique<ProfileManager>();
  m_engine = std::make_unique<AudioEngine>();
  m_hrirManager = std::make_unique<HrirManager>();
  m_hrirManager->initialise(nullptr /* convolver set after engine starts */,
                            48000.0);

  // Load default HRIR
  m_hrirManager->loadDefault();

  // Start audio engine
  m_engine->start();

  VLOG_INFO("App", "Startup complete");
}

void VirtuosoStandaloneApp::shutdown() {
  VLOG_INFO("App", "Shutting down");
  if (m_engine)
    m_engine->stop();
  m_hrirManager.reset();
  m_profileManager.reset();
  m_engine.reset();
  Logger::instance().shutdown();
}

void VirtuosoStandaloneApp::systemRequestedQuit() { quit(); }

void VirtuosoStandaloneApp::anotherInstanceStarted(const juce::String &) {
  // Bring window to front (Phase 3 GUI)
  VLOG_INFO("App", "Another instance attempted to start — ignoring");
}

} // namespace virtuoso
