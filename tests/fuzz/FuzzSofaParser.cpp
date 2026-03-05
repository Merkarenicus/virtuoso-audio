// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// FuzzSofaParser.cpp — libFuzzer harness for SofaHrirParser
// Fuzzes arbitrary byte sequences through the SOFA parser (libmysofa).
// Any crash, OOB read, or sanitizer report = security-relevant bug.
//
// Run: ./FuzzSofaParser -max_total_time=300 tests/fuzz/corpus/FuzzSofaParser/

#include "../../src/hrir/HrirValidator.h"
#include "../../src/hrir/SofaHrirParser.h"
#include <cstddef>
#include <cstdint>
#include <juce_core/juce_core.h>


using namespace virtuoso;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Fuzz input must have minimum size to avoid trivial rejections in
  // mysofa_open
  if (size < 8)
    return 0;

  juce::File tmp = juce::File::createTempFile(".fuzz.sofa");

  {
    auto stream = tmp.createOutputStream();
    if (!stream || !stream->openedOk())
      return 0;
    stream->write(data, size);
  }

  SofaHrirParser parser;
  auto result = parser.parse(tmp);

  // Validate any successfully-parsed SOFA data
  if (result.has_value()) {
    for (int s = 0; s < HrirSet::kSpeakerCount; ++s)
      HrirValidator::validateChannelPair(result->channels[s].left,
                                         result->channels[s].right,
                                         result->channels[s].sampleRate);
  }

  tmp.deleteFile();
  return 0;
}
