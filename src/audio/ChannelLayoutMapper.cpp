// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ChannelLayoutMapper.cpp

#include "ChannelLayoutMapper.h"

namespace virtuoso {

MappingResult
ChannelLayoutMapper::fromJuceChannelSet(const juce::AudioChannelSet &layout) {
  MappingResult result;
  result.sourceChannelCount = layout.size();

  // Map each canonical channel to its index in the source layout
  auto map = [&](CanonicalChannel canonical,
                 juce::AudioChannelSet::ChannelType juceType) {
    int idx = layout.getChannelIndexForType(juceType);
    result.map[static_cast<int>(canonical)] = idx; // -1 if not present
  };

  map(CanonicalChannel::FL, juce::AudioChannelSet::left);
  map(CanonicalChannel::FR, juce::AudioChannelSet::right);
  map(CanonicalChannel::C, juce::AudioChannelSet::centre);
  map(CanonicalChannel::LFE, juce::AudioChannelSet::LFE);
  map(CanonicalChannel::BL, juce::AudioChannelSet::leftSurroundRear);
  map(CanonicalChannel::BR, juce::AudioChannelSet::rightSurroundRear);
  map(CanonicalChannel::SL, juce::AudioChannelSet::leftSurround);
  map(CanonicalChannel::SR, juce::AudioChannelSet::rightSurround);

  result.requiresUpmix = (result.sourceChannelCount < 8);
  return result;
}

MappingResult ChannelLayoutMapper::fromWindowsChannelMask(uint32_t mask,
                                                          int numChannels) {
  MappingResult result;
  result.sourceChannelCount = numChannels;

  // Walk the mask bit by bit to get channel ordering
  // Windows channels are assigned in ascending mask-bit order
  auto assignChannel = [&](CanonicalChannel canonical, uint32_t maskBit) {
    if (!(mask & maskBit)) {
      result.map[static_cast<int>(canonical)] = -1;
      return;
    }
    // Count how many set bits precede this bit — that's the channel index
    uint32_t lower = mask & (maskBit - 1);
    int idx = 0;
    while (lower) {
      idx += (lower & 1);
      lower >>= 1;
    }
    result.map[static_cast<int>(canonical)] = idx;
  };

  assignChannel(CanonicalChannel::FL, SPEAKER_FRONT_LEFT);
  assignChannel(CanonicalChannel::FR, SPEAKER_FRONT_RIGHT);
  assignChannel(CanonicalChannel::C, SPEAKER_FRONT_CENTER);
  assignChannel(CanonicalChannel::LFE, SPEAKER_LOW_FREQUENCY);
  assignChannel(CanonicalChannel::BL, SPEAKER_BACK_LEFT);
  assignChannel(CanonicalChannel::BR, SPEAKER_BACK_RIGHT);
  assignChannel(CanonicalChannel::SL, SPEAKER_SIDE_LEFT);
  assignChannel(CanonicalChannel::SR, SPEAKER_SIDE_RIGHT);

  result.requiresUpmix = (numChannels < 8);
  return result;
}

MappingResult ChannelLayoutMapper::fromCoreAudioLayout(uint32_t layoutTag,
                                                       int numChannels) {
  // CoreAudio kAudioChannelLayoutTag_xxx — handle most common cases
  // kAudioChannelLayoutTag_MPEG_7_1_A = 173 (FL,FR,C,LFE,BL,BR,SL,SR)
  // kAudioChannelLayoutTag_MPEG_5_1_A = 100 (FL,FR,C,LFE,BL,BR)
  // kAudioChannelLayoutTag_Stereo     = 6656 (FL,FR)

  MappingResult result;
  result.sourceChannelCount = numChannels;

  constexpr uint32_t kStereo = 6656; // kAudioChannelLayoutTag_Stereo
  constexpr uint32_t k5_1_A = 100;   // kAudioChannelLayoutTag_MPEG_5_1_A
  constexpr uint32_t k7_1_A = 173;   // kAudioChannelLayoutTag_MPEG_7_1_A

  if (layoutTag == k7_1_A || numChannels == 8) {
    // Standard 7.1 — straight mapping
    for (int i = 0; i < 8; ++i)
      result.map[i] = i;
    result.requiresUpmix = false;
  } else if (layoutTag == k5_1_A || numChannels == 6) {
    result.map = {0, 1, 2, 3, 4, 5, -1, -1}; // FL FR C LFE BL BR (no SL/SR)
    result.requiresUpmix = true;
  } else {
    // Stereo or unknown — FL/FR only
    result.map = {0, 1, -1, -1, -1, -1, -1, -1};
    result.requiresUpmix = true;
  }

  return result;
}

void ChannelLayoutMapper::remap(const juce::AudioBuffer<float> &src,
                                juce::AudioBuffer<float> &dst,
                                const ChannelMap &map,
                                int numSamples) noexcept {
  jassert(dst.getNumChannels() == 8);

  for (int outCh = 0; outCh < 8; ++outCh) {
    float *out = dst.getWritePointer(outCh);
    int srcCh = map[outCh];
    if (srcCh < 0 || srcCh >= src.getNumChannels()) {
      juce::FloatVectorOperations::clear(out, numSamples);
    } else {
      juce::FloatVectorOperations::copy(out, src.getReadPointer(srcCh),
                                        numSamples);
    }
  }
}

} // namespace virtuoso
