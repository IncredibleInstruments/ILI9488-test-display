#include "Presets.h"
#include "Params.h"

namespace mf
{
namespace
{
// Mixer routing targets
constexpr int ST = 0, BOTH = 1, LD = 2;
// Steiner / ladder modes
constexpr int LP = 0, BP = 1, HP = 2, NOTCH = 3;
constexpr int LP24 = 0, LP12 = 1, LBP = 2, LHP = 3;
// LFO waves
constexpr int SINE = 0, TRI = 1, SAWUP = 2, SAWDN = 3, SQR = 4, SH = 5, RND = 6;
// Tempo divisions (index into divisionNames())
constexpr int D4_1 = 0, D2_1 = 1, D1_1 = 2, D1_2 = 3, D1_4 = 4, D1_4T = 5, D1_8 = 6, D1_8T = 7, D1_16 = 8, D1_16T = 9, D1_32 = 10;

// Small builder used to write presets tersely. Values are plain parameter units;
// anything not set stays at its default.
struct PB
{
    explicit PB (const juce::String& name) { p.name = name; }

    PB& s (const juce::String& id, float v)
    {
        p.values.emplace_back (id, v);
        return *this;
    }

    // Matrix route. For pitch destinations prefer pm(), which takes semitones.
    PB& m (int source, int dest, float amount) { return s (matrixId (source, dest), amount); }
    PB& pm (int source, int dest, float semis) { return m (source, dest, semisToPitchAmount (semis)); }

    PB& voice (int mode) { return s ("voiceMode", (float) mode); }
    PB& o1 (float saw, float square, float tri, float sub) { return s ("vco1Saw", saw).s ("vco1Square", square).s ("vco1Tri", tri).s ("vco1Sub", sub); }
    PB& o2 (float saw, float square, float tri, float sub) { return s ("vco2Saw", saw).s ("vco2Square", square).s ("vco2Tri", tri).s ("vco2Sub", sub); }
    PB& tune (int vco, float coarse, float fine = 0.0f)
    {
        const juce::String v = "vco" + juce::String (vco);
        return s (v + "Coarse", coarse).s (v + "Fine", fine);
    }
    PB& pw (int vco, float width) { return s ("vco" + juce::String (vco) + "PW", width); }
    PB& metal (int vco, float amount) { return s ("vco" + juce::String (vco) + "Metal", amount); }
    PB& ultra (int vco, float amount) { return s ("vco" + juce::String (vco) + "Ultra", amount); }
    PB& vco3 (int wave, float coarse, float fine = 0.0f) { return s ("vco3Wave", (float) wave).tune (3, coarse, fine); }
    PB& mix (float a, float b, float c, float noise) { return s ("mixVco1", a).s ("mixVco2", b).s ("mixVco3", c).s ("mixNoise", noise); }
    PB& route (int a, int b, int c, int noise)
    {
        return s ("routeVco1", (float) a).s ("routeVco2", (float) b).s ("routeVco3", (float) c).s ("routeNoise", (float) noise);
    }
    PB& st (int mode, float cutoff, float reso, float envAmt)
    {
        return s ("stMode", (float) mode).s ("stCutoff", cutoff).s ("stReso", reso).s ("stEnvAmt", envAmt);
    }
    PB& ld (int mode, float cutoff, float reso, float envAmt)
    {
        return s ("ldMode", (float) mode).s ("ldCutoff", cutoff).s ("ldReso", reso).s ("ldEnvAmt", envAmt);
    }
    PB& outs (float steiner, float ladder) { return s ("stOut", steiner).s ("ldOut", ladder); }
    PB& series() { return s ("filterConfig", 1); }
    PB& ladderOnly() { return route (LD, LD, LD, LD).outs (0.0f, 1.0f); }
    PB& steinerOnly() { return route (ST, ST, ST, ST).outs (1.0f, 0.0f); }
    PB& env (int n, float a, float d, float sus, float r)
    {
        const juce::String e = "env" + juce::String (n);
        return s (e + "A", a).s (e + "D", d).s (e + "S", sus).s (e + "R", r);
    }
    PB& lfo (int n, int wave, float hz)
    {
        const juce::String l = "lfo" + juce::String (n);
        return s (l + "Wave", (float) wave).s (l + "Rate", hz).s (l + "Sync", 0);
    }
    PB& lfoSync (int n, int wave, int div)
    {
        const juce::String l = "lfo" + juce::String (n);
        return s (l + "Wave", (float) wave).s (l + "Sync", 1).s (l + "Div", (float) div);
    }
    PB& vibrato (float semis = 0.12f, float hz = 5.5f) { return lfo (1, TRI, hz).pm (srcLfo1, dVco1Pitch, semis).pm (srcLfo1, dVco2Pitch, semis); }
    PB& reverb (float mix, float size = 0.75f, float damp = 0.5f) { return s ("rvOn", 1).s ("rvMix", mix).s ("rvSize", size).s ("rvDamp", damp); }
    PB& delay (int div, float fb, float mix, bool pingPong = true)
    {
        return s ("dlOn", 1).s ("dlSync", 1).s ("dlDiv", (float) div).s ("dlFeedback", fb).s ("dlMix", mix).s ("dlPingPong", pingPong ? 1.0f : 0.0f);
    }
    PB& chorus (float mix, float depth = 0.5f, float rate = 0.6f) { return s ("chType", 1).s ("chMix", mix).s ("chDepth", depth).s ("chRate", rate); }
    PB& flanger (float mix, float depth = 0.5f, float rate = 0.2f) { return s ("chType", 2).s ("chMix", mix).s ("chDepth", depth).s ("chRate", rate); }
    PB& arp (int pattern, int octaves, int div, float gate)
    {
        return s ("asMode", asArp).s ("arpPattern", (float) pattern).s ("arpOct", (float) octaves).s ("asDiv", (float) div).s ("asGate", gate);
    }
    PB& seq (int div, float gate, std::initializer_list<int> steps, std::initializer_list<int> gates = {})
    {
        s ("asMode", asSeq).s ("asDiv", (float) div).s ("asGate", gate);
        int i = 0;
        for (int st : steps)
            s (seqStepId (i++), (float) st);
        s ("seqLength", (float) i);
        int g = 0;
        for (int gate : gates)
            s (seqGateId (g++), (float) gate);
        return *this;
    }

