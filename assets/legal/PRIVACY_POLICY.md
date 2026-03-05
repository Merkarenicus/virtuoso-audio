# Privacy Policy — Virtuoso Audio

**Last updated**: March 4, 2026

## What We Collect

**Virtuoso Audio collects no personal data by default.**

All audio processing occurs entirely on your local device. Audio data is **never**
sent to any server, cloud service, or third party.

The following data stays entirely on your device:
- **User profiles** (EQ settings, HRIR selection) — stored encrypted in your Application Data folder
- **Log files** — stored locally in your Application Data folder; auto-rotated after 10 MB
- **HRIR files** — loaded from your local file system

## Crash Reporting (Opt-In Only)

If you choose to enable crash reporting via the Settings panel, Virtuoso Audio may
send anonymized crash reports (stack traces, OS version, hardware model) to our
crash reporting service. **This is disabled by default** and requires explicit opt-in.

Crash reports do **not** include:
- Audio content
- Personal files
- Profile data or EQ settings

## Third-Party Services

Virtuoso Audio does not embed analytics SDKs, advertising SDKs, or tracking pixels.

## Data Stored on Disk

| Location | Contents | Encrypted |
|---|---|---|
| `%APPDATA%\virtuoso-audio\profiles\` (Windows) | User profiles | Yes (XChaCha20-Poly1305) |
| `~/Library/Application Support/virtuoso-audio/` (macOS) | User profiles | Yes |
| `~/.local/share/virtuoso-audio/` (Linux) | User profiles | Yes |
| Platform temp dir | Log files | No (no personal data in logs) |

## Contact

For privacy questions: open a GitHub issue at https://github.com/Merkarenicus/virtuoso-audio
