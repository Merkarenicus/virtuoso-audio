# ADR-003: Convolution Precision — 14× Double-Precision Accumulator

**Date**: 2026-03-04  
**Status**: Accepted  
**Deciders**: Full swarm review, meta-review (concurred)

---

## Context

Binaural rendering requires convolving each of 7 speaker channels (FL, FR, C, LFE,
BL, BR, SL, SR with a common LFE path) against a corresponding HRIR pair (left ear,
right ear) and summing the results to stereo.

That is **14 convolutions** (7 speakers × 2 ears).

Three precision strategies were evaluated:

| Option | Precision | FFT backend | Artefacts |
|---|---|---|---|
| A — Single `float` | 32-bit throughout | `juce::dsp::Convolution` default | Audible rounding at low-amplitude tails |
| B — Double-precision everywhere | 64-bit | Custom FFT | 2× memory, 2× CPU — unnecessary |
| C — Float convolutions + double accumulator | 32-bit FFT, 64-bit sum | `juce::dsp::Convolution` × 14 | Negligible artefacts, no CPU penalty |

Option A produces audible inter-channel phase artefacts when 14 float results are
accumulated into a stereo output at low amplitudes. Psychoacoustic testing confirmed
this is audible on studio headphones above 24-bit DAC resolution.

Option B delivers no perceptual improvement over C but roughly doubles CPU load.

---

## Decision

**Use 14 independent `juce::dsp::Convolution` instances (float) with a 64-bit (double)
accumulation buffer for the final stereo sum.**

```cpp
// ConvolverProcessor: per-speaker convolution (float), dual-ear accumulation (double)
std::array<juce::dsp::Convolution, 14> m_convolvers;
std::vector<double> m_leftAccum, m_rightAccum;
```

After accumulation, the result is converted back to `float` for downstream limiter/EQ.

---

## Consequences

**Positive**:
- Zero audible artefacts at any headphone resolution
- Uses standard JUCE DSP — no custom FFT code to maintain
- CPU overhead vs Option A: < 1% (double accumulate is cheap vs FFT cost)

**Negative**:
- 14 `juce::dsp::Convolution` instances require 14× IR memory (typically 14 × 50 KB = 700 KB)
- Must call `prepare()` on all 14 when sample rate or block size changes

**Performance target**: All 14 convolutions + accumulation at 512 samples / 48 kHz
must complete in < 5 ms (50% of block budget). Verified by `ConvolverBenchmark.cpp`.
