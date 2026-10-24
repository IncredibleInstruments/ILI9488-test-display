#include "ArpSeq.h"

namespace mf
{
void ArpSeq::prepare (double sr)
{
    sampleRate = sr;
    held.clear();
    held.reserve (128);
    sortedScratch.reserve (128 * 4);
    running = false;
    playingNote = -1;
    samplesToNoteOff = -1.0;
    currentStep = -1;
}

double ArpSeq::stepLength (const Snapshot& p, double bpm) const
{
    const double beats = divisionBeats ((int) p.asDiv);
    const double base = sampleRate * 60.0 / bpm * beats;
    const double swing = p.seqSwing;
    return (stepIndex % 2 == 0) ? base * (1.0 + swing) : base * (1.0 - swing);
}

void ArpSeq::stopPlaying (juce::MidiBuffer& out, int pos)
{
    if (playingNote >= 0)
        out.addEvent (juce::MidiMessage::noteOff (1, playingNote), pos);
    playingNote = -1;
    samplesToNoteOff = -1.0;
}

int ArpSeq::nextArpNote (const Snapshot& p)
{
    const int pattern = (int) p.arpPattern;
    const int octaves = juce::jlimit (1, 4, (int) p.arpOct);

    sortedScratch.clear();
    for (int o = 0; o < octaves; ++o)
        for (auto& h : held)
            if (h.note + o * 12 < 128)
                sortedScratch.push_back (h.note + o * 12);

    if (pattern != 4) // all but "As Played"
        std::sort (sortedScratch.begin(), sortedScratch.end());

    const int n = (int) sortedScratch.size();
    if (n == 0)
        return -1;

    int index = 0;
    switch (pattern)
    {
        case 1: // Down
            index = n - 1 - (arpIndex % n);
            ++arpIndex;
            break;
        case 2: // Up/Down without repeating the end notes
        {
            const int cycle = std::max (1, 2 * n - 2);
            const int pos = arpIndex % cycle;
            index = pos < n ? pos : cycle - pos;
            ++arpIndex;
            break;
        }
        case 3: // Random
            index = random.nextInt (n);
            break;
        default: // Up, As Played
            index = arpIndex % n;
            ++arpIndex;
            break;
    }
    return sortedScratch[(size_t) index];
}

void ArpSeq::stepTrigger (juce::MidiBuffer& out, int pos, const Snapshot& p)
{
    stopPlaying (out, pos);

    const int mode = (int) p.asMode;
    int note = -1;
    float velocity = held.empty() ? 0.8f : held.back().velocity;

    if (mode == asArp)
        note = nextArpNote (p);
    else
    {
        const int length = juce::jlimit (1, kNumSeqSteps, (int) p.seqLength);
        const int step = stepIndex % length;
        currentStep = step;
        if (p.seqGate[step] >= 0.5f && ! held.empty())
            note = juce::jlimit (0, 127, held.back().note + (int) p.seqStep[step]);
    }

    if (mode == asArp)
        currentStep = stepIndex % kNumSeqSteps;

    if (note >= 0)
    {
        out.addEvent (juce::MidiMessage::noteOn (1, note, juce::jlimit (0.05f, 1.0f, velocity)), pos);
        playingNote = note;
    }
}

void ArpSeq::process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, const Snapshot& p, double bpm)
{
    const int mode = (int) p.asMode;

    if (mode != lastMode)
    {
        stopPlaying (out, 0);
        if (mode == asOff)
        {
            // Hand the keys that are still down back to the synth
            for (auto& h : held)
                out.addEvent (juce::MidiMessage::noteOn (1, h.note, h.velocity), 0);
        }
        else if (lastMode == asOff)
        {
            for (auto& h : held)
                out.addEvent (juce::MidiMessage::noteOff (1, h.note), 0);
        }
        running = ! held.empty() && mode != asOff;
        samplesToNextStep = 0.0;
        lastMode = mode;
    }

    if (mode == asOff)
    {
        currentStep = -1;
        for (const auto meta : in)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
                held.push_back ({ m.getNoteNumber(), m.getFloatVelocity() });
            else if (m.isNoteOff())
                held.erase (std::remove_if (held.begin(), held.end(), [&m] (const Held& h) { return h.note == m.getNoteNumber(); }), held.end());
            out.addEvent (m, meta.samplePosition);
        }
        return;
    }

    auto advanceTo = [&] (int& pos, int end)
    {
        while (running && pos < end)
        {
            double next = samplesToNextStep;
            if (samplesToNoteOff >= 0.0 && samplesToNoteOff < next)
                next = samplesToNoteOff;

            if (pos + next >= end)
            {
                const double elapsed = end - pos;
                samplesToNextStep -= elapsed;
                if (samplesToNoteOff >= 0.0)
                    samplesToNoteOff -= elapsed;
                pos = end;
                break;
            }

            const int eventPos = pos + (int) next;
            samplesToNextStep -= next;
            if (samplesToNoteOff >= 0.0)
                samplesToNoteOff -= next;
            pos = eventPos;

            if (samplesToNoteOff >= 0.0 && samplesToNoteOff <= 0.5)
                stopPlaying (out, pos);

            if (samplesToNextStep <= 0.5)
            {
                const double len = stepLength (p, bpm);
                stepTrigger (out, pos, p);
                ++stepIndex;
                samplesToNextStep += len;
                if (playingNote >= 0)
                    samplesToNoteOff = std::max (1.0, len * p.asGate);
            }
        }
        pos = std::max (pos, end);
    };

    int pos = 0;
    for (const auto meta : in)
    {
        const auto m = meta.getMessage();
        const int evPos = juce::jlimit (0, numSamples - 1, meta.samplePosition);
        advanceTo (pos, evPos);

        if (m.isNoteOn())
        {
            const bool wasEmpty = held.empty();
            if (held.size() < held.capacity())
                held.push_back ({ m.getNoteNumber(), m.getFloatVelocity() });
            if (wasEmpty)
            {
                running = true;
                stepIndex = 0;
                arpIndex = 0;
                samplesToNextStep = 0.0;
                advanceTo (pos, evPos + 1);
            }
        }
        else if (m.isNoteOff())
        {
            held.erase (std::remove_if (held.begin(), held.end(), [&m] (const Held& h) { return h.note == m.getNoteNumber(); }), held.end());
            if (held.empty())
            {
                running = false;
                stopPlaying (out, evPos);
                currentStep = -1;
            }
        }
        else
            out.addEvent (m, evPos);
    }

    advanceTo (pos, numSamples);
}
} // namespace mf
