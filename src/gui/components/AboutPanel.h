// SPDX-License-Identifier: MIT
#pragma once
#include "../LookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace virtuoso {

class AboutPanel : public juce::Component {
public:
  AboutPanel() {
    m_title.setText("Virtuoso Audio", juce::dontSendNotification);
    m_title.setFont(VirtuosoLookAndFeel::getVirtuosoFont(18.0f, true));
    m_title.setColour(juce::Label::textColourId, Colours::TextPrimary);
    m_title.setJustificationType(juce::Justification::centred);

    m_version.setText("Version 1.0.0 — Phase 3 Build",
                      juce::dontSendNotification);
    m_version.setFont(VirtuosoLookAndFeel::getVirtuosoFont(12.0f));
    m_version.setColour(juce::Label::textColourId, Colours::TextSecondary);
    m_version.setJustificationType(juce::Justification::centred);

    m_license.setText("MIT License | JUCE Commercial License required",
                      juce::dontSendNotification);
    m_license.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
    m_license.setColour(juce::Label::textColourId, Colours::TextDisabled);
    m_license.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(m_title);
    addAndMakeVisible(m_version);
    addAndMakeVisible(m_license);
  }

  void paint(juce::Graphics &g) override {
    g.setColour(Colours::Surface);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(16);
    m_title.setBounds(area.removeFromTop(36));
    m_version.setBounds(area.removeFromTop(24));
    m_license.setBounds(area.removeFromTop(20));
  }

private:
  juce::Label m_title, m_version, m_license;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPanel)
};

} // namespace virtuoso
