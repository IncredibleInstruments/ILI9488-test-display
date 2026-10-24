#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace mf
{
constexpr int kNumModSources = 10;
constexpr int kNumModDests   = 16;
constexpr int kNumSeqSteps   = 16;

enum ModSource { srcEnv1, srcEnv2, srcEnv3, srcLfo1, srcLfo2, srcVco3, srcVelocity, srcWheel, srcAftertouch, srcKey };

enum ModDest
{
    dVco1Pitch, dVco1PW, dVco1Metal, dVco2Pitch, dVco2PW, dVco2Metal, dVco3Pitch, dUltra,
    dCutoff, dStCutoff, dStReso, dLdCutoff, dLdReso, dBrute, dVca, dPan
};

enum VoiceMode { modeMono, modeLegato, modePara, modePoly };
enum ArpSeqMode { asOff, asArp, asSeq };

// Every scalar parameter. The identifier doubles as the parameter ID string.
#define MF_SCALAR_PARAMS(X) \
    X(master) X(voiceMode) X(glide) X(bendRange) X(drift) \
    X(vco1Coarse) X(vco1Fine) X(vco1Saw) X(vco1Square) X(vco1Tri) X(vco1Sub) X(vco1PW) X(vco1Metal) X(vco1Ultra) X(vco1FM) \
    X(vco2Coarse) X(vco2Fine) X(vco2Saw) X(vco2Square) X(vco2Tri) X(vco2Sub) X(vco2PW) X(vco2Metal) X(vco2Ultra) X(vco2Sync) \
    X(vco3Coarse) X(vco3Fine) X(vco3Wave) X(vco3Lfo) X(vco3Rate) X(noiseColor) \
    X(mixVco1) X(mixVco2) X(mixVco3) X(mixNoise) X(routeVco1) X(routeVco2) X(routeVco3) X(routeNoise) \
    X(stMode) X(stCutoff) X(stReso) X(stDrive) X(stBrute) X(stEnvAmt) X(stKeyTrack) X(stOut) \
    X(ldMode) X(ldCutoff) X(ldReso) X(ldDrive) X(ldEnvAmt) X(ldKeyTrack) X(ldOut) X(filterConfig) \
    X(env1A) X(env1D) X(env1S) X(env1R) X(env1Vel) \
    X(env2A) X(env2D) X(env2S) X(env2R) X(env2Vel) \
    X(env3Delay) X(env3A) X(env3D) X(env3S) X(env3R) \
    X(lfo1Wave) X(lfo1Rate) X(lfo1Sync) X(lfo1Div) X(lfo1Retrig) \
    X(lfo2Wave) X(lfo2Rate) X(lfo2Sync) X(lfo2Div) X(lfo2Retrig) \
    X(asMode) X(asDiv) X(asGate) X(arpPattern) X(arpOct) X(seqLength) X(seqSwing) \
    X(chType) X(chRate) X(chDepth) X(chMix) \
    X(dlOn) X(dlTime) X(dlSync) X(dlDiv) X(dlFeedback) X(dlMix) X(dlPingPong) \
    X(rvOn) X(rvSize) X(rvDamp) X(rvMix)

// Plain-value copy of every parameter, taken once per audio block.
struct Snapshot
{
#define MF_X(n) float n = 0.0f;
    MF_SCALAR_PARAMS(MF_X)
#undef MF_X
    float matrix[kNumModSources][kNumModDests] {};
    float seqStep[kNumSeqSteps] {};
    float seqGate[kNumSeqSteps] {};
};

struct ParamRefs
{
#define MF_X(n) std::atomic<float>* n = nullptr;
    MF_SCALAR_PARAMS(MF_X)
#undef MF_X
    std::atomic<float>* matrix[kNumModSources][kNumModDests] {};
    std::atomic<float>* seqStep[kNumSeqSteps] {};
    std::atomic<float>* seqGate[kNumSeqSteps] {};

    void init (juce::AudioProcessorValueTreeState& apvts);
    void load (Snapshot& s) const;
};

const juce::StringArray& modSourceNames();
const juce::StringArray& modSourceShortNames();
const juce::StringArray& modDestNames();
const juce::StringArray& modDestShortNames();
const juce::StringArray& modDestGroupNames();
int modDestGroup (int dest);

inline bool isPitchDest (int dest) { return dest == dVco1Pitch || dest == dVco2Pitch || dest == dVco3Pitch; }
// Pitch routes use a squared curve: fine control for vibrato, up to +-4 octaves at full scale.
inline float pitchAmountToSemis (float amount) { return amount * std::abs (amount) * 48.0f; }
inline float semisToPitchAmount (float semis) { return std::copysign (std::sqrt (std::abs (semis) / 48.0f), semis); }
constexpr float kCutoffModOctaves = 5.0f;
juce::String describeModAmount (int dest, float amount);
const juce::StringArray& divisionNames();
double divisionBeats (int index);

juce::String matrixId (int source, int dest);
juce::String seqStepId (int step);
juce::String seqGateId (int step);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace mf