    FactoryPreset p;
};

using BankFn = void (*) (PB&, int);

//==============================================================================
void bassBank (PB& b, int i)
{
    b.voice (modeMono).ladderOnly().s ("ldKeyTrack", 0.4f).m (srcWheel, dLdCutoff, 0.5f).m (srcVelocity, dLdCutoff, 0.12f);

    switch (i)
    {
        case 0: // Brute Sub: round sub with a short filter blip
            b.o1 (0.5f, 0, 0, 1).mix (0.9f, 0, 0, 0).ld (LP24, 160, 0.15f, 0.35f).s ("ldDrive", 0.5f)
             .env (1, 0.001f, 0.18f, 0.0f, 0.1f).env (2, 0.002f, 0.4f, 0.9f, 0.08f);
            break;
        case 1: // Steiner Growl: brute factor fuzz breathing with a slow LFO
            b.steinerOnly().o1 (1, 0, 0, 0.4f).o2 (0, 1, 0, 0).tune (2, -12).mix (0.7f, 0.5f, 0, 0)
             .st (LP, 260, 0.55f, 0.45f).s ("stBrute", 0.85f).s ("stDrive", 0.75f).env (1, 0.001f, 0.35f, 0.25f, 0.15f)
             .lfo (2, TRI, 0.35f).m (srcLfo2, dStCutoff, 0.18f).m (srcAftertouch, dBrute, 0.4f).m (srcWheel, dStCutoff, 0.5f)
             .env (2, 0.003f, 0.6f, 0.85f, 0.1f);
            break;
        case 2: // Rubber Pluck: bouncy resonant pulse with a hint of glide
            b.o1 (0, 1, 0, 0.3f).pw (1, 0.3f).mix (0.9f, 0, 0, 0).ld (LP24, 110, 0.68f, 0.85f).s ("glide", 0.04f)
             .env (1, 0.001f, 0.17f, 0.0f, 0.12f).env (2, 0.001f, 0.45f, 0.15f, 0.1f).pm (srcEnv3, dVco1Pitch, 2.0f)
             .env (3, 0.001f, 0.05f, 0.0f, 0.05f);
            break;
        case 3: // Acid Matrix: squelchy legato 303-style line
            b.voice (modeLegato).s ("glide", 0.07f).o1 (1, 0, 0, 0).mix (0.85f, 0, 0, 0).ld (LP24, 180, 0.86f, 0.7f).s ("ldDrive", 0.75f)
             .env (1, 0.001f, 0.22f, 0.0f, 0.1f).env (2, 0.001f, 0.3f, 0.8f, 0.05f).m (srcVelocity, dLdCutoff, 0.3f)
             .m (srcAftertouch, dLdReso, 0.2f).delay (D1_8T, 0.35f, 0.15f);
            break;
        case 4: // Square Thump: short percussive square with a pitch click
            b.steinerOnly().o1 (0, 1, 0, 0.8f).mix (0.9f, 0, 0, 0).st (LP, 700, 0.1f, 0.35f).env (1, 0.001f, 0.1f, 0.0f, 0.08f)
             .env (2, 0.001f, 0.28f, 0.0f, 0.1f).pm (srcEnv3, dVco1Pitch, 12.0f).env (3, 0.001f, 0.025f, 0.0f, 0.02f);
            break;
        case 5: // Fold Bass: wavefolded triangle that opens per note and pulses in time
            b.o1 (0, 0, 1, 0.4f).metal (1, 0.25f).mix (0.85f, 0, 0, 0).ld (LP24, 1600, 0.1f, 0.1f)
             .m (srcEnv3, dVco1Metal, 0.55f).env (3, 0.001f, 0.45f, 0.1f, 0.2f).lfoSync (1, TRI, D1_4).m (srcLfo1, dVco1Metal, 0.2f)
             .m (srcWheel, dVco1Metal, 0.6f).env (2, 0.002f, 0.6f, 0.8f, 0.1f);
            break;
        case 6: // Deep Sync: synced VCO2 sweeping down on each note
            b.o1 (1, 0, 0, 0.5f).o2 (1, 0, 0, 0).s ("vco2Sync", 1).mix (0.25f, 0.85f, 0, 0).ld (LP24, 900, 0.3f, 0.2f)
             .pm (srcEnv3, dVco2Pitch, 19.0f).env (3, 0.001f, 0.35f, 0.0f, 0.2f).pm (srcWheel, dVco2Pitch, 12.0f)
             .env (2, 0.002f, 0.5f, 0.8f, 0.1f);
            break;
        case 7: // Wobble Unit: tempo-synced filter wobble, wheel adds resonance
            b.o1 (1, 0, 0, 0.5f).o2 (1, 0, 0, 0).tune (2, -12, 8).mix (0.7f, 0.6f, 0, 0).ld (LP24, 130, 0.55f, 0.05f).s ("ldDrive", 0.6f)
             .lfoSync (1, SINE, D1_8).m (srcLfo1, dLdCutoff, 0.65f).m (srcWheel, dLdReso, 0.35f).s ("lfo1Retrig", 1)
             .env (2, 0.003f, 0.5f, 1.0f, 0.1f);
            break;
        case 8: // Reese Drift: detuned saws with slow phasing and stereo movement
            b.route (BOTH, BOTH, BOTH, BOTH).outs (0.5f, 0.6f).o1 (1, 0, 0, 0.3f).ultra (1, 0.5f).o2 (1, 0, 0, 0).tune (2, 0, 18)
             .mix (0.6f, 0.6f, 0, 0).st (NOTCH, 600, 0.3f, 0).ld (LP24, 420, 0.25f, 0.15f).lfo (2, SINE, 0.15f)
             .m (srcLfo2, dStCutoff, 0.4f).m (srcLfo2, dPan, 0.35f).chorus (0.25f).env (2, 0.005f, 0.5f, 1.0f, 0.15f);
            break;
        case 9: // Mono Punch: hard filter snap, velocity drives level
            b.o1 (1, 0, 0, 0.8f).mix (0.85f, 0, 0, 0).ld (LP24, 220, 0.2f, 0.8f).env (1, 0.001f, 0.08f, 0.0f, 0.05f)
             .env (2, 0.001f, 0.25f, 0.5f, 0.06f).m (srcVelocity, dVca, 0.3f).s ("ldDrive", 0.65f);
            break;
        case 10: // Velvet Floor: soft rounded bass with slow breathing
            b.o1 (0, 0, 1, 0.5f).o2 (0, 1, 0, 0).tune (2, -12).mix (0.8f, 0.3f, 0, 0).ld (LP12, 260, 0.1f, 0.15f)
             .env (2, 0.02f, 0.8f, 0.9f, 0.4f).lfo (2, SINE, 0.2f).m (srcLfo2, dLdCutoff, 0.1f);
            break;
        case 11: // Metal Bottom: sub plus a folded, LFO-scanned upper layer through the Steiner
            b.route (LD, ST, BOTH, ST).outs (0.7f, 0.8f).o1 (0, 0, 0, 1).o2 (0, 0, 1, 0).metal (2, 0.6f).mix (0.8f, 0.5f, 0, 0)
             .st (BP, 900, 0.4f, 0.2f).ld (LP24, 300, 0.1f, 0.1f).lfo (1, TRI, 2.0f).m (srcLfo1, dVco2Metal, 0.35f)
             .env (2, 0.003f, 0.5f, 0.9f, 0.1f);
            break;
        case 12: // FM Knock: VCO3 frequency-modulates a triangle for a woody attack
            b.o1 (0, 0, 1, 0.5f).s ("vco1FM", 0.55f).vco3 (0, 7).mix (0.9f, 0, 0, 0).ld (LP24, 2200, 0.1f, 0.3f)
             .env (1, 0.001f, 0.12f, 0.0f, 0.1f).env (2, 0.001f, 0.35f, 0.15f, 0.12f).pm (srcEnv3, dVco3Pitch, 12.0f)
             .env (3, 0.001f, 0.15f, 0.0f, 0.1f);
            break;
        case 13: // Ultra Sub: wide supersaw sub with stereo drift
            b.o1 (1, 0, 0, 1).ultra (1, 0.9f).mix (0.75f, 0, 0, 0).ld (LP12, 380, 0.15f, 0.25f).lfo (2, SINE, 0.3f)
             .m (srcLfo2, dPan, 0.3f).m (srcLfo2, dUltra, 0.2f).chorus (0.2f).env (2, 0.004f, 0.5f, 0.9f, 0.15f);
            break;
        case 14: // Funk Squelch: band-pass wah driven by velocity and aftertouch
            b.steinerOnly().o1 (0, 1, 0, 0.3f).pw (1, 0.25f).mix (0.9f, 0, 0, 0).st (BP, 450, 0.72f, 0.6f)
             .env (1, 0.001f, 0.2f, 0.1f, 0.1f).m (srcVelocity, dStCutoff, 0.25f).m (srcAftertouch, dStReso, 0.3f)
             .env (2, 0.001f, 0.3f, 0.6f, 0.08f);
            break;
        default: // Detroit Bass: PWM square with a delay tail
            b.o1 (0, 1, 0, 0).pw (1, 0.3f).o2 (1, 0, 0, 0).tune (2, -12).mix (0.7f, 0.4f, 0, 0).ld (LP24, 380, 0.3f, 0.4f)
             .lfo (1, TRI, 0.4f).m (srcLfo1, dVco1PW, 0.35f).env (1, 0.001f, 0.3f, 0.2f, 0.15f).delay (D1_8, 0.3f, 0.12f)
             .env (2, 0.002f, 0.4f, 0.7f, 0.1f);
            break;
    }
}

void leadBank (PB& b, int i)
{
    b.voice (modeLegato).m (srcAftertouch, dCutoff, 0.25f).env (2, 0.004f, 0.4f, 0.9f, 0.25f);

    switch (i)
    {
        case 0: // Solo Forge: two detuned saws, gentle vibrato, wheel opens filter
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 7).mix (0.7f, 0.6f, 0, 0).ld (LP24, 1800, 0.25f, 0.3f)
             .vibrato (0.12f).m (srcWheel, dLdCutoff, 0.4f).delay (D1_8, 0.3f, 0.18f);
            break;
        case 1: // Brute Lead: Steiner fuzz with sub weight
            b.steinerOnly().o1 (1, 0, 0, 0.5f).o2 (0, 1, 0, 0).tune (2, -12).mix (0.7f, 0.4f, 0, 0).st (LP, 1200, 0.3f, 0.3f)
             .s ("stBrute", 0.7f).s ("stDrive", 0.8f).s ("glide", 0.03f).m (srcWheel, dBrute, 0.3f);
            break;
        case 2: // Sync Scream: sync sweep per note, wheel pushes it further
            b.route (BOTH, BOTH, BOTH, BOTH).outs (0.5f, 0.6f).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).s ("vco2Sync", 1).mix (0.15f, 0.85f, 0, 0)
             .st (LP, 5000, 0.2f, 0).ld (LP24, 4000, 0.2f, 0).pm (srcEnv3, dVco2Pitch, 30.0f).env (3, 0.001f, 1.6f, 0.0f, 0.3f).s ("stBrute", 0.5f)
             .pm (srcWheel, dVco2Pitch, 12.0f).delay (D1_8T, 0.35f, 0.2f);
            break;
        case 3: // Liquid Square: counter-moving pulse widths, chorused
            b.ladderOnly().o1 (0, 1, 0, 0).o2 (0, 1, 0, 0).tune (2, 12, 6).pw (2, 0.2f).mix (0.7f, 0.4f, 0, 0).ld (LP12, 2500, 0.15f, 0.1f)
             .lfo (2, SINE, 0.7f).m (srcLfo2, dVco1PW, 0.35f).m (srcLfo2, dVco2PW, -0.35f).chorus (0.35f).vibrato (0.08f);
            break;
        case 4: // Steiner Sing: vowel-like resonant band-pass sweep
            b.steinerOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 7).mix (0.7f, 0.4f, 0, 0).st (BP, 900, 0.78f, 0.15f)
             .lfo (2, TRI, 0.25f).m (srcLfo2, dStCutoff, 0.4f).m (srcWheel, dStCutoff, 0.5f).reverb (0.25f);
            break;
        case 5: // Fold Solo: metalizer opens with the envelope and the wheel
            b.ladderOnly().o1 (0, 0, 1, 0).metal (1, 0.35f).mix (0.85f, 0, 0, 0).ld (LP24, 6000, 0.1f, 0)
             .m (srcEnv1, dVco1Metal, 0.5f).env (1, 0.01f, 0.8f, 0.3f, 0.3f).m (srcWheel, dVco1Metal, 0.5f).vibrato (0.1f)
             .env (2, 0.04f, 0.6f, 0.8f, 0.5f).lfo (2, TRI, 0.3f).m (srcLfo2, dVco1Metal, 0.3f).reverb (0.3f);
            break;
        case 6: // Mono Glide: long portamento, echoing
            b.voice (modeMono).s ("glide", 0.22f).ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, -12).mix (0.7f, 0.5f, 0, 0)
             .ld (LP24, 900, 0.45f, 0.35f).env (1, 0.01f, 0.6f, 0.4f, 0.3f).delay (D1_4, 0.4f, 0.22f);
            break;
        case 7: // Hot Wire: everything driven hard, aftertouch adds brute factor
            b.route (ST, ST, ST, ST).series().outs (1, 0.9f).o1 (1, 0, 0, 0).o2 (0, 1, 0, 0).tune (2, 12).mix (0.7f, 0.5f, 0, 0)
             .st (LP, 2000, 0.5f, 0.2f).ld (LP24, 5000, 0.2f, 0).s ("stDrive", 1).s ("ldDrive", 1).s ("stBrute", 1)
             .m (srcAftertouch, dBrute, -0.5f);
            break;
        case 8: // Classic 70s: saw + square, slow filter envelope
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (0, 1, 0, 0).tune (2, -12).mix (0.7f, 0.6f, 0, 0).ld (LP24, 650, 0.35f, 0.5f)
             .env (1, 0.06f, 0.7f, 0.4f, 0.4f).vibrato (0.15f, 5.0f);
            break;
        case 9: // Saw Cutter: high-pass into low-pass, envelope sweeps the HP
            b.route (ST, ST, ST, ST).series().outs (1, 1).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 10).mix (0.7f, 0.6f, 0, 0)
             .st (HP, 250, 0.4f, 0.45f).ld (LP24, 3500, 0.2f, 0).env (1, 0.001f, 0.4f, 0.0f, 0.2f).m (srcWheel, dStCutoff, 0.4f);
            break;
        case 10: // Whistle: pure triangle with breath and wide vibrato
            b.ladderOnly().o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 12).mix (0.85f, 0.2f, 0, 0.05f).ld (LP12, 3500, 0.1f, 0)
             .vibrato (0.25f, 6.0f).env (2, 0.05f, 0.4f, 0.9f, 0.3f).reverb (0.3f);
            break;
        case 11: // Ultra Lead: big supersaw lead with ping-pong echo
            b.ladderOnly().o1 (1, 0, 0, 0).ultra (1, 0.8f).o2 (1, 0, 0, 0).ultra (2, 0.6f).tune (2, -12).mix (0.65f, 0.5f, 0, 0)
             .ld (LP24, 3000, 0.2f, 0.15f).chorus (0.3f).delay (D1_8T, 0.35f, 0.22f).m (srcWheel, dUltra, -0.5f);
            break;
        case 12: // Distorted Line: series saturation, aftertouch opens both filters
            b.route (ST, ST, ST, ST).series().outs (1, 1).o1 (1, 0, 0, 0.4f).mix (0.85f, 0, 0, 0).st (LP, 800, 0.6f, 0.3f)
             .ld (LP12, 3000, 0.3f, 0).s ("stDrive", 1).s ("ldDrive", 0.8f).m (srcAftertouch, dCutoff, 0.4f)
             .s ("glide", 0.06f).env (1, 0.001f, 0.25f, 0.2f, 0.2f).delay (D1_16, 0.45f, 0.2f);
            break;
        case 13: // Breathy Flute: noisy band-pass breath over a triangle
            b.route (LD, LD, LD, ST).outs (0.6f, 0.8f).o1 (0, 0, 1, 0).mix (0.75f, 0, 0, 0.35f).s ("noiseColor", -0.3f)
             .st (BP, 1800, 0.35f, 0.2f).ld (LP12, 2500, 0.1f, 0.15f).env (1, 0.08f, 0.5f, 0.6f, 0.3f)
             .env (2, 0.12f, 0.3f, 0.85f, 0.25f).vibrato (0.1f, 5.0f).m (srcVelocity, dStCutoff, 0.2f).reverb (0.3f);
            break;
        case 14: // FM Bell Lead: VCO3 FM on a triangle, decaying brightness
            b.ladderOnly().o1 (0, 0, 1, 0).s ("vco1FM", 0.45f).vco3 (0, 19).ld (LP24, 5000, 0.1f, 0.2f)
             .env (2, 0.002f, 1.2f, 0.3f, 0.6f).m (srcWheel, dVco1Metal, 0.3f).metal (1, 0.1f).delay (D1_4, 0.3f, 0.2f).reverb (0.3f);
            break;
        default: // Ladder Lick: snappy resonant ladder with octave layer
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 12).mix (0.75f, 0.3f, 0, 0).ld (LP24, 380, 0.72f, 0.6f)
             .env (1, 0.001f, 0.3f, 0.1f, 0.2f).s ("glide", 0.05f).m (srcVelocity, dLdCutoff, 0.2f);
            break;
    }
}

void padBank (PB& b, int i)
{
    b.voice (modePoly).s ("drift", 0.35f).s ("env2Vel", 0.2f).env (2, 1.0f, 1.0f, 0.9f, 2.0f).m (srcWheel, dCutoff, 0.4f);

    switch (i)
    {
        case 0: // Warm Blanket
            b.ladderOnly().o1 (1, 0, 0, 0.2f).ultra (1, 0.3f).o2 (0, 1, 0, 0).tune (2, 0, 10).mix (0.6f, 0.5f, 0, 0)
             .ld (LP24, 700, 0.15f, 0.2f).env (1, 1.5f, 2.0f, 0.6f, 2.0f).chorus (0.45f).reverb (0.35f);
            break;
        case 1: // Glass Choir: folded triangles through a moving band-pass
            b.steinerOnly().o1 (0, 0, 1, 0).metal (1, 0.2f).o2 (0, 0, 1, 0).metal (2, 0.3f).tune (2, 12).mix (0.7f, 0.5f, 0, 0.05f)
             .st (BP, 1500, 0.5f, 0).lfo (2, SINE, 0.15f).m (srcLfo2, dStCutoff, 0.3f).reverb (0.5f, 0.9f);
            break;
        case 2: // Slow Bloom: very slow filter opening
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 7).mix (0.6f, 0.5f, 0, 0).ld (LP24, 180, 0.3f, 0.65f)
             .env (1, 4.0f, 3.0f, 0.7f, 3.0f).env (2, 2.5f, 1.0f, 1.0f, 3.0f).reverb (0.4f, 0.9f);
            break;
        case 3: // Matrix Clouds: PWM and S&H filter steps drifting in stereo
            b.route (ST, LD, BOTH, ST).outs (0.6f, 0.7f).o1 (0, 1, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 12).mix (0.6f, 0.5f, 0, 0)
             .st (LP, 2000, 0.3f, 0).ld (LP12, 1500, 0.2f, 0).lfo (1, TRI, 0.3f).m (srcLfo1, dVco1PW, 0.35f)
             .lfo (2, SH, 1.5f).m (srcLfo2, dStCutoff, 0.2f).m (srcLfo2, dPan, 0.7f).chorus (0.4f).reverb (0.4f);
            break;
        case 4: // Analog Haze: unstable oscillators and dark noise
            b.ladderOnly().s ("drift", 1.0f).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 20).mix (0.6f, 0.6f, 0, 0.15f)
             .s ("noiseColor", -0.6f).ld (LP24, 1200, 0.2f, 0.1f).lfo (2, RND, 0.4f).m (srcLfo2, dLdCutoff, 0.15f).reverb (0.4f);
            break;
        case 5: // Silk Pad: high-pass into low-pass
            b.route (ST, ST, ST, ST).series().outs (1, 1).o1 (1, 0, 0, 0).ultra (1, 0.7f).o2 (1, 0, 0, 0).tune (2, 12).mix (0.6f, 0.4f, 0, 0)
             .st (HP, 120, 0.25f, 0).ld (LP24, 3500, 0.1f, 0.1f).m (srcEnv3, dStCutoff, 0.45f).env (3, 3.0f, 2.0f, 0.6f, 2.0f)
             .chorus (0.6f, 0.7f).reverb (0.35f);
            break;
        case 6: // Ultra Field: breathing supersaw width
            b.ladderOnly().o1 (1, 0, 0, 0).ultra (1, 1).o2 (1, 0, 0, 0).ultra (2, 0.8f).tune (2, -12).mix (0.6f, 0.5f, 0, 0)
             .ld (LP12, 2500, 0.1f, 0).lfo (1, SINE, 0.2f).m (srcLfo1, dUltra, -0.5f).lfo (2, SINE, 0.11f).m (srcLfo2, dPan, 0.5f)
             .reverb (0.45f);
            break;
        case 7: // Dark Matter: low resonant pad with random filter drift
            b.ladderOnly().o1 (1, 0, 0, 0.5f).tune (1, -12).o2 (1, 0, 0, 0).mix (0.6f, 0.4f, 0, 0).ld (LP24, 220, 0.5f, 0.1f)
             .lfo (2, RND, 0.3f).m (srcLfo2, dLdCutoff, 0.35f).reverb (0.5f, 0.9f, 0.8f);
            break;
        case 8: // Sunrise Sweep: Env3 sweeps the filter open over six seconds
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (0, 1, 0, 0).tune (2, 12).mix (0.6f, 0.4f, 0, 0).ld (LP24, 150, 0.4f, 0)
             .m (srcEnv3, dLdCutoff, 0.75f).env (3, 6.0f, 1.0f, 1.0f, 2.0f).reverb (0.4f);
            break;
        case 9: // Hollow Air: filtered noise wind plus a triangle
            b.route (ST, ST, ST, ST).outs (1, 0).o1 (0, 0, 1, 0).mix (0.5f, 0, 0, 0.6f).st (BP, 1200, 0.65f, 0)
             .lfo (2, SINE, 0.08f).m (srcLfo2, dStCutoff, 0.5f).reverb (0.5f, 0.9f);
            break;
        case 10: // Metal Mist: slowly scanned wavefolder
            b.ladderOnly().o1 (0.5f, 0, 0, 0).o2 (0, 0, 1, 0).metal (2, 0.4f).tune (2, 7).mix (0.5f, 0.6f, 0, 0).ld (LP24, 3000, 0.1f, 0)
             .lfo (2, TRI, 0.2f).m (srcLfo2, dVco2Metal, 0.4f).chorus (0.35f).reverb (0.4f);
            break;
        case 11: // Breathing Room: slow tremolo and filter swell
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 9).mix (0.6f, 0.5f, 0, 0).ld (LP24, 900, 0.2f, 0)
             .lfo (2, SINE, 0.3f).m (srcLfo2, dVca, 0.65f).m (srcLfo2, dLdCutoff, 0.35f).m (srcLfo2, dPan, 0.4f).reverb (0.35f);
            break;
        case 12: // Nebula: VCO3 sine shimmer over a flanged saw
            b.ladderOnly().o1 (1, 0, 0, 0).ultra (1, 0.4f).vco3 (0, 19).mix (0.6f, 0, 0.3f, 0).ld (LP12, 2000, 0.1f, 0)
             .pm (srcLfo2, dVco3Pitch, 0.15f).lfo (2, SINE, 0.3f).flanger (0.4f).reverb (0.6f, 0.95f);
            break;
        case 13: // Polar Night: cold triangles with wandering pitch
            b.ladderOnly().o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 7).mix (0.7f, 0.5f, 0, 0).ld (LP12, 900, 0.1f, 0)
             .lfo (2, RND, 0.5f).pm (srcLfo2, dVco1Pitch, 0.12f).pm (srcLfo2, dVco2Pitch, -0.12f).reverb (0.55f, 0.95f, 0.7f);
            break;
        case 14: // Soft Focus: warm PWM, deep chorus
            b.ladderOnly().o1 (0, 1, 0, 0).pw (1, 0.4f).mix (0.8f, 0, 0, 0).ld (LP12, 1500, 0.1f, 0.1f).lfo (1, TRI, 0.5f)
             .m (srcLfo1, dVco1PW, 0.25f).chorus (0.6f, 0.8f).env (2, 0.4f, 1.0f, 0.9f, 1.5f);
            break;
        default: // Endless: huge tail, octave layer
            b.ladderOnly().o1 (1, 0, 0, 0).ultra (1, 0.5f).o2 (0, 1, 0, 0).tune (2, 12).mix (0.6f, 0.3f, 0, 0).ld (LP24, 1500, 0.15f, 0)
             .env (2, 2.5f, 2.0f, 1.0f, 8.0f).reverb (0.7f, 1.0f).delay (D1_2, 0.6f, 0.25f);
            break;
    }
}

