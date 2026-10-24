#pragma once

#include "DspCore.h"
#include "../Params.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace mf
{
struct ModRoute
{
    int source, dest;
    float amount;
};

// Per-render-chunk data shared by all voices.
struct VoiceContext
{
    const Snapshot* p = nullptr;
    float sampleRate = 44100.0f;
    float bendSemis = 0.0f;
    float wheel = 0.0f;
    float aftertouch = 0.0f;
    const float* lfo1 = nullptr;
    const float* lfo2 = nullptr;
    const ModRoute* routes = nullptr;
    int numRoutes = 0;
    bool panModulated = false;
    float gain = 0.5f;
};

class Voice
{
public:
    void prepare (float sampleRate, uint32_t seed);
    void start (int midiNote, float velocity, bool glide, const Snapshot& p);
    void changeNote (int midiNote, bool glide);
    void release();
    void kill();
    bool isActive() const noexcept { return active; }

    void render (float* left, float* right, int numSamples, const VoiceContext& ctx);

    int note = -1;
    bool held = false;       // key physically down
    bool sustained = false;  // key up but held by sustain pedal
    uint64_t age = 0;

private:
    void updateEnvelopes (const Snapshot& p);

    float sampleRate = 44100.0f;
    bool active = false;
    float velocity = 1.0f;
    float currentNote = 60.0f, targetNote = 60.0f;

    BruteVco vco1, vco2;
    SimpleVco vco3;
    NoiseSource noise;
    SteinerFilter steiner;
    LadderFilter ladder;
    Envelope env1, env2, env3;
    Rng rng;

    float vco3Last = 0.0f;
    float stCutSmooth = 1000.0f, ldCutSmooth = 1000.0f;
    float drift[3] {}, driftTarget[3] {};
    int driftCounter = 0;
};

class Synth
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr int kChunk = 256;

    void prepare (double sampleRate);
    void setTempo (double bpm) noexcept { tempo = bpm > 0.0 ? bpm : 120.0; }
    void handleMidi (const juce::MidiMessage& m, const Snapshot& p);
    void render (juce::AudioBuffer<float>& buffer, int start, int num, const Snapshot& p);
    void allNotesOff();
    int numActiveVoices() const;

private:
    void noteOn (int note, float vel, const Snapshot& p);
    void noteOff (int note, const Snapshot& p);
    void releaseSustained();
    int voiceLimit (const Snapshot& p) const;
    float lfoHz (int which, const Snapshot& p) const;

    Voice voices[kMaxVoices];
    Lfo lfo1 { 1001u }, lfo2 { 2002u };
    float lfo1Buf[kChunk] {}, lfo2Buf[kChunk] {};
    ModRoute routes[kNumModSources * kNumModDests] {};

    struct HeldNote { int note; float velocity; };
    std::vector<HeldNote> noteStack;

    float sampleRate = 44100.0f;
    double tempo = 120.0;
    float wheel = 0.0f, aftertouch = 0.0f, bend = 0.0f;
    bool sustainPedal = false;
    uint64_t ageCounter = 0;
    int lastVoiceMode = -1;
};
} // namespace mf
