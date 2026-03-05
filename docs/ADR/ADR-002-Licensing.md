# ADR-002: JUCE Commercial License Requirement

**Date**: 2026-03-04  
**Status**: Accepted  
**Deciders**: Lead developer

---

## Context

Virtuoso Audio uses JUCE 8 as its cross-platform audio framework. JUCE has a
dual-license model:

- **AGPLv3** (free) — requires all application code to be open-sourced under AGPLv3
- **Commercial license** — allows proprietary / MIT distribution

This project's codebase is MIT-licensed (`assets/legal/LICENSE`).

Distributing a JUCE-based application under MIT while using JUCE's free AGPLv3 tier
would create a license conflict: AGPLv3 is copyleft and cannot be re-licensed under MIT.

---

## Decision

**Virtuoso Audio requires a JUCE Commercial License before public distribution.**

The JUCE commercial license is purchased from Raw Material Software (Steinberg-owned)
and permits:
- Distribution of applications under any license (including MIT, proprietary)
- No AGPLv3 copyleff propagation to application code
- Commercial/production use

Monthly cost: ~$50/month (JUCE Indie) or ~$800/month (JUCE Pro) — see juce.com/pricing.

---

## Implementation Notes

- `CPMAddPackage(JUCE ...)` in `cmake/Dependencies.cmake` fetches JUCE from GitHub
- Build works without a license during development
- Before any public binary release, purchase license at juce.com and add license key
  to `JUCE_APPLICATION_LICENSE_EMAIL` CMake variable (or JUCE projucer config)

---

## Consequences

**Positive**:
- No AGPLv3 copyleft on application code
- Binary distribution under MIT is clean

**Negative**:
- Ongoing recurring cost ($50–$800/month)
- Must purchase before v1.0.0 release date

**Alternatives considered**:
- `miniaudio` + `RTAudio` — missing convolver DSP, HRIR pipeline, GUI components
- `PortAudio` — cross-platform but no DSP or GUI; requires more custom code
- Decided: JUCE is the only framework that provides WaveRT/CoreAudio integration,
  JUCE DSP (Convolution, Limiter, EQ), and cross-platform GUI in one package
