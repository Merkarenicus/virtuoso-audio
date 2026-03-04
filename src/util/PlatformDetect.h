// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// PlatformDetect.h — Compile-time and runtime OS/hardware detection utilities

#pragma once
#include <cstdint>
#include <string>


namespace virtuoso {

namespace platform {

// Compile-time platform tags
#if defined(_WIN32)
static constexpr bool kIsWindows = true;
static constexpr bool kIsMacOS = false;
static constexpr bool kIsLinux = false;
static constexpr char kName[] = "Windows";
#elif defined(__APPLE__)
static constexpr bool kIsWindows = false;
static constexpr bool kIsMacOS = true;
static constexpr bool kIsLinux = false;
static constexpr char kName[] = "macOS";
#else
static constexpr bool kIsWindows = false;
static constexpr bool kIsMacOS = false;
static constexpr bool kIsLinux = true;
static constexpr char kName[] = "Linux";
#endif

// Runtime info (populated lazily on first call)
std::string getOSVersion();  // e.g. "Windows 11 Build 22631"
std::string getCPUBrand();   // e.g. "Intel Core i9-13900K"
uint64_t getTotalMemoryMB(); // Total physical RAM in MB
bool hasAVX2();     // True if CPU supports AVX2 (JUCE DSP acceleration)
bool hasARM_NEON(); // True on Apple Silicon / ARM64 Linux

} // namespace platform
} // namespace virtuoso
