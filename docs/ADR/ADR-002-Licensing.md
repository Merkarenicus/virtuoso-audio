# ADR-002: JUCE Licensing Decision

**Status**: Accepted  
**Date**: 2026-03-04  
**Deciders**: Virtuoso Audio Project

---

## Context

The original plan stated a project license of "MIT" while using JUCE Framework under "GPLv3 / Commercial". This is a critical legal error flagged by all six trustworthy swarm reviewers.

**Actual JUCE license (verified at juce.com, 2026):**
JUCE is licensed under the **AGPLv3** (Affero General Public License v3), **not** GPLv3 as stated in the original plan. The distinction matters: AGPLv3 additionally requires that source be disclosed when the software is accessed over a network. For a desktop application this difference is moot, but the correct license name must appear in all legal screens.

Under the AGPL, distributing a binary that links JUCE without disclosing the full source code of your application is a license violation. The project does not wish to be fully open source under the AGPL.

## Decision

**Purchase a JUCE Commercial License (Indie or Pro tier).**

This allows Virtuoso Audio to be distributed under the MIT License while linking the JUCE framework, without triggering AGPL source disclosure obligations.

### License tiers (as of 2026)

| Tier | Annual cost | Revenue limit |
|---|---|---|
| Indie | ~$800 | < $50k/yr revenue |
| Pro / Studio | ~$2,000–$3,500 | No limit |

A JUCE Indie license is sufficient for an open-source project with no revenue. If the project begins generating revenue (paid HRIR packs, Pro tier), upgrade to Studio.

## Consequences

### License files to update
- `assets/legal/LICENSE` — MIT (project source)
- `assets/legal/THIRD_PARTY_LICENSES.md` — JUCE listed as "Commercial License (juce.com)"
- All installer legal screens
- `AboutPanel` in the GUI

### Compile-time
- `JUCE_DISPLAY_SPLASH_SCREEN=0` is permitted under commercial license
- `JUCE_COMMERCIAL_LICENSE=1` must be set in the CMake build (not a JUCE macro, just a reminder comment)

### SADIE II license correction
- Original plan stated: CC-BY-4.0
- **Correct license**: Apache License 2.0 (verified at University of York, sadie-binaural.s3.amazonaws.com)
- `ASSET_MANIFEST.json` license field must reflect `Apache-2.0` with required copyright notice and link to source

### Full dependency license matrix

| Dependency | License | Action required |
|---|---|---|
| JUCE 8 | Commercial (purchased) | Receipt stored in `docs/legal/` |
| libmysofa | BSD-3-Clause | Include notice in THIRD_PARTY_LICENSES.md |
| libsamplerate | BSD-2-Clause (or LGPL) | Include notice |
| nlohmann/json | MIT | Include notice |
| libsodium | ISC | Include notice |
| Catch2 | Boost Software License 1.0 | Include notice (test-only) |
| Inter Font | SIL Open Font License 1.1 | Include notice |
| CIPIC HRIRs | Public Domain (UC Davis) | Attribution in UI + manifest |
| MIT KEMAR | MIT | Citation: Bill Gardner & Keith Martin |
| SADIE II | Apache-2.0 | Copyright: University of York, 2016-present |
| OpenAL Soft | LGPL-2.1 | Attribution + link to source |

## Action Items
- [ ] Purchase JUCE commercial license. Store receipt in repo as `docs/legal/JUCE_LICENSE_RECEIPT.pdf` (gitignored for privacy, but referenced in CI)
- [ ] Update `assets/legal/LICENSE` header
- [ ] Update `assets/legal/THIRD_PARTY_LICENSES.md`
- [ ] Update `assets/hrir/ASSET_MANIFEST.json` (SADIE II → Apache-2.0)
- [ ] CI license-lint job fails build if ASSET_MANIFEST.json contains disallowed license IDs