void keysBank (PB& b, int i)
{
    b.voice (modePoly).s ("env2Vel", 0.6f).s ("env1Vel", 0.5f).m (srcWheel, dCutoff, 0.4f);

    switch (i)
    {
        case 0: // Poly Keys
            b.ladderOnly().o1 (0.6f, 0.6f, 0, 0).mix (0.8f, 0, 0, 0).ld (LP24, 1200, 0.2f, 0.4f).env (1, 0.002f, 0.6f, 0.2f, 0.4f)
             .env (2, 0.002f, 1.5f, 0.3f, 0.4f).m (srcVelocity, dLdCutoff, 0.3f).reverb (0.2f);
            break;
        case 1: // Analog Piano: hammer-bright, long natural decay
            b.ladderOnly().o1 (0.2f, 0, 0.8f, 0).o2 (0, 0, 1, 0).tune (2, 12, 3).mix (0.8f, 0.3f, 0, 0).ld (LP24, 2500, 0.1f, 0.3f)
             .env (1, 0.001f, 1.5f, 0.0f, 0.5f).env (2, 0.001f, 3.0f, 0.0f, 0.5f).m (srcVelocity, dLdCutoff, 0.35f).reverb (0.25f);
            break;
        case 2: // Tine Fold: velocity bends the metalizer for electric-piano bark
            b.ladderOnly().o1 (0, 0, 1, 0).metal (1, 0.12f).o2 (0, 0, 1, 0).tune (2, 12).mix (0.8f, 0.3f, 0, 0).ld (LP24, 4000, 0.1f, 0)
             .m (srcVelocity, dVco1Metal, 0.35f).m (srcEnv1, dVco1Metal, 0.35f).env (1, 0.001f, 0.4f, 0.0f, 0.3f)
             .env (2, 0.001f, 2.5f, 0.1f, 0.5f).lfo (2, SINE, 4.5f).m (srcLfo2, dPan, 0.3f).chorus (0.3f);
            break;
        case 3: // Drawbar Forge: organ stack with rotary-style tremolo
            b.ladderOnly().o1 (0, 0.6f, 0, 1).o2 (0, 0, 1, 0).tune (2, 12).vco3 (0, 19).mix (0.6f, 0.4f, 0.3f, 0)
             .ld (LP24, 6000, 0, 0).env (2, 0.004f, 0.1f, 1.0f, 0.06f).s ("env2Vel", 0).lfo (1, SINE, 6.5f)
             .m (srcLfo1, dVca, 0.15f).m (srcLfo1, dPan, 0.35f).m (srcWheel, dLdCutoff, -0.4f);
            break;
        case 4: // Clav Matrix: thin pulse through a snapping band-pass
            b.steinerOnly().o1 (0, 1, 0, 0).pw (1, 0.15f).mix (0.9f, 0, 0, 0).st (BP, 1200, 0.3f, 0.45f).env (1, 0.001f, 0.15f, 0.1f, 0.1f)
             .env (2, 0.001f, 0.4f, 0.1f, 0.1f).m (srcVelocity, dStCutoff, 0.3f);
            break;
        case 5: // Harpsi Pulse: bright high-passed pulses
            b.steinerOnly().o1 (0, 1, 0, 0).pw (1, 0.25f).o2 (0, 1, 0, 0).pw (2, 0.1f).tune (2, 12).mix (0.7f, 0.5f, 0, 0)
             .st (HP, 600, 0.2f, 0).env (2, 0.001f, 1.2f, 0.0f, 0.5f).reverb (0.2f);
            break;
        case 6: // Soft Tines: auto-panning triangle keys
            b.ladderOnly().o1 (0, 0, 1, 0).mix (0.9f, 0, 0, 0).ld (LP24, 1200, 0.1f, 0.2f).lfo (1, SINE, 4.0f)
             .m (srcLfo1, dPan, 0.6f).m (srcLfo1, dVca, 0.15f).env (2, 0.002f, 2.0f, 0.2f, 0.6f);
            break;
        case 7: // Bell Keys: inharmonic folded partials, long ring
            b.ladderOnly().o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 19).metal (2, 0.3f).vco3 (0, 24).mix (0.7f, 0.4f, 0.15f, 0)
             .ld (LP24, 5000, 0, 0).env (2, 0.001f, 2.5f, 0.0f, 2.0f).reverb (0.35f);
            break;
        case 8: // Reed Grit: driven reed organ with vibrato
            b.ladderOnly().o1 (0.8f, 0.3f, 0, 0).mix (0.85f, 0, 0, 0).ld (LP24, 1500, 0.25f, 0.1f).s ("ldDrive", 0.7f).lfo (1, SINE, 5.5f)
             .m (srcLfo1, dVca, 0.2f).env (2, 0.01f, 0.5f, 0.8f, 0.2f);
            break;
        case 9: // Stab Chord: fifth-stacked saws, short and echoed
            b.ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 7).mix (0.6f, 0.5f, 0, 0).ld (LP24, 900, 0.2f, 0.5f)
             .env (1, 0.001f, 0.2f, 0.0f, 0.2f).env (2, 0.001f, 0.25f, 0.0f, 0.2f).delay (D1_8, 0.4f, 0.2f).reverb (0.25f);
            break;
        case 10: // Hollow Organ: notch-filtered squares, fast chorus
            b.steinerOnly().o1 (0, 1, 0, 0).o2 (0, 1, 0, 0).tune (2, 12).mix (0.6f, 0.5f, 0, 0).st (NOTCH, 1000, 0.3f, 0)
             .env (2, 0.005f, 0.1f, 1.0f, 0.1f).s ("env2Vel", 0).chorus (0.5f, 0.6f, 5.0f).m (srcWheel, dStCutoff, 0.6f);
            break;
        case 11: // Vibes Tri: tremolo mallets
            b.ladderOnly().o1 (0, 0, 1, 0).mix (0.9f, 0, 0, 0).ld (LP24, 3000, 0, 0).lfo (1, SINE, 5.0f).m (srcLfo1, dVca, 0.35f)
             .env (2, 0.001f, 1.8f, 0.0f, 1.2f).reverb (0.3f);
            break;
        case 12: // Toy Piano: high folded triangle, short
            b.ladderOnly().o1 (0, 0, 1, 0).metal (1, 0.5f).tune (1, 12).o2 (0, 1, 0, 0).tune (2, 19).mix (0.7f, 0.25f, 0, 0)
             .ld (LP24, 6000, 0, 0).env (2, 0.001f, 0.4f, 0.0f, 0.3f).reverb (0.2f);
            break;
        case 13: // Velvet Keys: warm, slow-attack saw keys
            b.ladderOnly().o1 (1, 0, 0, 0).mix (0.85f, 0, 0, 0).ld (LP12, 600, 0.1f, 0.25f).env (1, 0.05f, 1.0f, 0.4f, 0.8f)
             .env (2, 0.03f, 2.0f, 0.5f, 0.8f).chorus (0.4f);
            break;
        case 14: // House Organ: percussive square stack with key click
            b.steinerOnly().o1 (0, 1, 0, 1).o2 (0, 0, 1, 0).tune (2, 12).mix (0.6f, 0.4f, 0, 0).st (LP, 3000, 0.1f, 0.2f)
             .env (1, 0.001f, 0.1f, 0.0f, 0.05f).env (2, 0.002f, 0.4f, 0.4f, 0.05f).s ("env2Vel", 0).pm (srcEnv3, dVco1Pitch, 12.0f)
             .env (3, 0.001f, 0.01f, 0.0f, 0.01f);
            break;
        default: // Detuned Upright: wobbly, drifting tuning
            b.ladderOnly().s ("drift", 0.8f).o1 (0.5f, 0, 0.5f, 0).o2 (1, 0, 0, 0).tune (2, 0, 22).mix (0.6f, 0.5f, 0, 0)
             .ld (LP24, 2000, 0.1f, 0.3f).env (2, 0.001f, 2.5f, 0.1f, 0.5f).reverb (0.2f);
            break;
    }
}

void brassBank (PB& b, int i)
{
    b.voice (modePoly).ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 7).mix (0.65f, 0.6f, 0, 0)
     .s ("env1Vel", 0.5f).m (srcVelocity, dLdCutoff, 0.15f).m (srcWheel, dLdCutoff, 0.4f).m (srcAftertouch, dLdCutoff, 0.2f);

    switch (i)
    {
        case 0:  b.ld (LP24, 600, 0.15f, 0.5f).s ("ldDrive", 0.5f).env (1, 0.08f, 0.5f, 0.55f, 0.3f).env (2, 0.03f, 0.5f, 0.85f, 0.3f).reverb (0.2f); break;
        case 1:  b.ld (LP24, 1100, 0.1f, 0.4f).env (1, 0.03f, 0.4f, 0.6f, 0.2f).env (2, 0.02f, 0.3f, 0.9f, 0.2f).vibrato (0.1f).reverb (0.3f); break;
        case 2:  b.tune (2, -12, 5).ultra (1, 0.3f).ld (LP24, 700, 0.15f, 0.45f).env (1, 0.07f, 0.6f, 0.6f, 0.3f).chorus (0.3f).reverb (0.25f); break;
        case 3:  b.ld (LP12, 400, 0.1f, 0.5f).env (1, 0.3f, 0.8f, 0.6f, 0.6f).env (2, 0.15f, 0.5f, 0.9f, 0.6f).reverb (0.3f); break;
        case 4:  b.ld (LP24, 500, 0.2f, 0.65f).env (1, 0.005f, 0.2f, 0.0f, 0.15f).env (2, 0.005f, 0.35f, 0.0f, 0.2f).reverb (0.3f).delay (D1_8, 0.3f, 0.15f); break;
        case 5:  b.voice (modeLegato).s ("glide", 0.03f).o2 (0, 1, 0, 0).pw (2, 0.35f).ld (LP24, 900, 0.2f, 0.4f).env (1, 0.05f, 0.4f, 0.6f, 0.2f)
                  .vibrato (0.15f, 5.5f).env (2, 0.03f, 0.4f, 0.9f, 0.2f); break;
        case 6:  b.tune (1, -12).tune (2, -12, 6).o1 (1, 0, 0, 0.5f).ld (LP24, 300, 0.2f, 0.45f).s ("ldDrive", 0.6f).env (1, 0.1f, 0.6f, 0.5f, 0.3f); break;
        case 7:  b.o2 (0, 1, 0, 0).ld (LP24, 800, 0.15f, 0.4f).env (1, 0.06f, 0.5f, 0.6f, 0.4f).chorus (0.4f).env (2, 0.04f, 0.4f, 0.85f, 0.4f)
                  .lfo (1, TRI, 0.8f).m (srcLfo1, dVco2PW, 0.35f).lfo (2, SINE, 0.15f).m (srcLfo2, dPan, 0.5f); break;
        case 8:  b.ld (LP24, 350, 0.25f, 0.55f).env (1, 1.2f, 1.0f, 0.7f, 0.8f).env (2, 0.8f, 0.5f, 1.0f, 0.8f).reverb (0.35f); break;
        case 9:  b.route (BOTH, BOTH, BOTH, BOTH).outs (0.5f, 0.8f).st (HP, 400, 0.2f, 0).ld (LP24, 1500, 0.2f, 0.35f).s ("ldDrive", 0.6f)
                  .env (1, 0.04f, 0.4f, 0.6f, 0.3f); break;
        case 10: b.voice (modeLegato).steinerOnly().o1 (1, 0, 0, 0.6f).tune (1, -12).o2 (0, 1, 0, 0).tune (2, -12, 6).s ("glide", 0.08f)
                  .st (LP, 450, 0.45f, 0.5f).s ("stBrute", 0.55f).s ("stDrive", 0.6f).env (1, 0.06f, 0.5f, 0.5f, 0.2f)
                  .vibrato (0.15f, 5.0f).delay (D1_4, 0.3f, 0.15f); break;
        case 11: b.ultra (1, 0.4f).ultra (2, 0.4f).tune (2, 12, 4).ld (LP24, 2600, 0.1f, 0.3f).env (1, 0.005f, 0.3f, 0.5f, 0.2f)
                  .env (2, 0.004f, 0.6f, 0.6f, 0.25f).chorus (0.4f).reverb (0.25f).delay (D1_8, 0.25f, 0.12f); break;
        case 12: b.ld (LP24, 450, 0.35f, 0.3f).lfo (2, TRI, 0.25f).m (srcLfo2, dLdCutoff, 0.3f).env (1, 0.1f, 0.6f, 0.6f, 0.4f); break;
        case 13: b.voice (modeMono).tune (1, -24).tune (2, -12).o1 (1, 0, 0, 1).ld (LP24, 250, 0.15f, 0.4f).env (1, 0.08f, 0.5f, 0.5f, 0.2f); break;
        case 14: b.route (ST, ST, ST, ST).outs (1, 0).o1 (0.6f, 0.5f, 0, 0).pw (1, 0.4f).st (LP, 900, 0.35f, 0.45f).env (1, 0.05f, 0.5f, 0.55f, 0.3f)
                  .m (srcVelocity, dStCutoff, 0.2f); break;
        default: b.ultra (1, 0.5f).ultra (2, 0.5f).ld (LP24, 900, 0.1f, 0.4f).env (1, 0.1f, 0.6f, 0.6f, 0.5f).chorus (0.5f).reverb (0.3f)
                  .lfo (2, SINE, 0.2f).m (srcLfo2, dPan, 0.3f); break;
    }
}

