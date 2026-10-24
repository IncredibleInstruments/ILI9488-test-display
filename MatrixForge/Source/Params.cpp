#include "Params.h"

namespace mf
{
void ParamRefs::init (juce::AudioProcessorValueTreeState& apvts)
{
    auto get = [&apvts] (const juce::String& id)
    {
        auto* p = apvts.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    };

#define MF_X(n) n = get (#n);
    MF_SCALAR_PARAMS(MF_X)
#undef MF_X

    for (int s = 0; s < kNumModSources; ++s)
        for (int d = 0; d < kNumModDests; ++d)
            matrix[s][d] = get (matrixId (s, d));

    for (int i = 0; i < kNumSeqSteps; ++i)
    {
        seqStep[i] = get (seqStepId (i));
        seqGate[i] = get (seqGateId (i));
    }
}

void ParamRefs::load (Snapshot& s) const
{
#define MF_X(n) s.n = n->load (std::memory_order_relaxed);
    MF_SCALAR_PARAMS(MF_X)
#undef MF_X

    for (int src = 0; src < kNumModSources; ++src)
        for (int d = 0; d < kNumModDests; ++d)
            s.matrix[src][d] = matrix[src][d]->load (std::memory_order_relaxed);

    for (int i = 0; i < kNumSeqSteps; ++i)
    {
        s.seqStep[i] = seqStep[i]->load (std::memory_order_relaxed);
        s.seqGate[i] = seqGate[i]->load (std::memory_order_relaxed);
    }
}

const juce::StringArray& modSourceNames()
{
    static const juce::StringArray names { "Env 1 (Filter)", "Env 2 (VCA)", "Env 3 (Mod)", "LFO 1", "LFO 2",
                                           "VCO 3", "Velocity", "Mod Wheel", "Aftertouch", "Keyboard" };
    return names;
}

const juce::StringArray& modSourceShortNames()
{
    static const juce::StringArray names { "ENV 1", "ENV 2", "ENV 3", "LFO 1", "LFO 2", "VCO 3", "VELO", "WHEEL", "AFTER", "KEY" };
    return names;
}

const juce::StringArray& modDestNames()
{
    static const juce::StringArray names { "VCO1 Pitch", "VCO1 Pulse Width", "VCO1 Metalizer", "VCO2 Pitch",
                                           "VCO2 Pulse Width", "VCO2 Metalizer", "VCO3 Pitch", "Ultrasaw",
                                           "Both Filter Cutoffs", "Steiner Cutoff", "Steiner Resonance", "Ladder Cutoff",
                                           "Ladder Resonance", "Brute Factor", "VCA Level", "Pan" };
    return names;
}

const juce::StringArray& modDestShortNames()
{
    static const juce::StringArray names { "PITCH", "PW", "FOLD", "PITCH", "PW", "FOLD", "PITCH", "ULTRA",
                                           "BOTH", "STNR", "RESO", "LADR", "RESO", "BRUTE", "LEVEL", "PAN" };
    return names;
}

const juce::StringArray& modDestGroupNames()
{
    static const juce::StringArray names { "VCO 1", "VCO 2", "VCO3", "SAW", "FILTER CUTOFF / RESO", "AMP" };
    return names;
}

int modDestGroup (int dest)
{
    static const int groups[kNumModDests] { 0, 0, 0, 1, 1, 1, 2, 3, 4, 4, 4, 4, 4, 4, 5, 5 };
    return groups[juce::jlimit (0, kNumModDests - 1, dest)];
}

juce::String describeModAmount (int dest, float a)
{
    auto sign = [] (float v) { return v > 0.0f ? juce::String ("+") : juce::String(); };
    if (isPitchDest (dest))
    {
        const float st = pitchAmountToSemis (a);
        return sign (st) + juce::String (st, std::abs (st) < 1.0f ? 2 : 1) + " st";
    }
    switch (dest)
    {
        case dCutoff: case dStCutoff: case dLdCutoff:
            return sign (a) + juce::String (a * kCutoffModOctaves, 2) + " oct";
        case dVco1PW: case dVco2PW:
            return sign (a) + juce::String (juce::roundToInt (a * 45.0f)) + " % width";
        default:
            return sign (a) + juce::String (juce::roundToInt (a * 100.0f)) + " %";
    }
}

const juce::StringArray& divisionNames()
{
    static const juce::StringArray names { "4/1", "2/1", "1/1", "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
    return names;
}

double divisionBeats (int index)
{
    static const double beats[] { 16.0, 8.0, 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    return beats[juce::jlimit (0, 10, index)];
}

juce::String matrixId (int source, int dest) { return "mm_" + juce::String (source) + "_" + juce::String (dest); }
juce::String seqStepId (int step)            { return "seqStep" + juce::String (step + 1); }
juce::String seqGateId (int step)            { return "seqGate" + juce::String (step + 1); }

namespace
{
using Range = juce::NormalisableRange<float>;
using StrFn = std::function<juce::String (float, int)>;

juce::String percent (float v, int)  { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String bipolar (float v, int)  { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String hertz (float v, int)    { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 10.0f ? 2 : 0) + " Hz"; }
juce::String seconds (float v, int)  { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s"; }
juce::String millis (float v, int)   { return juce::String (juce::roundToInt (v)) + " ms"; }
juce::String semis (float v, int)    { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; }
juce::String cents (float v, int)    { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct"; }

Range skewed (float lo, float hi, float centre)
{
    Range r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

struct LayoutBuilder
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    void f (const juce::String& id, const juce::String& name, Range range, float def, StrFn fn)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fn))));
    }

    void c (const juce::String& id, const juce::String& name, const juce::StringArray& choices, int def)
    {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, choices, def));
    }

