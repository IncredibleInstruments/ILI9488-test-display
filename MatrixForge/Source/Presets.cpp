#include "Presets.h"
#include "Params.h"

namespace mf
{
namespace
{
// Small builder used to write presets tersely. Values are plain parameter units.
struct PB
{
    PB (const juce::String& name, juce::int64 seed) : random (seed) { p.name = name; }

    PB& s (const juce::String& id, float v)
    {
        p.values.emplace_back (id, v);
        return *this;
    }
    PB& m (int source, int dest, float amount) { return s (matrixId (source, dest), amount); }

    float rr (float lo, float hi) { return lo + random.nextFloat() * (hi - lo); }
    int ri (int lo, int hi) { return lo + random.nextInt (hi - lo + 1); }
    bool chance (float probability) { return random.nextFloat() < probability; }

    PB& env (int n, float a, float d, float sus, float r)
    {
        const juce::String e = "env" + juce::String (n);
        return s (e + "A", a).s (e + "D", d).s (e + "S", sus).s (e + "R", r);
    }
    PB& reverb (float mix, float size = 0.75f) { return s ("rvOn", 1).s ("rvMix", mix).s ("rvSize", size); }
    PB& delay (int div, float fb, float mix) { return s ("dlOn", 1).s ("dlSync", 1).s ("dlDiv", (float) div).s ("dlFeedback", fb).s ("dlMix", mix); }
    PB& chorus (float mix, float depth = 0.5f, float rate = 0.6f) { return s ("chType", 1).s ("chMix", mix).s ("chDepth", depth).s ("chRate", rate); }
    PB& flanger (float mix, float depth = 0.5f, float rate = 0.2f) { return s ("chType", 2).s ("chMix", mix).s ("chDepth", depth).s ("chRate", rate); }
    PB& poly() { return s ("voiceMode", modePoly); }
    PB& vibrato (float amount = 0.006f) { return m (srcLfo1, dVco1Pitch, amount).m (srcLfo1, dVco2Pitch, amount); }
    // Route the selected sources only to the ladder filter
    PB& ladderOnly() { return s ("routeVco1", 2).s ("routeVco2", 2).s ("routeVco3", 2).s ("routeNoise", 2).s ("stOut", 0).s ("ldOut", 1); }
    PB& steinerOnly() { return s ("routeVco1", 0).s ("routeVco2", 0).s ("routeVco3", 0).s ("routeNoise", 0).s ("stOut", 1).s ("ldOut", 0); }
    PB& seq (std::initializer_list<int> steps, std::initializer_list<int> gates = {})
    {
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
    juce::Random random;
};

using BankFn = void (*) (PB&, int);

//==============================================================================
void bassBank (PB& b, int i)
{
    b.s ("voiceMode", modeMono).s ("vco1Saw", 1).s ("vco1Sub", b.rr (0.3f, 0.7f))
     .s ("vco2Saw", 0).s ("vco2Square", 1).s ("vco2Coarse", -12).s ("vco2Fine", b.rr (-6, 6))
     .s ("mixVco1", 0.8f).s ("mixVco2", b.rr (0.3f, 0.6f)).ladderOnly()
     .s ("ldCutoff", b.rr (150, 500)).s ("ldReso", b.rr (0.1f, 0.45f)).s ("ldEnvAmt", b.rr (0.3f, 0.55f))
     .s ("ldDrive", b.rr (0.2f, 0.6f)).s ("ldKeyTrack", 0.5f)
     .env (1, 0.001f, b.rr (0.15f, 0.45f), b.rr (0.0f, 0.3f), 0.15f)
     .env (2, 0.002f, 0.5f, b.rr (0.6f, 1.0f), b.rr (0.06f, 0.2f));

    switch (i)
    {
        case 0:  b.s ("vco1Sub", 1).s ("mixVco2", 0.15f).s ("ldCutoff", 260); break;
        case 1:  b.steinerOnly().s ("stCutoff", 380).s ("stReso", 0.5f).s ("stBrute", 0.65f).s ("stEnvAmt", 0.5f).s ("stDrive", 0.5f); break;
        case 2:  break;
        case 3:  b.env (1, 0.001f, 0.14f, 0.0f, 0.1f).s ("ldReso", 0.55f).s ("ldEnvAmt", 0.7f).s ("glide", 0.03f); break;
        case 4:  b.s ("voiceMode", modeLegato).s ("glide", 0.08f).s ("mixVco2", 0).s ("ldReso", 0.82f).s ("ldCutoff", 230)
                  .s ("ldEnvAmt", 0.62f).env (1, 0.001f, 0.25f, 0.0f, 0.1f).m (srcVelocity, dLdCutoff, 0.2f).delay (7, 0.3f, 0.15f); break;
        case 5:  b.s ("vco1Saw", 0).s ("vco1Square", 1).env (2, 0.001f, 0.3f, 0.3f, 0.1f); break;
        case 6:  b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.4f).m (srcEnv3, dVco1Metal, 0.4f).env (3, 0.001f, 0.4f, 0.0f, 0.2f); break;
        case 7:  b.s ("vco2Sync", 1).s ("vco2Coarse", 7).s ("vco2Saw", 1).s ("vco2Square", 0).s ("mixVco1", 0.3f).s ("mixVco2", 0.8f)
                  .m (srcEnv3, dVco2Pitch, 0.3f).env (3, 0.001f, 0.3f, 0.0f, 0.2f); break;
        case 8:  b.s ("lfo1Sync", 1).s ("lfo1Div", 7).s ("lfo1Wave", 0).m (srcLfo1, dLdCutoff, 0.45f).s ("ldReso", 0.5f).s ("ldEnvAmt", 0.1f); break;
        case 9:  b.s ("vco1Ultra", 0.4f).s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Coarse", 0).s ("vco2Fine", 14).chorus (0.25f)
                  .m (srcLfo2, dLdCutoff, 0.15f).s ("lfo2Rate", 0.2f); break;
        case 10: b.env (1, 0.001f, 0.08f, 0.0f, 0.1f).s ("ldEnvAmt", 0.75f).env (2, 0.001f, 0.2f, 0.7f, 0.08f); break;
        case 11: b.s ("vco1Tri", 1).s ("vco1Saw", 0.3f).s ("ldCutoff", 180).s ("ldReso", 0.2f).env (2, 0.01f, 0.6f, 0.9f, 0.3f); break;
        case 12: b.s ("vco2Tri", 1).s ("vco2Metal", 0.6f).s ("vco2Square", 0).m (srcLfo1, dVco2Metal, 0.25f).s ("lfo1Rate", 0.7f); break;
        case 13: b.s ("vco1Sub", 1).s ("vco1Saw", 0.4f).s ("vco1Ultra", 0.6f).s ("ldCutoff", 220); break;
        case 14: b.steinerOnly().s ("stMode", 1).s ("stCutoff", 500).s ("stReso", 0.6f).s ("stEnvAmt", 0.6f).env (1, 0.001f, 0.2f, 0.1f, 0.1f); break;
        default: b.s ("vco1Saw", 0).s ("vco1Square", 1).s ("vco1PW", 0.3f).m (srcLfo1, dVco1PW, 0.2f).s ("lfo1Rate", 0.4f).s ("ldCutoff", 400); break;
    }
}

void leadBank (PB& b, int i)
{
    b.s ("voiceMode", modeLegato).s ("glide", b.rr (0.0f, 0.06f)).s ("vco1Saw", 1).s ("vco2Saw", 1).s ("vco2Square", 0)
     .s ("vco2Fine", b.rr (4, 12)).s ("mixVco1", 0.7f).s ("mixVco2", 0.6f).s ("routeVco1", 1).s ("routeVco2", 1)
     .s ("stCutoff", b.rr (900, 2500)).s ("ldCutoff", b.rr (900, 2500)).s ("stEnvAmt", 0.3f).s ("ldEnvAmt", 0.3f)
     .s ("stOut", 0.6f).s ("ldOut", 0.6f).env (1, 0.005f, 0.6f, 0.4f, 0.3f).env (2, 0.003f, 0.4f, 0.9f, 0.25f)
     .vibrato().s ("lfo1Rate", 5.5f).m (srcAftertouch, dLdCutoff, 0.25f).m (srcWheel, dStCutoff, 0.3f)
     .delay (b.chance (0.5f) ? 6 : 7, 0.35f, b.rr (0.12f, 0.25f));

    switch (i)
    {
        case 0:  break;
        case 1:  b.s ("stBrute", 0.4f).s ("stDrive", 0.6f).s ("vco1Sub", 0.4f); break;
        case 2:  b.s ("vco2Sync", 1).s ("vco2Coarse", 12).m (srcEnv3, dVco2Pitch, 0.35f).env (3, 0.001f, 0.8f, 0.2f, 0.3f).s ("mixVco1", 0.2f); break;
        case 3:  b.s ("vco1Saw", 0).s ("vco1Square", 1).s ("vco2Saw", 0).s ("vco2Square", 1).s ("vco2PW", 0.3f).m (srcLfo2, dVco2PW, 0.25f).s ("lfo2Rate", 0.8f); break;
        case 4:  b.steinerOnly().s ("stMode", 1).s ("stReso", 0.55f).s ("stCutoff", 1400).m (srcLfo2, dStCutoff, 0.15f).s ("lfo2Rate", 0.3f); break;
        case 5:  b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.5f).m (srcEnv1, dVco1Metal, 0.3f).m (srcWheel, dVco1Metal, 0.5f); break;
        case 6:  b.s ("glide", 0.18f).s ("voiceMode", modeMono); break;
        case 7:  b.s ("stDrive", 0.9f).s ("ldDrive", 0.8f).s ("stBrute", 0.7f).s ("ldReso", 0.4f); break;
        case 8:  b.s ("vco2Square", 1).s ("vco2Saw", 0).s ("vco2Coarse", -12).ladderOnly().s ("ldReso", 0.35f).s ("ldEnvAmt", 0.45f); break;
        case 9:  b.env (1, 0.001f, 0.25f, 0.2f, 0.2f).s ("stEnvAmt", 0.6f).s ("stCutoff", 500).s ("stReso", 0.3f); break;
        case 10: b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco2Saw", 0).s ("vco2Tri", 1).s ("vco2Coarse", 12).s ("mixVco2", 0.3f)
                  .s ("stCutoff", 4000).vibrato (0.008f); break;
        case 11: b.s ("vco1Ultra", 0.7f).s ("vco2Ultra", 0.5f).chorus (0.3f); break;
        case 12: b.s ("stBrute", 1.0f).s ("stDrive", 1.0f).s ("stReso", 0.4f).s ("ldDrive", 1.0f); break;
        case 13: b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco2Saw", 0).s ("mixVco2", 0).s ("mixNoise", 0.12f).s ("noiseColor", -0.4f)
                  .env (2, 0.06f, 0.4f, 0.85f, 0.3f).reverb (0.25f); break;
        case 14: b.s ("glide", 0.35f).s ("voiceMode", modeMono).s ("vco2Coarse", 7); break;
        default: b.ladderOnly().s ("ldReso", 0.6f).s ("ldEnvAmt", 0.5f).env (1, 0.001f, 0.35f, 0.25f, 0.2f); break;
    }
}

void padBank (PB& b, int i)
{
    b.poly().s ("drift", 0.3f).s ("vco1Saw", 1).s ("vco1Ultra", b.rr (0.1f, 0.5f)).s ("vco2Saw", b.chance (0.5f) ? 1.0f : 0.0f)
     .s ("vco2Square", 0.6f).s ("vco2Fine", b.rr (6, 15)).s ("mixVco1", 0.6f).s ("mixVco2", 0.5f)
     .s ("ldCutoff", b.rr (600, 2200)).s ("stCutoff", b.rr (800, 3000)).s ("ldEnvAmt", 0.2f).s ("stEnvAmt", 0.15f)
     .env (1, 1.0f, 2.0f, 0.6f, 2.0f).env (2, b.rr (0.5f, 2.0f), 1.0f, 0.85f, b.rr (1.5f, 3.5f))
     .s ("env2Vel", 0.25f).m (srcLfo2, dLdCutoff, 0.1f).s ("lfo2Rate", b.rr (0.08f, 0.3f))
     .chorus (0.45f).reverb (0.35f, 0.85f);

    switch (i)
    {
        case 0:  b.s ("ldCutoff", 700).s ("vco1Sub", 0.3f); break;
        case 1:  b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.15f).s ("stMode", 1).s ("stReso", 0.5f).s ("mixNoise", 0.1f); break;
        case 2:  b.env (2, 3.0f, 2.0f, 0.9f, 4.0f).env (1, 3.5f, 3.0f, 0.5f, 3.0f).s ("ldEnvAmt", 0.4f); break;
        case 3:  b.m (srcLfo1, dVco1PW, 0.2f).s ("vco1Square", 0.7f).m (srcLfo2, dPan, 0.6f).s ("lfo1Rate", 0.3f); break;
        case 4:  b.s ("drift", 0.8f).s ("mixNoise", 0.08f).s ("noiseColor", -0.6f); break;
        case 5:  b.s ("vco1Ultra", 0.7f).s ("vco2Saw", 1).s ("stMode", 2).s ("stCutoff", 300).s ("routeVco1", 1); break;
        case 6:  b.s ("vco1Ultra", 1.0f).s ("vco2Ultra", 0.8f).s ("vco2Saw", 1); break;
        case 7:  b.s ("ldCutoff", 300).s ("ldReso", 0.4f).s ("vco1Coarse", -12).s ("vco1Sub", 0.5f); break;
        case 8:  b.env (1, 4.0f, 2.0f, 0.8f, 3.0f).s ("ldEnvAmt", 0.6f).s ("ldCutoff", 250).s ("ldReso", 0.35f); break;
        case 9:  b.steinerOnly().s ("stMode", 1).s ("stReso", 0.3f).s ("mixNoise", 0.2f).s ("noiseColor", 0.3f); break;
        case 10: b.s ("vco2Tri", 1).s ("vco2Metal", 0.5f).m (srcLfo2, dVco2Metal, 0.3f); break;
        case 11: b.m (srcLfo2, dVca, 0.2f).s ("lfo2Rate", 0.15f).s ("lfo2Wave", 0); break;
        case 12: b.s ("vco3Coarse", 19).s ("mixVco3", 0.25f).s ("vco3Wave", 0).reverb (0.5f, 0.95f); break;
        case 13: b.s ("ldCutoff", 450).s ("vco1Tri", 0.5f).s ("lfo2Wave", 6).m (srcLfo2, dLdCutoff, 0.2f); break;
        case 14: b.s ("ldCutoff", 900).s ("stCutoff", 900).s ("chMix", 0.6f); break;
        default: b.env (2, 2.5f, 2.0f, 1.0f, 8.0f).reverb (0.55f, 0.98f); break;
    }
}

