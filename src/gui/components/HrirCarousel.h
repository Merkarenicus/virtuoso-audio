// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirCarousel.h — Horizontal scrollable HRIR profile picker

#pragma once
#include "../../hrir/HrirManager.h"
#include "../LookAndFeel.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>


namespace virtuoso {

class HrirCarousel : public juce::Component {
public:
  explicit HrirCarousel(HrirManager &hrirMgr);

  void paint(juce::Graphics &) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &) override;

  std::function<void(int selectedIndex)> onSelectionChanged;

  void reload(); // Refresh from bundled + user HRIRs

private:
  HrirManager &m_hrirMgr;
  juce::TextButton m_prevBtn{"<"};
  juce::TextButton m_nextBtn{">"};
  juce::TextButton m_loadFileBtn{"Browse..."};
  juce::Label m_nameLabel;
  juce::Label m_descLabel;

  int m_selectedIndex{0};
  struct HrirEntry {
    std::string name;
    juce::File file;
  };
  std::vector<HrirEntry> m_entries;

  void updateDisplay();
  void loadSelected();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HrirCarousel)
};

} // namespace virtuoso
