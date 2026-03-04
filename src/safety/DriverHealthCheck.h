// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// DriverHealthCheck.h — Runtime verification that Virtuoso virtual drivers are
// operational

#pragma once
#include <juce_core/juce_core.h>
#include <optional>
#include <string>


namespace virtuoso {

struct DriverStatus {
  bool isInstalled{false};
  bool isVisible{false};        // Appears in OS audio panel
  bool isRoutingCorrect{false}; // Can receive audio
  std::string version;
  std::string errorDetail;
};

class DriverHealthCheck {
public:
  // Blocking check — call from setup/installer, not audio thread
  static DriverStatus checkWindows(); // SysVAD MME device presence
  static DriverStatus checkMacOS(); // AudioServerPlugIn HAL device enumeration
  static DriverStatus
  checkLinux(); // PipeWire null-sink / PulseAudio module check

  // Platform-dispatch wrapper
  static DriverStatus check();

  // Returns human-readable report for support bundle
  static juce::String formatReport(const DriverStatus &status);
};

} // namespace virtuoso