void keysBank (PB& b, int i)
{
    b.poly().s ("vco1Saw", 0.5f).s ("vco1Square", 0.6f).s ("vco2Square", 0).s ("vco2Tri", 0.8f).s ("vco2Coarse", 12)
     .s ("mixVco1", 0.7f).s ("mixVco2", 0.4f).s ("ldCutoff", b.rr (900, 2500)).s ("ldEnvAmt", 0.35f).s ("stCutoff", 3000)
     .env (1, 0.002f, b.rr (0.4f, 1.0f), 0.2f, 0.4f).env (2, 0.002f, b.rr (0.8f, 2.0f), b.rr (0.2f, 0.5f), 0.45f)
     .s ("env2Vel", 0.6f).s ("env1Vel", 0.5f).reverb (0.2f);

    switch (i)
    {
        case 0:  break;
        case 1:  b.s ("vco1Saw", 0.2f).s ("vco1Tri", 0.8f).s ("env2S", 0.0f).s ("env2D", 3.0f); break;
        case 2:  b.s ("vco1Saw", 0).s ("vco1Square", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.2f).m (srcVelocity, dVco1Metal, 0.25f).chorus (0.3f); break;
        case 3:  b.s ("vco1Saw", 0).s ("vco1Sub", 0.8f).s ("vco1Square", 0.6f).s ("vco2Tri", 1).env (2, 0.004f, 0.1f, 1.0f, 0.06f)
                  .s ("ldEnvAmt", 0).s ("ldCutoff", 5000).s ("env2Vel", 0).m (srcLfo2, dVco1Pitch, 0.003f).s ("lfo2Rate", 6.0f); break;
        case 4:  b.s ("vco1Saw", 0).s ("vco1PW", 0.15f).steinerOnly().s ("stMode", 1).s ("stCutoff", 1200).s ("stEnvAmt", 0.4f)
                  .env (2, 0.001f, 0.4f, 0.1f, 0.15f); break;
        case 5:  b.s ("vco1PW", 0.25f).s ("vco2Square", 0.6f).s ("vco2Tri", 0).s ("ldCutoff", 4000).env (2, 0.001f, 1.2f, 0.0f, 0.4f); break;
        case 6:  b.s ("vco1Saw", 0).s ("vco1Square", 0).s ("vco1Tri", 1).s ("ldCutoff", 1200).m (srcLfo1, dPan, 0.5f).s ("lfo1Rate", 4.0f)
                  .m (srcLfo1, dVca, 0.15f); break;
        case 7:  b.s ("vco2Coarse", 19).s ("vco2Tri", 1).s ("vco2Metal", 0.3f).env (2, 0.001f, 2.5f, 0.0f, 2.0f).reverb (0.35f); break;
        case 8:  b.s ("vco1Saw", 0.8f).s ("ldDrive", 0.6f).s ("ldReso", 0.25f).m (srcLfo1, dVca, 0.2f).s ("lfo1Rate", 5.5f); break;
        case 9:  b.s ("vco1Saw", 1).s ("vco2Saw", 1).s ("vco2Coarse", 7).env (2, 0.001f, 0.25f, 0.0f, 0.2f).delay (6, 0.4f, 0.2f); break;
        case 10: b.s ("vco1Saw", 0).s ("vco1Square", 1).s ("vco2Square", 0.7f).s ("vco2Tri", 0).env (2, 0.01f, 0.1f, 1.0f, 0.1f)
                  .s ("ldCutoff", 1800).s ("ldEnvAmt", 0).chorus (0.5f, 0.6f, 5.0f); break;
        case 11: b.s ("vco1Saw", 0).s ("vco1Square", 0).s ("vco1Tri", 1).s ("vco2Tri", 0).m (srcLfo1, dVca, 0.25f).s ("lfo1Rate", 5.0f)
                  .env (2, 0.001f, 1.8f, 0.0f, 1.2f); break;
        case 12: b.s ("vco1Coarse", 12).s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.45f).env (2, 0.001f, 0.6f, 0.0f, 0.3f); break;
        case 13: b.s ("ldCutoff", 700).s ("ldEnvAmt", 0.25f).chorus (0.35f); break;
        case 14: b.s ("vco1Saw", 0).s ("vco1Square", 1).s ("vco1Sub", 1).s ("vco2Tri", 1).env (2, 0.002f, 0.2f, 0.6f, 0.1f).s ("ldEnvAmt", 0.2f); break;
        default: b.s ("vco2Coarse", 0).s ("vco2Fine", 18).s ("vco2Tri", 0).s ("vco2Saw", 0.7f).s ("drift", 0.6f); break;
    }
}

