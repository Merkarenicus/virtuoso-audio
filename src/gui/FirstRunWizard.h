// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// FirstRunWizard.h — Modal first-run setup wizard
// Guides the user through:
//   Step 1: Driver installation check (and repair prompt if driver missing)
//   Step 2: HRIR selection (bundled or browse)
//   Step 3: Output device selection
//   Step 4: Quick listen test with pass/fail

#pragma once
#include "../audio/AudioEngine.h"
#include "../gui/LookAndFeel.h"
#include "../hrir/HrirManager.h"
#include "../safety/DriverHealthCheck.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>


namespace virtuoso {

class FirstRunWizard : public juce::Component, public juce::Button::Listener {
public:
  // Called with true when wizard completes, false if user cancels
  using CompletionCallback = std::function<void(bool completed)>;

  FirstRunWizard(AudioEngine &engine, HrirManager &hrirMgr,
                 CompletionCallback onComplete);
  ~FirstRunWizard() override = default;

  void paint(juce::Graphics &) override;
  void resized() override;
  void buttonClicked(juce::Button *b) override;

  // Show as modal dialog
  static void runModal(juce::Component &parent, AudioEngine &engine,
                       HrirManager &hrirMgr, CompletionCallback cb);

private:
  enum class Step {
    DriverCheck,
    HrirSelect,
    DeviceSelect,
    ListenTest,
    Complete
  };

  AudioEngine &m_engine;
  HrirManager &m_hrirMgr;
  CompletionCallback m_onComplete;

  VirtuosoLookAndFeel m_laf;
  Step m_currentStep{Step::DriverCheck};

  // Shared nav buttons
  juce::TextButton m_nextBtn{"Next →"}, m_backBtn{"← Back"},
      m_cancelBtn{"Cancel"};
  juce::Label m_stepLabel, m_titleLabel, m_bodyLabel;
  juce::Label m_stepCountLabel;

  // Step-specific controls
  juce::TextButton m_installDriverBtn{"Install Driver"};
  juce::TextButton m_listentestBtn{"Play Test Tone"};
  juce::ComboBox m_outputDeviceCombo;

  // Step progression
  void showStep(Step step);
  void paintStepIndicator(juce::Graphics &g);
  bool checkDriver();
  void advanceToNext();
  void markWizardComplete();

  static constexpr int kTotalSteps = 4;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FirstRunWizard)
};

} // namespace virtuoso
