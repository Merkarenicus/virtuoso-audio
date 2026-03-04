# BUILD.md — Virtuoso Audio Build Guide

Virtuoso Audio is a C++20 project using CMake 3.24+, JUCE 8, and CPM for dependency management.

## Prerequisites

| Requirement | Version | Notes |
|---|---|---|
| CMake | 3.24+ | [cmake.org/download](https://cmake.org/download/) |
| **Windows** | VS 2022 Build Tools (`Microsoft.VisualStudio.Workload.VCTools`) | WDK 10.0.26100+ for driver |
| **macOS** | Xcode 15+ with Command Line Tools | macOS 13+ target |
| **Linux** | GCC 13+ or Clang 17+, Ninja, PipeWire headers (`libpipewire-0.3-dev`) | |
| JUCE | 8.x (fetched by CPM) | **Commercial license required** — AGPLv3 not compatible |
| libsodium | latest stable | Fetched by CPM |
| libsamplerate | 0.2.x | Fetched by CPM |
| libmysofa | ≥ 1.3 | Fetched by CPM |
| nlohmann/json | 3.11.x | Fetched by CPM |
| Catch2 | 3.x | Fetched by CPM (tests only) |

---

## Quick Start (Windows)

```powershell
# 1. Install CMake (if not already present)
winget install Kitware.CMake

# 2. Install VS 2022 Build Tools with C++ workload
winget install Microsoft.VisualStudio.2022.BuildTools `
    --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

# 3. Configure (uses CMakePresets.json — no generator flag needed)
cmake --preset windows-vs2022

# 4. Build Debug
cmake --build build/vs2022-x64 --config Debug --parallel

# 5. Run tests
ctest --preset windows-test --output-on-failure
```

> **Note on the CMake error**: If your IDE shows `Running 'nmake' failed`, it is using
> the wrong generator. The `CMakePresets.json` in this repo fixes this by specifying
> `"Visual Studio 17 2022"` explicitly. Make sure your IDE picks up the presets file.

---

## Quick Start (macOS)

```bash
# 1. Install Xcode and Command Line Tools
xcode-select --install

# 2. Install dependencies (Homebrew)
brew install cmake ninja libsodium libsamplerate libmysofa

# 3. Configure for Universal Binary (arm64 + x86_64)
cmake --preset macos-universal

# 4. Build
xcodebuild -workspace build/xcode-universal/VirtuosoAudio.xcworkspace \
           -scheme VirtuosoAudio -configuration Debug build

# 5. Install macOS AudioServerPlugIn driver
sudo cp -r drivers/macos/VirtuosoAudioPlugin.driver \
          /Library/Audio/Plug-Ins/HAL/
sudo codesign --sign "Developer ID Application: YOUR_NAME" \
              /Library/Audio/Plug-Ins/HAL/VirtuosoAudioPlugin.driver
```

---

## Quick Start (Linux / Ubuntu 24.04+)

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y cmake ninja-build gcc-13 g++-13 \
    libsodium-dev libsamplerate-dev libmysofa-dev \
    libpipewire-0.3-dev libsecret-1-dev pkg-config

# 2. Configure (AddressSanitizer enabled in debug)
cmake --preset linux-debug

# 3. Build
cmake --build build/linux-debug --parallel

# 4. Set up virtual audio device
chmod +x scripts/setup-linux-sink.sh
./scripts/setup-linux-sink.sh

# 5. Run tests
ctest --test-dir build/linux-debug --output-on-failure
```

---

## Windows Driver Build (requires WDK)

The SysVAD kernel driver is in `drivers/windows/`. It requires WDK 10.0.26100+ and
a separate Visual Studio project (`VirtuosoVAD.vcxproj`, Phase 4b work).

```powershell
# Install WDK
winget install Microsoft.WindowsDriverKit

# Open VirtuosoVAD.vcxproj in Visual Studio and build Release|x64
# For test signing (dev machines only):
bcdedit /set testsigning on
devcon install drivers/windows/VirtuosoVAD.inf Root\VirtuosoVAD
```

> **Production**: Requires EV code-signing certificate. Do not enable test signing in production.

---

## macOS Driver Build (requires Apple Developer ID)

```bash
xcodebuild -project drivers/macos/VirtuosoAudioPlugin.xcodeproj \
           -scheme VirtuosoAudioPlugin -configuration Release build

# Sign
codesign --sign "Developer ID Application: ..." \
         --entitlements drivers/macos/VirtuosoPlugin.entitlements \
         --deep build/Release/VirtuosoAudioPlugin.driver

# Notarize (mandatory for macOS 13+)
xcrun notarytool submit VirtuosoAudioPlugin.driver.zip \
    --apple-id YOUR_APPLE_ID --team-id YOUR_TEAM_ID --wait
```

---

## Project Structure

```
05_IMPLEMENTATION/
├── CMakeLists.txt          # Main build file (requires JUCE commercial license)
├── CMakePresets.json       # Generator presets: VS 2022, Ninja, Xcode, Linux
├── cmake/
│   ├── CPM.cmake           # Dependency manager
│   ├── Dependencies.cmake  # Fetches JUCE, libsodium, libsamplerate, etc.
│   ├── CodeSigning.cmake   # EV/Apple signing helpers
│   └── SBOM.cmake          # Software Bill of Materials
├── src/
│   ├── Main.cpp            # JUCE app entry point
│   ├── StandaloneApp.*     # App lifecycle
│   ├── audio/              # DSP chain + platform capture backends
│   ├── hrir/               # HRIR pipeline (parse, validate, resample)
│   ├── gui/                # MainWindow, LookAndFeel, SystemTray, Wizard
│   ├── profile/            # Profile schema + encrypted storage
│   ├── safety/             # TransactionJournal, DriverHealthCheck
│   ├── logging/            # Logger, SupportBundle
│   ├── export/             # OfflineRenderer, ExportProgressModel
│   └── util/               # Crypto, SingleInstance, PlatformDetect
├── drivers/
│   ├── windows/            # SysVAD WaveRT kernel driver (.inf + .cpp)
│   └── macos/              # AudioServerPlugIn HAL plugin
├── tests/
│   ├── unit/               # Catch2 unit tests
│   ├── integration/        # End-to-end audio pipeline tests
│   ├── perf/               # Latency benchmarks
│   └── fuzz/               # libFuzzer fuzz harnesses
├── assets/
│   ├── fonts/Inter-Variable.ttf
│   ├── hrir/               # Bundled HRIR files (MIT KEMAR etc.)
│   └── legal/              # LICENSE, third-party notices
└── scripts/
    └── setup-linux-sink.sh # PipeWire / PulseAudio null-sink setup
```

---

## CI / CD

CI is defined in `.github/workflows/`:
- **`ci.yml`**: Builds on Windows (VS 2022), macOS (Xcode), and Linux (GCC 13 + Ninja); runs all Catch2 tests
- **`fuzz.yml`**: libFuzzer harnesses for HRIR parsers; runs 300s per target on Linux

---

## JUCE License

> JUCE is used under the **JUCE Commercial License**. You must purchase a JUCE commercial
> license before distributing Virtuoso Audio. Without it, JUCE is licensed under AGPLv3,
> which conflicts with this project's MIT license. See `assets/legal/THIRD_PARTY_LICENSES.md`.