void brassBank (PB& b, int i)
{
    b.poly().s ("vco1Saw", 1).s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Fine", b.rr (5, 10)).s ("mixVco1", 0.7f).s ("mixVco2", 0.7f)
     .ladderOnly().s ("ldCutoff", b.rr (500, 900)).s ("ldReso", 0.15f).s ("ldEnvAmt", b.rr (0.4f, 0.55f))
     .env (1, b.rr (0.04f, 0.12f), 0.5f, 0.55f, 0.3f).env (2, 0.03f, 0.5f, 0.85f, 0.3f).s ("env1Vel", 0.5f)
     .vibrato (0.004f).s ("lfo1Rate", 5.0f).reverb (0.2f);

    switch (i)
    {
        case 0:  b.s ("ldDrive", 0.5f); break;
        case 1:  b.env (1, 0.05f, 0.3f, 0.7f, 0.3f).s ("ldCutoff", 900); break;
        case 2:  b.s ("vco2Coarse", -12).s ("vco1Ultra", 0.3f).chorus (0.3f); break;
        case 3:  b.env (1, 0.25f, 0.8f, 0.6f, 0.6f).env (2, 0.15f, 0.5f, 0.9f, 0.6f).s ("ldCutoff", 450); break;
        case 4:  b.env (1, 0.005f, 0.2f, 0.2f, 0.15f).env (2, 0.005f, 0.35f, 0.0f, 0.2f).s ("ldEnvAmt", 0.6f); break;
        case 5:  b.s ("voiceMode", modeLegato).s ("vco2Square", 1).s ("vco2Saw", 0).s ("vco2PW", 0.35f).s ("glide", 0.03f); break;
        case 6:  b.s ("vco1Coarse", -12).s ("vco2Coarse", -12).s ("vco1Sub", 0.5f).s ("ldCutoff", 350); break;
        case 7:  b.s ("ldCutoff", 700).chorus (0.4f); break;
        case 8:  b.env (1, 1.2f, 1.0f, 0.7f, 0.8f).env (2, 0.8f, 0.5f, 1.0f, 0.8f).s ("ldEnvAmt", 0.5f); break;
        case 9:  b.s ("ldCutoff", 1500).s ("ldDrive", 0.6f).s ("ldReso", 0.3f); break;
        case 10: b.s ("voiceMode", modeMono).s ("vco1Sub", 0.4f); break;
        case 11: b.s ("vco2Coarse", 0).s ("vco2Fine", 14).s ("vco1Ultra", 0.4f).s ("ldCutoff", 1400).env (1, 0.01f, 0.4f, 0.6f, 0.3f).chorus (0.4f); break;
        case 12: b.m (srcLfo2, dLdCutoff, 0.2f).s ("lfo2Rate", 0.25f); break;
        case 13: b.s ("voiceMode", modeMono).s ("vco1Coarse", -24).s ("vco2Coarse", -12).s ("vco1Sub", 1).s ("ldCutoff", 250); break;
        case 14: b.s ("vco1Square", 0.5f).s ("vco1PW", 0.4f).s ("ldReso", 0.3f); break;
        default: b.s ("vco1Ultra", 0.5f).s ("vco2Ultra", 0.5f).chorus (0.5f).reverb (0.3f); break;
    }
}

void stringsBank (PB& b, int i)
{
    b.poly().s ("vco1Saw", 1).s ("vco1Ultra", b.rr (0.2f, 0.5f)).s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Coarse", 12)
     .s ("vco2Fine", b.rr (4, 9)).s ("mixVco1", 0.7f).s ("mixVco2", 0.35f).ladderOnly().s ("ldMode", 1)
     .s ("ldCutoff", b.rr (1800, 4000)).s ("ldEnvAmt", 0.1f).env (2, b.rr (0.3f, 0.8f), 1.0f, 0.9f, b.rr (0.8f, 1.6f))
     .vibrato (0.003f).s ("lfo1Rate", 5.0f).chorus (0.6f, 0.7f, 0.5f).reverb (0.3f, 0.85f);

    switch (i)
    {
        case 0:  break;
        case 1:  b.s ("vco1Ultra", 0.6f); break;
        case 2:  b.s ("voiceMode", modeLegato).s ("vco1Coarse", -12).s ("vco2Coarse", -12).s ("ldCutoff", 1500).env (2, 0.15f, 1.0f, 0.9f, 0.5f); break;
        case 3:  b.s ("voiceMode", modeLegato).s ("ldCutoff", 3500).vibrato (0.008f).env (2, 0.08f, 1.0f, 0.9f, 0.4f); break;
        case 4:  b.env (2, 0.001f, 0.3f, 0.0f, 0.25f).env (1, 0.001f, 0.2f, 0.0f, 0.2f).s ("ldEnvAmt", 0.4f).s ("ldCutoff", 900); break;
        case 5:  b.env (2, 2.0f, 1.0f, 1.0f, 2.5f); break;
        case 6:  b.s ("ldCutoff", 7000).s ("ldMode", 3).s ("ldCutoff", 200).s ("stOut", 0); break;
        case 7:  b.s ("vco2Coarse", 0).s ("mixVco2", 0.6f).s ("ldCutoff", 2000); break;
        case 8:  b.env (1, 3.0f, 2.0f, 1.0f, 2.0f).s ("ldEnvAmt", 0.5f).s ("ldCutoff", 400).env (2, 1.5f, 1.0f, 1.0f, 2.0f); break;
        case 9:  b.s ("vco2Saw", 0).s ("vco2Square", 1).s ("vco1Ultra", 0.8f).s ("chMix", 0.8f); break;
        case 10: b.s ("ldCutoff", 900).s ("vco1Coarse", -12); break;
        case 11: b.m (srcLfo2, dVca, 0.35f).s ("lfo2Rate", 7.0f).s ("lfo2Wave", 1); break;
        case 12: b.s ("vco1Coarse", 12).s ("vco2Coarse", 24).s ("ldCutoff", 5000); break;
        case 13: b.s ("vco1Coarse", -12).s ("vco2Coarse", 0).s ("vco1Sub", 0.4f).s ("ldCutoff", 1200); break;
        case 14: b.s ("vco2Tri", 1).s ("vco2Saw", 0).s ("vco2Metal", 0.3f).reverb (0.45f, 0.9f); break;
        default: b.env (2, 1.0f, 1.0f, 1.0f, 3.0f).s ("vco1Ultra", 0.7f).reverb (0.5f, 0.95f).delay (4, 0.3f, 0.12f); break;
    }
}

