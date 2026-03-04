// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// SystemTray.cpp

#include "SystemTray.h"
#include "LookAndFeel.h"
#include "MainWindow.h"


namespace virtuoso {

VirtuosoSystemTray::VirtuosoSystemTray(MainWindow &window) : m_window(window) {
  setIconImage(makeTrayIcon(true), makeTrayIcon(true));
  setIconTooltip("Virtuoso Audio — Active");
}

void VirtuosoSystemTray::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isRightButtonDown())
    showContextMenu();
  else
    m_window.toggleVisibility();
}

void VirtuosoSystemTray::mouseDoubleClick(const juce::MouseEvent &) {
  m_window.toggleVisibility();
}

void VirtuosoSystemTray::showContextMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "Show / Hide Window");
  menu.addSeparator();
  menu.addItem(2, "Enable Virtuoso", true, true);
  menu.addSeparator();
  menu.addItem(3, "Quit Virtuoso Audio");

  menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
    switch (result) {
    case 1:
      m_window.toggleVisibility();
      break;
    case 3:
      juce::JUCEApplication::getInstance()->systemRequestedQuit();
      break;
    default:
      break;
    }
  });
}

juce::Image VirtuosoSystemTray::makeTrayIcon(bool active) {
  // 32×32 tray icon drawn procedurally — a stylised "V" in a circle
  juce::Image img(juce::Image::ARGB, 32, 32, true);
  juce::Graphics g(img);

  g.setColour(active ? Colours::Accent : Colours::TextDisabled);
  g.fillEllipse(2.0f, 2.0f, 28.0f, 28.0f);

  g.setColour(Colours::TextPrimary);
  g.setFont(VirtuosoLookAndFeel::getVirtuosoFont(16.0f, true));
  g.drawText("V", 0, 0, 32, 32, juce::Justification::centred);
  return img;
}

} // namespace virtuoso
