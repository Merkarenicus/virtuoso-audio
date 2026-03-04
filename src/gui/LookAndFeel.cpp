// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// LookAndFeel.cpp

#include "LookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace virtuoso {

VirtuosoLookAndFeel::VirtuosoLookAndFeel() { applyDefaultColours(); }

void VirtuosoLookAndFeel::applyDefaultColours() {
  // Window
  setColour(juce::ResizableWindow::backgroundColourId, Colours::Background);

  // Text
  setColour(juce::Label::textColourId, Colours::TextPrimary);
  setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

  // TextButton
  setColour(juce::TextButton::buttonColourId, Colours::SurfaceElevated);
  setColour(juce::TextButton::buttonOnColourId, Colours::Accent);
  setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
  setColour(juce::TextButton::textColourOnId, Colours::TextPrimary);

  // ToggleButton
  setColour(juce::ToggleButton::textColourId, Colours::TextPrimary);
  setColour(juce::ToggleButton::tickColourId, Colours::Accent);
  setColour(juce::ToggleButton::tickDisabledColourId, Colours::TextDisabled);

  // Slider
  setColour(juce::Slider::thumbColourId, Colours::Accent);
  setColour(juce::Slider::trackColourId, Colours::Border);
  setColour(juce::Slider::backgroundColourId, Colours::Surface);
  setColour(juce::Slider::textBoxTextColourId, Colours::TextSecondary);
  setColour(juce::Slider::textBoxOutlineColourId, Colours::Border);
  setColour(juce::Slider::textBoxBackgroundColourId, Colours::Surface);

  // ComboBox
  setColour(juce::ComboBox::backgroundColourId, Colours::SurfaceElevated);
  setColour(juce::ComboBox::textColourId, Colours::TextPrimary);
  setColour(juce::ComboBox::outlineColourId, Colours::Border);
  setColour(juce::ComboBox::arrowColourId, Colours::TextSecondary);

  // PopupMenu
  setColour(juce::PopupMenu::backgroundColourId, Colours::Surface);
  setColour(juce::PopupMenu::textColourId, Colours::TextPrimary);
  setColour(juce::PopupMenu::highlightedBackgroundColourId,
            Colours::Accent.withAlpha(0.3f));
  setColour(juce::PopupMenu::highlightedTextColourId, Colours::TextPrimary);

  // ScrollBar
  setColour(juce::ScrollBar::thumbColourId, Colours::Border);
  setColour(juce::ScrollBar::trackColourId, Colours::Background);

  // GroupComponent
  setColour(juce::GroupComponent::outlineColourId, Colours::Border);
  setColour(juce::GroupComponent::textColourId, Colours::TextSecondary);
}

juce::Font VirtuosoLookAndFeel::getVirtuosoFont(float size, bool bold) {
  // Inter is loaded from binary resources; fall back to system sans-serif
  return juce::Font(juce::FontOptions().withHeight(size).withStyle(
      bold ? "Bold" : "Regular"));
}

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------
void VirtuosoLookAndFeel::drawButtonBackground(juce::Graphics &g,
                                               juce::Button &b,
                                               const juce::Colour &,
                                               bool highlighted, bool down) {
  auto bounds = b.getLocalBounds().toFloat().reduced(0.5f);
  const float cornerR = 6.0f;

  juce::Colour baseColour =
      b.getToggleState() ? Colours::Accent : Colours::SurfaceElevated;
  if (highlighted)
    baseColour = baseColour.brighter(0.15f);
  if (down)
    baseColour = baseColour.darker(0.15f);

  // Fill
  g.setColour(baseColour);
  g.fillRoundedRectangle(bounds, cornerR);

  // Border
  g.setColour(b.getToggleState() ? Colours::AccentBright : Colours::Border);
  g.drawRoundedRectangle(bounds, cornerR, 1.0f);

  // Glow for active toggle buttons
  if (b.getToggleState()) {
    drawGlowRect(g, bounds, Colours::AccentGlow, cornerR, 4.0f);
  }
}

void VirtuosoLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b,
                                         bool, bool) {
  g.setFont(getVirtuosoFont(13.5f));
  g.setColour(b.getToggleState() ? Colours::TextPrimary
                                 : Colours::TextSecondary);
  g.drawFittedText(b.getButtonText(), b.getLocalBounds(),
                   juce::Justification::centred, 1);
}

void VirtuosoLookAndFeel::drawToggleButton(juce::Graphics &g,
                                           juce::ToggleButton &b,
                                           bool highlighted, bool) {
  const float toggleW = 36.0f, toggleH = 20.0f;
  auto bounds = b.getLocalBounds().toFloat();
  auto toggleBounds = juce::Rectangle<float>(
      bounds.getX(), bounds.getCentreY() - toggleH * 0.5f, toggleW, toggleH);

  // Track
  g.setColour(b.getToggleState() ? Colours::Accent : Colours::Border);
  g.fillRoundedRectangle(toggleBounds, toggleH * 0.5f);

  // Thumb
  const float thumbSize = toggleH - 4.0f;
  float thumbX = b.getToggleState() ? toggleBounds.getRight() - thumbSize - 2.0f
                                    : toggleBounds.getX() + 2.0f;
  g.setColour(juce::Colours::white);
  g.fillEllipse(thumbX, toggleBounds.getY() + 2.0f, thumbSize, thumbSize);

  // Label
  g.setFont(getVirtuosoFont(13.0f));
  g.setColour(Colours::TextPrimary);
  g.drawFittedText(
      b.getButtonText(),
      juce::Rectangle<int>((int)(toggleBounds.getRight() + 8), 0,
                           b.getWidth() - (int)toggleBounds.getRight() - 8,
                           b.getHeight()),
      juce::Justification::centredLeft, 1);
}

