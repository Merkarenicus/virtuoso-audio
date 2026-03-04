// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// MainWindow.cpp

#include "MainWindow.h"
#include "../logging/Logger.h"

namespace virtuoso {

// ---------------------------------------------------------------------------
// ContentView — the inner component laid out inside the document window
// ---------------------------------------------------------------------------
class MainWindow::ContentView : public juce::Component {
public:
  ContentView(AudioEngine &engine, HrirManager &hrirMgr)
      : m_masterToggle("Virtuoso Audio"), m_hrirCarousel(hrirMgr) {
    addAndMakeVisible(m_masterToggle);
    addAndMakeVisible(m_hrirCarousel);
    addAndMakeVisible(m_latencyMonitor);
    setSize(kWindowW, kWindowH);
  }

  void paint(juce::Graphics &g) override {
    g.fillAll(Colours::Background);

    // Header gradient
    juce::ColourGradient headerGrad(Colours::Surface, 0.0f, 0.0f,
                                    Colours::Background, 0.0f, 80.0f, false);
    g.setGradientFill(headerGrad);
    g.fillRect(0, 0, getWidth(), 80);

    // Logo text
    g.setFont(VirtuosoLookAndFeel::getVirtuosoFont(22.0f, true));
    g.setColour(Colours::TextPrimary);
    g.drawText("VIRTUOSO", 24, 18, 200, 30, juce::Justification::centredLeft);
    g.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
    g.setColour(Colours::Accent);
    g.drawText("7.1 \u2192 BINAURAL", 24, 46, 200, 16,
               juce::Justification::centredLeft);

    // Separator
    g.setColour(Colours::Border);
    g.drawHorizontalLine(80, 0.0f, (float)getWidth());
  }

  void resized() override {
    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(80); // Header

    m_masterToggle.setBounds(area.removeFromTop(60).reduced(0, 8));
    m_hrirCarousel.setBounds(area.removeFromTop(120).reduced(0, 4));
    m_latencyMonitor.setBounds(area.removeFromBottom(48));
  }

  MasterToggle m_masterToggle;
  HrirCarousel m_hrirCarousel;
  LatencyMonitor m_latencyMonitor;
};

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------
MainWindow::MainWindow(const juce::String &title, AudioEngine &engine,
                       HrirManager &hrirMgr)
    : juce::DocumentWindow(title, Colours::Surface, DocumentWindow::allButtons),
      m_engine(engine), m_hrirMgr(hrirMgr) {
  setUsingNativeTitleBar(false);
  setResizable(false, false);
  setTitleBarHeight(0);

  juce::LookAndFeel::setDefaultLookAndFeel(&m_laf);

  m_contentView = std::make_unique<ContentView>(engine, hrirMgr);
  setContentOwned(m_contentView.release(), true);
  centreWithSize(kWindowW, kWindowH);

  // System tray
  m_systemTray = std::make_unique<VirtuosoSystemTray>(*this);

  setVisible(true);
  startTimerHz(10); // 10 Hz meter refresh
}

MainWindow::~MainWindow() {
  stopTimer();
  juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainWindow::closeButtonPressed() {
  // Hide to tray instead of quitting
  setVisible(false);
}

void MainWindow::buttonClicked(juce::Button *) {}

void MainWindow::timerCallback() {
  // Update meters (engine metrics)
  // Phase 3+: forward CPU / gain reduction to UI components
}

void MainWindow::toggleVisibility() {
  setVisible(!isVisible());
  if (isVisible())
    toFront(true);
}

} // namespace virtuoso
