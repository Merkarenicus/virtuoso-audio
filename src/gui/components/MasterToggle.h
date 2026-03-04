// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// MasterToggle.h — Main power button + status display

#pragma once
#include "../../audio/AudioEngine.h"
#include "../LookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace virtuoso {

class MasterToggle : public juce::Component {
public:
  explicit MasterToggle(const juce::String &label = "Virtuoso Audio");

  void paint(juce::Graphics &) override;
  void resized() override;

  void setEnabled(bool on);
  bool getEnabled() const noexcept { return m_enabled; }

  std::function<void(bool)> onToggle;

private:
  juce::ToggleButton m_toggle;
  juce::Label m_label;
  juce::Label m_statusLabel;
  bool m_enabled{true};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterToggle)
};

} // namespace virtuoso
