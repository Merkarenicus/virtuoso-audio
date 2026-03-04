// SPDX-License-Identifier: MIT
#include "HrirCarousel.h"
namespace virtuoso {

HrirCarousel::HrirCarousel(HrirManager &hrirMgr) : m_hrirMgr(hrirMgr) {
  m_nameLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(14.0f, true));
  m_nameLabel.setColour(juce::Label::textColourId, Colours::TextPrimary);
  m_nameLabel.setJustificationType(juce::Justification::centred);

  m_descLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
  m_descLabel.setColour(juce::Label::textColourId, Colours::TextSecondary);
  m_descLabel.setJustificationType(juce::Justification::centred);

  m_prevBtn.onClick = [this] {
    if (m_selectedIndex > 0) {
      --m_selectedIndex;
      updateDisplay();
      loadSelected();
    }
  };
  m_nextBtn.onClick = [this] {
    if (m_selectedIndex + 1 < (int)m_entries.size()) {
      ++m_selectedIndex;
      updateDisplay();
      loadSelected();
    }
  };
  m_loadFileBtn.onClick = [this] {
    juce::FileChooser fc("Open HRIR File", {}, "*.wav;*.sofa");
    if (fc.browseForFileToOpen()) {
      auto f = fc.getResult();
      m_entries.push_back({f.getFileNameWithoutExtension().toStdString(), f});
      m_selectedIndex = static_cast<int>(m_entries.size()) - 1;
      updateDisplay();
      loadSelected();
    }
  };

  addAndMakeVisible(m_prevBtn);
  addAndMakeVisible(m_nextBtn);
  addAndMakeVisible(m_loadFileBtn);
  addAndMakeVisible(m_nameLabel);
  addAndMakeVisible(m_descLabel);

  reload();
}

void HrirCarousel::paint(juce::Graphics &g) {
  g.setColour(Colours::Surface);
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f);
  g.setColour(Colours::Border);
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);
}

void HrirCarousel::resized() {
  auto area = getLocalBounds().reduced(8);
  auto navRow = area.removeFromTop(36);
  m_prevBtn.setBounds(navRow.removeFromLeft(36).withSizeKeepingCentre(32, 28));
  m_nextBtn.setBounds(navRow.removeFromRight(36).withSizeKeepingCentre(32, 28));
  m_nameLabel.setBounds(navRow);
  m_descLabel.setBounds(area.removeFromTop(24));
  m_loadFileBtn.setBounds(area.removeFromBottom(28).reduced(50, 0));
}

void HrirCarousel::mouseDown(const juce::MouseEvent &) {}

void HrirCarousel::reload() {
  m_entries.clear();
  for (auto &[name, file] : HrirManager::getBundledHrirs())
    m_entries.push_back({name, file});
  m_selectedIndex = 0;
  updateDisplay();
}

void HrirCarousel::updateDisplay() {
  if (m_entries.empty()) {
    m_nameLabel.setText("No HRIRs available", juce::dontSendNotification);
    m_descLabel.setText("Browse to load a .wav or .sofa file",
                        juce::dontSendNotification);
  } else {
    m_nameLabel.setText(m_entries[m_selectedIndex].name,
                        juce::dontSendNotification);
    m_descLabel.setText(juce::String(m_selectedIndex + 1) + " / " +
                            juce::String((int)m_entries.size()),
                        juce::dontSendNotification);
  }
  if (onSelectionChanged)
    onSelectionChanged(m_selectedIndex);
}

void HrirCarousel::loadSelected() {
  if (m_entries.empty())
    return;
  juce::String err;
  m_hrirMgr.loadSync(m_entries[m_selectedIndex].file, err);
}

} // namespace virtuoso
