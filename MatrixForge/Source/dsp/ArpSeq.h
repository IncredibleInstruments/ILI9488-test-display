#pragma once

#include "../Params.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

namespace mf
{
// Turns held keys into an arpeggio or a transposable 16-step sequence.
// Runs before the synth and rewrites the block's MIDI.
class ArpSeq
{
public:
    void prepare (double sampleRate);
    void process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, const Snapshot& p, double bpm);

    std::atomic<int> currentStep { -1 }; // for the editor's step display

private:
    void stepTrigger (juce::MidiBuffer& out, int pos, const Snapshot& p);
    void stopPlaying (juce::MidiBuffer& out, int pos);
    int nextArpNote (const Snapshot& p);
    double stepLength (const Snapshot& p, double bpm) const;

    struct Held { int note; float velocity; };
    std::vector<Held> held;          // in the order played
    std::vector<int> sortedScratch;  // preallocated arp note pool

    double sampleRate = 44100.0;
    double samplesToNextStep = 0.0, samplesToNoteOff = -1.0;
    int playingNote = -1;
    int stepIndex = 0, arpIndex = 0, arpDirection = 1;
    bool running = false;
    int lastMode = asOff;
    juce::Random random;
};
} // namespace mf
