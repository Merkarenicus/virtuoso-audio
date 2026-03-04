// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// SystemTray.h — System tray icon and context menu

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace virtuoso {

class MainWindow;

class VirtuosoSystemTray : public juce::SystemTrayIconComponent {
public:
  explicit VirtuosoSystemTray(MainWindow &window);

  void mouseDown(const juce::MouseEvent &) override;
  void mouseDoubleClick(const juce::MouseEvent &) override;

private:
  MainWindow &m_window;

  void showContextMenu();
  static juce::Image makeTrayIcon(bool active);
};

} // namespace virtuoso
