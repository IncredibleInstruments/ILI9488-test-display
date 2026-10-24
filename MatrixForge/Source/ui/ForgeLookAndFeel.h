#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mf
{
namespace colours
{
const juce::Colour background { 0xff121417 };
const juce::Colour panel      { 0xff1c2026 };
const juce::Colour panelEdge  { 0xff2e353e };
const juce::Colour accent     { 0xff3fa9f5 };
const juce::Colour accentAlt  { 0xffff8a3d };
const juce::Colour text       { 0xffd9dee5 };
const juce::Colour textDim    { 0xff8a95a3 };
const juce::Colour lcdBack    { 0xff05080b };
const juce::Colour lcdText    { 0xff7fd8ff };
} // namespace colours

class ForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ForgeLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};
} // namespace mf