void pluckBank (PB& b, int i)
{
    b.poly().s ("vco1Saw", b.rr (0.4f, 1.0f)).s ("vco1Square", b.rr (0.0f, 0.6f)).s ("vco2Square", 0).s ("vco2Tri", 0.6f)
     .s ("vco2Coarse", 12).s ("mixVco1", 0.7f).s ("mixVco2", 0.4f).ladderOnly().s ("ldCutoff", b.rr (300, 800))
     .s ("ldReso", b.rr (0.1f, 0.4f)).s ("ldEnvAmt", b.rr (0.45f, 0.7f)).env (1, 0.001f, b.rr (0.1f, 0.3f), 0.0f, 0.2f)
     .env (2, 0.001f, b.rr (0.25f, 0.7f), 0.0f, 0.3f).s ("env1Vel", 0.6f).delay (6, 0.35f, 0.18f).reverb (0.2f);

    switch (i)
    {
        case 0:  break;
        case 1:  b.s ("vco1Saw", 0).s ("vco1Square", 1).s ("vco1PW", 0.2f).env (2, 0.001f, 1.2f, 0.0f, 1.0f); break;
        case 2:  b.s ("vco1Saw", 0).s ("vco1Square", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.35f).m (srcEnv1, dVco1Metal, 0.4f); break;
        case 3:  b.s ("ldCutoff", 250).s ("ldEnvAmt", 0.4f).env (2, 0.001f, 0.15f, 0.0f, 0.1f).s ("dlOn", 0); break;
        case 4:  b.s ("vco2Coarse", 19).s ("vco2Tri", 1).s ("vco2Metal", 0.4f).env (2, 0.001f, 1.5f, 0.0f, 1.5f); break;
        case 5:  b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco2Coarse", 24).s ("ldCutoff", 1500).reverb (0.4f); break;
        case 6:  b.s ("vco1Saw", 0).s ("vco1Square", 0).s ("vco1Tri", 1).s ("vco2Tri", 0).env (2, 0.001f, 0.35f, 0.0f, 0.2f).s ("dlOn", 0); break;
        case 7:  b.s ("vco1Saw", 0).s ("vco1Square", 0).s ("vco1Tri", 1).s ("vco2Coarse", 24).s ("vco2Tri", 0.3f).env (2, 0.001f, 0.6f, 0.0f, 0.4f); break;
        case 8:  b.s ("vco1Saw", 0).s ("vco1Square", 1).steinerOnly().s ("stMode", 1).s ("stCutoff", 900).s ("stEnvAmt", 0.5f); break;
        case 9:  b.s ("ldMode", 1).s ("ldCutoff", 600).s ("vco1Square", 0.4f).chorus (0.25f); break;
        case 10: b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.2f).s ("vco2Coarse", 7).s ("vco2Tri", 1).env (2, 0.001f, 0.9f, 0.0f, 0.5f); break;
        case 11: b.s ("ldCutoff", 450).s ("vco2Saw", 1).s ("vco2Coarse", 0).s ("vco2Fine", 12); break;
        case 12: b.s ("vco1Saw", 1).s ("vco2Coarse", 7).s ("vco2Saw", 1).delay (5, 0.55f, 0.35f).s ("dlPingPong", 1); break;
        case 13: b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco2Coarse", 31).env (2, 0.001f, 0.2f, 0.0f, 0.2f).delay (8, 0.6f, 0.4f).reverb (0.45f); break;
        case 14: b.s ("vco2Sync", 1).s ("vco2Saw", 1).s ("vco2Coarse", 9).m (srcEnv1, dVco2Pitch, 0.25f); break;
        default: b.s ("vco1Saw", 0).s ("vco1Square", 0.5f).s ("vco1Tri", 1).steinerOnly().s ("stMode", 1).s ("stCutoff", 700).s ("stReso", 0.4f)
                  .env (2, 0.001f, 0.12f, 0.0f, 0.1f).s ("dlOn", 0); break;
    }
}

void arpBank (PB& b, int i)
{
    b.s ("asMode", asArp).s ("asDiv", 8).s ("asGate", b.rr (0.3f, 0.6f)).s ("arpPattern", (float) (i % 5)).s ("arpOct", (float) (1 + i % 3))
     .s ("voiceMode", modeMono).s ("vco1Saw", 1).s ("vco1Square", 0.5f).s ("vco2Square", 1).s ("vco2Coarse", 12)
     .s ("mixVco1", 0.7f).s ("mixVco2", 0.3f).ladderOnly().s ("ldCutoff", b.rr (400, 900)).s ("ldReso", b.rr (0.2f, 0.5f))
     .s ("ldEnvAmt", 0.5f).env (1, 0.001f, 0.18f, 0.1f, 0.15f).env (2, 0.001f, 0.3f, 0.4f, 0.15f)
     .delay (7, 0.4f, 0.22f).reverb (0.15f).m (srcWheel, dLdCutoff, 0.4f);

    switch (i)
    {
        case 0:  b.s ("arpPattern", 0).s ("arpOct", 2); break;
        case 1:  b.s ("arpPattern", 1).s ("asDiv", 7); break;
        case 2:  b.s ("arpPattern", 2).s ("arpOct", 3).s ("asDiv", 8); break;
        case 3:  b.s ("arpPattern", 3).s ("arpOct", 2).m (srcLfo2, dLdCutoff, 0.2f).s ("lfo2Wave", 5).s ("lfo2Sync", 1).s ("lfo2Div", 8); break;
        case 4:  b.s ("arpPattern", 4).s ("voiceMode", modePoly); break;
        case 5:  b.s ("arpPattern", 0).s ("arpOct", 4).s ("asDiv", 8); break;
        case 6:  b.s ("arpPattern", 2).s ("arpOct", 2).s ("ldReso", 0.55f).m (srcLfo1, dLdCutoff, 0.25f).s ("lfo1Rate", 0.1f); break;
        case 7:  b.s ("asGate", 0.25f).s ("vco1Ultra", 0.5f).s ("vco2Saw", 1).s ("arpOct", 1).reverb (0.3f); break;
        case 8:  b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", 0.3f).s ("asDiv", 10).s ("arpPattern", 3); break;
        case 9:  b.s ("vco2Sync", 1).s ("vco2Saw", 1).m (srcLfo1, dVco2Pitch, 0.2f).s ("lfo1Rate", 0.15f).s ("mixVco2", 0.7f); break;
        case 10: b.s ("vco1Saw", 0).s ("vco1Tri", 1).m (srcEnv1, dVco1Metal, 0.6f).s ("vco1Tri", 1); break;
        case 11: b.s ("seqSwing", 0.18f).s ("arpPattern", 0); break;
        case 12: b.s ("asDiv", 6).s ("voiceMode", modePoly).env (2, 0.05f, 1.0f, 0.6f, 1.2f).chorus (0.4f).reverb (0.35f); break;
        case 13: b.s ("asDiv", 10).s ("asGate", 0.5f).s ("arpOct", 2); break;
        case 14: b.s ("arpPattern", 2).s ("arpOct", 4).s ("asDiv", 9); break;
        default: b.s ("vco1Saw", 1).s ("vco2Square", 0.5f).s ("ldDrive", 0.6f).s ("arpOct", 3); break;
    }
}

