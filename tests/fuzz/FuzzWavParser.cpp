// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// FuzzWavParser.cpp
// libFuzzer / AFL++ harness for WavHrirParser.
//
// Build with: clang++ -fsanitize=fuzzer,address -o FuzzWavParser
// FuzzWavParser.cpp ... AFL++:      afl-clang-fast++ -o FuzzWavParser
// FuzzWavParser.cpp ...
//             afl-fuzz -i testdata -o fuzz_out -- ./FuzzWavParser @@
//
// Goal: 10 million iterations with zero ASAN/UBSAN crashes.
// Any crash or UBSAN error must be treated as a STOP-SHIP bug.

#include "hrir/HrirValidator.h"
#include "hrir/WavHrirParser.h"
#include <cstddef>
#include <cstdint>


using namespace virtuoso;

// Entry point for libFuzzer. AFL++ uses LLVMFuzzerTestOneInput with same
// signature.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Hard reject before any processing — mirrors HrirValidator pre-parse check
  if (size > static_cast<size_t>(HrirValidator::kMaxFileSizeBytes)) {
    return 0;
  }

  WavHrirParser parser;

  // Parse from raw bytes (no file I/O — safe for fuzzing)
  auto result = parser.parseFromMemory(data, size);

  // We do NOT care whether parsing succeeded or failed —
  // we only care that it does NOT crash, OOM, or trigger UBSAN.
  // result is a std::expected — either the set or an error string.
  // Both are valid outcomes for malformed input.

  if (result.has_value()) {
    // If parsing succeeded, validate the result for good measure
    const auto &hrirSet = *result;
    // Run validation on the parsed data — should not crash
    for (int s = 0; s < static_cast<int>(hrirSet.channels.size()); ++s) {
      HrirValidator::validateChannelPair(hrirSet.channels[s].left,
                                         hrirSet.channels[s].right,
                                         hrirSet.channels[s].sampleRate);
    }
  }

  return 0;
}
