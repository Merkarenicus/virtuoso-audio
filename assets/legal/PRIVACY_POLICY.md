# Virtuoso Audio — Privacy Policy

**Version**: 1.0  
**Effective Date**: 2026-03-04

---

## Summary

Virtuoso Audio collects **no data** about you by default. All audio processing
happens entirely on your device. Nothing is sent to any server, ever, unless
you explicitly opt in to a specific feature.

---

## What We Do NOT Collect (By Default)

- Audio content (we never see or hear what you play)
- Usage data, analytics, or telemetry
- Crash reports (opt-in only — see below)
- Personal information of any kind
- Device identifiers sent to external servers

---

## Local Data Storage

The following data is stored **locally on your device only**:

| Data | Location | Purpose |
|---|---|---|
| Profiles (EQ settings, HRIR selections) | OS-native encrypted keystore | Save your configuration |
| Log files | Platform log directory | Technical diagnostics |
| Support bundle (created on request) | Desktop (you choose) | Send to support |

**Profiles** are encrypted using your operating system's secure keystore
(Windows DPAPI, macOS Keychain, or Linux libsecret). This data is never
transmitted and is only readable on your device under your user account.

**Log files** contain diagnostic information (audio device names, error
messages, performance metrics). They do NOT contain audio content or
personal information. Log files rotate automatically and are deleted after
10 days.

---

## Opt-In Features

### Crash Telemetry (Optional)
If enabled in Settings > Privacy, anonymous crash reports may be submitted
to help improve the application. These reports:
- Contain no audio data
- Contain no personally identifiable information
- Are one-way (you cannot be identified)
- Can be disabled at any time

### Support Bundle
When you click "Generate Support Bundle," a diagnostic ZIP file is created
**locally on your device**. You can review it before sending. It contains:
- Log files (with usernames and paths replaced with placeholders)
- Audio device configuration (no audio content)
- Application version and settings

You choose whether to send it and to whom. Nothing is transmitted automatically.

---

## Third-Party Audio Processing

We do NOT use Dolby, DTS, THX, or any other proprietary audio processing
technology. All spatial audio processing is performed using freely licensed
impulse responses (CIPIC, MIT KEMAR, SADIE II, OpenAL Soft) or your own
imported files.

---

## Your Rights

You can delete all local application data at any time:
1. Uninstall Virtuoso Audio (the uninstaller removes all data)
2. Or run: `virtuoso-audio --restore-defaults --delete-all-data`

---

## Contact

Questions about privacy: open an issue at
https://github.com/virtuoso-audio/virtuoso-audio

*This policy applies to Virtuoso Audio standalone application only.*
*VST3/AU plugin versions inherit the host application's privacy policy.*
