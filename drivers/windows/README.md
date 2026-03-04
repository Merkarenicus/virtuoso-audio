# Windows Virtual Audio Driver — VirtuosoVAD

## Overview

The Windows virtual audio driver creates a "Virtuoso Virtual 7.1" render endpoint visible in the Windows Sound control panel. It is a WDK (Windows Driver Kit) KMDF driver based on the Microsoft SysVAD virtual audio sample.

**Critical design principle**: The driver does ZERO audio processing. It is a pure passthrough buffer endpoint only. All convolution, EQ, and DSP runs in the user-space Virtuoso application.

## Architecture

```
App/Game (7.1 audio) ──▶ "Virtuoso Virtual 7.1" endpoint (KMDF driver)
                                    │
                                    ▼ (shared ring buffer / loopback capture)
                         Virtuoso App (user space)
                                    │
                               [DSP pipeline]
                                    │
                                    ▼
                         Physical headphones (WASAPI output)
```

## Build Requirements

- Windows 11 SDK (22621 or later)
- Windows Driver Kit (WDK) 10.0.22621.0 or later
- Visual Studio 2022 with WDK integration
- EV Code Signing Certificate (DigiCert / Sectigo)
- Microsoft Partner Center account (for attestation signing)

## Files

| File | Purpose |
|---|---|
| `VirtuosoVAD.inf` | Driver installation manifest, 7.1 device capabilities |
| `VirtuosoVAD.cpp` | Miniport + stream handlers (SysVAD-based) |
| `VirtuosoVAD.h` | Endpoint definitions, channel mask |
| `common.h` | Shared structures (IPC shared memory layout) |
| `VirtuosoVAD.vcxproj` | WDK project file |

## Key Configuration

- **Channel mask**: `KSAUDIO_SPEAKER_7POINT1_SURROUND` (0x63F)
- **Supported formats**: PCM 16/24/32-bit float, 44.1/48/96 kHz
- **Device name**: "Virtuoso Virtual 7.1" (localizable via INF)
- **DevNode location**: Enumerated under "Software Component" (no hardware dependency)

## Driver Signing for Distribution

### Development (test mode)
```powershell
# Enable test signing (requires bcdedit — admin)
bcdedit /set testsigning on
# Install driver
pnputil /add-driver VirtuosoVAD.inf /install
```

### Production (EV-signed)
1. Obtain EV Code Signing Certificate from DigiCert or Sectigo (~$400–800/yr)
2. Sign the `.sys` file: `signtool sign /sha1 <thumbprint> /tr http://timestamp.digicert.com /td sha256 /fd sha256 VirtuosoVAD.sys`
3. Submit to Microsoft Partner Center for attestation signing: https://partner.microsoft.com/en-us/dashboard/hardware/driver/new
4. Attestation-signed `.cab` is returned by Microsoft
5. Install via `pnputil /add-driver VirtuosoVAD_signed.inf /install`

**Without attestation signing, the driver will NOT load on Windows 10/11 with Secure Boot and HVCI enabled.**

## Status

Phase 1: **Stub/documentation only.** Full driver implementation begins after EV certificate is obtained and WDK environment is confirmed.

## References

- [Microsoft SysVAD Sample](https://github.com/microsoft/Windows-driver-samples/tree/main/audio/sysvad)
- [Virtual Audio Device Driver Documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/virtual-audio-devices)
- [Audio Driver Signing Requirements](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/audio-driver-signing)