void stringsBank (PB& b, int i)
{
    b.voice (modePoly).ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 12, 5).mix (0.65f, 0.35f, 0, 0)
     .env (2, 0.4f, 1.0f, 0.9f, 1.2f).m (srcWheel, dLdCutoff, 0.4f).chorus (0.55f, 0.7f, 0.5f);

    switch (i)
    {
        case 0:  b.o2 (0, 1, 0, 0).pw (2, 0.3f).ld (LP12, 3500, 0.1f, 0).chorus (0.7f, 0.8f).reverb (0.3f); break;
        case 1:  b.ultra (1, 0.6f).ld (LP12, 2500, 0.1f, 0).lfo (2, SINE, 0.2f).m (srcLfo2, dPan, 0.4f).reverb (0.35f); break;
        case 2:  b.voice (modeLegato).tune (1, -12).tune (2, -12, 5).s ("glide", 0.03f).ld (LP24, 1400, 0.1f, 0.2f).env (1, 0.15f, 1.0f, 0.6f, 0.4f)
                  .env (2, 0.12f, 1.0f, 0.9f, 0.4f).vibrato (0.15f, 5.0f).s ("chType", 0).reverb (0.3f); break;
        case 3:  b.voice (modeLegato).route (BOTH, BOTH, BOTH, BOTH).outs (0.5f, 0.7f).st (BP, 2500, 0.3f, 0).ld (LP12, 3500, 0.1f, 0)
                  .vibrato (0.25f, 6.0f).env (2, 0.08f, 1.0f, 0.9f, 0.4f).s ("chType", 0).reverb (0.3f); break;
        case 4:  b.ld (LP24, 700, 0.15f, 0.5f).env (1, 0.001f, 0.15f, 0.0f, 0.15f).env (2, 0.001f, 0.3f, 0.0f, 0.25f).s ("chType", 0).reverb (0.3f); break;
        case 5:  b.ld (LP12, 2500, 0.1f, 0).env (2, 2.0f, 1.0f, 1.0f, 2.5f).reverb (0.4f, 0.9f); break;
        case 6:  b.route (ST, ST, ST, ST).series().outs (1, 1).st (HP, 600, 0.35f, 0).ld (LP24, 4000, 0.1f, 0).chorus (0.5f)
                  .env (2, 0.05f, 0.6f, 0.7f, 0.5f).lfo (2, TRI, 0.4f).m (srcLfo2, dStCutoff, 0.35f); break;
        case 7:  b.tune (2, 0, 6).mix (0.6f, 0.6f, 0, 0).ld (LP12, 2000, 0.1f, 0).reverb (0.35f); break;
        case 8:  b.ld (LP24, 400, 0.2f, 0.5f).env (1, 3.0f, 2.0f, 1.0f, 2.0f).env (2, 1.5f, 1.0f, 1.0f, 2.0f).reverb (0.45f, 0.9f); break;
        case 9:  b.o1 (0, 1, 0, 0).o2 (0, 1, 0, 0).pw (1, 0.3f).pw (2, 0.7f).lfo (1, TRI, 0.5f).m (srcLfo1, dVco1PW, 0.2f).m (srcLfo1, dVco2PW, -0.2f)
                  .ld (LP12, 3000, 0.1f, 0).chorus (0.8f, 0.9f); break;
        case 10: b.tune (1, -12).tune (2, 0, 5).ld (LP24, 800, 0.15f, 0.1f).reverb (0.4f, 0.85f, 0.8f); break;
        case 11: b.ld (LP12, 2500, 0.1f, 0).lfo (1, TRI, 7.0f).m (srcLfo1, dVca, 0.5f).env (2, 0.1f, 1.0f, 0.9f, 0.6f); break;
        case 12: b.route (ST, ST, ST, ST).series().outs (1, 1).tune (1, 12).tune (2, 24, 5).st (HP, 900, 0.2f, 0).ld (LP12, 6000, 0.1f, 0)
                  .vibrato (0.12f, 6.0f).env (2, 0.15f, 1.0f, 0.9f, 0.8f).reverb (0.45f, 0.9f); break;
        case 13: b.tune (1, -12).tune (2, 0, 4).o1 (1, 0, 0, 0.4f).ld (LP24, 1200, 0.1f, 0.15f).env (2, 0.25f, 1.0f, 0.9f, 0.8f); break;
        case 14: b.o1 (0, 0, 1, 0).metal (1, 0.25f).o2 (0, 0, 1, 0).metal (2, 0.4f).mix (0.6f, 0.5f, 0, 0).ld (LP12, 5000, 0.1f, 0)
                  .env (2, 0.1f, 1.0f, 0.8f, 2.5f).lfo (2, SINE, 0.5f).m (srcLfo2, dVco2Metal, 0.4f).m (srcLfo2, dVco1Metal, -0.3f)
                  .s ("chType", 0).reverb (0.6f, 0.95f); break;
        default: b.ultra (1, 0.7f).ld (LP12, 2800, 0.1f, 0.1f).env (2, 1.0f, 1.0f, 1.0f, 3.0f).reverb (0.5f, 0.95f).delay (D1_4, 0.3f, 0.12f); break;
    }
}

void pluckBank (PB& b, int i)
{
    b.voice (modePoly).ladderOnly().s ("env1Vel", 0.6f).s ("env2Vel", 0.5f).m (srcVelocity, dLdCutoff, 0.2f).m (srcWheel, dCutoff, 0.4f);

    switch (i)
    {
        case 0:  b.o1 (1, 0.4f, 0, 0).mix (0.85f, 0, 0, 0).ld (LP24, 300, 0.25f, 0.7f).env (1, 0.001f, 0.2f, 0.0f, 0.2f)
                  .env (2, 0.001f, 0.5f, 0.0f, 0.3f).delay (D1_8, 0.35f, 0.18f).reverb (0.2f); break;
        case 1:  b.o1 (0, 1, 0, 0).pw (1, 0.2f).mix (0.85f, 0, 0, 0).ld (LP24, 1500, 0.1f, 0.4f).env (1, 0.001f, 0.6f, 0.0f, 0.6f)
                  .env (2, 0.001f, 1.2f, 0.0f, 1.0f).reverb (0.35f); break;
        case 2:  b.o1 (0, 0, 1, 0).metal (1, 0.3f).mix (0.85f, 0, 0, 0).ld (LP24, 4000, 0, 0).m (srcEnv1, dVco1Metal, 0.5f)
                  .env (1, 0.001f, 0.3f, 0.0f, 0.3f).env (2, 0.001f, 0.8f, 0.0f, 0.5f).pm (srcEnv3, dVco1Pitch, -0.8f).env (3, 0.001f, 0.12f, 0.0f, 0.1f); break;
        case 3:  b.o1 (1, 0, 0, 0).mix (0.9f, 0, 0, 0).ld (LP24, 250, 0.15f, 0.4f).env (1, 0.001f, 0.1f, 0.0f, 0.1f).env (2, 0.001f, 0.15f, 0.0f, 0.1f); break;
        case 4:  b.o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 19).metal (2, 0.4f).mix (0.7f, 0.5f, 0, 0).ld (LP24, 5000, 0, 0)
                  .env (2, 0.001f, 1.5f, 0.0f, 1.5f).reverb (0.3f); break;
        case 5:  b.o1 (0, 0, 1, 0).tune (1, 12).mix (0.9f, 0, 0, 0).ld (LP24, 2000, 0.1f, 0.3f).env (2, 0.001f, 0.7f, 0.0f, 0.6f)
                  .reverb (0.45f, 0.85f).delay (D1_8T, 0.35f, 0.2f); break;
        case 6:  b.o1 (0, 0, 1, 0).mix (0.9f, 0, 0, 0).ld (LP24, 3000, 0, 0).env (2, 0.001f, 0.35f, 0.0f, 0.2f).pm (srcEnv3, dVco1Pitch, 12.0f)
                  .env (3, 0.001f, 0.01f, 0.0f, 0.01f); break;
        case 7:  b.o1 (0, 0, 1, 0).tune (1, 12).o2 (0, 0, 1, 0).tune (2, 24).mix (0.8f, 0.3f, 0, 0).ld (LP24, 4000, 0, 0)
                  .env (2, 0.001f, 0.6f, 0.0f, 0.4f).lfo (2, SINE, 3.0f).m (srcLfo2, dPan, 0.3f); break;
        case 8:  b.steinerOnly().o1 (0, 1, 0, 0).mix (0.9f, 0, 0, 0).st (BP, 900, 0.3f, 0.5f).env (1, 0.001f, 0.2f, 0.0f, 0.15f)
                  .env (2, 0.001f, 0.3f, 0.0f, 0.2f); break;
        case 9:  b.o1 (0.5f, 0, 0.6f, 0).mix (0.85f, 0, 0, 0).ld (LP12, 600, 0.1f, 0.5f).env (1, 0.001f, 0.4f, 0.0f, 0.3f)
                  .env (2, 0.001f, 1.0f, 0.0f, 0.6f).chorus (0.25f); break;
        case 10: b.o1 (0, 0, 1, 0).metal (1, 0.2f).o2 (0, 0, 1, 0).tune (2, 7).vco3 (0, 24).mix (0.6f, 0.5f, 0.2f, 0).ld (LP24, 4000, 0, 0)
                  .env (2, 0.001f, 0.9f, 0.0f, 0.5f).reverb (0.25f); break;
        case 11: b.o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 12).mix (0.6f, 0.5f, 0, 0).ld (LP24, 450, 0.2f, 0.6f).env (1, 0.001f, 0.3f, 0.0f, 0.2f)
                  .env (2, 0.001f, 0.45f, 0.0f, 0.3f).delay (D1_8, 0.35f, 0.2f); break;
        case 12: b.steinerOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 7).mix (0.6f, 0.5f, 0, 0).st (BP, 800, 0.4f, 0.3f)
                  .env (2, 0.001f, 0.35f, 0.0f, 0.2f).delay (D1_4T, 0.6f, 0.35f).reverb (0.3f); break;
        case 13: b.o1 (0, 0, 1, 0).tune (1, 24).mix (0.9f, 0, 0, 0).ld (LP24, 6000, 0, 0).env (2, 0.001f, 0.2f, 0.0f, 0.2f)
                  .delay (D1_16, 0.6f, 0.4f).reverb (0.45f).lfo (2, SH, 4.0f).m (srcLfo2, dPan, 0.8f); break;
        case 14: b.o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).s ("vco2Sync", 1).tune (2, 9).mix (0.3f, 0.8f, 0, 0).ld (LP24, 2500, 0.1f, 0.2f)
                  .pm (srcEnv1, dVco2Pitch, 18.0f).env (1, 0.001f, 0.6f, 0.0f, 0.4f).env (2, 0.001f, 1.1f, 0.0f, 0.6f)
                  .delay (D1_8T, 0.45f, 0.25f).reverb (0.3f); break;
        default: b.steinerOnly().o1 (0, 0.5f, 1, 0).mix (0.85f, 0, 0, 0).st (BP, 700, 0.45f, 0.2f).env (2, 0.001f, 0.12f, 0.0f, 0.1f); break;
    }
}

