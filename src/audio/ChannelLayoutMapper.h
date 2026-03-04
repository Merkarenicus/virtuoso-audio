// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ChannelLayoutMapper.h
// Maps platform channel masks to Virtuoso's canonical 8-channel order:
//   Ch 0=FL, 1=FR, 2=C, 3=LFE, 4=BL, 5=BR, 6=SL, 7=SR

#pragma once
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>


namespace virtuoso {

// Canonical 8-channel layout
enum class CanonicalChannel : int {
  FL = 0,
  FR = 1,
  C = 2,
  LFE = 3,
  BL = 4,
  BR = 5,
  SL = 6,
  SR = 7,
  Count = 8
};

// Mapping: output[canonicalIndex] = source channel index (-1 = not present /
// silence)
using ChannelMap = std::array<int, static_cast<int>(CanonicalChannel::Count)>;

struct MappingResult {
  ChannelMap map;
  int sourceChannelCount{0}; // actual channels present in input
  bool requiresUpmix{false}; // true if source < 8 channels
};

class ChannelLayoutMapper {
public:
  ChannelLayoutMapper() = default;

  // Create mapping from a JUCE AudioChannelSet
  static MappingResult fromJuceChannelSet(const juce::AudioChannelSet &layout);

  // Create mapping from Windows WAVEFORMATEXTENSIBLE dwChannelMask
  static MappingResult fromWindowsChannelMask(uint32_t dwChannelMask,
                                              int numChannels);

  // Create mapping from macOS AudioChannelLayout tag
  static MappingResult fromCoreAudioLayout(uint32_t layoutTag, int numChannels);

  // Apply mapping: reorder src into dst (canonical 8-channel output)
  // dst must have exactly 8 channels; src.numChannels == map.sourceChannelCount
  // Channels not present in src are cleared (silence).
  static void remap(const juce::AudioBuffer<float> &src,
                    juce::AudioBuffer<float> &dst, const ChannelMap &map,
                    int numSamples) noexcept;

private:
  // Windows dwChannelMask bit positions
  static constexpr uint32_t SPEAKER_FRONT_LEFT = 0x1;
  static constexpr uint32_t SPEAKER_FRONT_RIGHT = 0x2;
  static constexpr uint32_t SPEAKER_FRONT_CENTER = 0x4;
  static constexpr uint32_t SPEAKER_LOW_FREQUENCY = 0x8;
  static constexpr uint32_t SPEAKER_BACK_LEFT = 0x10;
  static constexpr uint32_t SPEAKER_BACK_RIGHT = 0x20;
  static constexpr uint32_t SPEAKER_SIDE_LEFT = 0x200;
  static constexpr uint32_t SPEAKER_SIDE_RIGHT = 0x400;
  static constexpr uint32_t SPEAKER_7POINT1_SURROUND =
      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
      SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT |
      SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;
};

} // namespace virtuoso
