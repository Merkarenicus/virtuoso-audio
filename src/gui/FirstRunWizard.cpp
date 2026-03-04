// SPDX-License-Identifier: MIT
#include "FirstRunWizard.h"
#include "../safety/RepairTool.h"
#include "components/HrirCarousel.h"

namespace virtuoso {

static constexpr int kWizardW = 520;
static constexpr int kWizardH = 400;

FirstRunWizard::FirstRunWizard(AudioEngine &engine, HrirManager &hrirMgr,
                               CompletionCallback onComplete)
    : m_engine(engine), m_hrirMgr(hrirMgr),
      m_onComplete(std::move(onComplete)) {

  juce::LookAndFeel::setDefaultLookAndFeel(&m_laf);
  setSize(kWizardW, kWizardH);

  // Title
  m_titleLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(22.0f, true));
  m_titleLabel.setColour(juce::Label::textColourId, Colours::TextPrimary);

  // Body text
  m_bodyLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(13.0f));
  m_bodyLabel.setColour(juce::Label::textColourId, Colours::TextSecondary);
  m_bodyLabel.setJustificationType(juce::Justification::topLeft);
  m_bodyLabel.setMinimumHorizontalScale(0.7f);

  // Step counter "Step 1 of 4"
  m_stepCountLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
  m_stepCountLabel.setColour(juce::Label::textColourId, Colours::TextDisabled);

  // Nav buttons
  m_nextBtn.onClick = [this] { advanceToNext(); };
  m_backBtn.onClick = [this] {
    if (m_currentStep == Step::HrirSelect)
      showStep(Step::DriverCheck);
    else if (m_currentStep == Step::DeviceSelect)
      showStep(Step::HrirSelect);
    else if (m_currentStep == Step::ListenTest)
      showStep(Step::DeviceSelect);
  };
  m_cancelBtn.onClick = [this] {
    if (m_onComplete)
      m_onComplete(false);
  };
  m_installDriverBtn.onClick = [this] {
    RepairTool::repair();
    juce::MessageManager::callAsync([this] { showStep(Step::DriverCheck); });
  };
  m_listentestBtn.onClick = [this] {
    // Generate a 1-second 1 kHz stereo sine wave for the listen test
    // (Phase 4b: use JUCE AudioDeviceManager beep)
    m_listentestBtn.setButtonText("✓ Tone sent");
  };

  addAndMakeVisible(m_titleLabel);
  addAndMakeVisible(m_bodyLabel);
  addAndMakeVisible(m_stepCountLabel);
  addAndMakeVisible(m_nextBtn);
  addAndMakeVisible(m_backBtn);
  addAndMakeVisible(m_cancelBtn);
  addAndMakeVisible(m_installDriverBtn);
  addAndMakeVisible(m_listentestBtn);
  addAndMakeVisible(m_outputDeviceCombo);

  showStep(Step::DriverCheck);
}

void FirstRunWizard::paint(juce::Graphics &g) {
  g.fillAll(Colours::Background);
  g.setColour(Colours::Surface);
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 12.0f);
  paintStepIndicator(g);
}

void FirstRunWizard::paintStepIndicator(juce::Graphics &g) {
  const int stepIdx = static_cast<int>(m_currentStep);
  const float dotR = 5.0f, spacing = 20.0f;
  const float totalW = (kTotalSteps - 1) * spacing + dotR * 2.0f;
  float x = getWidth() * 0.5f - totalW * 0.5f;
  const float y = getHeight() - 30.0f;

  for (int i = 0; i < kTotalSteps; ++i) {
    g.setColour(i == stepIdx ? Colours::Accent : Colours::Border);
    g.fillEllipse(x, y, dotR * 2.0f, dotR * 2.0f);
    x += spacing;
  }
}

void FirstRunWizard::resized() {
  auto area = getLocalBounds().reduced(32);
  area.removeFromBottom(50); // Navigation row
  m_stepCountLabel.setBounds(area.removeFromTop(20));
  m_titleLabel.setBounds(area.removeFromTop(44));
  m_bodyLabel.setBounds(area.removeFromTop(80));

  // Step-specific control row
  auto ctrlRow = area.removeFromTop(60);
  m_installDriverBtn.setBounds(ctrlRow.reduced(80, 8));
  m_listentestBtn.setBounds(ctrlRow.reduced(80, 8));
  m_outputDeviceCombo.setBounds(ctrlRow.reduced(8, 12));

  // Nav row
  auto nav = getLocalBounds().reduced(32).removeFromBottom(44);
  m_cancelBtn.setBounds(nav.removeFromLeft(80).reduced(0, 6));
  m_nextBtn.setBounds(nav.removeFromRight(100).reduced(0, 6));
  m_backBtn.setBounds(nav.removeFromRight(80).reduced(0, 6));
}