void arpBank (PB& b, int i)
{
    b.voice (modeMono).ladderOnly().m (srcWheel, dLdCutoff, 0.5f).m (srcAftertouch, dLdReso, 0.3f);

    switch (i)
    {
        case 0:  b.arp (0, 2, D1_16, 0.5f).o1 (1, 0.5f, 0, 0).mix (0.85f, 0, 0, 0).ld (LP24, 600, 0.3f, 0.5f).env (1, 0.001f, 0.18f, 0.1f, 0.1f)
                  .env (2, 0.001f, 0.3f, 0.4f, 0.1f).delay (D1_8T, 0.35f, 0.2f); break;
        case 1:  b.arp (1, 2, D1_8T, 0.6f).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).s ("vco2Sync", 1).tune (2, 7).mix (0.3f, 0.7f, 0, 0)
                  .ld (LP24, 3000, 0.2f, 0).pm (srcEnv3, dVco2Pitch, 12.0f).env (3, 0.001f, 0.2f, 0.0f, 0.1f).env (2, 0.001f, 0.3f, 0.5f, 0.1f); break;
        case 2:  b.arp (2, 3, D1_16, 0.5f).o1 (0, 0, 1, 0).metal (1, 0.3f).mix (0.9f, 0, 0, 0).ld (LP24, 4000, 0, 0)
                  .env (2, 0.001f, 0.25f, 0.2f, 0.15f).delay (D1_8, 0.45f, 0.25f).reverb (0.25f); break;
        case 3:  b.arp (3, 2, D1_16, 0.5f).o1 (0, 1, 0, 0).mix (0.9f, 0, 0, 0).ld (LP24, 700, 0.4f, 0.2f).lfoSync (2, SH, D1_16)
                  .m (srcLfo2, dLdCutoff, 0.45f).env (2, 0.001f, 0.3f, 0.4f, 0.1f); break;
        case 4:  b.arp (4, 1, D1_16, 0.7f).voice (modePoly).o1 (1, 0, 0, 0).o2 (0, 1, 0, 0).tune (2, 12).mix (0.6f, 0.4f, 0, 0)
                  .ld (LP12, 1500, 0.15f, 0.2f).env (2, 0.02f, 0.5f, 0.5f, 0.4f).chorus (0.35f); break;
        case 5:  b.arp (0, 4, D1_16, 0.45f).o1 (1, 0, 0, 0).mix (0.9f, 0, 0, 0).ld (LP24, 1500, 0.2f, 0.3f).env (2, 0.001f, 0.2f, 0.3f, 0.1f); break;
        case 6:  b.arp (2, 2, D1_16, 0.5f).o1 (1, 0, 0, 0.4f).mix (0.9f, 0, 0, 0).ld (LP24, 450, 0.6f, 0.35f).lfo (1, SINE, 0.1f)
                  .m (srcLfo1, dLdCutoff, 0.35f).env (1, 0.001f, 0.15f, 0.1f, 0.1f).delay (D1_8T, 0.4f, 0.2f); break;
        case 7:  b.arp (0, 1, D1_16, 0.25f).o1 (1, 0, 0, 0).ultra (1, 0.7f).o2 (1, 0, 0, 0).tune (2, 12).mix (0.6f, 0.4f, 0, 0)
                  .ld (LP24, 3000, 0.1f, 0).env (2, 0.001f, 0.1f, 0.8f, 0.05f).reverb (0.3f).delay (D1_8T, 0.3f, 0.15f); break;
        case 8:  b.arp (3, 2, D1_32, 0.5f).steinerOnly().o1 (0, 0, 1, 0).metal (1, 0.3f).mix (0.9f, 0, 0, 0).st (BP, 1200, 0.6f, 0.2f)
                  .lfoSync (2, SH, D1_32).m (srcLfo2, dStCutoff, 0.4f).env (2, 0.001f, 0.12f, 0.0f, 0.05f); break;
        case 9:  b.arp (3, 3, D1_16T, 0.5f).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).s ("vco2Sync", 1).mix (0.2f, 0.8f, 0, 0).ld (LP24, 4000, 0.1f, 0)
                  .lfo (1, TRI, 0.15f).pm (srcLfo1, dVco2Pitch, 18.0f).env (2, 0.001f, 0.12f, 0.0f, 0.1f).reverb (0.35f).delay (D1_8, 0.4f, 0.2f); break;
        case 10: b.arp (1, 3, D1_16, 0.5f).o1 (0, 0, 1, 0).mix (0.9f, 0, 0, 0).ld (LP24, 5000, 0, 0).m (srcEnv1, dVco1Metal, 0.6f)
                  .env (1, 0.001f, 0.15f, 0.0f, 0.1f).env (2, 0.001f, 0.25f, 0.3f, 0.1f); break;
        case 11: b.arp (0, 2, D1_16, 0.55f).s ("seqSwing", 0.2f).o1 (0, 1, 0, 0).pw (1, 0.35f).mix (0.9f, 0, 0, 0).ld (LP24, 900, 0.3f, 0.3f)
                  .env (2, 0.001f, 0.25f, 0.3f, 0.1f).delay (D1_8, 0.3f, 0.15f); break;
        case 12: b.arp (2, 2, D1_8, 0.9f).voice (modePoly).o1 (1, 0, 0, 0).ultra (1, 0.4f).o2 (0, 1, 0, 0).tune (2, 12).mix (0.6f, 0.4f, 0, 0)
                  .ld (LP24, 1200, 0.1f, 0.2f).env (2, 0.05f, 1.0f, 0.6f, 1.2f).chorus (0.4f).reverb (0.35f); break;
        case 13: b.arp (0, 2, D1_32, 0.5f).o1 (1, 0, 0, 0).mix (0.9f, 0, 0, 0).ld (LP24, 1200, 0.3f, 0.3f).env (2, 0.001f, 0.1f, 0.3f, 0.05f); break;
        case 14: b.arp (2, 4, D1_16T, 0.5f).o1 (0, 1, 0, 0).o2 (0, 0, 1, 0).tune (2, 12).mix (0.6f, 0.5f, 0, 0).ld (LP24, 2000, 0.2f, 0.2f)
                  .env (2, 0.001f, 0.2f, 0.3f, 0.1f).delay (D1_16T, 0.4f, 0.2f); break;
        default: b.arp (0, 3, D1_16, 0.5f).o1 (1, 0, 0, 0.5f).mix (0.9f, 0, 0, 0).ld (LP24, 800, 0.35f, 0.4f).s ("ldDrive", 0.6f)
                  .env (1, 0.001f, 0.2f, 0.1f, 0.1f).env (2, 0.001f, 0.3f, 0.4f, 0.1f).delay (D1_8T, 0.4f, 0.2f); break;
    }
}

void seqBank (PB& b, int i)
{
    b.voice (modeMono).ladderOnly().m (srcWheel, dLdCutoff, 0.5f).m (srcAftertouch, dLdReso, 0.3f);

    switch (i)
    {
        case 0:  b.seq (D1_16, 0.5f, { 0, 0, 12, 0, 0, 10, 0, 7, 0, 0, 12, 0, 3, 0, 5, 7 }).o1 (1, 0, 0, 0.5f).mix (0.85f, 0, 0, 0)
                  .ld (LP24, 350, 0.35f, 0.5f).env (1, 0.001f, 0.2f, 0.05f, 0.1f).env (2, 0.001f, 0.25f, 0.5f, 0.1f); break;
        case 1:  b.seq (D1_16, 0.4f, { 0, 12, 0, 12, 0, 12, 0, 12 }).o1 (0, 1, 0, 0).pw (1, 0.3f).mix (0.9f, 0, 0, 0).ld (LP24, 800, 0.2f, 0.4f)
                  .lfo (1, TRI, 0.2f).m (srcLfo1, dVco1PW, 0.3f).env (2, 0.001f, 0.2f, 0.4f, 0.1f).delay (D1_8T, 0.3f, 0.15f); break;
        case 2:  b.seq (D1_16, 0.6f, { 0, 3, 7, 10, 12, 10, 7, 3 }).o1 (0, 0, 1, 0).metal (1, 0.25f).mix (0.9f, 0, 0, 0).ld (LP24, 4000, 0, 0)
                  .env (2, 0.001f, 0.3f, 0.3f, 0.15f).reverb (0.3f); break;
        case 3:  b.seq (D1_16, 0.3f, { 0, 0, 0, 12, 0, 0, 12, 0 }, { 1, 0, 1, 1, 1, 0, 1, 1 }).steinerOnly().o1 (1, 0, 0, 0.5f).mix (0.9f, 0, 0, 0)
                  .st (LP, 500, 0.6f, 0.4f).s ("stBrute", 0.5f).env (1, 0.001f, 0.12f, 0.0f, 0.1f).env (2, 0.001f, 0.15f, 0.3f, 0.05f); break;
        case 4:  b.seq (D1_16, 0.55f, { 0, 12, 0, 0, 15, 0, 12, 10, 0, 0, 7, 12, 0, 3, 0, 0 }).voice (modeLegato).s ("glide", 0.06f)
                  .o1 (1, 0, 0, 0).mix (0.85f, 0, 0, 0).ld (LP24, 200, 0.85f, 0.65f).s ("ldDrive", 0.7f).env (1, 0.001f, 0.2f, 0.0f, 0.1f)
                  .env (2, 0.001f, 0.3f, 0.8f, 0.05f).delay (D1_8T, 0.35f, 0.15f); break;
        case 5:  b.seq (D1_16, 0.5f, { 0, 7, 12, 15, 19, 15, 12, 7, 0, 7, 12, 14, 17, 14, 12, 7 }).o1 (1, 0, 0, 0).o2 (0, 1, 0, 0).tune (2, 12)
                  .mix (0.6f, 0.3f, 0, 0).ld (LP24, 700, 0.45f, 0.3f).lfo (1, SINE, 0.07f).m (srcLfo1, dLdCutoff, 0.35f)
                  .env (2, 0.001f, 0.3f, 0.4f, 0.15f).delay (D1_8T, 0.45f, 0.25f).reverb (0.3f); break;
        case 6:  b.seq (D1_16, 0.5f, { 0, 2, 4, 7, 9, 12, 9, 7, 4, 2 }).o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 12).mix (0.8f, 0.3f, 0, 0)
                  .ld (LP24, 3000, 0, 0).env (2, 0.001f, 0.4f, 0.0f, 0.3f).delay (D1_8, 0.4f, 0.25f); break;
        case 7:  b.seq (D1_8, 0.5f, { 0, 7, 0, 7, 0, 7, 12, 7 }, { 1, 1, 0, 1, 1, 0, 1, 1 }).voice (modePoly).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0)
                  .tune (2, 7).mix (0.6f, 0.5f, 0, 0).ld (LP24, 900, 0.2f, 0.4f).env (2, 0.001f, 0.3f, 0.2f, 0.2f).reverb (0.25f); break;
        case 8:  b.seq (D1_8, 0.9f, { 0, 4, 7, 12, 5, 9, 12, 17, 7, 11, 14, 19, 5, 9, 12, 17 }).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 9)
                  .mix (0.6f, 0.5f, 0, 0).ld (LP24, 700, 0.45f, 0.35f).env (1, 0.01f, 0.4f, 0.3f, 0.3f).env (2, 0.01f, 0.5f, 0.7f, 0.4f).chorus (0.4f); break;
        case 9:  b.seq (D1_16, 0.3f, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 0, 1, 1, 0, 1, 0, 1 }).o1 (0, 0, 1, 0).mix (0.5f, 0, 0, 0.5f)
                  .ld (LP24, 2500, 0.2f, 0.3f).pm (srcEnv3, dVco1Pitch, 12.0f).env (3, 0.001f, 0.04f, 0.0f, 0.03f).env (2, 0.001f, 0.1f, 0.0f, 0.05f); break;
        case 10: b.seq (D1_16, 0.3f, { 0, 0, 1, 0, 0, 1, 0, 0, 3, 0, 1, 0, 0, 1, 0, 1 }).o1 (1, 0, 0, 0.6f).mix (0.85f, 0, 0, 0)
                  .ld (LP24, 160, 0.7f, 0.45f).s ("ldDrive", 0.6f).lfo (2, SINE, 0.07f).m (srcLfo2, dLdCutoff, 0.35f).env (2, 0.001f, 0.25f, 0.6f, 0.1f).delay (D1_4T, 0.3f, 0.15f); break;
        case 11: b.seq (D1_16, 0.5f, { 0, 0, -12, 0, 3, 0, -12, 0 }).steinerOnly().o1 (1, 0, 0, 0.5f).o2 (0, 1, 0, 0).tune (2, -12).mix (0.7f, 0.4f, 0, 0)
                  .st (LP, 250, 0.4f, 0.4f).s ("stBrute", 0.6f).s ("stDrive", 0.6f).env (2, 0.001f, 0.3f, 0.6f, 0.1f); break;
        case 12: b.seq (D1_16, 0.45f, { 0, 12, 10, 0, 7, 0, 12, 5 }, { 1, 1, 0, 1, 1, 0, 1, 1 }).s ("seqSwing", 0.2f).steinerOnly()
                  .o1 (0, 1, 0, 0).pw (1, 0.25f).mix (0.9f, 0, 0, 0).st (BP, 700, 0.6f, 0.5f).env (1, 0.001f, 0.15f, 0.1f, 0.1f)
                  .env (2, 0.001f, 0.2f, 0.4f, 0.1f); break;
        case 13: b.seq (D1_8T, 0.5f, { 0, 0, 12, 0, 0 }, { 1, 0, 1, 0, 1 }).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 0, 15).mix (0.6f, 0.6f, 0, 0)
                  .ld (LP24, 500, 0.3f, 0.45f).env (2, 0.001f, 0.3f, 0.3f, 0.2f).reverb (0.3f).delay (D1_4, 0.4f, 0.2f); break;
        case 14: b.seq (D1_16, 0.5f, { 0, 1, 0, 5, 0, 1, 7, 5 }).o1 (0, 0, 1, 0).metal (1, 0.4f).mix (0.9f, 0, 0, 0).ld (LP24, 2000, 0.4f, 0.2f)
                  .lfoSync (1, TRI, D1_1).m (srcLfo1, dVco1Metal, 0.4f).env (2, 0.001f, 0.25f, 0.4f, 0.1f); break;
        default: b.seq (D1_8, 0.7f, { 0, 3, 7, 3, 0, 5, 8, 5, 0, 3, 7, 10, 12, 10, 7, 3 }).voice (modePoly).o1 (1, 0, 0, 0).ultra (1, 0.4f)
                  .mix (0.85f, 0, 0, 0).ld (LP24, 1000, 0.15f, 0.2f).env (2, 0.01f, 0.4f, 0.6f, 0.4f).chorus (0.3f).reverb (0.35f); break;
    }
}

