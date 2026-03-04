# ADR-003: Convolution Engine Precision

**Status**: Accepted  
**Date**: 2026-03-04  
**Deciders**: Virtuoso Audio Project  
**Primary source**: JUCE 8 API documentation + Grok 4.1/4.2 review

---

## Context

The original plan specified "JUCE 8's partitioned double-precision convolution." This claim is **factually incorrect**.

`juce::dsp::Convolution` is:
1. **Float-only** — processes `AudioBuffer<float>`, with no double-precision overload
2. **Stereo-input configured** — internally processes stereo blocks via FFT on `float` samples

Additionally, `juce::dsp::Convolution` internally correlates its template argument with `juce::dsp::ProcessContextReplacing<float>`. There is no `double` template instantiation in any JUCE 8 release.

Furthermore, the original plan's reference to a single `ConvolverProcessor` managing "8-to-2 convolution in one instance" is incorrect. JUCE's class handles a single stereo IR applied to a stereo input. Processing 8 independent mono channels each through a different stereo HRIR pair requires **8 separate instances** (or 16 mono instances).

## Decision

**Use 14 `juce::dsp::Convolution` instances (float), one per ear per speaker, with a double-precision accumulator for the final summation step.**

### Rationale

1. **Float is sufficient for 24-bit audio.** 32-bit float provides ~144 dB of dynamic range, well above the ~144 dB of 24-bit audio. Double precision would provide no audible benefit and would require a custom FFT backend.

2. **Custom double-precision FFT backend deferred.** If a blind A/B test ever demonstrates audible artifacts from float accumulation, a `kissfft`-based double backend can be added in v1.1. This is premature optimization for MVP.

3. **Double accumulator for summation.** Summing 14 float values into a `double` accumulator before casting back to `float` prevents rounding error accumulation across the summation stage.

## Implementation Contract

```
ConvolverProcessor owns:
  - 14 × juce::dsp::Convolution (float)
    → convolvers[SPEAKER][EAR]   // SPEAKER in {FL,FR,C,SL,SR,BL,BR}, EAR in {L,R}
  - 1  × BassManagement (4th-order Butterworth LPF @ 120 Hz for LFE)

For each audio block:
  for each speaker channel s in {FL,FR,C,SL,SR,BL,BR}:
    mono_in = input.getChannelData(channelIndex[s])
    convolvers[s][L].process(mono_in → conv_L[s])
    convolvers[s][R].process(mono_in → conv_R[s])

  lfe_in = input.getChannelData(LFE)
  lfe_filtered = bassManagement.process(lfe_in)  // 120 Hz LPF + 6 dB gain

  // Double-precision summation
  double sumL = lfe_filtered_L
  double sumR = lfe_filtered_R
  for s in {FL,FR,C,SL,SR,BL,BR}:
    sumL += (double)conv_L[s]
    sumR += (double)conv_R[s]

  output[L] = (float)sumL
  output[R] = (float)sumR
```

### HRIR Loading

Each HRIR pack contains stereo impulse responses. For the 7-speaker configuration:

- From a HeSuVi 14-channel WAV: extract column pairs (see CHANNEL_MAPPING.md)
- From a SOFA file: extract HRIRs for the 7 canonical azimuth/elevation positions closest to the standard 7.1 speaker positions
- Resample HRIR to engine sample rate (offline, at import time — not at runtime)
- Store as `HrirData` struct with metadata (sample rate, length, channel map)

### Acceptance Test

Unit test `ConvolverDiracPerChannel`:
1. Load known HRIR for FL channel (left ear = known impulse A, right ear = known impulse B)
2. Feed Dirac impulse on FL channel input, zeros on all others
3. Assert: output[L] equals convolution of Dirac with impulse A within RMS ≤ 1e-6
4. Assert: output[R] equals convolution of Dirac with impulse B within RMS ≤ 1e-6

## Consequences

- Remove all references to "double-precision convolution" from documentation and marketing
- Remove all references to a single `ConvolverProcessor` managing the 8→2 conversion in one call
- Add `ConvolverProcessor::resetState()` method used by `OfflineRenderer` to ensure deterministic offline output
