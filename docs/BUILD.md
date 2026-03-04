# Virtuoso Audio — Build Guide

## Prerequisites

### All Platforms
- CMake ≥ 3.24
- C++20-capable compiler (MSVC 17.8+, Clang 16+, GCC 13+)
- Git
- JUCE commercial license (stored in `docs/legal/JUCE_LICENSE_RECEIPT.pdf`)

### Windows
- Visual Studio 2022 (Community or higher)
- Windows SDK ≥ 10.0.22621
- Windows Driver Kit (WDK) — **required for driver build only**
- NASM (for libsodium)

### macOS
- Xcode ≥ 15
- macOS 12.3+ SDK
- Apple Developer Program membership (for signing)

### Linux
- GCC 13+ or Clang 16+
- `libpipewire-0.3-dev` `libpulse-dev` `libsecret-1-dev`
- `libasound2-dev` `libfreetype6-dev` `libcurl4-openssl-dev` `libgl1-mesa-dev`
- `pkg-config`

```bash
# Ubuntu 24.04
sudo apt-get install -y \
  cmake build-essential git \
  libpipewire-0.3-dev libpulse-dev libsecret-1-dev \
  libasound2-dev libfreetype6-dev libcurl4-openssl-dev libgl1-mesa-dev \
  pkg-config
```

---

## Building the Application (All Platforms)

```bash
# Clone
git clone https://github.com/virtuoso-audio/virtuoso-audio.git
cd virtuoso-audio/05_IMPLEMENTATION

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DVIRTUALOSO_BUILD_TESTS=ON

# Build
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure
```

## Debug Build with ASAN + UBSAN

```bash
cmake -B build-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVIRTUALOSO_ENABLE_ASAN=ON \
  -DVIRTUALOSO_ENABLE_UBSAN=ON
cmake --build build-debug
ctest --test-dir build-debug
```

## ARM64 Builds

```bash
# macOS Universal Binary (x64 + ARM64)
cmake -B build-mac-universal \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-mac-universal

# Windows ARM64 (cross-compile from x64)
cmake -B build-win-arm64 -A ARM64
cmake --build build-win-arm64 --config Release
```

---

## Building the Windows Driver (Separate)

> **Requires**: Windows Driver Kit (WDK) + EV Code Signing Certificate

```powershell
# Open VirtuosoVAD.vcxproj in Visual Studio with WDK integration
# Or via MSBuild:
msbuild drivers/windows/VirtuosoVAD/VirtuosoVAD.vcxproj /p:Configuration=Release /p:Platform=x64

# Sign (replace thumbprint with your EV cert)
signtool sign /sha1 <YOUR_THUMBPRINT> /tr http://timestamp.digicert.com /td SHA256 /fd SHA256 VirtuosoVAD.sys

# Install for testing (test-signed)
bcdedit /set testsigning on  # Requires admin; restart required
pnputil /add-driver VirtuosoVAD.inf /install
```

---

## Building the macOS AudioServerPlugIn (Separate)

```bash
xcodebuild -project drivers/macos/VirtuosoAudioDriver.xcodeproj \
  -scheme VirtuosoAudioDriver \
  -configuration Release \
  BUILD_DIR=build-driver

codesign --deep --force --options runtime \
  --sign "Developer ID Application: <Your Name (TEAMID)>" \
  build-driver/Release/VirtuosoAudioDriver.driver
```

---

## Running Fuzz Tests

```bash
# Requires Clang with libFuzzer
CC=clang CXX=clang++ cmake -B build-fuzz \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer,address,undefined"
cmake --build build-fuzz --target FuzzWavParser

# Run (adjust -max_total_time as needed)
./build-fuzz/FuzzWavParser tests/testdata/ -max_total_time=600
```

---

## Pinning Dependencies

All CPM dependencies should have their `GIT_TAG` replaced with an exact commit SHA-256
before the first release. Follow this workflow:

1. Build and verify with a tag (e.g., `v3.11.3`)
2. Look up the commit hash: `git ls-remote https://github.com/nlohmann/json refs/tags/v3.11.3`
3. Replace the tag with the hash in `cmake/Dependencies.cmake`
4. CI will fail if the hash doesn't match

---

## Generating the SBOM

```bash
# Install syft (recommended)
# macOS: brew install syft
# Linux: curl -sSfL https://raw.githubusercontent.com/anchore/syft/main/install.sh | sh

cmake --build build --target VirtuosoAudio
# SBOM is generated post-build to build/sbom/virtuoso-audio-<version>.spdx.json
```