void seqBank (PB& b, int i)
{
    b.s ("asMode", asSeq).s ("asDiv", 8).s ("asGate", b.rr (0.35f, 0.65f)).s ("voiceMode", modeMono)
     .s ("vco1Saw", 1).s ("vco1Sub", 0.5f).s ("vco2Square", 1).s ("vco2Coarse", -12).s ("mixVco1", 0.8f).s ("mixVco2", 0.35f)
     .ladderOnly().s ("ldCutoff", b.rr (250, 700)).s ("ldReso", b.rr (0.25f, 0.55f)).s ("ldEnvAmt", 0.5f)
     .env (1, 0.001f, 0.2f, 0.05f, 0.1f).env (2, 0.001f, 0.25f, 0.5f, 0.1f).delay (7, 0.3f, 0.15f)
     .m (srcWheel, dLdCutoff, 0.4f).m (srcAftertouch, dLdReso, 0.3f);

    switch (i)
    {
        case 0:  b.seq ({ 0, 0, 12, 0, 0, 10, 0, 7, 0, 0, 12, 0, 3, 0, 5, 7 }); break;
        case 1:  b.seq ({ 0, 12, 0, 12, 0, 12, 0, 12 }); break;
        case 2:  b.seq ({ 0, 3, 7, 10, 12, 10, 7, 3 }); break;
        case 3:  b.seq ({ 0, 0, 0, 12, 0, 0, 12, 0 }, { 1, 0, 1, 1, 1, 0, 1, 1 }).s ("ldReso", 0.6f); break;
        case 4:  b.seq ({ 0, 12, 0, 0, 15, 0, 12, 10, 0, 0, 7, 12, 0, 3, 0, 0 }).s ("voiceMode", modeLegato).s ("glide", 0.06f)
                  .s ("vco2Square", 0).s ("mixVco2", 0).s ("ldReso", 0.8f).s ("ldEnvAmt", 0.65f); break;
        case 5:  b.seq ({ 0, 7, 12, 15, 19, 15, 12, 7, 0, 7, 12, 14, 17, 14, 12, 7 }).s ("vco1Sub", 0).reverb (0.3f); break;
        case 6:  b.seq ({ 0, 2, 4, 7, 9, 12, 9, 7, 4, 2 }); break;
        case 7:  b.seq ({ 0, 7, 0, 7, 0, 7, 12, 7 }, { 1, 1, 0, 1, 1, 0, 1, 1 }); break;
        case 8:  b.seq ({ 0, 4, 7, 12, 5, 9, 12, 17, 7, 11, 14, 19, 5, 9, 12, 17 }).s ("voiceMode", modeMono); break;
        case 9:  b.seq ({ 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 0, 1, 1, 0, 1, 0, 1 }).s ("asDiv", 8).m (srcEnv1, dVco1Pitch, 0.1f); break;
        case 10: b.seq ({ 0, 0, 1, 0, 0, 1, 0, 0, 3, 0, 1, 0, 0, 1, 0, 1 }).m (srcLfo2, dLdCutoff, 0.25f).s ("lfo2Rate", 0.07f); break;
        case 11: b.seq ({ 0, 0, -12, 0, 3, 0, -12, 0 }).s ("ldCutoff", 200).s ("stBrute", 0.5f); break;
        case 12: b.seq ({ 0, 12, 10, 0, 7, 0, 12, 5 }, { 1, 1, 0, 1, 1, 0, 1, 1 }).s ("seqSwing", 0.2f).s ("vco1Square", 1).s ("vco1Saw", 0); break;
        case 13: b.seq ({ 0, 0, 12, 0, 0 }, { 1, 0, 1, 0, 1 }); break;
        case 14: b.seq ({ 0, 1, 0, 5, 0, 1, 7, 5 }).s ("ldReso", 0.5f); break;
        default: b.seq ({ 0, 3, 7, 3, 0, 5, 8, 5, 0, 3, 7, 10, 12, 10, 7, 3 }).s ("asDiv", 6).s ("voiceMode", modeMono)
                  .env (2, 0.01f, 0.4f, 0.6f, 0.3f).reverb (0.3f); break;
    }
}

void syncBank (PB& b, int i)
{
    b.s ("voiceMode", i % 4 == 2 ? modePoly : modeLegato).s ("vco2Sync", 1).s ("vco2Saw", 1).s ("vco2Square", 0)
     .s ("vco2Coarse", (float) b.ri (5, 19)).s ("vco1Saw", 1).s ("mixVco1", 0.25f).s ("mixVco2", 0.8f)
     .s ("routeVco1", 1).s ("routeVco2", 1).s ("stCutoff", 3000).s ("ldCutoff", 2500).s ("stOut", 0.6f).s ("ldOut", 0.6f)
     .env (2, 0.003f, 0.5f, 0.85f, 0.3f).env (3, 0.001f, b.rr (0.3f, 1.2f), b.rr (0.0f, 0.3f), 0.4f)
     .m (srcEnv3, dVco2Pitch, b.rr (0.15f, 0.4f)).m (srcWheel, dVco2Pitch, 0.3f).vibrato (0.005f).s ("lfo1Rate", 5.5f)
     .delay (6, 0.3f, 0.15f);

    switch (i)
    {
        case 0:  break;
        case 1:  b.m (srcLfo2, dVco2Pitch, 0.25f).s ("lfo2Rate", 0.25f).s ("lfo2Wave", 1); break;
        case 2:  b.s ("voiceMode", modeMono).s ("vco2Coarse", -5).s ("vco1Coarse", -12).ladderOnly().s ("ldCutoff", 600).s ("ldEnvAmt", 0.4f); break;
        case 3:  b.env (2, 0.05f, 0.5f, 0.8f, 0.4f).s ("vco1Ultra", 0.3f).chorus (0.3f); break;
        case 4:  b.m (srcEnv3, dVco2Pitch, 0.6f).env (3, 0.001f, 0.25f, 0.0f, 0.2f); break;
        case 5:  b.m (srcLfo1, dVco2Pitch, 0.3f).s ("lfo1Sync", 1).s ("lfo1Div", 6).s ("lfo1Wave", 2); break;
        case 6:  b.env (2, 1.2f, 1.0f, 0.9f, 2.0f).m (srcLfo2, dVco2Pitch, 0.15f).s ("lfo2Rate", 0.1f).reverb (0.4f); break;
        case 7:  b.env (2, 0.001f, 0.35f, 0.0f, 0.3f).m (srcEnv3, dVco2Pitch, 0.45f).env (3, 0.001f, 0.15f, 0.0f, 0.1f); break;
        case 8:  b.s ("stBrute", 0.7f).s ("stDrive", 0.8f).s ("vco2Coarse", 24).m (srcEnv3, dVco2Pitch, -0.4f); break;
        case 9:  b.m (srcWheel, dVco2Pitch, 0.6f).m (srcEnv3, dVco2Pitch, 0.0f); break;
        case 10: b.env (2, 0.002f, 1.0f, 0.3f, 0.4f).s ("voiceMode", modePoly); break;
        case 11: b.s ("stBrute", 1.0f).s ("stReso", 0.5f).m (srcLfo1, dVco2Pitch, 0.1f).s ("lfo1Rate", 7.0f); break;
        case 12: b.m (srcVelocity, dVco2Pitch, 0.35f); break;
        case 13: b.s ("asMode", asArp).s ("arpOct", 2).s ("asDiv", 8).s ("asGate", 0.4f); break;
        case 14: b.env (3, 2.0f, 3.0f, 0.4f, 1.0f).m (srcEnv3, dVco2Pitch, 0.4f); break;
        default: b.s ("vco2Tri", 1).s ("vco2Saw", 0).s ("vco2Metal", 0.3f).env (2, 0.4f, 1.0f, 0.9f, 1.5f).s ("voiceMode", modePoly).reverb (0.35f); break;
    }
}

