// SPDX-License-Identifier: MIT
#include "MasterToggle.h"
namespace virtuoso {

MasterToggle::MasterToggle(const juce::String &label) {
  m_label.setText(label, juce::dontSendNotification);
  m_label.setFont(VirtuosoLookAndFeel::getVirtuosoFont(15.0f, true));
  m_label.setColour(juce::Label::textColourId, Colours::TextPrimary);

  m_statusLabel.setText("Active", juce::dontSendNotification);
  m_statusLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
  m_statusLabel.setColour(juce::Label::textColourId, Colours::Success);

  m_toggle.setToggleState(true, juce::dontSendNotification);
  m_toggle.onClick = [this] {
    m_enabled = m_toggle.getToggleState();
    m_statusLabel.setText(m_enabled ? "Active" : "Bypassed",
                          juce::dontSendNotification);
    m_statusLabel.setColour(juce::Label::textColourId,
                            m_enabled ? Colours::Success
                                      : Colours::TextDisabled);
    if (onToggle)
      onToggle(m_enabled);
  };

  addAndMakeVisible(m_label);
  addAndMakeVisible(m_statusLabel);
  addAndMakeVisible(m_toggle);
}

void MasterToggle::paint(juce::Graphics &g) {
  g.setColour(Colours::SurfaceElevated);
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f);
  g.setColour(Colours::Border);
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);
}

void MasterToggle::resized() {
  auto area = getLocalBounds().reduced(16, 8);
  m_toggle.setBounds(area.removeFromRight(50).withSizeKeepingCentre(44, 24));
  m_label.setBounds(area.removeFromTop(area.getHeight() / 2));
  m_statusLabel.setBounds(area);
}

void MasterToggle::setEnabled(bool on) {
  m_enabled = on;
  m_toggle.setToggleState(on, juce::sendNotification);
}

} // namespace virtuoso