void syncBank (PB& b, int i)
{
    b.voice (modeLegato).o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).s ("vco2Sync", 1).mix (0.2f, 0.8f, 0, 0).ladderOnly().ld (LP24, 4000, 0.15f, 0)
     .env (2, 0.003f, 0.5f, 0.85f, 0.3f).pm (srcWheel, dVco2Pitch, 12.0f);

    switch (i)
    {
        case 0:  b.tune (2, 7).pm (srcEnv3, dVco2Pitch, 19.0f).env (3, 0.001f, 0.6f, 0.2f, 0.4f).vibrato (0.1f).delay (D1_8, 0.3f, 0.15f); break;
        case 1:  b.tune (2, 5).lfo (2, TRI, 0.25f).pm (srcLfo2, dVco2Pitch, 14.0f).chorus (0.3f); break;
        case 2:  b.voice (modeMono).tune (1, -12).tune (2, -5).o1 (1, 0, 0, 0.5f).ld (LP24, 600, 0.3f, 0.4f).env (1, 0.001f, 0.3f, 0.1f, 0.1f)
                  .pm (srcEnv3, dVco2Pitch, 24.0f).env (3, 0.001f, 0.25f, 0.0f, 0.1f); break;
        case 3:  b.voice (modePoly).ultra (1, 0.3f).tune (2, 12).pm (srcEnv3, dVco2Pitch, 7.0f).env (3, 0.08f, 0.6f, 0.3f, 0.4f)
                  .ld (LP24, 1200, 0.15f, 0.4f).env (1, 0.06f, 0.5f, 0.6f, 0.3f).env (2, 0.05f, 0.5f, 0.85f, 0.4f).chorus (0.3f); break;
        case 4:  b.tune (2, 0).pm (srcEnv3, dVco2Pitch, 36.0f).env (3, 0.001f, 0.18f, 0.0f, 0.15f); break;
        case 5:  b.tune (2, 3).lfoSync (1, SAWDN, D1_16).pm (srcLfo1, dVco2Pitch, 24.0f).s ("lfo1Retrig", 1).env (2, 0.001f, 0.4f, 0.6f, 0.2f)
                  .delay (D1_8, 0.3f, 0.15f); break;
        case 6:  b.voice (modePoly).env (2, 1.2f, 1.0f, 0.9f, 2.0f).lfo (2, SINE, 0.1f).pm (srcLfo2, dVco2Pitch, 10.0f).tune (2, 12)
                  .reverb (0.45f, 0.9f); break;
        case 7:  b.voice (modePoly).env (2, 0.001f, 0.4f, 0.0f, 0.3f).pm (srcEnv3, dVco2Pitch, 20.0f).env (3, 0.001f, 0.12f, 0.0f, 0.1f)
                  .delay (D1_8T, 0.35f, 0.2f); break;
        case 8:  b.route (ST, ST, ST, ST).outs (1, 0).st (LP, 2500, 0.45f, 0).s ("stBrute", 0.75f).s ("stDrive", 0.8f).tune (2, 24)
                  .pm (srcEnv3, dVco2Pitch, -20.0f).env (3, 0.001f, 0.8f, 0.0f, 0.3f); break;
        case 9:  b.voice (modePoly).tune (2, 4).pm (srcWheel, dVco2Pitch, 30.0f).m (srcWheel, dLdCutoff, -0.2f).env (2, 0.3f, 1.0f, 0.9f, 1.2f)
                  .ld (LP24, 1800, 0.2f, 0).chorus (0.4f).reverb (0.35f); break;
        case 10: b.voice (modePoly).env (2, 0.002f, 1.0f, 0.3f, 0.4f).tune (2, 12).pm (srcVelocity, dVco2Pitch, 10.0f).ld (LP24, 2500, 0.1f, 0.3f)
                  .env (1, 0.001f, 0.5f, 0.2f, 0.3f); break;
        case 11: b.route (ST, ST, ST, ST).outs (1, 0).st (LP, 3000, 0.5f, 0).s ("stBrute", 1).tune (2, 9).lfo (1, SQR, 7.0f)
                  .pm (srcLfo1, dVco2Pitch, 5.0f); break;
        case 12: b.voice (modePoly).tune (2, 2).pm (srcVelocity, dVco2Pitch, 20.0f).pm (srcAftertouch, dVco2Pitch, 10.0f)
                  .env (2, 0.001f, 0.6f, 0.0f, 0.4f).ld (LP24, 2500, 0.2f, 0.3f).env (1, 0.001f, 0.3f, 0.0f, 0.2f); break;
        case 13: b.arp (2, 2, D1_16, 0.45f).voice (modeMono).tune (2, 12).pm (srcEnv3, dVco2Pitch, 12.0f).env (3, 0.001f, 0.15f, 0.0f, 0.1f)
                  .env (2, 0.001f, 0.25f, 0.4f, 0.1f).delay (D1_8T, 0.35f, 0.2f); break;
        case 14: b.tune (2, 0).pm (srcEnv3, dVco2Pitch, 24.0f).env (3, 2.5f, 3.0f, 0.4f, 1.0f).env (2, 0.2f, 1.0f, 0.9f, 0.8f).reverb (0.3f); break;
        default: b.voice (modePoly).o2 (0, 0, 1, 0).metal (2, 0.3f).tune (2, 12).env (2, 0.4f, 1.0f, 0.9f, 1.5f).lfo (2, SINE, 0.15f)
                  .pm (srcLfo2, dVco2Pitch, 7.0f).m (srcLfo2, dVco2Metal, 0.3f).reverb (0.4f, 0.9f).chorus (0.3f); break;
    }
}

void ultrasawBank (PB& b, int i)
{
    b.voice (modePoly).ladderOnly().o1 (1, 0, 0, 0).ultra (1, 0.85f).o2 (1, 0, 0, 0).ultra (2, 0.7f).mix (0.6f, 0.5f, 0, 0)
     .ld (LP12, 4000, 0.1f, 0.1f).env (2, 0.005f, 0.6f, 0.9f, 0.5f).m (srcWheel, dUltra, -0.6f).chorus (0.3f);

    switch (i)
    {
        case 0:  b.tune (2, -12).reverb (0.25f); break;
        case 1:  b.voice (modeLegato).tune (2, 0, 8).vibrato (0.1f).delay (D1_8, 0.35f, 0.2f); break;
        case 2:  b.env (2, 0.8f, 1.0f, 0.9f, 2.0f).tune (2, 12).reverb (0.45f, 0.9f).lfo (2, SINE, 0.15f).m (srcLfo2, dLdCutoff, 0.15f); break;
        case 3:  b.voice (modeMono).tune (1, -12).tune (2, -12).o1 (1, 0, 0, 0.6f).ld (LP24, 450, 0.2f, 0.45f).env (1, 0.001f, 0.3f, 0.2f, 0.2f)
                  .s ("chType", 0); break;
        case 4:  b.voice (modeLegato).s ("glide", 0.15f).tune (2, -12).o1 (1, 0.6f, 0, 0).pw (1, 0.2f).lfo (1, TRI, 0.6f).m (srcLfo1, dVco1PW, 0.3f)
                  .lfo (2, SINE, 0.5f).m (srcLfo2, dUltra, -0.4f).ld (LP24, 2500, 0.2f, 0); break;
        case 5:  b.tune (2, 12).lfo (2, TRI, 0.3f).m (srcLfo2, dUltra, -0.4f).m (srcLfo2, dLdCutoff, 0.2f).reverb (0.45f, 0.9f).delay (D1_4, 0.3f, 0.15f); break;
        case 6:  b.env (2, 0.4f, 1.0f, 0.9f, 1.4f).tune (2, 12, 4).ld (LP12, 3000, 0.1f, 0).chorus (0.5f, 0.7f); break;
        case 7:  b.ld (LP24, 1000, 0.2f, 0.4f).env (1, 0.001f, 0.8f, 0.4f, 0.6f).delay (D1_8T, 0.3f, 0.15f).reverb (0.3f); break;
        case 8:  b.env (2, 0.001f, 0.4f, 0.0f, 0.3f).env (1, 0.001f, 0.2f, 0.0f, 0.2f).ld (LP24, 600, 0.2f, 0.55f).delay (D1_8, 0.4f, 0.2f); break;
        case 9:  b.lfo (1, SINE, 0.4f).m (srcLfo1, dUltra, -0.6f).lfo (2, SINE, 0.13f).m (srcLfo2, dPan, 0.6f).tune (2, 7); break;
        case 10: b.ld (LP24, 800, 0.15f, 0.45f).env (1, 0.08f, 0.5f, 0.6f, 0.3f).env (2, 0.05f, 0.5f, 0.85f, 0.3f); break;
        case 11: b.tune (2, 7).env (2, 0.01f, 1.0f, 0.9f, 1.0f).reverb (0.4f, 0.9f).delay (D1_8T, 0.3f, 0.15f); break;
        case 12: b.voice (modeMono).s ("glide", 0.12f).tune (2, -12, 10).ld (LP24, 900, 0.55f, 0.4f).env (1, 0.001f, 0.4f, 0.3f, 0.2f)
                  .s ("chType", 0).delay (D1_8T, 0.4f, 0.2f); break;
        case 13: b.lfo (2, SINE, 0.25f).m (srcLfo2, dPan, 0.9f).s ("chMix", 0.6f).tune (2, -12); break;
        case 14: b.env (2, 1.5f, 1.0f, 1.0f, 3.0f).ld (LP24, 1200, 0.15f, 0).lfo (2, SINE, 0.08f).m (srcLfo2, dLdCutoff, 0.3f).reverb (0.55f, 0.95f); break;
        default: b.tune (2, -12).o1 (1, 0, 0, 0.4f).ld (LP24, 700, 0.15f, 0.35f).env (1, 0.5f, 1.0f, 0.8f, 1.0f).m (srcEnv3, dUltra, -0.5f)
                  .env (3, 2.0f, 1.0f, 1.0f, 1.0f); break;
    }
}

void metalBank (PB& b, int i)
{
    b.voice (i % 2 == 0 ? modeLegato : modePoly).ladderOnly().o1 (0, 0, 1, 0).metal (1, 0.35f).mix (0.8f, 0, 0, 0).ld (LP24, 6000, 0.1f, 0)
     .env (2, 0.003f, 0.8f, 0.7f, 0.5f).m (srcWheel, dVco1Metal, 0.5f).reverb (0.2f);

    switch (i)
    {
        case 0:  b.m (srcEnv1, dVco1Metal, 0.4f).env (1, 0.01f, 0.6f, 0.3f, 0.3f).vibrato (0.12f); break;
        case 1:  b.o2 (0, 0, 1, 0).tune (2, 19).metal (2, 0.4f).mix (0.6f, 0.4f, 0, 0).env (2, 0.001f, 2.5f, 0.0f, 2.0f).reverb (0.4f); break;
        case 2:  b.voice (modeMono).tune (1, -12).o1 (0, 0, 1, 0.5f).ld (LP24, 900, 0.2f, 0.3f).m (srcEnv3, dVco1Metal, 0.5f)
                  .env (3, 0.001f, 0.3f, 0.1f, 0.2f).env (1, 0.001f, 0.3f, 0.2f, 0.2f); break;
        case 3:  b.o2 (0, 0, 1, 0).tune (2, 6, 37).metal (2, 0.5f).mix (0.6f, 0.5f, 0, 0).env (2, 0.005f, 5.0f, 0.0f, 5.0f).reverb (0.5f, 0.95f); break;
        case 4:  b.lfo (2, TRI, 0.2f).m (srcLfo2, dVco1Metal, 0.55f).metal (1, 0.2f); break;
        case 5:  b.metal (1, 0.9f).o2 (0, 0, 1, 0).tune (2, 13).metal (2, 0.8f).mix (0.6f, 0.4f, 0, 0).env (2, 0.001f, 0.6f, 0.0f, 0.5f); break;
        case 6:  b.env (2, 1.0f, 1.0f, 0.9f, 2.0f).o2 (0, 0, 1, 0).tune (2, 7).metal (2, 0.3f).mix (0.6f, 0.5f, 0, 0).lfo (2, SINE, 0.1f)
                  .m (srcLfo2, dVco2Metal, 0.45f).m (srcLfo2, dVco1Metal, -0.3f).chorus (0.4f); break;
        case 7:  b.env (2, 0.001f, 0.35f, 0.0f, 0.3f).m (srcEnv1, dVco1Metal, 0.6f).env (1, 0.001f, 0.2f, 0.0f, 0.2f).delay (D1_8, 0.35f, 0.2f); break;
        case 8:  b.metal (1, 1.0f).tune (1, -12).route (ST, ST, ST, ST).outs (1, 0).st (LP, 3000, 0.3f, 0).s ("stBrute", 0.7f).s ("stDrive", 0.7f); break;
        case 9:  b.lfo (1, SINE, 2.0f).m (srcLfo1, dVco1Metal, 0.35f).s ("glide", 0.1f).chorus (0.3f); break;
        case 10: b.route (ST, ST, ST, ST).outs (1, 0).st (HP, 600, 0.3f, 0).env (2, 0.01f, 1.0f, 0.7f, 0.8f).lfo (2, SINE, 0.3f).m (srcLfo2, dStCutoff, 0.3f); break;
        case 11: b.mix (0.8f, 0, 0, 0.1f).ld (LP24, 1200, 0.3f, 0.2f).s ("ldDrive", 0.7f).lfo (2, RND, 1.0f).m (srcLfo2, dVco1Metal, 0.3f); break;
        case 12: b.route (ST, ST, ST, ST).outs (1, 0).st (BP, 1500, 0.5f, 0).env (2, 0.6f, 1.0f, 0.9f, 1.5f).lfo (2, SINE, 0.2f)
                  .m (srcLfo2, dStCutoff, 0.35f).reverb (0.4f); break;
        case 13: b.metal (1, 0.6f).tune (1, 24).env (2, 0.001f, 0.3f, 0.0f, 0.2f).delay (D1_16, 0.4f, 0.2f); break;
        case 14: b.arp (2, 2, D1_16, 0.4f).voice (modeMono).m (srcEnv3, dVco1Metal, 0.5f).env (3, 0.001f, 0.12f, 0.0f, 0.1f).delay (D1_8T, 0.4f, 0.2f); break;
        default: b.env (2, 2.0f, 1.0f, 1.0f, 4.0f).o2 (0, 0, 1, 0).tune (2, -12).metal (2, 0.4f).mix (0.6f, 0.5f, 0, 0).lfo (2, SINE, 0.05f)
                  .m (srcLfo2, dVco1Metal, 0.4f).m (srcLfo2, dVco2Metal, -0.4f).reverb (0.5f, 0.95f); break;
    }
}