void ultrasawBank (PB& b, int i)
{
    b.s ("voiceMode", i % 3 == 1 ? modeLegato : modePoly).s ("vco1Saw", 1).s ("vco1Ultra", b.rr (0.6f, 1.0f)).s ("vco2Saw", 1)
     .s ("vco2Square", 0).s ("vco2Ultra", b.rr (0.4f, 1.0f)).s ("vco2Fine", b.rr (-12, 12)).s ("mixVco1", 0.65f).s ("mixVco2", 0.55f)
     .ladderOnly().s ("ldMode", 1).s ("ldCutoff", b.rr (2000, 6000)).s ("ldEnvAmt", 0.15f)
     .env (2, 0.005f, 0.6f, 0.9f, 0.5f).chorus (0.35f).reverb (0.25f);

    switch (i)
    {
        case 0:  b.s ("vco2Coarse", -12); break;
        case 1:  b.vibrato (0.005f).s ("lfo1Rate", 5.5f).delay (6, 0.35f, 0.2f); break;
        case 2:  b.env (2, 0.8f, 1.0f, 0.9f, 2.0f).reverb (0.4f, 0.9f); break;
        case 3:  b.s ("voiceMode", modeMono).s ("vco1Coarse", -12).s ("vco2Coarse", -12).s ("vco1Sub", 0.6f).s ("ldCutoff", 500).s ("ldEnvAmt", 0.4f)
                  .env (1, 0.001f, 0.3f, 0.2f, 0.2f); break;
        case 4:  b.s ("voiceMode", modeLegato).s ("glide", 0.15f).s ("vco2Coarse", -12).s ("vco1Square", 0.5f).s ("vco1PW", 0.2f)
                  .m (srcLfo1, dVco1PW, 0.2f).m (srcLfo2, dUltra, 0.4f).s ("lfo2Rate", 0.5f); break;
        case 5:  b.s ("vco2Coarse", 12).m (srcLfo2, dUltra, 0.3f).s ("lfo2Rate", 0.3f).reverb (0.4f, 0.9f); break;
        case 6:  b.env (2, 0.4f, 1.0f, 0.9f, 1.4f).s ("vco2Coarse", 12).s ("ldCutoff", 3000); break;
        case 7:  b.env (1, 0.001f, 0.8f, 0.5f, 0.6f).s ("ldEnvAmt", 0.35f).s ("ldCutoff", 1200).delay (7, 0.3f, 0.15f); break;
        case 8:  b.env (2, 0.001f, 0.4f, 0.0f, 0.3f).env (1, 0.001f, 0.2f, 0.0f, 0.2f).s ("ldEnvAmt", 0.5f).s ("ldCutoff", 700).delay (6, 0.4f, 0.2f); break;
        case 9:  b.m (srcLfo1, dUltra, 0.5f).m (srcLfo2, dPan, 0.5f).s ("lfo1Rate", 0.4f).s ("lfo2Rate", 0.13f); break;
        case 10: b.env (1, 0.08f, 0.5f, 0.6f, 0.3f).s ("ldEnvAmt", 0.45f).s ("ldCutoff", 900); break;
        case 11: b.s ("vco2Coarse", 7).env (2, 0.01f, 1.0f, 0.9f, 1.0f).reverb (0.35f, 0.9f).delay (6, 0.3f, 0.15f); break;
        case 12: b.s ("voiceMode", modeMono).s ("glide", 0.04f); break;
        case 13: b.m (srcLfo2, dPan, 0.9f).s ("lfo2Rate", 0.25f).s ("chMix", 0.6f); break;
        case 14: b.env (2, 1.5f, 1.0f, 1.0f, 3.0f).s ("ldCutoff", 1500).m (srcLfo2, dLdCutoff, 0.15f).s ("lfo2Rate", 0.08f).reverb (0.5f, 0.95f); break;
        default: b.s ("vco2Coarse", -12).s ("vco1Sub", 0.4f).env (1, 0.5f, 1.0f, 0.8f, 1.0f).s ("ldEnvAmt", 0.35f).s ("ldCutoff", 900); break;
    }
}

void metalBank (PB& b, int i)
{
    b.s ("voiceMode", i % 2 == 0 ? modeLegato : modePoly).s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco1Metal", b.rr (0.2f, 0.7f))
     .s ("vco2Saw", 0).s ("vco2Square", 0).s ("vco2Tri", 1).s ("vco2Metal", b.rr (0.1f, 0.5f)).s ("vco2Coarse", (float) (b.chance (0.5f) ? 12 : 7))
     .s ("mixVco1", 0.7f).s ("mixVco2", 0.4f).s ("routeVco1", 1).s ("routeVco2", 1).s ("stCutoff", 5000).s ("ldCutoff", 5000)
     .s ("stOut", 0.6f).s ("ldOut", 0.6f).env (2, 0.003f, 0.8f, 0.6f, 0.5f).env (3, 0.001f, b.rr (0.2f, 1.0f), 0.2f, 0.5f)
     .m (srcEnv3, dVco1Metal, b.rr (0.2f, 0.5f)).m (srcWheel, dVco1Metal, 0.5f).reverb (0.25f);

    switch (i)
    {
        case 0:  b.vibrato (0.005f).s ("lfo1Rate", 5.0f); break;
        case 1:  b.s ("vco2Coarse", 19).env (2, 0.001f, 2.5f, 0.0f, 2.0f).reverb (0.4f); break;
        case 2:  b.s ("voiceMode", modeMono).s ("vco1Coarse", -12).s ("vco1Sub", 0.5f).ladderOnly().s ("ldCutoff", 900).s ("ldEnvAmt", 0.3f); break;
        case 3:  b.s ("vco2Coarse", 6).s ("vco2Fine", 37).env (2, 0.005f, 5.0f, 0.0f, 5.0f).reverb (0.5f, 0.95f); break;
        case 4:  b.m (srcLfo2, dVco1Metal, 0.5f).s ("lfo2Rate", 0.2f).s ("lfo2Wave", 1); break;
        case 5:  b.s ("vco1Metal", 0.9f).env (2, 0.001f, 0.6f, 0.0f, 0.5f).s ("vco2Coarse", 13); break;
        case 6:  b.env (2, 1.0f, 1.0f, 0.9f, 2.0f).m (srcLfo2, dVco2Metal, 0.4f).s ("lfo2Rate", 0.1f).chorus (0.4f); break;
        case 7:  b.env (2, 0.001f, 0.35f, 0.0f, 0.3f).m (srcEnv1, dVco1Metal, 0.5f).delay (6, 0.35f, 0.2f); break;
        case 8:  b.s ("vco1Metal", 1.0f).s ("stBrute", 0.6f).s ("vco1Coarse", -12); break;
        case 9:  b.m (srcLfo1, dVco1Metal, 0.3f).s ("lfo1Rate", 2.0f).s ("glide", 0.1f); break;
        case 10: b.s ("stMode", 2).s ("stCutoff", 600).env (2, 0.01f, 1.0f, 0.7f, 0.8f); break;
        case 11: b.s ("stCutoff", 1200).s ("ldCutoff", 1200).s ("stDrive", 0.7f).s ("mixNoise", 0.06f); break;
        case 12: b.env (2, 0.6f, 1.0f, 0.9f, 1.5f).s ("stMode", 1).s ("stReso", 0.5f).s ("stCutoff", 1500).reverb (0.4f); break;
        case 13: b.s ("vco1Metal", 0.6f).s ("vco1Coarse", 24).env (2, 0.001f, 0.3f, 0.0f, 0.2f); break;
        case 14: b.s ("asMode", asArp).s ("arpPattern", 2).s ("arpOct", 2).s ("asGate", 0.4f).delay (7, 0.4f, 0.2f); break;
        default: b.env (2, 2.0f, 1.0f, 1.0f, 4.0f).m (srcLfo2, dVco1Metal, 0.3f).s ("lfo2Rate", 0.05f).reverb (0.5f, 0.95f); break;
    }
}

