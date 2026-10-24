#pragma once

#include "../Params.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace mf
{
// 10 x 16 grid of bipolar modulation amounts. Drag a cell up/down to set it,
// double-click to clear, hold Shift for fine adjustment.
class ModMatrixComponent : public juce::Component
{
public:
    explicit ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void (const juce::String& name, const juce::String& value)> onHover;

private:
    juce::Rectangle<float> cellBounds (int source, int dest) const;
    bool cellAt (juce::Point<float> p, int& source, int& dest) const;
    void reportHover (int source, int dest);

    struct Cell
    {
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<juce::ParameterAttachment> attachment;
        float value = 0.0f; // plain (denormalised) amount
    };

    Cell cells[kNumModSources][kNumModDests];
    int dragSource = -1, dragDest = -1, hoverSource = -1, hoverDest = -1;
    float dragStartValue = 0.0f; // normalised

    static constexpr float labelWidth = 52.0f, headerHeight = 22.0f;
};
} // namespace mf