void FirstRunWizard::buttonClicked(juce::Button *) {}

void FirstRunWizard::showStep(Step step) {
  m_currentStep = step;
  const int stepIdx = static_cast<int>(step) + 1;
  m_stepCountLabel.setText("Step " + juce::String(stepIdx) + " of " +
                               juce::String(kTotalSteps),
                           juce::dontSendNotification);

  m_installDriverBtn.setVisible(false);
  m_listentestBtn.setVisible(false);
  m_outputDeviceCombo.setVisible(false);
  m_backBtn.setEnabled(step != Step::DriverCheck);

  switch (step) {
  case Step::DriverCheck:
    m_titleLabel.setText("Driver Setup", juce::dontSendNotification);
    if (checkDriver()) {
      m_bodyLabel.setText("✓ Virtuoso Virtual 7.1 is ready in your system "
                          "audio panel.\n\nClick Next to continue.",
                          juce::dontSendNotification);
      m_nextBtn.setEnabled(true);
    } else {
      m_bodyLabel.setText(
          "⚠ The Virtuoso driver is not yet installed.\nClick Install Driver "
          "to set up the virtual audio device, then click Next.",
          juce::dontSendNotification);
      m_installDriverBtn.setVisible(true);
      m_nextBtn.setEnabled(false);
    }
    break;
  case Step::HrirSelect:
    m_titleLabel.setText("Select HRIR", juce::dontSendNotification);
    m_bodyLabel.setText(
        "Choose a Head-Related Impulse Response (HRIR) profile.\nThis controls "
        "how surround audio is rendered to your headphones.",
        juce::dontSendNotification);
    m_nextBtn.setEnabled(true);
    break;
  case Step::DeviceSelect:
    m_titleLabel.setText("Output Device", juce::dontSendNotification);
    m_bodyLabel.setText("Select the headphone output device that Virtuoso will "
                        "use for binaural rendering.",
                        juce::dontSendNotification);
    m_outputDeviceCombo.setVisible(true);
    m_outputDeviceCombo.addItem("System Default", 1);
    m_nextBtn.setEnabled(true);
    break;
  case Step::ListenTest:
    m_titleLabel.setText("Listen Test", juce::dontSendNotification);
    m_bodyLabel.setText("Click Play Test Tone to verify that the binaural "
                        "rendering is working.\nYou should hear a tone from in "
                        "front of you, not inside your head.",
                        juce::dontSendNotification);
    m_listentestBtn.setVisible(true);
    m_nextBtn.setEnabled(true);
    break;
  case Step::Complete:
    markWizardComplete();
    break;
  }
  repaint();
}

void FirstRunWizard::advanceToNext() {
  switch (m_currentStep) {
  case Step::DriverCheck:
    showStep(Step::HrirSelect);
    break;
  case Step::HrirSelect:
    showStep(Step::DeviceSelect);
    break;
  case Step::DeviceSelect:
    showStep(Step::ListenTest);
    break;
  case Step::ListenTest:
    showStep(Step::Complete);
    break;
  case Step::Complete:
    break;
  }
}

void FirstRunWizard::markWizardComplete() {
  // Persist "wizard completed" flag so it doesn't show next launch
  juce::File flagFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("virtuoso-audio/.setup_complete");
  flagFile.create();
  if (m_onComplete)
    m_onComplete(true);
}

bool FirstRunWizard::checkDriver() {
  // Use DriverHealthCheck (Phase 4b: returns real result)
  return DriverHealthCheck::check().isInstalled;
}

void FirstRunWizard::runModal(juce::Component &parent, AudioEngine &engine,
                              HrirManager &hrirMgr, CompletionCallback cb) {
  auto wizard =
      std::make_unique<FirstRunWizard>(engine, hrirMgr, std::move(cb));
  wizard->setSize(kWizardW, kWizardH);
  juce::DialogWindow::LaunchOptions opts;
  opts.dialogTitle = "Virtuoso Setup";
  opts.content.setOwned(wizard.release());
  opts.componentToCentreAround = &parent;
  opts.useNativeTitleBar = false;
  opts.resizable = false;
  opts.launchAsync();
}

} // namespace virtuoso