void droneBank (PB& b, int i)
{
    b.poly().s ("drift", 0.5f).s ("vco1Saw", 0.7f).s ("vco1Tri", 0.5f).s ("vco2Saw", 0.6f).s ("vco2Square", 0)
     .s ("vco2Coarse", (float) (b.chance (0.5f) ? -12 : 7)).s ("vco2Fine", b.rr (-10, 10)).s ("mixVco1", 0.6f).s ("mixVco2", 0.5f)
     .s ("mixNoise", b.rr (0.0f, 0.2f)).s ("noiseColor", b.rr (-0.8f, 0.2f)).s ("ldCutoff", b.rr (300, 1200)).s ("stCutoff", b.rr (400, 2000))
     .env (2, b.rr (1.5f, 4.0f), 1.0f, 1.0f, b.rr (3.0f, 8.0f)).env (1, 3.0f, 4.0f, 0.6f, 4.0f).s ("ldEnvAmt", 0.2f)
     .s ("lfo1Rate", b.rr (0.05f, 0.2f)).s ("lfo2Rate", b.rr (0.02f, 0.1f)).s ("lfo2Wave", 6)
     .m (srcLfo1, dLdCutoff, 0.2f).m (srcLfo2, dStCutoff, 0.25f).m (srcLfo2, dPan, 0.5f).reverb (0.55f, 0.95f).delay (2, 0.5f, 0.2f);

    switch (i)
    {
        case 0:  b.s ("vco1Coarse", -24).s ("vco1Sub", 0.6f).s ("ldCutoff", 250); break;
        case 1:  b.s ("mixVco1", 0).s ("mixVco2", 0).s ("mixNoise", 0.9f).steinerOnly().s ("stMode", 1).s ("stReso", 0.6f)
                  .m (srcLfo2, dStCutoff, 0.6f); break;
        case 2:  b.s ("mixNoise", 0.5f).s ("noiseColor", -0.9f).s ("ldCutoff", 300).m (srcLfo1, dVca, 0.3f).s ("lfo1Rate", 0.08f); break;
        case 3:  b.s ("vco3Coarse", 24).s ("mixVco3", 0.2f).s ("vco3Wave", 0).m (srcLfo1, dVco3Pitch, 0.05f); break;
        case 4:  b.s ("vco1Square", 1).s ("vco1Saw", 0).s ("vco1Tri", 0).s ("vco1PW", 0.1f).s ("mixVco1", 0.3f).s ("mixVco2", 0.2f).s ("lfo1Rate", 6.0f).m (srcLfo1, dVca, 0.3f).s ("stBrute", 0.5f); break;
        case 5:  b.s ("vco1Tri", 1).s ("vco1Saw", 0.2f).s ("vco2Coarse", 12).reverb (0.7f, 1.0f); break;
        case 6:  b.s ("vco1Coarse", -12).s ("vco2Coarse", -11).s ("mixVco1", 0.4f).s ("mixVco2", 0.35f).s ("stBrute", 0.6f).s ("stDrive", 0.6f); break;
        case 7:  b.s ("vco1Tri", 1).s ("vco1Metal", 0.3f).s ("mixNoise", 0.2f).s ("noiseColor", 0.5f).s ("ldCutoff", 3000); break;
        case 8:  b.s ("stMode", 2).s ("stCutoff", 300).s ("routeVco1", 1).s ("mixNoise", 0.2f); break;
        case 9:  b.s ("vco1Ultra", 0.8f).m (srcLfo1, dUltra, 0.4f).flanger (0.4f); break;
        case 10: b.s ("mixNoise", 0.4f).s ("noiseColor", 0.8f).s ("lfo2Wave", 5).s ("lfo2Rate", 6.0f).m (srcLfo2, dLdCutoff, 0.3f); break;
        case 11: b.s ("vco2Coarse", 6).m (srcLfo1, dVco2Pitch, 0.02f).flanger (0.5f, 0.6f, 0.05f); break;
        case 12: b.s ("vco1Coarse", -24).s ("vco2Coarse", -17).s ("ldCutoff", 180).s ("ldReso", 0.5f); break;
        case 13: b.s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco2Tri", 1).s ("vco2Saw", 0).s ("vco2Coarse", 19).chorus (0.5f).reverb (0.6f, 1.0f); break;
        case 14: b.s ("vco1Coarse", -24).s ("ldCutoff", 120).s ("ldReso", 0.7f).m (srcLfo2, dLdCutoff, 0.4f); break;
        default: b.env (2, 5.0f, 1.0f, 1.0f, 10.0f).reverb (0.8f, 1.0f).s ("dlFeedback", 0.7f); break;
    }
}

void percBank (PB& b, int i)
{
    b.poly().s ("vco1Saw", 0).s ("vco1Tri", 1).s ("vco2Square", 0).s ("mixVco1", 0.9f).s ("mixVco2", 0)
     .ladderOnly().s ("ldCutoff", 4000).s ("ldKeyTrack", 0).s ("env2Vel", 0.7f)
     .env (2, 0.001f, 0.3f, 0.0f, 0.2f).env (3, 0.001f, 0.06f, 0.0f, 0.05f).m (srcEnv3, dVco1Pitch, 0.6f);

    switch (i)
    {
        case 0:  b.s ("vco1Coarse", -12).env (2, 0.001f, 0.45f, 0.0f, 0.3f).s ("ldDrive", 0.5f); break;
        case 1:  b.s ("mixVco1", 0.3f).s ("mixNoise", 0.9f).steinerOnly().s ("stMode", 1).s ("stCutoff", 2500).s ("stReso", 0.2f)
                  .env (2, 0.001f, 0.2f, 0.0f, 0.15f).reverb (0.15f); break;
        case 2:  b.s ("mixVco1", 0).s ("mixNoise", 1).steinerOnly().s ("stMode", 2).s ("stCutoff", 7000).env (2, 0.001f, 0.06f, 0.0f, 0.05f); break;
        case 3:  b.s ("vco1Metal", 0.3f).env (2, 0.001f, 0.5f, 0.0f, 0.3f).m (srcEnv3, dVco1Pitch, 0.4f).env (3, 0.001f, 0.25f, 0.0f, 0.2f); break;
        case 4:  b.s ("mixVco1", 0).s ("mixNoise", 1).steinerOnly().s ("stMode", 1).s ("stCutoff", 1400).s ("stReso", 0.4f)
                  .m (srcLfo1, dVca, 0.8f).s ("lfo1Wave", 3).s ("lfo1Rate", 40.0f).s ("lfo1Retrig", 1).env (2, 0.001f, 0.15f, 0.0f, 0.1f).reverb (0.2f); break;
        case 5:  b.s ("vco1Tri", 0).s ("vco1Square", 1).s ("vco2Square", 1).s ("vco2Coarse", 7).s ("vco2Fine", 40).s ("mixVco2", 0.8f)
                  .steinerOnly().s ("stMode", 1).s ("stCutoff", 1000).s ("stReso", 0.5f).m (srcEnv3, dVco1Pitch, 0).env (2, 0.001f, 0.25f, 0.0f, 0.1f); break;
        case 6:  b.s ("mixNoise", 0.5f).s ("vco1Coarse", 12).env (2, 0.001f, 0.05f, 0.0f, 0.04f); break;
        case 7:  b.m (srcEnv3, dVco1Pitch, 1.0f).env (3, 0.001f, 0.12f, 0.0f, 0.1f).s ("vco1Coarse", -12); break;
        case 8:  b.s ("mixVco1", 0).s ("mixNoise", 1).s ("noiseColor", 0.6f).steinerOnly().s ("stMode", 2).s ("stCutoff", 4000)
                  .env (2, 0.02f, 0.12f, 0.0f, 0.1f); break;
        case 9:  b.s ("ldMode", 2).s ("ldCutoff", 600).s ("ldReso", 0.6f).m (srcEnv3, dVco1Pitch, 0.25f).env (2, 0.001f, 0.25f, 0.0f, 0.15f); break;
        case 10: b.s ("vco1Metal", 0.8f).s ("vco2Tri", 1).s ("vco2Metal", 0.7f).s ("vco2Coarse", 17).s ("mixVco2", 0.6f).env (2, 0.001f, 1.2f, 0.0f, 1.0f).reverb (0.3f); break;
        case 11: b.m (srcEnv3, dVco1Pitch, -0.8f).env (3, 0.001f, 0.3f, 0.0f, 0.2f).delay (8, 0.4f, 0.25f); break;
        case 12: b.s ("mixVco1", 0).s ("mixNoise", 1).s ("noiseColor", 0.3f).env (2, 0.001f, 2.0f, 0.0f, 2.0f).flanger (0.5f).reverb (0.3f); break;
        case 13: b.s ("vco1Coarse", 24).s ("ldMode", 2).s ("ldCutoff", 1500).env (2, 0.001f, 0.08f, 0.0f, 0.05f).m (srcEnv3, dVco1Pitch, 0.2f); break;
        case 14: b.s ("vco1Coarse", -24).s ("vco1Tri", 1).s ("ldDrive", 0.8f).env (2, 0.001f, 1.5f, 0.0f, 1.0f).m (srcEnv3, dVco1Pitch, 0.5f)
                  .env (3, 0.001f, 0.15f, 0.0f, 0.1f); break;
        default: b.s ("asMode", asSeq).seq ({ 0, 12, 7, 12, 0, 19, 7, 12 }, { 1, 0, 1, 1, 1, 0, 1, 1 }).s ("voiceMode", modeMono)
                  .s ("asGate", 0.3f).delay (7, 0.35f, 0.2f); break;
    }
}

