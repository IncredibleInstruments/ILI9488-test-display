#include "FxChain.h"

namespace mf
{
void FxChain::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    chorus.prepare ({ sr, (juce::uint32) maxBlockSize, 2 });
    reverb.setSampleRate (sr);
    delayL.assign ((size_t) (sr * 2.6) + 4, 0.0f);
    delayR.assign (delayL.size(), 0.0f);
    reset();
}

void FxChain::reset()
{
    chorus.reset();
    reverb.reset();
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    writePos = 0;
    delaySmoothed = 0.0f;
    dampL = dampR = 0.0f;
    delayTail = 0.0f;
}

void FxChain::process (juce::AudioBuffer<float>& buffer, const Snapshot& p, double bpm)
{
    const int chorusType = (int) p.chType;
    if (chorusType != 0 && buffer.getNumChannels() >= 2)
    {
        const bool flanger = chorusType == 2;
        chorus.setRate (p.chRate);
        chorus.setDepth (flanger ? 0.2f + p.chDepth * 0.8f : p.chDepth);
        chorus.setCentreDelay (flanger ? 2.0f : 9.0f);
        chorus.setFeedback (flanger ? 0.55f : 0.0f);
        chorus.setMix (p.chMix);
        juce::dsp::AudioBlock<float> block (buffer);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    if (p.dlOn >= 0.5f || delayTail > 1.0e-5f)
        processDelay (buffer, p, bpm);

    if (p.rvOn >= 0.5f && buffer.getNumChannels() >= 2)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + p.rvSize * 0.68f;
        rp.damping = p.rvDamp;
        rp.wetLevel = p.rvMix * 0.7f;
        rp.dryLevel = 1.0f - p.rvMix * 0.5f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
    }
}

void FxChain::processDelay (juce::AudioBuffer<float>& buffer, const Snapshot& p, double bpm)
{
    const bool on = p.dlOn >= 0.5f;
    const int size = (int) delayL.size();
    const float maxDelay = (float) (size - 3);

    float targetSamples = p.dlSync >= 0.5f ? (float) (sampleRate * 60.0 / bpm * divisionBeats ((int) p.dlDiv))
                                           : (float) (p.dlTime * 0.001 * sampleRate);
    targetSamples = juce::jlimit (16.0f, maxDelay, targetSamples);
    if (delaySmoothed <= 0.0f)
        delaySmoothed = targetSamples;

    const float feedback = on ? p.dlFeedback : 0.0f;
    const float mix = on ? p.dlMix : 0.0f;
    const bool pingPong = p.dlPingPong >= 0.5f;
    const float smooth = 1.0f - std::exp (-1.0f / (0.08f * (float) sampleRate));
    const float dampCoef = 0.35f;

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    float tail = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        delaySmoothed += (targetSamples - delaySmoothed) * smooth;

        float readPos = (float) writePos - delaySmoothed;
        if (readPos < 0.0f)
            readPos += (float) size;
        const int i0 = (int) readPos;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - (float) i0;
        const float wetL = delayL[(size_t) i0] + frac * (delayL[(size_t) i1] - delayL[(size_t) i0]);
        const float wetR = delayR[(size_t) i0] + frac * (delayR[(size_t) i1] - delayR[(size_t) i0]);

        dampL += (wetL - dampL) * dampCoef;
        dampR += (wetR - dampR) * dampCoef;

        const float inL = left[i];
        const float inR = right != nullptr ? right[i] : inL;
        const float inGain = on ? 1.0f : 0.0f;

        if (pingPong)
        {
            delayL[(size_t) writePos] = (inL + inR) * 0.5f * inGain + dampR * feedback;
            delayR[(size_t) writePos] = dampL * feedback;
        }
        else
        {
            delayL[(size_t) writePos] = inL * inGain + dampL * feedback;
            delayR[(size_t) writePos] = inR * inGain + dampR * feedback;
        }
        writePos = (writePos + 1) % size;

        left[i] = inL + wetL * mix;
        if (right != nullptr)
            right[i] = inR + wetR * mix;

        tail = std::max (tail, std::abs (wetL) + std::abs (wetR));
    }

    delayTail = on ? 1.0f : tail * feedback;
    if (! on && delayTail <= 1.0e-5f)
    {
        std::fill (delayL.begin(), delayL.end(), 0.0f);
        std::fill (delayR.begin(), delayR.end(), 0.0f);
    }
}
} // namespace mf
