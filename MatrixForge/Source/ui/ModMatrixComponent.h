#pragma once

#include "../Params.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace mf
{
// 10 x 16 grid of bipolar modulation amounts. Drag a cell up/down to set it,
// Shift for fine control, double-click to clear, right-click for a menu.
// Row labels show the live level of each source; cells glow while they modulate.
class ModMatrixComponent : public juce::Component, private juce::Timer
{
public:
    ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts, std::function<float (int)> sourceValue);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void (const juce::String& name, const juce::String& value)> onHover;

    // Why a route would be inaudible with the current patch, or empty if it is fine.
    juce::String explainSilentRoute (int source, int dest) const;

private:
    void timerCallback() override;
    juce::Rectangle<float> cellBounds (int source, int dest) const;
    bool cellAt (juce::Point<float> p, int& source, int& dest) const;
    void reportHover (int source, int dest);
    void showCellMenu (int source, int dest);
    void setCell (int source, int dest, float amount);
    float param (const char* id) const;

    struct Cell
    {
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<juce::ParameterAttachment> attachment;
        float value = 0.0f; // plain (denormalised) amount
    };

    juce::AudioProcessorValueTreeState& state;
    std::function<float (int)> getSourceValue;
    Cell cells[kNumModSources][kNumModDests];
    float liveSources[kNumModSources] {};
    int dragSource = -1, dragDest = -1, hoverSource = -1, hoverDest = -1;
    float dragStartValue = 0.0f; // normalised
    juce::String footerWarning;

    static constexpr float labelWidth = 66.0f, groupHeight = 13.0f, headerHeight = 28.0f, footerHeight = 16.0f;
};
} // namespace mf
