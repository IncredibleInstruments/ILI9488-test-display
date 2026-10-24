#include "ForgeLookAndFeel.h"

namespace mf
{
ForgeLookAndFeel::ForgeLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, colours::lcdBack);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::outlineColourId, colours::panelEdge);
    setColour (juce::ComboBox::arrowColourId, colours::accent);
    setColour (juce::PopupMenu::backgroundColourId, colours::panel);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, colours::panel);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Label::textColourId, colours::textDim);
    setColour (juce::ResizableWindow::backgroundColourId, colours::background);
}

void ForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcRadius = radius - 2.0f;
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;

    // Track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (colours::panelEdge.brighter (0.2f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (slider.isEnabled() ? colours::accent : colours::textDim);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body
    const float bodyR = radius * 0.72f;
    juce::ColourGradient grad (juce::Colour (0xff3a414b), centre.x, centre.y - bodyR, juce::Colour (0xff15181c), centre.x, centre.y + bodyR, false);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Pointer
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -bodyR, 3.0f, bodyR * 0.55f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (slider.isMouseOverOrDragging() ? 1.0f : 0.85f));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void ForgeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float cx = bounds.getCentreX();
    g.setColour (colours::lcdBack);
    g.fillRoundedRectangle (cx - 2.5f, bounds.getY(), 5.0f, bounds.getHeight(), 2.5f);
    g.setColour (colours::accent);
    g.fillRoundedRectangle (cx - 1.5f, pos, 3.0f, bounds.getBottom() - pos, 1.5f);

    const float capW = juce::jmin (bounds.getWidth() - 4.0f, 26.0f), capH = 12.0f;
    auto cap = juce::Rectangle<float> (cx - capW * 0.5f, pos - capH * 0.5f, capW, capH);
    juce::ColourGradient grad (juce::Colour (0xffe8ecf0), cap.getX(), cap.getY(), juce::Colour (0xff9aa3ad), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (cap, 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawHorizontalLine ((int) pos, cap.getX() + 2.0f, cap.getRight() - 2.0f);
}

void ForgeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (colours::lcdBack);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (highlighted ? colours::accent : colours::panelEdge);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    const float led = 7.0f;
    auto ledArea = juce::Rectangle<float> (bounds.getX() + 6.0f, bounds.getCentreY() - led * 0.5f, led, led);
    g.setColour (on ? colours::accent : colours::panelEdge.brighter (0.3f));
    g.fillEllipse (ledArea);
    if (on)
    {
        g.setColour (colours::accent.withAlpha (0.3f));
        g.fillEllipse (ledArea.expanded (3.0f));
    }

    g.setColour (on ? colours::text : colours::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (b.getButtonText(), bounds.withTrimmedLeft (17.0f), juce::Justification::centred);
}

void ForgeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (down ? colours::accent.withAlpha (0.4f) : colours::panel.brighter (highlighted ? 0.15f : 0.05f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (highlighted ? colours::accent : colours::panelEdge);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void ForgeLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (colours::lcdBack);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (box.isMouseOver (true) ? colours::accent : colours::panelEdge);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 11.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (colours::accent);
    g.fillPath (arrow);
}

juce::Font ForgeLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::Font (juce::FontOptions (12.0f)); }
juce::Font ForgeLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return juce::Font (juce::FontOptions (12.0f, juce::Font::bold)); }
juce::Font ForgeLookAndFeel::getPopupMenuFont() { return juce::Font (juce::FontOptions (14.0f)); }

void ForgeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (2, 0, box.getWidth() - 18, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}
} // namespace mf
