// SPDX-License-Identifier: MIT
#pragma once
#include <functional>
#include <juce_core/juce_core.h>

namespace virtuoso {

// Progress tracking model for offline export operations
class ExportProgressModel {
public:
  using ProgressCallback =
      std::function<void(float fraction, const juce::String &statusMessage)>;

  void setProgress(float fraction) {
    m_fraction = juce::jlimit(0.0f, 1.0f, fraction);
    if (m_callback)
      m_callback(m_fraction, m_status);
  }
  void setStatus(const juce::String &status) { m_status = status; }
  void setCallback(ProgressCallback cb) { m_callback = std::move(cb); }
  float getProgress() const noexcept { return m_fraction; }
  bool isComplete() const noexcept { return m_fraction >= 1.0f; }

private:
  float m_fraction{0.0f};
  juce::String m_status;
  ProgressCallback m_callback;
};

} // namespace virtuoso