void droneBank (PB& b, int i)
{
    b.voice (modePoly).s ("drift", 0.5f).env (2, 2.5f, 1.0f, 1.0f, 5.0f).reverb (0.55f, 0.95f).m (srcWheel, dCutoff, 0.5f);

    switch (i)
    {
        case 0:  b.ladderOnly().o1 (1, 0, 0, 0.7f).tune (1, -24).o2 (1, 0, 0, 0).tune (2, -12, 6).mix (0.6f, 0.5f, 0, 0).ld (LP24, 220, 0.3f, 0)
                  .lfo (2, SINE, 0.05f).m (srcLfo2, dLdCutoff, 0.3f); break;
        case 1:  b.steinerOnly().mix (0, 0, 0, 0.9f).st (BP, 900, 0.65f, 0).lfo (2, RND, 0.25f).m (srcLfo2, dStCutoff, 0.6f)
                  .lfo (1, SINE, 0.07f).m (srcLfo1, dPan, 0.7f); break;
        case 2:  b.ladderOnly().o1 (1, 0, 0, 0).tune (1, -12).mix (0.4f, 0, 0, 0.5f).s ("noiseColor", -0.9f).ld (LP24, 300, 0.3f, 0)
                  .lfo (1, SINE, 0.08f).m (srcLfo1, dVca, 0.35f).m (srcLfo1, dLdCutoff, 0.3f); break;
        case 3:  b.ladderOnly().o1 (0, 0, 1, 0).vco3 (0, 24).mix (0.6f, 0, 0.25f, 0).ld (LP12, 2000, 0.1f, 0).lfo (2, SINE, 0.3f)
                  .pm (srcLfo2, dVco3Pitch, 0.3f).flanger (0.35f, 0.6f, 0.05f); break;
        case 4:  b.route (ST, ST, ST, ST).outs (1, 0).o1 (0, 1, 0, 0).pw (1, 0.1f).o2 (1, 0, 0, 0).tune (2, -12).mix (0.3f, 0.25f, 0, 0.1f)
                  .st (LP, 1500, 0.4f, 0).s ("stBrute", 0.6f).lfo (1, SQR, 6.0f).m (srcLfo1, dVca, 0.3f).lfo (2, SINE, 0.1f).m (srcLfo2, dVco1PW, 0.3f); break;
        case 5:  b.ladderOnly().o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 12).vco3 (0, 19).mix (0.6f, 0.4f, 0.2f, 0).ld (LP12, 3000, 0, 0)
                  .reverb (0.75f, 1.0f, 0.3f); break;
        case 6:  b.route (ST, ST, ST, ST).outs (1, 0).o1 (1, 0, 0, 0).tune (1, -12).o2 (1, 0, 0, 0).tune (2, -11).mix (0.7f, 0.6f, 0, 0)
                  .st (LP, 1400, 0.5f, 0).s ("stBrute", 0.7f).s ("stDrive", 0.6f).lfo (2, RND, 0.2f).m (srcLfo2, dBrute, 0.3f); break;
        case 7:  b.ladderOnly().o1 (0, 0, 1, 0).metal (1, 0.3f).mix (0.6f, 0, 0, 0.2f).s ("noiseColor", 0.5f).ld (LP24, 3000, 0.1f, 0)
                  .lfo (2, SINE, 0.06f).m (srcLfo2, dVco1Metal, 0.3f); break;
        case 8:  b.route (BOTH, BOTH, BOTH, BOTH).outs (0.6f, 0.5f).o1 (1, 0, 0, 0).mix (0.5f, 0, 0, 0.2f).st (HP, 300, 0.3f, 0)
                  .ld (LP24, 1500, 0.2f, 0).lfo (2, SINE, 0.04f).m (srcLfo2, dCutoff, 0.4f); break;
        case 9:  b.ladderOnly().o1 (1, 0, 0, 0).ultra (1, 0.8f).mix (0.7f, 0, 0, 0).ld (LP24, 1200, 0.2f, 0).lfo (1, SINE, 0.2f)
                  .m (srcLfo1, dUltra, -0.5f).flanger (0.4f); break;
        case 10: b.steinerOnly().mix (0, 0, 0, 0.6f).s ("noiseColor", 0.8f).st (LP, 2000, 0.3f, 0).lfoSync (2, SH, D1_16)
                  .m (srcLfo2, dStCutoff, 0.4f).m (srcLfo2, dPan, 0.6f); break;
        case 11: b.ladderOnly().o1 (1, 0, 0, 0).o2 (1, 0, 0, 0).tune (2, 6).mix (0.5f, 0.5f, 0, 0).ld (LP24, 800, 0.3f, 0)
                  .lfo (1, SINE, 0.1f).pm (srcLfo1, dVco2Pitch, 1.0f).flanger (0.5f, 0.6f, 0.05f).mix (0.5f, 0.5f, 0, 0.25f)
                  .s ("noiseColor", -0.5f).ld (LP24, 500, 0.4f, 0); break;
        case 12: b.ladderOnly().o1 (1, 0, 0, 0).tune (1, -24).o2 (0, 1, 0, 0).tune (2, -17).mix (0.6f, 0.4f, 0, 0).ld (LP24, 180, 0.55f, 0)
                  .lfo (2, RND, 0.4f).m (srcLfo2, dLdCutoff, 0.25f); break;
        case 13: b.ladderOnly().o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).tune (2, 19).mix (0.6f, 0.4f, 0, 0).ld (LP12, 3000, 0, 0).chorus (0.5f)
                  .lfo (2, SINE, 0.12f).m (srcLfo2, dPan, 0.8f).reverb (0.65f, 1.0f); break;
        case 14: b.ladderOnly().o1 (1, 0, 0, 0).tune (1, -24).mix (0.7f, 0, 0, 0).ld (LP24, 120, 0.72f, 0).lfo (2, SINE, 0.03f)
                  .m (srcLfo2, dLdCutoff, 0.5f); break;
        default: b.ladderOnly().o1 (1, 0, 0, 0).ultra (1, 0.5f).o2 (0, 0, 1, 0).tune (2, 12).mix (0.5f, 0.4f, 0, 0).ld (LP24, 1500, 0.1f, 0)
                  .env (2, 5.0f, 1.0f, 1.0f, 10.0f).reverb (0.8f, 1.0f).delay (D1_2, 0.7f, 0.3f); break;
    }
}

void percBank (PB& b, int i)
{
    b.voice (modePoly).ladderOnly().s ("ldKeyTrack", 0).s ("env2Vel", 0.7f).o1 (0, 0, 1, 0).mix (0.9f, 0, 0, 0).ld (LP24, 4000, 0, 0)
     .env (2, 0.001f, 0.3f, 0.0f, 0.2f);

    switch (i)
    {
        case 0:  b.tune (1, -12).pm (srcEnv3, dVco1Pitch, 24.0f).env (3, 0.001f, 0.06f, 0.0f, 0.05f).env (2, 0.001f, 0.45f, 0.0f, 0.3f).s ("ldDrive", 0.5f); break;
        case 1:  b.steinerOnly().mix (0.3f, 0, 0, 0.9f).st (BP, 2500, 0.2f, 0).pm (srcEnv3, dVco1Pitch, 12.0f).env (3, 0.001f, 0.03f, 0.0f, 0.03f)
                  .env (2, 0.001f, 0.2f, 0.0f, 0.15f).reverb (0.15f); break;
        case 2:  b.steinerOnly().mix (0, 0, 0, 1).st (HP, 7000, 0.1f, 0).env (2, 0.001f, 0.06f, 0.0f, 0.05f); break;
        case 3:  b.metal (1, 0.3f).pm (srcEnv3, dVco1Pitch, 10.0f).env (3, 0.001f, 0.25f, 0.0f, 0.2f).env (2, 0.001f, 0.5f, 0.0f, 0.3f); break;
        case 4:  b.steinerOnly().mix (0, 0, 0, 1).st (BP, 1400, 0.4f, 0).lfo (1, SAWDN, 40.0f).s ("lfo1Retrig", 1).m (srcLfo1, dVca, 0.8f)
                  .env (2, 0.001f, 0.15f, 0.0f, 0.1f).reverb (0.2f); break;
        case 5:  b.steinerOnly().o1 (0, 1, 0, 0).o2 (0, 1, 0, 0).tune (2, 7, 40).mix (0.7f, 0.7f, 0, 0).st (BP, 1000, 0.5f, 0)
                  .env (2, 0.001f, 0.25f, 0.0f, 0.1f); break;
        case 6:  b.tune (1, 12).mix (0.6f, 0, 0, 0.5f).env (2, 0.001f, 0.05f, 0.0f, 0.04f).pm (srcEnv3, dVco1Pitch, 7.0f).env (3, 0.001f, 0.02f, 0.0f, 0.02f); break;
        case 7:  b.tune (1, -12).pm (srcEnv3, dVco1Pitch, 40.0f).env (3, 0.001f, 0.12f, 0.0f, 0.1f).env (2, 0.001f, 0.35f, 0.0f, 0.2f); break;
        case 8:  b.steinerOnly().mix (0, 0, 0, 1).s ("noiseColor", 0.6f).st (HP, 4000, 0.1f, 0).env (2, 0.02f, 0.12f, 0.0f, 0.1f); break;
        case 9:  b.ld (LBP, 600, 0.6f, 0).pm (srcEnv3, dVco1Pitch, 5.0f).env (3, 0.001f, 0.04f, 0.0f, 0.03f).env (2, 0.001f, 0.25f, 0.0f, 0.15f); break;
        case 10: b.metal (1, 0.8f).o2 (0, 0, 1, 0).metal (2, 0.7f).tune (2, 17).mix (0.6f, 0.6f, 0, 0).env (2, 0.001f, 1.2f, 0.0f, 1.0f).reverb (0.3f); break;
        case 11: b.pm (srcEnv3, dVco1Pitch, -30.0f).env (3, 0.001f, 0.3f, 0.0f, 0.2f).delay (D1_16, 0.4f, 0.25f); break;
        case 12: b.mix (0, 0, 0, 1).s ("noiseColor", 0.3f).ld (LP24, 9000, 0, 0).env (2, 0.001f, 2.0f, 0.0f, 2.0f).flanger (0.5f).reverb (0.3f); break;
        case 13: b.tune (1, 24).ld (LBP, 1500, 0.3f, 0).env (2, 0.001f, 0.08f, 0.0f, 0.05f).pm (srcEnv3, dVco1Pitch, 3.0f).env (3, 0.001f, 0.02f, 0.0f, 0.02f); break;
        case 14: b.tune (1, -24).s ("ldDrive", 0.8f).env (2, 0.001f, 1.5f, 0.0f, 1.0f).pm (srcEnv3, dVco1Pitch, 19.0f).env (3, 0.001f, 0.15f, 0.0f, 0.1f); break;
        default: b.seq (D1_16, 0.3f, { 0, 12, 7, 12, 0, 19, 7, 12 }, { 1, 0, 1, 1, 1, 0, 1, 1 }).voice (modeMono).metal (1, 0.3f)
                  .pm (srcEnv3, dVco1Pitch, 10.0f).env (3, 0.001f, 0.04f, 0.0f, 0.03f).delay (D1_8T, 0.35f, 0.2f); break;
    }
}

