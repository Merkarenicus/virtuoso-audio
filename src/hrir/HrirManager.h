// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirManager.h
// High-level HRIR lifecycle manager.
// Coordinates async loading via HrirParserWorker, optional resampling via
// HrirResampler, and delivery to ConvolverProcessor.

#pragma once
#include "../audio/ConvolverProcessor.h"
#include "HrirParserWorker.h"
#include "SofaHrirParser.h"
#include <atomic>
#include <functional>
#include <juce_core/juce_core.h>
#include <memory>


namespace virtuoso {

enum class HrirLoadResult {
  Success,
  Cancelled,
  ParseFailed,
  ValidationFailed,
  ResampleFailed
};

class HrirManager {
public:
  HrirManager() = default;

  // Called after constructing — sets target convolver and engine sample rate
  void initialise(ConvolverProcessor *convolver, double engineSampleRate);

  // ---------------------------------------------------------------------------
  // Async load (calls back on message thread)
  // ---------------------------------------------------------------------------
  void
  loadAsync(const juce::File &hrirFile,
            std::function<void(HrirLoadResult, const juce::String &errorMsg)>
                onComplete);

  // Cancel any in-progress load
  void cancelLoad();

  // ---------------------------------------------------------------------------
  // Sync load — for offline renderer use only
  // ---------------------------------------------------------------------------
  HrirLoadResult loadSync(const juce::File &hrirFile, juce::String &errorOut);

  // ---------------------------------------------------------------------------
  // Bundled defaults (shipped with Virtuoso)
  // ---------------------------------------------------------------------------
  // Returns a list of bundled HRIR names
  static std::vector<std::pair<std::string, juce::File>> getBundledHrirs();

  // Load the default bundled HRIR (MIT KEMAR)
  HrirLoadResult loadDefault();

  // Returns the currently loaded HRIR name
  juce::String currentHrirName() const { return m_currentName; }
  bool isLoaded() const { return m_loaded.load(std::memory_order_relaxed); }

private:
  ConvolverProcessor *m_convolver{nullptr};
  HrirParserWorker m_parserWorker;
  double m_engineSampleRate{48000.0};
  juce::String m_currentName;
  std::atomic<bool> m_loaded{false};
  std::atomic<bool> m_cancelled{false};

  // Resample loaded HRIR to engine sample rate if needed
  std::expected<HrirSet, std::string> resampleIfNeeded(HrirSet hrirSet) const;
};

} // namespace virtuoso
