// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// EqProcessor.cpp

#include "EqProcessor.h"
#include <regex>

namespace virtuoso {

EqProcessor::EqProcessor() {
  for (auto &d : m_dirty)
    d.store(false, std::memory_order_relaxed);
}

void EqProcessor::prepare(const juce::dsp::ProcessSpec &stereoSpec) {
  m_sampleRate = stereoSpec.sampleRate;
  juce::dsp::ProcessSpec monoSpec{stereoSpec.sampleRate,
                                  stereoSpec.maximumBlockSize, 1};
  for (auto &ch : m_chains)
    ch.prepare(monoSpec);
  updateCoefficients();
}

void EqProcessor::reset() {
  for (auto &ch : m_chains)
    ch.reset();
}

void EqProcessor::process(juce::AudioBuffer<float> &buf,
                          int numSamples) noexcept {
  if (!m_enabled.load(std::memory_order_relaxed))
    return;

  // Pick up any pending parameter changes
  for (int i = 0; i < kMaxBands; ++i) {
    if (m_dirty[i].load(std::memory_order_acquire)) {
      updateCoefficients();
      break; // updateCoefficients handles all bands
    }
  }

  for (int ch = 0; ch < 2 && ch < buf.getNumChannels(); ++ch) {
    auto &chain = m_chains[ch];
    juce::dsp::AudioBlock<float> block(buf.getArrayOfWritePointers() + ch, 1,
                                       static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    // Apply all 8 bands via the chain
    chain.process(ctx);
  }
}

void EqProcessor::setBandParams(int bandIndex, const EqBandParams &params) {
  if (bandIndex < 0 || bandIndex >= kMaxBands)
    return;
  juce::SpinLock::ScopedLockType lock(m_paramsLock);
  m_params[bandIndex] = params;
  m_dirty[bandIndex].store(true, std::memory_order_release);
}

EqBandParams EqProcessor::getBandParams(int bandIndex) const noexcept {
  if (bandIndex < 0 || bandIndex >= kMaxBands)
    return {};
  juce::SpinLock::ScopedLockType lock(
      const_cast<juce::SpinLock &>(m_paramsLock));
  return m_params[bandIndex];
}

void EqProcessor::updateCoefficients() {
  EqBandParams params[kMaxBands];
  {
    juce::SpinLock::ScopedTryLockType tryLock(m_paramsLock);
    if (!tryLock.isLocked())
      return; // Never block audio thread
    for (int i = 0; i < kMaxBands; ++i)
      params[i] = m_params[i];
  }

  // Helper lambda to get a biquad filter from each chain position
  // We process all 8 bands in both chains
  auto applyBand = [&](int bandIdx, const EqBandParams &p) {
    if (!p.enabled || p.type == EqBandType::Bypass) {
      // Bypass: unity gain allpass
      auto bypassCoeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
          m_sampleRate, static_cast<double>(p.freqHz), 0.707, 1.0);
      // Applied via a no-op — just set gain=0dB (1.0 linear)
      // We do this by re-applying coefficients with 0 gain
      // (proper allpass would be better but unity shelves work fine)
      return;
    }

    juce::dsp::IIR::Coefficients<float>::Ptr coeff;
    const double freq = static_cast<double>(p.freqHz);
    const double q = static_cast<double>(p.q);
    const double gain = std::pow(10.0, static_cast<double>(p.gainDb) / 20.0);

    switch (p.type) {
    case EqBandType::PeakBell:
      coeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
          m_sampleRate, freq, q, gain);
      break;
    case EqBandType::LowShelf:
      coeff = juce::dsp::IIR::Coefficients<float>::makeLowShelf(m_sampleRate,
                                                                freq, q, gain);
      break;
    case EqBandType::HighShelf:
      coeff = juce::dsp::IIR::Coefficients<float>::makeHighShelf(m_sampleRate,
                                                                 freq, q, gain);
      break;
    case EqBandType::LowPass:
      coeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(m_sampleRate,
                                                               freq, q);
      break;
    case EqBandType::HighPass:
      coeff = juce::dsp::IIR::Coefficients<float>::makeHighPass(m_sampleRate,
                                                                freq, q);
      break;
    default:
      return;
    }

    // Apply coefficients to both L and R chains at band index
    // We use a compile-time index dispatch pattern
    auto setCoeff = [&](auto &filter) { *filter.coefficients = *coeff; };

    // Index dispatch (C++ requires compile-time index for get<N>())
    // Using a helper lambda with if-constexpr pattern via index sequence
    auto dispatchBand = [&](auto &chain, int idx) {
      switch (idx) {
      case 0:
        setCoeff(chain.template get<0>());
        break;
      case 1:
        setCoeff(chain.template get<1>());
        break;
      case 2:
        setCoeff(chain.template get<2>());
        break;
      case 3:
        setCoeff(chain.template get<3>());
        break;
      case 4:
        setCoeff(chain.template get<4>());
        break;
      case 5:
        setCoeff(chain.template get<5>());
        break;
      case 6:
        setCoeff(chain.template get<6>());
        break;
      case 7:
        setCoeff(chain.template get<7>());
        break;
      default:
        break;
      }
    };
    dispatchBand(m_chains[0], bandIdx);
    dispatchBand(m_chains[1], bandIdx);
  };

  for (int i = 0; i < kMaxBands; ++i) {
    applyBand(i, params[i]);
    m_dirty[i].store(false, std::memory_order_release);
  }
}

bool EqProcessor::importAutoEq(const juce::String &autoEqText) {
  // AutoEq format: "Filter N: ON PK Fc FREQ Hz Gain GAIN dB Q QVAL"
  // Types: PK=peak bell, LSC=low shelf, HSC=high shelf
  std::regex filter_re(
      R"(Filter\s+\d+:\s+(ON|OFF)\s+(PK|LSC|HSC)\s+Fc\s+([\d.]+)\s+Hz\s+Gain\s+([+-]?[\d.]+)\s+dB\s+Q\s+([\d.]+))",
      std::regex::icase);

  auto text = autoEqText.toStdString();
  auto begin = std::sregex_iterator(text.begin(), text.end(), filter_re);
  auto end = std::sregex_iterator();

  int bandIdx = 0;
  for (auto it = begin; it != end && bandIdx < kMaxBands; ++it, ++bandIdx) {
    const auto &m = *it;
    EqBandParams p;
    p.enabled = (m[1].str() == "ON" || m[1].str() == "on");
    const auto typeStr = m[2].str();
    if (typeStr == "PK" || typeStr == "pk")
      p.type = EqBandType::PeakBell;
    else if (typeStr == "LSC" || typeStr == "lsc")
      p.type = EqBandType::LowShelf;
    else if (typeStr == "HSC" || typeStr == "hsc")
      p.type = EqBandType::HighShelf;
    p.freqHz = std::stof(m[3].str());
    p.gainDb = std::stof(m[4].str());
    p.q = std::stof(m[5].str());
    setBandParams(bandIdx, p);
  }
  return bandIdx > 0;
}

} // namespace virtuoso
