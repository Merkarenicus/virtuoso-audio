// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// LookAndFeel.h — Virtuoso custom JUCE look-and-feel
// Dark premium aesthetic: deep navy/charcoal backgrounds, electric blue
// accents, Inter font, smooth gradients, subtle glassmorphism panels.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace virtuoso {

namespace Colours {
// Core palette
static const juce::Colour Background{0xFF0D0F14};      // Near-black
static const juce::Colour Surface{0xFF161A22};         // Card surface
static const juce::Colour SurfaceElevated{0xFF1E2330}; // Raised panels
static const juce::Colour Border{0xFF2A3045};          // Subtle borders
static const juce::Colour Accent{0xFF3A8EFF};          // Electric blue
static const juce::Colour AccentBright{0xFF60AAFF};    // Hover accent
static const juce::Colour AccentGlow{0x303A8EFF};    // Glow/halo (30% opacity)
static const juce::Colour TextPrimary{0xFFEEF2FF};   // Near-white
static const juce::Colour TextSecondary{0xFF8899BB}; // Muted text
static const juce::Colour TextDisabled{0xFF445566};  // Disabled
static const juce::Colour Success{0xFF2EC086};       // Green (active/ok)
static const juce::Colour Warning{0xFFFFAA33};       // Amber
static const juce::Colour Error{0xFFFF4455};         // Red
static const juce::Colour Meter{0xFF3A8EFF};         // Level meter
static const juce::Colour MeterClip{0xFFFF4455};     // Clipping red
} // namespace Colours

class VirtuosoLookAndFeel : public juce::LookAndFeel_V4 {
public:
  VirtuosoLookAndFeel();

  // ---------------------------------------------------------------------------
  // Buttons
  // ---------------------------------------------------------------------------
  void drawButtonBackground(juce::Graphics &g, juce::Button &b,
                            const juce::Colour &bgColour, bool highlighted,
                            bool down) override;
  void drawButtonText(juce::Graphics &g, juce::TextButton &b, bool highlighted,
                      bool down) override;
  void drawToggleButton(juce::Graphics &g, juce::ToggleButton &b,
                        bool highlighted, bool down) override;

  // ---------------------------------------------------------------------------
  // Sliders
  // ---------------------------------------------------------------------------
  void drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h,
                        float sliderPos, float minPos, float maxPos,
                        juce::Slider::SliderStyle, juce::Slider &) override;
  void drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                        float sliderPos, float startAngle, float endAngle,
                        juce::Slider &) override;
  juce::Slider::SliderLayout getSliderLayout(juce::Slider &) override;

  // ---------------------------------------------------------------------------
  // ComboBox
  // ---------------------------------------------------------------------------
  void drawComboBox(juce::Graphics &g, int w, int h, bool down, int bx, int by,
                    int bw, int bh, juce::ComboBox &) override;

  // ---------------------------------------------------------------------------
  // General
  // ---------------------------------------------------------------------------
  void drawGroupComponentOutline(juce::Graphics &g, int w, int h,
                                 const juce::String &text,
                                 const juce::Justification &,
                                 juce::GroupComponent &) override;

  // Shared font (Inter)
  static juce::Font getVirtuosoFont(float size = 14.0f, bool bold = false);

private:
  juce::Typeface::Ptr m_interTypeface;
  void applyDefaultColours();

  // Helper: draw a glowing rounded rect (used for active elements)
  static void drawGlowRect(juce::Graphics &g, juce::Rectangle<float> bounds,
                           juce::Colour glowColour, float cornerRadius,
                           float glowRadius);
};

} // namespace virtuoso
