// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// MainWindow.h — Virtuoso main application window

#pragma once
#include "../audio/AudioEngine.h"
#include "../hrir/HrirManager.h"
#include "LookAndFeel.h"
#include "SystemTray.h"
#include "components/AboutPanel.h"
#include "components/HrirCarousel.h"
#include "components/LatencyMonitor.h"
#include "components/MasterToggle.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>


namespace virtuoso {

class MainWindow : public juce::DocumentWindow,
                   public juce::Button::Listener,
                   public juce::Timer {
public:
  static constexpr int kWindowW = 540;
  static constexpr int kWindowH = 400;

  MainWindow(const juce::String &title, AudioEngine &engine,
             HrirManager &hrirMgr);
  ~MainWindow() override;

  // DocumentWindow
  void closeButtonPressed() override;

  // Button::Listener
  void buttonClicked(juce::Button *b) override;

  // Timer — update meters
  void timerCallback() override;

  // Show/hide (from system tray)
  void toggleVisibility();

private:
  VirtuosoLookAndFeel m_laf;

  // Content component (owned by DocumentWindow)
  class ContentView;
  std::unique_ptr<ContentView> m_contentView;

  // Refs to engine / HRIR manager
  AudioEngine &m_engine;
  HrirManager &m_hrirMgr;

  // System tray
  std::unique_ptr<VirtuosoSystemTray> m_systemTray;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace virtuoso
