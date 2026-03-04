# macOS AudioServerPlugIn Driver — VirtuosoAudioDriver

## Overview

The macOS virtual audio driver creates a "Virtuoso Virtual 7.1" audio device using the `AudioServerPlugIn` API (CoreAudio user-space audio server plug-in). This is Apple's official mechanism for creating virtual audio devices without a kernel extension (kext).

**Critical design principle**: The plug-in is a pure passthrough buffer. All DSP runs in the user-space Virtuoso application.

## Architecture

```
App/Game (7.1 audio) ──▶ "Virtuoso Virtual 7.1" (AudioServerPlugIn)
                                    │
                                    ▼ (in-process loopback output stream)
                         Virtuoso App (CoreAudio)
                                    │
                               [DSP pipeline]
                                    │
                                    ▼
                         Physical headphones (CoreAudio output)
```

## Key Facts

- **API**: `AudioServerPlugIn` (available macOS 10.12+, preferred 12.3+)
- **No kext required**: Runs entirely in user space within the `coreaudiod` daemon
- **Bundle location**: `/Library/Audio/Plug-Ins/HAL/VirtuosoAudioDriver.driver`
- **One-time restart required**: `coreaudiod` must be restarted at install time (installer handles this)
- **Channel layout**: `kAudioChannelLayoutTag_MPEG_7_1_A` (7.1 surround)

## Build Requirements

- macOS 12.3 SDK or later (Xcode 14+)
- Apple Developer Program membership ($99/yr)
- Developer ID Application certificate (for signing)
- Notarization via `xcrun notarytool` (required for Gatekeeper approval)

## Files

| File | Purpose |
|---|---|
| `VirtuosoAudioDriver.cpp` | AudioServerPlugIn interface implementation |
| `VirtuosoAudioDriver.h` | Plugin class and stream buffer declarations |
| `Info.plist` | Bundle metadata (device name, 7.1 layout, sample rates) |
| `VirtuosoAudioDriver.entitlements` | Entitlements for hardened runtime |

## Signing and Notarization

```bash
# 1. Build the plug-in bundle
xcodebuild -target VirtuosoAudioDriver -configuration Release

# 2. Sign with Developer ID
codesign --deep --force --options runtime \
    --sign "Developer ID Application: Your Name (TEAMID)" \
    VirtuosoAudioDriver.driver

# 3. Package for notarization
ditto -c -k --keepParent VirtuosoAudioDriver.driver VirtuosoAudioDriver.zip
xcrun notarytool submit VirtuosoAudioDriver.zip \
    --apple-id you@example.com \
    --team-id TEAMID \
    --password APP_SPECIFIC_PASSWORD \
    --wait

# 4. Staple the ticket
xcrun stapler staple VirtuosoAudioDriver.driver
```

## Installation (handled by .pkg installer)

```bash
sudo cp -R VirtuosoAudioDriver.driver /Library/Audio/Plug-Ins/HAL/
sudo launchctl kickstart -k system/com.apple.audio.coreaudiod  # one-time restart
```

## Status

Phase 1: **Stub/documentation only.** Full implementation pending Apple Developer Program enrollment and Xcode environment setup.

## References

- [AudioServerPlugIn API](https://developer.apple.com/documentation/coreaudio/audio_server_plug_in)
- [Creating a Virtual Audio Device](https://developer.apple.com/library/archive/samplecode/HALLab/Introduction/Intro.html)
- [WWDC Spatial Audio Session](https://developer.apple.com/videos/play/wwdc2021/10038/)
