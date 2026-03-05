# ADR-001: Virtual Device Driver Track Requirement

**Date**: 2026-03-04  
**Status**: Accepted  
**Deciders**: Full swarm review (6 reviewers) + meta-review

---

## Context

Virtuoso Audio delivers binaural 7.1 surround rendering to headphones. To intercept
system-wide audio (from games, video players, DAWs), the software must appear as a
standard audio output device selectable in the OS sound panel — just like VB-Audio
Virtual Cable or Voicemeeter.

Early alternatives considered:
1. **ASIO loopback** — only works with ASIO-aware apps (< 5% of use cases)
2. **VAC-style pipe** — requires a third-party driver (VB-Cable, etc.), degrades UX
3. **User-space kext shim** on macOS — disallowed since SIP enforcement in macOS 12
4. **WaveOut/CoreAudio app capture** — cannot capture DRM streams; no system-wide routing

---

## Decision

**Virtuoso MUST ship its own first-party virtual audio device driver on all platforms.**

| Platform | Driver Type | Appears As |
|---|---|---|
| Windows 10/11 | WDK WaveRT port driver (`VirtuosoVAD.sys`) | "Virtuoso Virtual 7.1" in Sound panel |
| macOS 13+ | CoreAudio `AudioServerPlugIn` (HAL plugin) | "Virtuoso Virtual 7.1" in System Settings |
| Linux | PipeWire null-sink + PulseAudio fallback (`setup-linux-sink.sh`) | Configures at user session level |

Channel geometry: **8 channels (7.1)**, KSAUDIO_SPEAKER_7POINT1_SURROUND, channel mask `0x63F`.

---

## Consequences

**Positive**:
- Virtuoso is self-contained — no dependency on VB-Cable, Voicemeeter, or BlackHole
- Appears in every app's output device list
- Survives OS updates without user action (driver signed/versioned)

**Negative**:
- Windows requires EV code-signing certificate and WDK kernel development
- macOS requires Apple Developer ID + notarization for every release
- Linux setup requires a user-run script (cannot install kernel modules without root)
- Certification cost: ~$300/year (EV cert) + $99/year (Apple Developer Program)

**Risk mitigations**:
- EV cert from DigiCert or Sectigo — covered in Phase 4b
- macOS notarization automated via `build-pkg.sh --notarize`
- Linux installer script: `./scripts/setup-linux-sink.sh` (rootless, PipeWire user module)
