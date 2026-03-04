# ADR-001: Virtual Audio Device Architecture

**Status**: Accepted  
**Date**: 2026-03-04  
**Deciders**: Virtuoso Audio Project  
**Synthesized from**: All 13 swarm + meta reviews (04_PLANS/)

---

## Context

The original plan assumed Windows would provide a user-mode API to create a virtual 7.1 audio endpoint at runtime. All six trustworthy reviewers (ChatGPT, Codex, DeepSeek, Google, Grok 4.1, Grok 4.2) unanimously confirmed this assumption is **false**: Microsoft's WASAPI and MMDevice APIs have no mechanism for creating a visible, selectable audio endpoint without a kernel-mode driver. The original "no third-party drivers" constraint therefore directly contradicted the core product promise.

## Decision

**Virtuoso Audio will ship first-party virtual audio device drivers on every supported platform.**

The application MUST appear as a normal, selectable playback device ("Virtuoso Virtual 7.1") in each OS's native audio control panel — exactly like VB-Cable, BlackHole, or Windows Sonic. All audio processing (convolution, upmix, EQ) continues to run in user space in the main application process.

| Platform | Technology | Kernel involvement |
|---|---|---|
| Windows 10/11 | WDK SysVAD-based KMDF virtual audio driver | Kernel (minimal, passthrough only) |
| macOS 12.3+ | `AudioServerPlugIn` (user-space audio server plug-in) | User space (no kext required) |
| Linux | PipeWire virtual sink/module | User space (PipeWire daemon) |

## Design Principles

1. **Zero DSP in the driver/kernel.** The virtual device driver is a pure passthrough endpoint — it accepts N-channel PCM from applications and buffers it for the Virtuoso app to read. It does **not** convolve, mix, or process audio. This minimizes kernel attack surface and eliminates BSOD risk from DSP bugs.

2. **Paired render + capture endpoints.** The virtual device exposes both a render endpoint (what apps play to) and a paired loopback/capture endpoint that the Virtuoso app reads from via WASAPI shared mode / CoreAudio / PipeWire.

3. **App reads capture endpoint, outputs to physical device.** The Virtuoso app process is responsible for all DSP processing. It reads the 8-channel stream from the virtual device's capture endpoint and writes processed stereo audio to the user's physical headphone device.

4. **Graceful degradation.** If the driver fails to register (fresh boot with missing driver, OS update that reverted it), the DeviceWatchdog detects the absence and the app offers one-click repair (`virtuoso-audio --repair`).

## Consequences

### Positive
- "Virtuoso Virtual 7.1" appears natively in OS sound settings — no configuration required from users
- All games, browsers, and applications work without any per-app configuration
- DSP bugs cannot BSOD the system (driver is passthrough-only)
- macOS solution (AudioServerPlugIn) requires no kext and is Apple-approved

### Negative
- Windows driver requires an EV code signing certificate (~$400–800/yr) and Microsoft Partner Center attestation signing
- Driver development requires the Windows Driver Kit (WDK) in addition to the main JUCE build
- App must be running for audio to route through (no background service needed on Linux/macOS, Windows driver can optionally buffer)

### Rejected alternatives
- **VB-Cable / BlackHole dependency**: User friction; violates "install once and forget" goal
- **APO (Audio Processing Object)**: Complex COM registration; no cleaner than our own driver; some anti-virus software flags APOs
- **Per-app manual loopback routing**: Worse UX; does not work for all applications
- **Stereo-only loopback**: Defeats the entire purpose (no 7.1 content to virtualize)

## Windows Driver Notes

- Based on: Microsoft SysVAD virtual audio sample (`sysvad/`) from the Windows-driver-samples repository
- Channel configuration: 7.1 (`KSAUDIO_SPEAKER_7POINT1_SURROUND`)
- Sample formats: PCM 16-bit, 24-bit, 32-bit float; 44.1 kHz, 48 kHz, 96 kHz
- Driver must be **attestation signed** via Microsoft Hardware Dev Center before Windows will load it on systems with Secure Boot + driver signature enforcement enabled
- Development and testing must use a VM with test-signing enabled (`bcdedit /set testsigning on`)

## macOS Driver Notes

- Uses the `AudioServerPlugIn` API (available since macOS 10.12, stable since 12.3)
- Installed to `/Library/Audio/Plug-Ins/HAL/VirtuosoAudioDriver.driver`
- `coreaudiod` must be restarted once after installation (done by the installer, not on every app launch)
- Bundle must be notarized by Apple for Gatekeeper approval

## Linux Notes

- PipeWire virtual sink via `pw-loopback` module + `module-null-sink` pattern
- PipeWire socket connection from the Virtuoso app reads the virtual sink's monitor port
- PulseAudio fallback: `module-null-sink` + `module-loopback` for pre-PipeWire systems
- No driver signing required; runs entirely in user space

## Related ADRs

- [ADR-002-Licensing.md](ADR-002-Licensing.md)
- [ADR-003-Convolution-Precision.md](ADR-003-Convolution-Precision.md)
- [ADR-004-Crypto-Backend.md](ADR-004-Crypto-Backend.md)
