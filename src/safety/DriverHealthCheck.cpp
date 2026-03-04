// SPDX-License-Identifier: MIT
#include "DriverHealthCheck.h"

namespace virtuoso {

DriverStatus DriverHealthCheck::check() {
#if defined(_WIN32)
  return checkWindows();
#elif defined(__APPLE__)
  return checkMacOS();
#else
  return checkLinux();
#endif
}

DriverStatus DriverHealthCheck::checkWindows() {
  // TODO Phase 4: query MMDevice API for "Virtuoso Virtual 7.1" in device list
  return {false, false, false, "0.0.0",
          "Windows driver check not yet implemented"};
}

DriverStatus DriverHealthCheck::checkMacOS() {
  return {false, false, false, "0.0.0",
          "macOS driver check not yet implemented"};
}

DriverStatus DriverHealthCheck::checkLinux() {
  return {false, false, false, "0.0.0",
          "Linux driver check not yet implemented"};
}

juce::String DriverHealthCheck::formatReport(const DriverStatus &s) {
  return juce::String("Driver installed: ") + (s.isInstalled ? "YES" : "NO") +
         "\nVisible in OS: " + (s.isVisible ? "YES" : "NO") +
         "\nRouting OK: " + (s.isRoutingCorrect ? "YES" : "NO") +
         "\nVersion: " + juce::String(s.version) +
         (s.errorDetail.empty() ? ""
                                : "\nError: " + juce::String(s.errorDetail));
}

} // namespace virtuoso
