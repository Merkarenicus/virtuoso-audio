// SPDX-License-Identifier: MIT
#pragma once
#include <juce_core/juce_core.h>
namespace virtuoso {

// Ensures only one instance of Virtuoso Audio runs at a time
// Windows: named mutex; macOS/Linux: lock file
class SingleInstance {
public:
  SingleInstance();
  ~SingleInstance();
  bool isOwner() const noexcept { return m_owner; }

private:
  bool m_owner{false};
#if defined(_WIN32)
  void *m_mutex{nullptr};
#else
  int m_lockFd{-1};
  juce::File m_lockFile;
#endif
};

} // namespace virtuoso
