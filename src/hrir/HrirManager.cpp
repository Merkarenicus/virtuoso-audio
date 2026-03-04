// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirManager.cpp

#include "HrirManager.h"
#include "../logging/Logger.h"

namespace virtuoso {

void HrirManager::initialise(ConvolverProcessor *convolver,
                             double engineSampleRate) {
  m_convolver = convolver;
  m_engineSampleRate = engineSampleRate;
}

HrirLoadResult HrirManager::loadAsync(
    const juce::File &hrirFile,
    std::function<void(HrirLoadResult, const juce::String &)> onComplete) {
  m_cancelled.store(false, std::memory_order_relaxed);

  m_parserWorker.parseAsync(hrirFile, [this,
                                       onComplete](HrirParseResult result) {
    if (m_cancelled.load(std::memory_order_relaxed)) {
      if (onComplete)
        onComplete(HrirLoadResult::Cancelled, "Cancelled");
      return;
    }
    if (!result.has_value()) {
      VLOG_ERROR("HrirManager", "Parse failed: " + result.error());
      if (onComplete)
        onComplete(HrirLoadResult::ParseFailed, juce::String(result.error()));
      return;
    }
    auto resampled = resampleIfNeeded(std::move(*result));
    if (!resampled.has_value()) {
      if (onComplete)
        onComplete(HrirLoadResult::ResampleFailed,
                   juce::String(resampled.error()));
      return;
    }
    m_currentName = resampled->name;
    if (m_convolver)
      m_convolver->loadHrir(*resampled);
    m_loaded.store(true, std::memory_order_release);
    VLOG_INFO("HrirManager", "HRIR loaded: " + m_currentName.toStdString());
    if (onComplete)
      onComplete(HrirLoadResult::Success, {});
  });
  return HrirLoadResult::Success; // Async: result delivered via callback
}

HrirLoadResult HrirManager::loadSync(const juce::File &hrirFile,
                                     juce::String &errorOut) {
  auto result = m_parserWorker.parseSync(hrirFile);
  if (!result.has_value()) {
    errorOut = juce::String(result.error());
    return HrirLoadResult::ParseFailed;
  }
  auto resampled = resampleIfNeeded(std::move(*result));
  if (!resampled.has_value()) {
    errorOut = juce::String(resampled.error());
    return HrirLoadResult::ResampleFailed;
  }
  m_currentName = resampled->name;
  if (m_convolver)
    m_convolver->loadHrir(*resampled);
  m_loaded.store(true, std::memory_order_release);
  return HrirLoadResult::Success;
}

HrirLoadResult HrirManager::loadDefault() {
  auto bundled = getBundledHrirs();
  if (bundled.empty())
    return HrirLoadResult::ParseFailed;

  juce::String err;
  return loadSync(bundled[0].second, err);
}

std::vector<std::pair<std::string, juce::File>> HrirManager::getBundledHrirs() {
  // Bundled HRIRs are in assets/hrir/ (relative to executable)
  juce::File assetDir =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory()
          .getChildFile("../share/virtuoso-audio/hrir");
  if (!assetDir.isDirectory())
    assetDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                   .getParentDirectory()
                   .getChildFile("assets/hrir");

  std::vector<std::pair<std::string, juce::File>> result;
  juce::Array<juce::File> hrirFiles;
  assetDir.findChildFiles(hrirFiles, juce::File::findFiles, false,
                          "*.wav;*.sofa");
  for (auto &f : hrirFiles)
    result.emplace_back(f.getFileNameWithoutExtension().toStdString(), f);

  return result;
}

void HrirManager::cancelLoad() {
  m_cancelled.store(true, std::memory_order_release);
  m_parserWorker.shutdown();
}

std::expected<HrirSet, std::string>
HrirManager::resampleIfNeeded(HrirSet hrirSet) const {
  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    auto &pair = hrirSet.channels[s];
    if (std::abs(pair.sampleRate - m_engineSampleRate) < 0.5)
      continue;

    // Resample IR using libsamplerate
    const double ratio = m_engineSampleRate / pair.sampleRate;
    const int oldLen = pair.left.getNumSamples();
    const int newLen = static_cast<int>(std::ceil(oldLen * ratio));

    juce::AudioBuffer<float> newLeft(1, newLen), newRight(1, newLen);
    ResamplerProcessor rsL, rsR;
    rsL.prepare(pair.sampleRate, m_engineSampleRate, 1);
    rsR.prepare(pair.sampleRate, m_engineSampleRate, 1);

    juce::AudioBuffer<float> outL(1, newLen), outR(1, newLen);
    rsL.process(pair.left, outL, oldLen);
    rsR.process(pair.right, outR, oldLen);

    pair.left = outL;
    pair.right = outR;
    pair.sampleRate = m_engineSampleRate;
  }
  return hrirSet;
}

} // namespace virtuoso
