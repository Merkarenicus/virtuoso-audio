# Virtuoso Audio — Third-Party Licenses

This document lists all third-party libraries, frameworks, assets, and fonts
used by Virtuoso Audio. Required as part of the project's license compliance.

---

## JUCE Framework

**License**: Commercial License (purchased — see docs/legal/JUCE_LICENSE_RECEIPT.pdf)  
**Author**: Raw Material Software Limited  
**URL**: https://juce.com  
**Notes**: Required for standalone audio application and plugin targets.
The JUCE commercial license allows distribution of closed-source applications
without triggering the AGPLv3 source disclosure requirement.

---

## libmysofa

**License**: BSD 3-Clause  
**Author**: Symonics GmbH (Christian Hoene)  
**URL**: https://github.com/hoene/libmysofa  
**Copyright**: Copyright (c) 2016-2021 Symonics GmbH

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met: (standard BSD-3)

---

## libsamplerate

**License**: BSD 2-Clause  
**Author**: Erik de Castro Lopo  
**URL**: https://github.com/libsndfile/libsamplerate  
**Copyright**: Copyright (c) 2002-2021 Erik de Castro Lopo

---

## nlohmann/json

**License**: MIT  
**Author**: Niels Lohmann  
**URL**: https://github.com/nlohmann/json  
**Copyright**: Copyright (c) 2013-2022 Niels Lohmann

---

## libsodium

**License**: ISC License  
**Author**: Frank Denis and contributors  
**URL**: https://libsodium.org  
**Copyright**: Copyright (c) 2013-2022 Frank Denis

---

## Inter Font

**License**: SIL Open Font License 1.1  
**Author**: Rasmus Andersson  
**URL**: https://rsms.me/inter/  
**Copyright**: Copyright (c) 2016 The Inter Project Authors

---

## HRIR Datasets

### CIPIC HRTF Database
**License**: Public Domain (University of California, Davis Regents)  
**Citation**: Algazi, V.R., Duda, R.O., Thompson, D.M., Avendano, C. (2001).
The CIPIC HRTF Database. Proc. 2001 IEEE ASSP Workshop on Applications of
Signal Processing to Audio and Acoustics, New Paltz, NY.  
**URL**: https://www.ece.ucdavis.edu/cisl/research/hrtf.html

### MIT Media Lab KEMAR HRTF
**License**: MIT License  
**Citation**: Gardner, W.G., Martin, K.D. (1995). HRTF Measurements of a KEMAR
Dummy-Head Microphone. MIT Media Lab, Technical Report No. 280.  
**URL**: https://sound.media.mit.edu/resources/KEMAR.html

### SADIE II Database
**License**: Apache License 2.0  
**Copyright**: Copyright (c) 2016-2019 University of York  
**Citation**: Granier, J., Coleman, P., Pike, C., Carpentier, T. (2019).
The SADIE II Database. J. Audio Eng. Soc., 67(3), 99-110.  
**URL**: https://www.york.ac.uk/sadie-project/database.html  
**Source**: https://sadie-binaural.s3.amazonaws.com  
**Note**: License is Apache-2.0. Attribution required. No DRM.

### OpenAL Soft Default HRTF
**License**: LGPL-2.1-or-later  
**Author**: kcat and contributors  
**URL**: https://github.com/kcat/openal-soft/tree/master/hrtf  
**Notes**: Source available at the URL above. Unmodified data files distributed.

---

## Windows Driver (SysVAD-based)

The Windows virtual audio driver in `drivers/windows/` is based on the
Microsoft SysVAD virtual audio sample, which is released under the MIT License.
**Copyright**: Copyright (c) Microsoft Corporation  
**URL**: https://github.com/microsoft/Windows-driver-samples

---

*This file is auto-generated from assets/hrir/ASSET_MANIFEST.json during release builds.
If you find an error in this file, update ASSET_MANIFEST.json and run `cmake --build . --target GenerateLicenses`.*
