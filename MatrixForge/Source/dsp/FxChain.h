#pragma once

#include "../Params.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace mf
{
// Chorus/flanger -> stereo (ping-pong) delay -> reverb.
class FxChain
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Snapshot& p, double bpm);

private:
    void processDelay (juce::AudioBuffer<float>& buffer, const Snapshot& p, double bpm);

    double sampleRate = 44100.0;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;

    std::vector<float> delayL, delayR;
    int writePos = 0;
    float delaySmoothed = 0.0f;
    float dampL = 0.0f, dampR = 0.0f;
    float delayTail = 0.0f;
};
} // namespace mf
