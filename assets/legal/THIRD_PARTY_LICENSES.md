# Third-Party Licenses — Virtuoso Audio

This document lists all third-party libraries used by Virtuoso Audio, their
licenses, and their source repositories. The SBOM in `build/sbom/virtuoso-audio-sbom.json`
(CycloneDX 1.6 JSON) is the machine-readable equivalent of this file.

---

## JUCE

- **Version**: 8.0.4  
- **License**: JUCE Commercial License (required for binary distribution) or AGPLv3  
- **Source**: https://github.com/juce-framework/JUCE  
- **Homepage**: https://juce.com  
- **Note**: A JUCE Commercial License MUST be purchased before distributing Virtuoso Audio binaries. See ADR-002.

---

## libsodium

- **Version**: 1.0.20  
- **License**: ISC License  
- **Source**: https://github.com/jedisct1/libsodium  
- **Copyright**: Copyright (c) 2013-2024 Frank Denis

```
Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.
```

---

## libsamplerate

- **Version**: 0.2.2  
- **License**: BSD 2-Clause  
- **Source**: https://github.com/libsndfile/libsamplerate  
- **Copyright**: Copyright (c) 2002-2021, Erik de Castro Lopo

---

## libmysofa

- **Version**: 1.3.2  
- **License**: BSD 3-Clause  
- **Source**: https://github.com/hoene/libmysofa  
- **Copyright**: Copyright (c) 2016-2024, Symonics GmbH, Christian Hoene

---

## nlohmann/json

- **Version**: 3.11.3  
- **License**: MIT License  
- **Source**: https://github.com/nlohmann/json  
- **Copyright**: Copyright (c) 2013-2025 Niels Lohmann

---

## Catch2

- **Version**: 3.6.0  
- **License**: Boost Software License 1.0  
- **Source**: https://github.com/catchorg/Catch2  
- **Note**: Used only in development/testing builds; not shipped in release binaries.

---

## Inter Variable Font

- **License**: SIL Open Font License 1.1  
- **Source**: https://rsms.me/inter/  
- **Copyright**: Copyright (c) 2016-2024, The Inter Project Authors

---

## Platform System Libraries

The following platform system libraries are linked dynamically and are provided
by the operating system. They are not bundled with Virtuoso Audio:

| Library | Platform | License |
|---|---|---|
| Windows CNG (Bcrypt.dll) | Windows | Microsoft Proprietary |
| DPAPI | Windows | Microsoft Proprietary |
| CoreAudio | macOS | Apple Proprietary |
| CommonCrypto | macOS | Apple Proprietary |
| Keychain Services | macOS | Apple Proprietary |
| PipeWire | Linux | MIT / LGPL |
| libsecret | Linux | LGPL 2.1+ |
| ALSA | Linux | LGPL 2.1+ |
