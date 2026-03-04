// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
// See assets/legal/THIRD_PARTY_LICENSES.md — requires JUCE Commercial License
//
// ConvolverProcessor.h
// 7.1-to-binaural convolution engine.
//
// Architecture (per ADR-003):
//   - 14 × juce::dsp::Convolution (7 speaker channels × 2 ears)
//   - LFE handled separately via BassManagement (120 Hz LPF, +6 dB, omnidirectional)
//   - Summation in double accumulator, output as float
//   - HRIR hot-swap via lock-free queue (UI thread → audio thread)
//
// Input channel order (canonical — see ChannelLayoutMapper):
//   0=FL, 1=FR, 2=C, 3=LFE, 4=BL, 5=BR, 6=SL, 7=SR

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <span>

namespace virtuoso {

// ---------------------------------------------------------------------------
// HrirData — immutable, ref-counted HRIR for a single speaker channel
// ---------------------------------------------------------------------------
struct HrirChannelPair {
    juce::AudioBuffer<float> left;   // Left-ear impulse response (mono)
    juce::AudioBuffer<float> right;  // Right-ear impulse response (mono)
    double                   sampleRate{48000.0};
    juce::String             label;  // e.g. "FL", "FR", "C", ...

    bool isValid() const noexcept {
        return left.getNumSamples() > 0
            && right.getNumSamples() > 0
            && left.getNumSamples() == right.getNumSamples()
            && sampleRate > 0.0;
    }
};

// 7 speaker positions (LFE excluded — handled by BassManagement)
enum class SpeakerIndex : int {
    FL = 0, FR = 1, C = 2, BL = 3, BR = 4, SL = 5, SR = 6,
    Count = 7
};

// Complete HRIR set: one pair per speaker
struct HrirSet {
    static constexpr int kSpeakerCount = static_cast<int>(SpeakerIndex::Count);
    std::array<HrirChannelPair, kSpeakerCount> channels;
    juce::String name;   // e.g. "CIPIC Subject 003"
    juce::String source; // e.g. "CIPIC / MIT KEMAR / SADIE II"

    bool isValid() const noexcept {
        for (auto& ch : channels)
            if (!ch.isValid()) return false;
        return true;
    }
};

// ---------------------------------------------------------------------------
// ConvolverProcessor
// Thread-safety: prepare/reset must be called on audio thread.
// loadHrir() may be called from any thread (uses lock-free exchange).
// process() must be called on the audio thread only.
// ---------------------------------------------------------------------------
class ConvolverProcessor {
public:
    ConvolverProcessor();
    ~ConvolverProcessor() = default;

    // Not copyable; may be moved
    ConvolverProcessor(const ConvolverProcessor&) = delete;
    ConvolverProcessor& operator=(const ConvolverProcessor&) = delete;

    // -----------------------------------------------------------------------
    // Audio-thread API
    // -----------------------------------------------------------------------

    // Call before first use (or after device change).
    // spec.numChannels is ignored — ConvolverProcessor always inputs 8 ch and outputs 2.
    void prepare(const juce::dsp::ProcessSpec& spec);

    // Reset convolver state (clears delay lines). Call before offline rendering.
    void reset();

    // Process one block. input must have >= 8 channels, output must have >= 2.
    // Channels in input beyond 7 are ignored. input/output may overlap in time
    // but NOT in memory (no in-place processing).
    //
    // Returns false if no HRIR is loaded (output is silence).
    bool process(const juce::AudioBuffer<float>& input8ch,
                 juce::AudioBuffer<float>&       output2ch) noexcept;

    // -----------------------------------------------------------------------
    // HRIR management (thread-safe — may be called from any thread)
    // -----------------------------------------------------------------------

    // Enqueue a new HRIR set for hot-swap. The audio thread will pick it up
    // at the start of the next process() call. This method does NOT block.
    void loadHrir(std::shared_ptr<const HrirSet> hrirSet);

    // Returns the currently active HRIR name (may be empty if none loaded).
    juce::String getCurrentHrirName() const;

    // -----------------------------------------------------------------------
    // Status / diagnostics
    // -----------------------------------------------------------------------
    bool isHrirLoaded() const noexcept { return m_hrirLoaded.load(std::memory_order_relaxed); }
    double getSampleRate() const noexcept { return m_sampleRate; }
    int    getBlockSize()  const noexcept { return m_blockSize; }

private:
    static constexpr int kSpeakers = static_cast<int>(SpeakerIndex::Count);  // 7
    static constexpr int kEars     = 2;  // L=0, R=1

    // 14 convolvers: [speaker][ear]
    using Conv = juce::dsp::Convolution;
    std::array<std::array<std::unique_ptr<Conv>, kEars>, kSpeakers> m_convolvers;

    // Temporary per-speaker mono buffers (allocated in prepare())
    std::array<juce::AudioBuffer<float>, kSpeakers> m_monoBufs;

    // Current spec
    double m_sampleRate{48000.0};
    int    m_blockSize{512};

    // Hot-swap: pending HRIR set from UI thread
    // Uses compare-exchange on a raw shared_ptr to avoid mutex on audio thread
    juce::AbstractFifo             m_pendingFifo{1};
    std::shared_ptr<const HrirSet> m_pendingHrir;          // written by UI thread
    std::shared_ptr<const HrirSet> m_activeHrir;           // read by audio thread only
    std::atomic<bool>              m_hrirLoaded{false};
    juce::SpinLock                 m_pendingLock;           // protects m_pendingHrir write

    // Apply a newly received HRIR set on the audio thread
    void applyPendingHrir_AudioThread();
    void loadHrirIntoConvolvers(const HrirSet& hrirSet);
};

} // namespace virtuoso
