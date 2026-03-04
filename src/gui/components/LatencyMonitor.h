// SPDX-License-Identifier: MIT
#pragma once
#include "../../audio/AudioEngine.h"
#include "../LookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace virtuoso {

// Real-time latency and CPU load meter strip
class LatencyMonitor : public juce::Component {
public:
  LatencyMonitor() {
    m_cpuLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
    m_cpuLabel.setColour(juce::Label::textColourId, Colours::TextSecondary);
    m_latLabel.setFont(VirtuosoLookAndFeel::getVirtuosoFont(11.0f));
    m_latLabel.setColour(juce::Label::textColourId, Colours::TextSecondary);
    addAndMakeVisible(m_cpuLabel);
    addAndMakeVisible(m_latLabel);
  }

  void update(float cpu, float latMs) {
    m_cpuLabel.setText("CPU " + juce::String(cpu, 1) + "%",
                       juce::dontSendNotification);
    m_latLabel.setText("Lat " + juce::String(latMs, 1) + " ms",
                       juce::dontSendNotification);
  }

  void resized() override {
    auto area = getLocalBounds();
    m_cpuLabel.setBounds(
        area.removeFromLeft(area.getWidth() / 2).reduced(4, 0));
    m_latLabel.setBounds(area.reduced(4, 0));
  }

  void paint(juce::Graphics &g) override {
    g.setColour(Colours::Border);
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());
  }

private:
  juce::Label m_cpuLabel{"", "CPU --%"}, m_latLabel{"", "Lat -- ms"};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyMonitor)
};

} // namespace virtuoso