    void b (const juce::String& id, const juce::String& name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    }

    void i (const juce::String& id, const juce::String& name, int lo, int hi, int def, StrFn fn = {})
    {
        auto attrs = juce::AudioParameterIntAttributes();
        if (fn)
            attrs = attrs.withStringFromValueFunction ([fn] (int v, int len) { return fn ((float) v, len); });
        params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id, 1 }, name, lo, hi, def, attrs));
    }
};
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    LayoutBuilder L;
    const Range unit (0.0f, 1.0f);
    const Range bip (-1.0f, 1.0f);
    const auto cutoffRange = skewed (20.0f, 20000.0f, 1000.0f);
    const auto envTime = skewed (0.001f, 10.0f, 0.4f);
    const auto lfoRate = skewed (0.02f, 40.0f, 3.0f);
    const juce::StringArray routes { "Steiner", "Both", "Ladder" };
    const juce::StringArray lfoWaves { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Random" };

    // Global
    L.f ("master", "Master Volume", unit, 0.7f, percent);
    L.c ("voiceMode", "Voice Mode", { "Mono", "Legato", "Para 3", "Poly 8" }, 0);
    L.f ("glide", "Glide", skewed (0.0f, 2.0f, 0.25f), 0.0f, seconds);
    L.i ("bendRange", "Bend Range", 0, 24, 2, semis);
    L.f ("drift", "Analog Drift", unit, 0.15f, percent);

    // VCO 1 & 2
    for (int n = 1; n <= 2; ++n)
    {
        const juce::String p = "vco" + juce::String (n);
        const juce::String name = "VCO" + juce::String (n) + " ";
        L.i (p + "Coarse", name + "Coarse", -24, 24, 0, semis);
        L.f (p + "Fine", name + "Fine", Range (-100.0f, 100.0f), 0.0f, cents);
        L.f (p + "Saw", name + "Saw Level", unit, n == 1 ? 1.0f : 0.0f, percent);
        L.f (p + "Square", name + "Square Level", unit, n == 2 ? 1.0f : 0.0f, percent);
        L.f (p + "Tri", name + "Triangle Level", unit, 0.0f, percent);
        L.f (p + "Sub", name + "Sub Level", unit, 0.0f, percent);
        L.f (p + "PW", name + "Pulse Width", Range (0.05f, 0.95f), 0.5f, percent);
        L.f (p + "Metal", name + "Metalizer", unit, 0.0f, percent);
        L.f (p + "Ultra", name + "Ultrasaw", unit, 0.0f, percent);
        if (n == 1)
            L.f ("vco1FM", "VCO1 FM from VCO3", unit, 0.0f, percent);
        else
            L.b ("vco2Sync", "VCO2 Sync to VCO1", false);
    }

    // VCO 3 / LFO 3 and noise
    L.i ("vco3Coarse", "VCO3 Coarse", -24, 24, -12, semis);
    L.f ("vco3Fine", "VCO3 Fine", Range (-100.0f, 100.0f), 0.0f, cents);
    L.c ("vco3Wave", "VCO3 Wave", { "Sine", "Triangle", "Saw", "Square" }, 1);
    L.b ("vco3Lfo", "VCO3 LFO Mode", false);
    L.f ("vco3Rate", "VCO3 LFO Rate", lfoRate, 2.0f, hertz);
    L.f ("noiseColor", "Noise Color", bip, 0.0f, bipolar);

    // Mixer
    L.f ("mixVco1", "Mixer VCO1", unit, 0.8f, percent);
    L.f ("mixVco2", "Mixer VCO2", unit, 0.0f, percent);
    L.f ("mixVco3", "Mixer VCO3", unit, 0.0f, percent);
    L.f ("mixNoise", "Mixer Noise", unit, 0.0f, percent);
    L.c ("routeVco1", "Route VCO1", routes, 0);
    L.c ("routeVco2", "Route VCO2", routes, 2);
    L.c ("routeVco3", "Route VCO3", routes, 1);
    L.c ("routeNoise", "Route Noise", routes, 0);

    // Steiner-Parker style multimode filter
    L.c ("stMode", "Steiner Mode", { "Low Pass", "Band Pass", "High Pass", "Notch" }, 0);
    L.f ("stCutoff", "Steiner Cutoff", cutoffRange, 6000.0f, hertz);
    L.f ("stReso", "Steiner Resonance", unit, 0.1f, percent);
    L.f ("stDrive", "Steiner Drive", unit, 0.2f, percent);
    L.f ("stBrute", "Brute Factor", unit, 0.0f, percent);
    L.f ("stEnvAmt", "Steiner Env Amount", bip, 0.0f, bipolar);
    L.f ("stKeyTrack", "Steiner Key Track", unit, 0.5f, percent);
    L.f ("stOut", "Steiner Output", unit, 0.8f, percent);

    // Ladder filter
    L.c ("ldMode", "Ladder Mode", { "LP 24dB", "LP 12dB", "Band Pass", "High Pass" }, 0);
    L.f ("ldCutoff", "Ladder Cutoff", cutoffRange, 6000.0f, hertz);
    L.f ("ldReso", "Ladder Resonance", unit, 0.1f, percent);
    L.f ("ldDrive", "Ladder Drive", unit, 0.2f, percent);
    L.f ("ldEnvAmt", "Ladder Env Amount", bip, 0.0f, bipolar);
    L.f ("ldKeyTrack", "Ladder Key Track", unit, 0.5f, percent);
    L.f ("ldOut", "Ladder Output", unit, 0.8f, percent);
    L.c ("filterConfig", "Filter Config", { "Parallel", "Series" }, 0);

    // Envelopes
    for (int n = 1; n <= 3; ++n)
    {
        const juce::String p = "env" + juce::String (n);
        const juce::String name = "Env" + juce::String (n) + " ";
        if (n == 3)
            L.f ("env3Delay", "Env3 Delay", skewed (0.0f, 5.0f, 0.5f), 0.0f, seconds);
        L.f (p + "A", name + "Attack", envTime, n == 2 ? 0.003f : 0.01f, seconds);
        L.f (p + "D", name + "Decay", envTime, 0.4f, seconds);
        L.f (p + "S", name + "Sustain", unit, n == 2 ? 1.0f : 0.5f, percent);
        L.f (p + "R", name + "Release", envTime, 0.25f, seconds);
        if (n != 3)
            L.f (p + "Vel", name + "Velocity", unit, n == 2 ? 0.5f : 0.3f, percent);
    }

    // LFOs
    for (int n = 1; n <= 2; ++n)
    {
        const juce::String p = "lfo" + juce::String (n);
        const juce::String name = "LFO" + juce::String (n) + " ";
        L.c (p + "Wave", name + "Wave", lfoWaves, n == 1 ? 1 : 0);
        L.f (p + "Rate", name + "Rate", lfoRate, n == 1 ? 5.0f : 0.5f, hertz);
        L.b (p + "Sync", name + "Tempo Sync", false);
        L.c (p + "Div", name + "Division", divisionNames(), 4);
        L.b (p + "Retrig", name + "Key Retrigger", false);
    }

    // Modulation matrix
    for (int s = 0; s < kNumModSources; ++s)
        for (int d = 0; d < kNumModDests; ++d)
            L.f (matrixId (s, d), modSourceNames()[s] + " > " + modDestNames()[d], bip, 0.0f,
                 [d] (float v, int) { return describeModAmount (d, v); });

    // Arpeggiator / step sequencer
    L.c ("asMode", "Arp/Seq Mode", { "Off", "Arp", "Seq" }, 0);
    L.c ("asDiv", "Arp/Seq Rate", divisionNames(), 8);
    L.f ("asGate", "Arp/Seq Gate", Range (0.05f, 1.0f), 0.5f, percent);
    L.c ("arpPattern", "Arp Pattern", { "Up", "Down", "Up/Down", "Random", "As Played" }, 0);
    L.i ("arpOct", "Arp Octaves", 1, 4, 1);
    L.i ("seqLength", "Seq Length", 1, kNumSeqSteps, kNumSeqSteps);
    L.f ("seqSwing", "Swing", Range (0.0f, 0.5f), 0.0f, [] (float v, int) { return juce::String (juce::roundToInt (50.0f + v * 50.0f)) + " %"; });
    for (int i = 0; i < kNumSeqSteps; ++i)
    {
        L.i (seqStepId (i), "Seq Step " + juce::String (i + 1), -24, 24, 0, semis);
        L.b (seqGateId (i), "Seq Gate " + juce::String (i + 1), true);
    }

    // Effects
    L.c ("chType", "Chorus Type", { "Off", "Chorus", "Flanger" }, 0);
    L.f ("chRate", "Chorus Rate", skewed (0.05f, 5.0f, 0.8f), 0.6f, hertz);
    L.f ("chDepth", "Chorus Depth", unit, 0.4f, percent);
    L.f ("chMix", "Chorus Mix", unit, 0.4f, percent);
    L.b ("dlOn", "Delay On", false);
    L.f ("dlTime", "Delay Time", skewed (10.0f, 2000.0f, 350.0f), 375.0f, millis);
    L.b ("dlSync", "Delay Sync", true);
    L.c ("dlDiv", "Delay Division", divisionNames(), 7);
    L.f ("dlFeedback", "Delay Feedback", Range (0.0f, 0.95f), 0.35f, percent);
    L.f ("dlMix", "Delay Mix", unit, 0.25f, percent);
    L.b ("dlPingPong", "Delay Ping Pong", true);
    L.b ("rvOn", "Reverb On", false);
    L.f ("rvSize", "Reverb Size", unit, 0.6f, percent);
    L.f ("rvDamp", "Reverb Damping", unit, 0.5f, percent);
    L.f ("rvMix", "Reverb Mix", unit, 0.25f, percent);

    return { L.params.begin(), L.params.end() };
}
} // namespace mf
