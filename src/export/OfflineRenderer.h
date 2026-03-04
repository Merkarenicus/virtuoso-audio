// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// OfflineRenderer.h
// Bit-exact offline renderer using the SAME DSP objects as the real-time path.
// Ensures RMS ≤ 0.05 dB parity with real-time output.
//
// Key design decisions (per ADR-003):
//   - Uses the same ConvolverProcessor instance as real-time (after reset())
//   - Processes identical-length blocks to ensure identical FFT partitioning
//   - Progress + cancellation via ExportProgressModel (thread-safe)

#pragma once
#include "../export/ExportProgressModel.h"
#include "BassManagement.h"
#include "ConvolverProcessor.h"
#include "EqProcessor.h"
#include "LimiterProcessor.h"
#include "UpmixProcessor.h"
#include <atomic>
#include <functional>
#include <juce_audio_formats/juce_audio_formats.h>


namespace virtuoso {

struct OfflineRenderOptions {
  juce::File inputFile;
  juce::File outputFile;
  std::shared_ptr<const HrirSet> hrirSet;
  double outputSampleRate{48000.0};
  int blockSize{4096};              // Must match partition size for parity
  juce::String outputFormat{"wav"}; // "wav" or "flac"
  int outputBitDepth{24};
};

class OfflineRenderer {
public:
  OfflineRenderer() = default;

  // Render blocking (intended for background thread / worker).
  // Returns true on success, false on failure or cancellation.
  // Progress and cancellation via ExportProgressModel.
  bool render(const OfflineRenderOptions &opts, ExportProgressModel &progress);

  // Last error message (valid if render() returned false)
  juce::String lastError() const { return m_lastError; }

private:
  juce::String m_lastError;

  bool setupProcessors(const OfflineRenderOptions &opts,
                       ConvolverProcessor &convolver, UpmixProcessor &upmixer,
                       BassManagement &bass, EqProcessor &eq,
                       LimiterProcessor &limiter,
                       const juce::dsp::ProcessSpec &spec);
};

} // namespace virtuoso
