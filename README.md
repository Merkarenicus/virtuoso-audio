# Virtuoso Audio

**Binaural 7.1 surround sound for headphones — privacy-first, driver-backed, open architecture.**

[![CI](https://github.com/Merkarenicus/virtuoso-audio/actions/workflows/ci.yml/badge.svg)](https://github.com/Merkarenicus/virtuoso-audio/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](assets/legal/LICENSE)

---

## What Is Virtuoso Audio?

Virtuoso Audio intercepts your system's audio output and renders it binaurally through
your headphones — making games, movies, and music sound like they're happening around you,
with proper front/back/height localisation.

It works for **all** applications system-wide (not just specific games or players) by
appearing as a selectable audio output device — **"Virtuoso Virtual 7.1"** — directly
in your OS sound settings, exactly like a real sound card.

---

## Features

| Feature | Detail |
|---|---|
| **System-wide audio** | Works for any app — games, browsers, video players, DAWs |
| **7.1 binaural rendering** | 14 HRIR convolutions (7 speakers × 2 ears) at up to 192 kHz |
| **HRIR support** | HeSuVi WAV (14-ch), SOFA files, bundled MIT KEMAR / SADIE II |
| **8-band parametric EQ** | AutoEq import, per-profile persistence |
| **True-peak limiter** | Brick-wall limiter prevents clipping on headphone outputs |
| **Encrypted profiles** | XChaCha20-Poly1305 with OS keystore key storage |
| **Zero telemetry** | No data leaves your device by default |
| **Install once, forget** | Virtual device persists across reboots; system tray icon |

---

## Platform Support

| Platform | Driver | Status |
|---|---|---|
| Windows 10/11 | WDK WaveRT (`VirtuosoVAD.sys`) | 🟡 Driver scaffold — Phase 6b needed for ring buffer wiring |
| macOS 13+ | CoreAudio `AudioServerPlugIn` | 🟡 Driver scaffold — Phase 6b I/O cycle wiring needed |
| Linux | PipeWire null-sink + PulseAudio | ✅ `scripts/setup-linux-sink.sh` — ready to use |

---

## Quick Start

### Prerequisites

- **Windows**: VS 2022 Build Tools, CMake 3.28+
- **macOS**: Xcode 15+, Homebrew (`libsodium libsamplerate libmysofa`)
- **Linux**: GCC 13+, Ninja, `apt-get install libsodium-dev libsamplerate-dev libmysofa-dev libpipewire-0.3-dev`

See [`BUILD.md`](BUILD.md) for full instructions.

### Build (Windows)

```powershell
cmake --preset windows-vs2022 -S 05_IMPLEMENTATION -B 05_IMPLEMENTATION/build/vs2022-x64
cmake --build 05_IMPLEMENTATION/build/vs2022-x64 --config Debug --parallel
```

### Linux Audio Setup

```bash
chmod +x 05_IMPLEMENTATION/scripts/setup-linux-sink.sh
./05_IMPLEMENTATION/scripts/setup-linux-sink.sh
```

---

## Architecture

```
System Audio (any app)
       │
       ▼
┌─────────────────────────────┐
│  Virtuoso Virtual 7.1       │  ← OS audio device (WaveRT / AudioServerPlugIn / PipeWire)
│  (VirtuosoVAD / HAL Plugin) │
└─────────────────────────────┘
       │  7.1 PCM
       ▼
┌─────────────────────────────┐
│  AudioEngine (JUCE)         │
│  ┌──────────────────────┐   │
│  │ BassManagement       │   │  Sub-bass redirect to LFE
│  │ ConvolverProcessor   │   │  14× HRIR convolution (7 speakers × 2 ears)
│  │ EqProcessor          │   │  8-band parametric EQ
│  │ LimiterProcessor     │   │  True-peak brick-wall
│  └──────────────────────┘   │
└─────────────────────────────┘
       │  Binaural stereo
       ▼
  Your headphones
```

---

## Repository Structure

```
05_IMPLEMENTATION/
├── CMakeLists.txt           # Main build (JUCE Commercial License required)
├── CMakePresets.json        # VS 2022 / Xcode / Ninja presets
├── BUILD.md                 # Full build instructions
├── cmake/
│   ├── Dependencies.cmake   # CPM dependency management
│   ├── CodeSigning.cmake    # EV / Developer ID / GPG signing helpers
│   └── SBOM.cmake           # CycloneDX 1.6 SBOM generator
├── src/                     # Application source code (C++20)
├── drivers/
│   ├── windows/             # WDK WaveRT kernel driver
│   └── macos/               # CoreAudio AudioServerPlugIn
├── installer/
│   ├── windows/             # NSIS installer script
│   └── macos/               # pkgbuild + productbuild + notarize
├── tests/
│   ├── unit/                # Catch2 unit tests
│   ├── integration/         # End-to-end pipeline tests
│   ├── perf/                # Latency & benchmark tests
│   └── fuzz/                # libFuzzer harnesses
├── docs/adr/                # Architecture Decision Records
├── assets/
│   ├── hrir/                # Bundled HRIR files + ASSET_MANIFEST.json
│   └── legal/               # MIT LICENSE, THIRD_PARTY_LICENSES, PRIVACY_POLICY
└── scripts/
    └── setup-linux-sink.sh  # PipeWire + PulseAudio setup
```

---

## Architecture Decision Records

| ADR | Decision |
|---|---|
| [ADR-001](docs/adr/ADR-001-Virtual-Device-Track.md) | Ship first-party virtual audio drivers on all platforms |
| [ADR-002](docs/adr/ADR-002-Licensing.md) | JUCE Commercial License required for binary distribution |
| [ADR-003](docs/adr/ADR-003-Convolution-Precision.md) | 14× float convolution + double accumulator for binaural sum |
| [ADR-004](docs/adr/ADR-004-Crypto-Backend.md) | XChaCha20-Poly1305 (libsodium) + OS keystore key management |

---

## License

MIT License — see [`assets/legal/LICENSE`](assets/legal/LICENSE).

> **JUCE note**: JUCE is used under its **Commercial License** for binary distribution.
> JUCE Commercial License is required before releasing Virtuoso Audio binaries.
> See [ADR-002](docs/adr/ADR-002-Licensing.md) and [THIRD_PARTY_LICENSES.md](assets/legal/THIRD_PARTY_LICENSES.md).

---

## Contributing

This project is in active development. See [BUILD.md](BUILD.md) to set up your environment.
Open issues or pull requests on [GitHub](https://github.com/Merkarenicus/virtuoso-audio).