void fxBank (PB& b, int i)
{
    b.voice (modeMono).route (BOTH, BOTH, BOTH, BOTH).outs (0.6f, 0.6f).o1 (1, 0, 0, 0).o2 (0, 1, 0, 0).tune (2, 7)
     .mix (0.45f, 0.35f, 0, 0).st (LP, 2500, 0.2f, 0).ld (LP24, 2500, 0.2f, 0).env (2, 0.005f, 1.0f, 1.0f, 0.8f).reverb (0.3f)
     .m (srcWheel, dCutoff, 0.5f);

    switch (i)
    {
        case 0:  b.pm (srcEnv3, dVco1Pitch, 30.0f).pm (srcEnv3, dVco2Pitch, 30.0f).env (3, 0.001f, 0.25f, 0.0f, 0.1f).env (2, 0.001f, 0.3f, 0.0f, 0.2f)
                  .delay (D1_8, 0.5f, 0.25f); break;
        case 1:  b.lfo (1, TRI, 0.6f).pm (srcLfo1, dVco1Pitch, 7.0f).pm (srcLfo1, dVco2Pitch, 7.0f); break;
        case 2:  b.o1 (0, 1, 0, 0).lfo (1, SQR, 3.0f).pm (srcLfo1, dVco1Pitch, 5.0f).pm (srcLfo1, dVco2Pitch, 5.0f); break;
        case 3:  b.mix (0.4f, 0.3f, 0, 0.4f).st (LP, 200, 0.4f, 0).ld (LP24, 200, 0.4f, 0).m (srcEnv3, dCutoff, 0.8f)
                  .pm (srcEnv3, dVco1Pitch, 12.0f).env (3, 4.0f, 0.1f, 1.0f, 0.5f); break;
        case 4:  b.pm (srcEnv3, dVco1Pitch, -30.0f).pm (srcEnv3, dVco2Pitch, -30.0f).env (3, 3.0f, 0.1f, 1.0f, 0.5f).m (srcEnv3, dCutoff, -0.4f); break;
        case 5:  b.steinerOnly().o1 (0, 1, 0, 0).pw (1, 0.2f).mix (0.8f, 0, 0, 0).st (BP, 1000, 0.75f, 0).lfo (1, SH, 9.0f)
                  .m (srcLfo1, dStCutoff, 0.6f); break;
        case 6:  b.o1 (0, 0, 1, 0).lfo (1, SH, 8.0f).pm (srcLfo1, dVco1Pitch, 24.0f).pm (srcLfo1, dVco2Pitch, 24.0f).delay (D1_16, 0.5f, 0.3f); break;
        case 7:  b.o1 (0, 0, 1, 0).o2 (0, 0, 1, 0).lfo (1, SINE, 12.0f).pm (srcLfo1, dVco1Pitch, 3.0f).pm (srcLfo1, dVco2Pitch, 3.0f)
                  .pm (srcEnv3, dVco1Pitch, -24.0f).pm (srcEnv3, dVco2Pitch, -24.0f).env (3, 0.001f, 2.0f, 0.0f, 1.0f).flanger (0.5f); break;
        case 8:  b.mix (0.4f, 0.2f, 0, 0.5f).lfo (2, RND, 2.0f).pm (srcLfo2, dVco1Pitch, 20.0f).m (srcLfo2, dStCutoff, 0.4f).steinerOnly().st (BP, 1500, 0.4f, 0); break;
        case 9:  b.ladderOnly().lfo (1, RND, 12.0f).m (srcLfo1, dLdCutoff, 0.5f).ld (LP24, 1200, 0.8f, 0); break;
        case 10: b.ladderOnly().mix (0, 0, 0, 1).s ("noiseColor", -1).ld (LP24, 400, 0.3f, 0).env (2, 0.3f, 4.0f, 0.0f, 4.0f)
                  .m (srcEnv2, dLdCutoff, 0.5f).reverb (0.6f, 1.0f); break;
        case 11: b.lfoSync (1, SH, D1_32).pm (srcLfo1, dVco1Pitch, 12.0f).m (srcLfo1, dVca, 0.5f).m (srcLfo1, dPan, 0.7f); break;
        case 12: b.steinerOnly().mix (0, 0, 0, 1).st (BP, 1000, 0.7f, 0).lfo (2, SINE, 0.15f).m (srcLfo2, dStCutoff, 0.7f).m (srcLfo2, dPan, 0.6f); break;
        case 13: b.pm (srcEnv3, dVco1Pitch, 36.0f).lfo (1, SINE, 20.0f).pm (srcLfo1, dVco1Pitch, 2.0f).env (3, 0.001f, 1.0f, 0.0f, 0.5f).flanger (0.6f); break;
        case 14: b.o1 (0, 1, 0, 0).mix (0.7f, 0, 0, 0).lfo (1, SQR, 12.0f).pm (srcLfo1, dVco1Pitch, 12.0f); break;
        default: b.mix (0.35f, 0.3f, 0, 0).s ("stBrute", 1).s ("stReso", 0.8f).lfo (1, SH, 15.0f).m (srcLfo1, dStCutoff, 0.5f)
                  .pm (srcEnv3, dVco1Pitch, 20.0f).env (3, 0.001f, 0.4f, 0.0f, 0.2f); break;
    }
}

void templateBank (PB& b, int i)
{
    switch (i)
    {
        case 0:  break; // Init Patch: every parameter at its default
        case 1:  b.voice (modePoly); break;
        case 2:  b.o1 (1, 0, 0, 0.6f).ladderOnly().ld (LP24, 400, 0.2f, 0.4f).env (1, 0.001f, 0.3f, 0.1f, 0.1f); break;
        case 3:  b.voice (modePoly).env (2, 1.0f, 1.0f, 0.9f, 2.0f).chorus (0.4f).reverb (0.35f); break;
        case 4:  b.voice (modeLegato).o2 (1, 0, 0, 0).tune (2, 0, 8).mix (0.8f, 0.7f, 0, 0).route (ST, ST, ST, ST).vibrato (0.12f); break;
        case 5:  b.arp (0, 2, D1_16, 0.5f); break;
        case 6:  b.seq (D1_16, 0.5f, { 0, 3, 7, 12, 7, 3, 0, -12 }); break;
        case 7:  b.o1 (1, 0, 0, 0).ladderOnly().ld (LP24, 2500, 0.15f, 0.3f).env (1, 0.001f, 0.5f, 0.4f, 0.3f); break;
        case 8:  b.o1 (0, 1, 0, 0); break;
        case 9:  b.o1 (0, 0, 1, 0); break;
        case 10: b.voice (modePoly).o2 (1, 0, 0, 0).tune (2, 0, 10).mix (0.7f, 0.7f, 0, 0).route (LD, LD, LD, LD).outs (0, 1).ld (LP24, 3000, 0.1f, 0); break;
        case 11: b.o2 (1, 0, 0, 0).tune (2, 0, 8).vco3 (2, 0, -8).mix (0.6f, 0.6f, 0.6f, 0).route (ST, ST, ST, ST); break;
        case 12: b.steinerOnly().st (LP, 800, 0.5f, 0.4f).s ("stBrute", 0.4f).env (1, 0.001f, 0.5f, 0.2f, 0.3f)
                  .lfo (2, TRI, 0.3f).m (srcLfo2, dStCutoff, 0.2f); break;
        case 13: b.ladderOnly().ld (LP24, 600, 0.6f, 0.45f).env (1, 0.001f, 0.5f, 0.2f, 0.3f).lfo (2, TRI, 2.0f).m (srcLfo2, dLdCutoff, 0.3f); break;
        case 14: // Matrix Demo: one audible route per source; hover cells to read them
            b.o1 (1, 0.6f, 0.4f, 0).o2 (1, 0, 0, 0).tune (2, 7).mix (0.6f, 0.5f, 0, 0).route (BOTH, BOTH, BOTH, BOTH).outs (0.6f, 0.6f)
             .lfo (1, TRI, 0.5f).m (srcLfo1, dVco1PW, 0.35f).lfo (2, SINE, 0.2f).m (srcLfo2, dStCutoff, 0.3f).m (srcEnv1, dLdCutoff, 0.4f)
             .pm (srcEnv3, dVco2Pitch, 7.0f).env (3, 0.001f, 0.3f, 0.0f, 0.2f).m (srcVelocity, dVca, 0.25f).m (srcWheel, dVco1Metal, 0.6f)
             .pm (srcAftertouch, dVco1Pitch, 0.5f).m (srcKey, dPan, 0.5f).vco3 (0, 0).s ("vco3Lfo", 1).s ("vco3Rate", 3.0f)
             .m (srcVco3, dUltra, 0.4f).ultra (1, 0.2f);
            break;
        default: // Full Forge: every section working at once
            b.o1 (1, 0, 0, 0.5f).ultra (1, 0.3f).o2 (1, 0, 0, 0).tune (2, -12).vco3 (3, 7).mix (0.6f, 0.5f, 0.25f, 0).route (BOTH, BOTH, BOTH, BOTH)
             .s ("stBrute", 0.5f).s ("stDrive", 0.5f).s ("ldDrive", 0.5f).series().ld (LP24, 900, 0.2f, 0.35f).env (1, 0.001f, 0.6f, 0.3f, 0.3f)
             .lfo (2, SINE, 0.25f).m (srcLfo2, dCutoff, 0.2f).m (srcWheel, dBrute, 0.4f).delay (D1_8T, 0.3f, 0.15f);
            break;
    }
}

//==============================================================================
struct BankDef
{
    const char* name;
    BankFn fn;
    const char* presets[kPresetsPerBank];
};

const BankDef banks[kNumBanks] {
    { "Bass", bassBank, { "Brute Sub", "Steiner Growl", "Rubber Pluck", "Acid Matrix", "Square Thump", "Fold Bass", "Deep Sync", "Wobble Unit",
                          "Reese Drift", "Mono Punch", "Velvet Floor", "Metal Bottom", "FM Knock", "Ultra Sub", "Funk Squelch", "Detroit Bass" } },
    { "Lead", leadBank, { "Solo Forge", "Brute Lead", "Sync Scream", "Liquid Square", "Steiner Sing", "Fold Solo", "Mono Glide", "Hot Wire",
                          "Classic 70s", "Saw Cutter", "Whistle", "Ultra Lead", "Distorted Line", "Breathy Flute", "FM Bell Lead", "Ladder Lick" } },
    { "Pad", padBank, { "Warm Blanket", "Glass Choir", "Slow Bloom", "Matrix Clouds", "Analog Haze", "Silk Pad", "Ultra Field", "Dark Matter",
                        "Sunrise Sweep", "Hollow Air", "Metal Mist", "Breathing Room", "Nebula", "Polar Night", "Soft Focus", "Endless" } },
    { "Keys", keysBank, { "Poly Keys", "Analog Piano", "Tine Fold", "Drawbar Forge", "Clav Matrix", "Harpsi Pulse", "Soft Tines", "Bell Keys",
                          "Reed Grit", "Stab Chord", "Hollow Organ", "Vibes Tri", "Toy Piano", "Velvet Keys", "House Organ", "Detuned Upright" } },
    { "Brass", brassBank, { "Forge Brass", "Fanfare", "Horn Section", "Soft Horns", "Brass Stab", "Synth Trumpet", "Low Brass", "Poly Brass",
                            "Swell Horns", "Bright Brass", "Brass Mono", "80s Jump", "Filter Brass", "Tuba Sub", "Euro Brass", "Ensemble Brass" } },
    { "Strings", stringsBank, { "String Machine", "Ensemble", "Cello Mono", "Violin Lead", "Pizzicato", "Slow Strings", "Thin Strings", "Chamber",
                                "Orchestral Swell", "Vintage Ensemble", "Dark Strings", "Tremolo Strings", "High Strings", "Low Strings", "Glass Strings", "Movie Strings" } },
    { "Pluck", pluckBank, { "Pluck Forge", "Harp Pulse", "Koto Fold", "Muted Guitar", "Bell Pluck", "Glass Pluck", "Marimba Tri", "Kalimba",
                            "Pizz Square", "Nylon", "Steel Drum", "Pop Pluck", "Dub Chord", "Rain Drops", "Plucked Sync", "Wooden" } },
    { "Arp", arpBank, { "Arp Up 16", "Arp Down Trip", "Cascade", "Random Matrix", "Played Order", "Octave Jumper", "Berlin Arp", "Trance Gate",
                        "Bubbles", "Sync Arp", "Fold Arp", "Swing Arp", "Slow Arp Pad", "Fast 32nds", "Bouncing Ball", "Arp Machine" } },
    { "Sequence", seqBank, { "Seq Bassline", "Octave Walk", "Minor Run", "Techno Pulse", "Acid Line", "Berlin School", "Pentatonic", "Gated Fifths",
                             "Broken Chords", "Rhythm Box", "Hypnotic", "Dark Ride", "Funky Seq", "Five Step", "Phrygian", "Long Journey" } },
    { "Sync", syncBank, { "Hard Sync", "Sync Sweep", "Sync Bass", "Sync Brass", "Env Sync", "LFO Sync", "Sync Pad", "Sync Pluck",
                          "Tearing Metal", "Sync Wheel", "Sync Keys", "Scream Machine", "Velocity Sync", "Sync Arp", "Slow Sync", "Sync Choir" } },
    { "Ultrasaw", ultrasawBank, { "Ultra Wall", "Supersaw Lead", "Trance Pad", "Ultra Bass", "Hoover Forge", "Shimmer Saw", "Ultra Strings", "Big Room",
                                  "Ultra Pluck", "Swirling Saws", "Ultra Brass", "Euphoria", "Ultra Mono", "Wide Saws", "Saw Ocean", "Anthem" } },
    { "Metalizer", metalBank, { "Fold Lead", "Metal Bell", "Wavefold Bass", "Gong Fold", "Fold Sweep", "Clang", "Iron Pad", "Fold Pluck",
                                "Anvil", "Liquid Metal", "Chrome", "Rust", "Folded Choir", "Tin Can", "Metal Arp", "Fold Drone" } },
    { "Drone", droneBank, { "Deep Drone", "Wind Tunnel", "Ocean Floor", "Space Hum", "Machine Room", "Cathedral", "Dark Ritual", "Frozen Lake",
                            "Desert Heat", "Solar Wind", "Static Field", "Ghost Ship", "Underground", "Aurora", "Black Hole", "Infinite Hall" } },
    { "Percussion", percBank, { "Analog Kick", "Noise Snare", "Hi-Hat", "Fold Tom", "Clap Burst", "Cowbell", "Rim Shot", "Zap Kick",
                                "Shaker", "Conga", "Metal Hit", "Laser Tom", "Noise Crash", "Woodblock", "Sub Boom", "Perc Seq" } },
    { "FX", fxBank, { "Laser", "Siren", "Alarm", "Riser", "Downlifter", "Robot Talk", "Sci-Fi Bleeps", "UFO Landing",
                      "Radio Tuning", "Bubbling", "Thunder", "Glitch Steps", "Wind Sweep", "Teleport", "Computer", "Zap Storm" } },
    { "Templates", templateBank, { "Init Patch", "Init Poly", "Init Bass", "Init Pad", "Init Lead", "Init Arp", "Init Seq", "Basic Saw",
                                   "Basic Square", "Basic Triangle", "Two Osc Detune", "Three Osc Stack", "Steiner Demo", "Ladder Demo", "Matrix Demo", "Full Forge" } },
};

std::vector<FactoryPreset> buildPresets()
{
    std::vector<FactoryPreset> result;
    result.reserve (kNumFactoryPresets);
    for (int bank = 0; bank < kNumBanks; ++bank)
    {
        for (int i = 0; i < kPresetsPerBank; ++i)
        {
            PB b (banks[bank].presets[i]);
            banks[bank].fn (b, i);
            result.push_back (std::move (b.p));
        }
    }
    return result;
}
} // namespace

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = buildPresets();
    return presets;
}

const juce::StringArray& getBankNames()
{
    static const juce::StringArray names = []
    {
        juce::StringArray n;
        for (auto& b : banks)
            n.add (b.name);
        return n;
    }();
    return names;
}

juce::String getFactoryPresetDisplayName (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return {};
    const char bankLetter = (char) ('A' + index / kPresetsPerBank);
    return juce::String::charToString (bankLetter) + juce::String (index % kPresetsPerBank + 1).paddedLeft ('0', 2) + " "
           + presets[(size_t) index].name;
}
} // namespace mf
