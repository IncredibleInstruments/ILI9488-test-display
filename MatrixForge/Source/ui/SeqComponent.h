#pragma once

#include "../Params.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace mf
{
// 16 step pitch lanes plus gate toggles. Drag a lane to set its transpose,
// click the strip below a lane to toggle its gate.
class SeqComponent : public juce::Component
{
public:
    explicit SeqComponent (juce::AudioProcessorValueTreeState& apvts);

    void setPlayingStep (int step);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    std::function<void (const juce::String& name, const juce::String& value)> onHover;

private:
    struct Lane
    {
        juce::RangedAudioParameter* step = nullptr;
        juce::RangedAudioParameter* gate = nullptr;
        std::unique_ptr<juce::ParameterAttachment> stepAttachment, gateAttachment;
        float stepValue = 0.0f, gateValue = 1.0f;
    };

    int laneAt (float x) const;
    float gateStripTop() const { return (float) getHeight() - 16.0f; }
    void setFromY (int lane, float y);

    Lane lanes[kNumSeqSteps];
    juce::RangedAudioParameter* lengthParam = nullptr;
    std::unique_ptr<juce::ParameterAttachment> lengthAttachment;
    int length = kNumSeqSteps;
    int playingStep = -1, dragLane = -1;
};
} // namespace mf