void fxBank (PB& b, int i)
{
    b.s ("voiceMode", modeMono).s ("vco1Saw", 1).s ("vco2Square", 0.8f).s ("vco2Coarse", (float) b.ri (-12, 12)).s ("mixVco1", 0.6f)
     .s ("mixVco2", 0.5f).s ("routeVco1", 1).s ("routeVco2", 1).s ("stCutoff", 2500).s ("ldCutoff", 2500)
     .env (2, 0.005f, 1.0f, 1.0f, 0.8f).delay (b.chance (0.5f) ? 6 : 7, 0.5f, 0.25f).reverb (0.3f);

    switch (i)
    {
        case 0:  b.m (srcEnv3, dVco1Pitch, 1.0f).m (srcEnv3, dVco2Pitch, 1.0f).env (3, 0.001f, 0.25f, 0.0f, 0.1f).env (2, 0.001f, 0.3f, 0.0f, 0.2f); break;
        case 1:  b.m (srcLfo1, dVco1Pitch, 0.3f).m (srcLfo1, dVco2Pitch, 0.3f).s ("lfo1Wave", 1).s ("lfo1Rate", 0.6f); break;
        case 2:  b.m (srcLfo1, dVco1Pitch, 0.25f).m (srcLfo1, dVco2Pitch, 0.25f).s ("lfo1Wave", 4).s ("lfo1Rate", 3.0f).s ("vco1Square", 1); break;
        case 3:  b.m (srcEnv3, dVco1Pitch, 0.5f).m (srcEnv3, dStCutoff, 0.6f).s ("mixNoise", 0.4f).env (3, 4.0f, 0.1f, 1.0f, 0.5f)
                  .s ("stCutoff", 200); break;
        case 4:  b.m (srcEnv3, dVco1Pitch, -0.8f).m (srcEnv3, dVco2Pitch, -0.8f).env (3, 3.0f, 0.1f, 1.0f, 0.5f); break;
        case 5:  b.s ("vco1Square", 1).s ("vco1PW", 0.2f).steinerOnly().s ("stMode", 1).s ("stReso", 0.7f).m (srcLfo1, dStCutoff, 0.6f)
                  .s ("lfo1Wave", 5).s ("lfo1Rate", 9.0f); break;
        case 6:  b.s ("lfo1Wave", 5).s ("lfo1Rate", 8.0f).m (srcLfo1, dVco1Pitch, 0.5f).m (srcLfo1, dVco2Pitch, 0.5f).s ("vco1Saw", 0).s ("vco1Tri", 1); break;
        case 7:  b.m (srcLfo1, dVco1Pitch, 0.1f).m (srcLfo2, dVco1Pitch, 0.4f).s ("lfo1Rate", 8.0f).s ("lfo2Wave", 3).s ("lfo2Rate", 0.2f).flanger (0.5f); break;
        case 8:  b.s ("mixNoise", 0.5f).m (srcLfo2, dVco1Pitch, 0.6f).s ("lfo2Wave", 6).s ("lfo2Rate", 2.0f).steinerOnly().s ("stMode", 1); break;
        case 9:  b.s ("lfo1Wave", 6).s ("lfo1Rate", 12.0f).m (srcLfo1, dLdCutoff, 0.5f).s ("ldReso", 0.8f).ladderOnly(); break;
        case 10: b.s ("mixVco1", 0).s ("mixVco2", 0).s ("mixNoise", 1).s ("noiseColor", -1).ladderOnly().s ("ldCutoff", 400)
                  .env (2, 0.3f, 4.0f, 0.0f, 4.0f).reverb (0.6f, 1.0f); break;
        case 11: b.s ("lfo1Wave", 5).s ("lfo1Sync", 1).s ("lfo1Div", 10).m (srcLfo1, dVco1Pitch, 0.5f).m (srcLfo1, dVca, 0.5f); break;
        case 12: b.s ("mixVco1", 0).s ("mixVco2", 0).s ("mixNoise", 1).steinerOnly().s ("stMode", 1).s ("stReso", 0.7f)
                  .m (srcLfo2, dStCutoff, 0.7f).s ("lfo2Rate", 0.15f); break;
        case 13: b.m (srcEnv3, dVco1Pitch, 0.9f).m (srcLfo1, dVco1Pitch, 0.2f).s ("lfo1Rate", 20.0f).env (3, 0.001f, 1.0f, 0.0f, 0.5f).flanger (0.6f); break;
        case 14: b.s ("lfo1Wave", 4).s ("lfo1Rate", 12.0f).m (srcLfo1, dVco1Pitch, 0.2f).s ("vco1Saw", 0).s ("vco1Square", 1).s ("mixVco2", 0); break;
        default: b.s ("mixVco1", 0.35f).s ("mixVco2", 0.3f).s ("stBrute", 1.0f).s ("stReso", 0.8f).m (srcLfo1, dStCutoff, 0.5f).s ("lfo1Wave", 5).s ("lfo1Rate", 15.0f)
                  .m (srcEnv3, dVco1Pitch, 0.6f).env (3, 0.001f, 0.4f, 0.0f, 0.2f); break;
    }
}

void classicBank (PB& b, int i)
{
    switch (i)
    {
        case 0:  break; // Init Patch: every parameter at its default
        case 1:  b.poly(); break;
        case 2:  b.s ("vco1Sub", 0.6f).ladderOnly().s ("ldCutoff", 400).s ("ldEnvAmt", 0.4f).env (1, 0.001f, 0.3f, 0.1f, 0.1f); break;
        case 3:  b.poly().env (2, 1.0f, 1.0f, 0.9f, 2.0f).chorus (0.4f).reverb (0.35f); break;
        case 4:  b.s ("voiceMode", modeLegato).s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Fine", 8).s ("mixVco2", 0.7f).vibrato().s ("lfo1Rate", 5.5f); break;
        case 5:  b.s ("asMode", asArp).s ("arpOct", 2); break;
        case 6:  b.s ("asMode", asSeq).seq ({ 0, 3, 7, 12, 7, 3, 0, -12 }); break;
        case 7:  b.s ("vco1Saw", 1).s ("mixVco2", 0); break;
        case 8:  b.s ("vco1Saw", 0).s ("vco1Square", 1); break;
        case 9:  b.s ("vco1Saw", 0).s ("vco1Tri", 1); break;
        case 10: b.s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Fine", 10).s ("mixVco2", 0.8f); break;
        case 11: b.s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Fine", 8).s ("mixVco2", 0.7f).s ("vco3Wave", 2).s ("vco3Fine", -8).s ("mixVco3", 0.6f); break;
        case 12: b.steinerOnly().s ("stCutoff", 800).s ("stReso", 0.5f).s ("stEnvAmt", 0.4f).s ("stBrute", 0.4f).env (1, 0.001f, 0.5f, 0.2f, 0.3f); break;
        case 13: b.ladderOnly().s ("ldCutoff", 600).s ("ldReso", 0.6f).s ("ldEnvAmt", 0.45f).env (1, 0.001f, 0.5f, 0.2f, 0.3f); break;
        case 14: b.s ("vco2Saw", 1).s ("vco2Square", 0).s ("mixVco2", 0.6f).m (srcLfo1, dVco1PW, 0.2f).m (srcLfo2, dStCutoff, 0.2f)
                  .m (srcEnv3, dVco2Pitch, 0.1f).m (srcVelocity, dVca, 0.2f).m (srcWheel, dLdCutoff, 0.4f).m (srcAftertouch, dVco1Pitch, 0.02f)
                  .m (srcKey, dPan, 0.4f).s ("vco1Square", 0.6f); break;
        default: b.s ("vco1Saw", 1).s ("vco1Sub", 0.5f).s ("vco1Ultra", 0.3f).s ("vco2Saw", 1).s ("vco2Square", 0).s ("vco2Coarse", -12)
                  .s ("mixVco2", 0.6f).s ("vco3Wave", 3).s ("mixVco3", 0.3f).s ("vco3Coarse", 7).s ("stBrute", 0.5f).s ("stDrive", 0.5f)
                  .s ("ldDrive", 0.5f).s ("filterConfig", 1).s ("routeVco1", 1).s ("routeVco2", 1).s ("routeVco3", 1)
                  .s ("ldCutoff", 900).s ("ldEnvAmt", 0.35f).env (1, 0.001f, 0.6f, 0.3f, 0.3f).delay (7, 0.3f, 0.15f); break;
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
    { "Bass", bassBank, { "Brute Sub", "Steiner Growl", "Ladder Low", "Rubber Pluck", "Acid Matrix", "Square Thump", "Fold Bass", "Deep Sync",
                          "Wobble Unit", "Reese Drift", "Mono Punch", "Velvet Floor", "Metal Bottom", "Ultra Sub", "Funk Squelch", "Detroit Bass" } },
    { "Lead", leadBank, { "Solo Forge", "Brute Lead", "Sync Scream", "Liquid Square", "Steiner Sing", "Fold Solo", "Mono Glide", "Hot Wire",
                          "Classic 70s", "Saw Cutter", "Whistle Tri", "Ultra Lead", "Distorted Line", "Soft Flute", "Portamento King", "Ladder Lick" } },
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
    { "Templates", classicBank, { "Init Patch", "Init Poly", "Init Bass", "Init Pad", "Init Lead", "Init Arp", "Init Seq", "Basic Saw",
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
            PB b (banks[bank].presets[i], 0x5eed0000 + bank * 131 + i * 7);
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