// ---------------------------------------------------------------------------
// Sliders
// ---------------------------------------------------------------------------
void VirtuosoLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                           int w, int h, float sliderPos,
                                           float minPos, float maxPos,
                                           juce::Slider::SliderStyle style,
                                           juce::Slider &s) {
  if (style == juce::Slider::LinearHorizontal) {
    const float trackY = y + h * 0.5f;
    const float trackH = 4.0f;

    // Track background
    g.setColour(Colours::Border);
    g.fillRoundedRectangle((float)x, trackY - trackH * 0.5f, (float)w, trackH,
                           trackH * 0.5f);

    // Track fill (left of thumb)
    g.setColour(Colours::Accent);
    g.fillRoundedRectangle((float)x, trackY - trackH * 0.5f, sliderPos - x,
                           trackH, trackH * 0.5f);

    // Thumb
    const float thumbR = 8.0f;
    g.setColour(Colours::Accent);
    g.fillEllipse(sliderPos - thumbR, trackY - thumbR, thumbR * 2.0f,
                  thumbR * 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawEllipse(sliderPos - thumbR, trackY - thumbR, thumbR * 2.0f,
                  thumbR * 2.0f, 1.5f);
  } else {
    LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minPos, maxPos,
                                     style, s);
  }
}

void VirtuosoLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                           int w, int h, float sliderPos,
                                           float startAngle, float endAngle,
                                           juce::Slider &) {
  auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h)
                    .reduced(4.0f);
  auto centre = bounds.getCentre();
  const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
  const float lineW = 3.0f;
  const float angle = startAngle + sliderPos * (endAngle - startAngle);

  // Outer arc track
  juce::Path arcPath;
  arcPath.addCentredArc(centre.x, centre.y, radius - lineW * 0.5f,
                        radius - lineW * 0.5f, 0.0f, startAngle, endAngle,
                        true);
  g.setColour(Colours::Border);
  g.strokePath(arcPath,
               juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Filled arc
  juce::Path fillPath;
  fillPath.addCentredArc(centre.x, centre.y, radius - lineW * 0.5f,
                         radius - lineW * 0.5f, 0.0f, startAngle, angle, true);
  g.setColour(Colours::Accent);
  g.strokePath(fillPath,
               juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Thumb dot
  const float thumbX = centre.x + std::sin(angle) * (radius - lineW);
  const float thumbY = centre.y - std::cos(angle) * (radius - lineW);
  g.setColour(Colours::AccentBright);
  g.fillEllipse(thumbX - 4.0f, thumbY - 4.0f, 8.0f, 8.0f);
}

juce::Slider::SliderLayout
VirtuosoLookAndFeel::getSliderLayout(juce::Slider &s) {
  auto layout = LookAndFeel_V4::getSliderLayout(s);
  return layout;
}

// ---------------------------------------------------------------------------
// ComboBox
// ---------------------------------------------------------------------------
void VirtuosoLookAndFeel::drawComboBox(juce::Graphics &g, int w, int h, bool,
                                       int, int, int, int,
                                       juce::ComboBox &box) {
  auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)w, (float)h);
  g.setColour(Colours::SurfaceElevated);
  g.fillRoundedRectangle(bounds, 5.0f);
  g.setColour(Colours::Border);
  g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);

  // Arrow
  const float arrowSize = 8.0f;
  juce::Path arrow;
  arrow.startNewSubPath(w - 18.0f, h * 0.5f - arrowSize * 0.3f);
  arrow.lineTo(w - 18.0f + arrowSize * 0.5f, h * 0.5f + arrowSize * 0.3f);
  arrow.lineTo(w - 18.0f + arrowSize, h * 0.5f - arrowSize * 0.3f);
  g.setColour(Colours::TextSecondary);
  g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

void VirtuosoLookAndFeel::drawGroupComponentOutline(juce::Graphics &g, int w,
                                                    int h,
                                                    const juce::String &text,
                                                    const juce::Justification &,
                                                    juce::GroupComponent &) {
  const float cornerR = 8.0f;
  const float textH = 13.0f;
  const float indent = 12.0f;
  auto font = getVirtuosoFont(textH);
  const float textW = font.getStringWidthFloat(text) + 8.0f;

  juce::Path p;
  p.addRoundedRectangle(0.5f, textH * 0.5f, (float)w - 1.0f,
                        (float)h - textH * 0.5f - 0.5f, cornerR);
  g.setColour(Colours::Border);
  g.strokePath(p, juce::PathStrokeType(1.0f));

  // Title text
  g.setFont(font);
  g.setColour(Colours::TextSecondary);
  g.drawText(text, (int)indent, 0, (int)textW, (int)textH,
             juce::Justification::centred, true);
}

void VirtuosoLookAndFeel::drawGlowRect(juce::Graphics &g,
                                       juce::Rectangle<float> bounds,
                                       juce::Colour glowColour,
                                       float cornerRadius, float glowRadius) {
  juce::Path glowPath;
  glowPath.addRoundedRectangle(bounds.expanded(glowRadius),
                               cornerRadius + glowRadius);
  juce::DropShadow shadow(glowColour, static_cast<int>(glowRadius), {});
  shadow.drawForPath(g, glowPath);
}

} // namespace virtuoso
