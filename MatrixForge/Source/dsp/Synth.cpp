#include "Synth.h"

namespace mf
{
//==============================================================================
void Voice::prepare (float sr, uint32_t seed)
{
    sampleRate = sr;
    rng = Rng (seed);
    noise = NoiseSource (seed * 7919u + 13u);
    for (auto* e : { &env1, &env2, &env3 })
        e->setSampleRate (sr);
    vco1.reset (rng.next());
    vco2.reset (rng.next());
    vco3.reset (rng.next());
    kill();
}

void Voice::updateEnvelopes (const Snapshot& p)
{
    env1.setParams (0.0f, p.env1A, p.env1D, p.env1S, p.env1R);
    env2.setParams (0.0f, p.env2A, p.env2D, p.env2S, p.env2R);
    env3.setParams (p.env3Delay, p.env3A, p.env3D, p.env3S, p.env3R);
}

void Voice::start (int midiNote, float vel, bool glide, const Snapshot& p)
{
    if (! active)
    {
        steiner.reset();
        ladder.reset();
        stCutSmooth = p.stCutoff;
        ldCutSmooth = p.ldCutoff;
        glide = false;
    }

    note = midiNote;
    targetNote = (float) midiNote;
    if (! glide)
        currentNote = targetNote;

    velocity = vel;
    held = true;
    sustained = false;
    active = true;

    updateEnvelopes (p);
    env1.noteOn();
    env2.noteOn();
    env3.noteOn();
}

void Voice::changeNote (int midiNote, bool glide)
{
    note = midiNote;
    targetNote = (float) midiNote;
    if (! glide)
        currentNote = targetNote;
    held = true;
}

void Voice::release()
{
    held = false;
    sustained = false;
    env1.noteOff();
    env2.noteOff();
    env3.noteOff();
}

void Voice::kill()
{
    active = false;
    held = sustained = false;
    note = -1;
    env1.reset();
    env2.reset();
    env3.reset();
}

void Voice::render (float* left, float* right, int numSamples, const VoiceContext& ctx)
{
    if (! active)
        return;

    const Snapshot& p = *ctx.p;
    updateEnvelopes (p);

    const float sr = sampleRate;
    const float glideCoef = p.glide > 0.0005f ? std::exp (-1.0f / (p.glide * 0.35f * sr)) : 0.0f;
    const float env1Scale = 1.0f - p.env1Vel * (1.0f - velocity);
    const float ampVel = 1.0f - p.env2Vel * (1.0f - velocity);
    const float driftDepth = p.drift * 0.12f; // up to +-12 cents
    const bool series = p.filterConfig >= 0.5f;
    const int stMode = (int) p.stMode, ldMode = (int) p.ldMode, vco3Wave = (int) p.vco3Wave;
    const bool vco3Lfo = p.vco3Lfo >= 0.5f, sync = p.vco2Sync >= 0.5f;
    const int route[4] { (int) p.routeVco1, (int) p.routeVco2, (int) p.routeVco3, (int) p.routeNoise };
    const float cutSmoothCoef = 1.0f - std::exp (-1.0f / (0.004f * sr));

    float src[kNumModSources];
    src[srcVelocity] = velocity * 2.0f - 1.0f;
    src[srcWheel] = ctx.wheel;
    src[srcAftertouch] = ctx.aftertouch;

    for (int i = 0; i < numSamples; ++i)
    {
        // Portamento
        currentNote = glideCoef > 0.0f ? targetNote + (currentNote - targetNote) * glideCoef : targetNote;

        // Slow random pitch drift per oscillator
        if (--driftCounter <= 0)
        {
            driftCounter = (int) (sr * 0.25f);
            for (auto& t : driftTarget)
                t = rng.bipolar();
        }
        for (int d = 0; d < 3; ++d)
            drift[d] += (driftTarget[d] - drift[d]) * 0.00005f;

        const float e1 = env1.process() * env1Scale;
        const float e2 = env2.process();
        const float e3 = env3.process();

        src[srcEnv1] = e1;
        src[srcEnv2] = e2;
        src[srcEnv3] = e3;
        src[srcLfo1] = ctx.lfo1[i];
        src[srcLfo2] = ctx.lfo2[i];
        src[srcVco3] = vco3Last;
        src[srcKey] = (currentNote - 60.0f) / 60.0f;

        float mod[kNumModDests] {};
        for (int r = 0; r < ctx.numRoutes; ++r)
            mod[ctx.routes[r].dest] += src[ctx.routes[r].source] * ctx.routes[r].amount;

        const float base = currentNote + ctx.bendSemis;

        // VCO3 (computed first: it can frequency-modulate VCO1)
        const float f3 = vco3Lfo ? p.vco3Rate * std::exp2 (mod[dVco3Pitch] * 4.0f)
                                 : midiToHz (base + p.vco3Coarse + p.vco3Fine * 0.01f + drift[2] * driftDepth + mod[dVco3Pitch] * 24.0f);
        const float o3 = vco3.process (f3, sr, vco3Wave);
        vco3Last = o3;

        float f1 = midiToHz (base + p.vco1Coarse + p.vco1Fine * 0.01f + drift[0] * driftDepth + mod[dVco1Pitch] * 24.0f);
        f1 *= std::max (0.0f, 1.0f + p.vco1FM * o3 * 2.0f);
        const float f2 = midiToHz (base + p.vco2Coarse + p.vco2Fine * 0.01f + drift[1] * driftDepth + mod[dVco2Pitch] * 24.0f);

        const float ultra = std::clamp (mod[dUltra], -1.0f, 1.0f);
        VcoShape s1 { p.vco1Saw, p.vco1Square, p.vco1Tri, p.vco1Sub, p.vco1PW + mod[dVco1PW] * 0.45f,
                      std::clamp (p.vco1Metal + mod[dVco1Metal], 0.0f, 1.0f), std::clamp (p.vco1Ultra + ultra, 0.0f, 1.0f) };
        VcoShape s2 { p.vco2Saw, p.vco2Square, p.vco2Tri, p.vco2Sub, p.vco2PW + mod[dVco2PW] * 0.45f,
                      std::clamp (p.vco2Metal + mod[dVco2Metal], 0.0f, 1.0f), std::clamp (p.vco2Ultra + ultra, 0.0f, 1.0f) };

        const float o1 = vco1.process (f1, sr, s1, false);
        const float o2 = vco2.process (f2, sr, s2, sync && vco1.wrapped);
        const float on = noise.process (p.noiseColor) * std::clamp (1.0f + mod[dNoise], 0.0f, 2.0f);

        // Mixer with per-source routing to the two filters
        const float levels[4] { o1 * p.mixVco1, o2 * p.mixVco2, o3 * p.mixVco3, on * p.mixNoise };
        float stBus = 0.0f, ldBus = 0.0f;
        for (int k = 0; k < 4; ++k)
        {
            if (route[k] <= 1) stBus += levels[k];
            if (route[k] >= 1) ldBus += levels[k];
        }
        stBus *= 0.5f;
        ldBus *= 0.5f;

        // Filters
        const float key = (currentNote - 60.0f) / 12.0f;
        stCutSmooth += (p.stCutoff - stCutSmooth) * cutSmoothCoef;
        ldCutSmooth += (p.ldCutoff - ldCutSmooth) * cutSmoothCoef;
        const float stCut = stCutSmooth * std::exp2 (key * p.stKeyTrack + p.stEnvAmt * e1 * 7.0f + mod[dStCutoff] * 5.0f);
        const float ldCut = ldCutSmooth * std::exp2 (key * p.ldKeyTrack + p.ldEnvAmt * e1 * 7.0f + mod[dLdCutoff] * 5.0f);

        const float st = steiner.process (stBus, stCut, p.stReso + mod[dStReso], p.stDrive,
                                          std::clamp (p.stBrute + mod[dBrute], 0.0f, 1.0f), stMode, sr);
        const float ldIn = series ? ldBus + st * p.stOut : ldBus;
        const float ld = ladder.process (ldIn, ldCut, p.ldReso + mod[dLdReso], p.ldDrive, ldMode, sr);
        const float filtered = series ? ld * p.ldOut : st * p.stOut + ld * p.ldOut;

        // VCA
        const float amp = e2 * ampVel * std::clamp (1.0f + mod[dVca], 0.0f, 2.0f) * ctx.gain;
        const float out = filtered * amp;

        if (ctx.panModulated)
        {
            const float pan = (std::clamp (mod[dPan], -1.0f, 1.0f) + 1.0f) * (kPi * 0.25f);
            left[i] += out * std::cos (pan) * 1.41421356f;
            right[i] += out * std::sin (pan) * 1.41421356f;
        }
        else
        {
            left[i] += out;
            right[i] += out;
        }
    }

    if (env2.isIdle())
        kill();
}

//==============================================================================
void Synth::prepare (double sr)
{
    sampleRate = (float) sr;
    noteStack.clear();
    noteStack.reserve (128);
    for (int i = 0; i < kMaxVoices; ++i)
        voices[i].prepare (sampleRate, 0x1234u + (uint32_t) i * 977u);
    lfo1.reset();
    lfo2.reset();
    lastVoiceMode = -1;
}

int Synth::voiceLimit (const Snapshot& p) const
{
    switch ((int) p.voiceMode)
    {
        case modePara: return 3;
        case modePoly: return kMaxVoices;
        default:       return 1;
    }
}

int Synth::numActiveVoices() const
{
    int n = 0;
    for (auto& v : voices)
        n += v.isActive() ? 1 : 0;
    return n;
}

void Synth::allNotesOff()
{
    for (auto& v : voices)
        v.kill();
    noteStack.clear();
    sustainPedal = false;
}

void Synth::handleMidi (const juce::MidiMessage& m, const Snapshot& p)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
    else if (m.isNoteOff())
        noteOff (m.getNoteNumber(), p);
    else if (m.isPitchWheel())
        bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure())
        aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())
        aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        if (cc == 1)
            wheel = (float) m.getControllerValue() / 127.0f;
        else if (cc == 64)
        {
            sustainPedal = m.getControllerValue() >= 64;
            if (! sustainPedal)
                releaseSustained();
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        allNotesOff();
}

void Synth::noteOn (int note, float vel, const Snapshot& p)
{
    const bool wasEmpty = noteStack.empty();
    noteStack.erase (std::remove_if (noteStack.begin(), noteStack.end(), [note] (const HeldNote& h) { return h.note == note; }),
                     noteStack.end());
    if (noteStack.size() < noteStack.capacity())
        noteStack.push_back ({ note, vel });

    if (wasEmpty)
    {
        if (p.lfo1Retrig >= 0.5f) lfo1.reset();
        if (p.lfo2Retrig >= 0.5f) lfo2.reset();
    }

    const int mode = (int) p.voiceMode;
    const bool glide = p.glide > 0.0005f;

    if (mode == modeMono || mode == modeLegato)
    {
        Voice& v = voices[0];
        if (mode == modeLegato && ! wasEmpty && v.isActive())
            v.changeNote (note, glide);
        else
            v.start (note, vel, glide, p);
        v.age = ++ageCounter;
        return;
    }

    const int limit = voiceLimit (p);
    Voice* target = nullptr;

    for (int i = 0; i < limit && target == nullptr; ++i)
        if (voices[i].isActive() && voices[i].note == note)
            target = &voices[i];

    for (int i = 0; i < limit && target == nullptr; ++i)
        if (! voices[i].isActive())
            target = &voices[i];

    if (target == nullptr) // steal: prefer the oldest released voice, then the oldest held one
    {
        for (int pass = 0; pass < 2 && target == nullptr; ++pass)
        {
            for (int i = 0; i < limit; ++i)
            {
                const bool candidate = pass == 1 || ! (voices[i].held || voices[i].sustained);
                if (candidate && (target == nullptr || voices[i].age < target->age))
                    target = &voices[i];
            }
        }
    }

    target->start (note, vel, glide && target->isActive(), p);
    target->age = ++ageCounter;
}

void Synth::noteOff (int note, const Snapshot& p)
{
    noteStack.erase (std::remove_if (noteStack.begin(), noteStack.end(), [note] (const HeldNote& h) { return h.note == note; }),
                     noteStack.end());

    const int mode = (int) p.voiceMode;
    if (mode == modeMono || mode == modeLegato)
    {
        Voice& v = voices[0];
        if (! v.isActive() || v.note != note)
            return;

        if (! noteStack.empty())
            v.changeNote (noteStack.back().note, p.glide > 0.0005f);
        else if (sustainPedal)
        {
            v.held = false;
            v.sustained = true;
        }
        else
            v.release();
        return;
    }

    for (auto& v : voices)
    {
        if (v.isActive() && v.held && v.note == note)
        {
            if (sustainPedal)
            {
                v.held = false;
                v.sustained = true;
            }
            else
                v.release();
        }
    }
}

void Synth::releaseSustained()
{
    for (auto& v : voices)
        if (v.isActive() && v.sustained)
            v.release();
}

float Synth::lfoHz (int which, const Snapshot& p) const
{
    const bool sync = (which == 0 ? p.lfo1Sync : p.lfo2Sync) >= 0.5f;
    if (! sync)
        return which == 0 ? p.lfo1Rate : p.lfo2Rate;
    const int div = (int) (which == 0 ? p.lfo1Div : p.lfo2Div);
    return (float) (tempo / 60.0 / divisionBeats (div));
}

void Synth::render (juce::AudioBuffer<float>& buffer, int start, int num, const Snapshot& p)
{
    const int mode = (int) p.voiceMode;
    if (mode != lastVoiceMode)
    {
        if (lastVoiceMode >= 0)
            allNotesOff();
        lastVoiceMode = mode;
    }

    VoiceContext ctx;
    ctx.p = &p;
    ctx.sampleRate = sampleRate;
    ctx.bendSemis = bend * p.bendRange;
    ctx.wheel = wheel;
    ctx.aftertouch = aftertouch;
    ctx.lfo1 = lfo1Buf;
    ctx.lfo2 = lfo2Buf;
    ctx.gain = (mode == modeMono || mode == modeLegato) ? 0.7f : 0.4f;

    int numRoutes = 0;
    for (int s = 0; s < kNumModSources; ++s)
        for (int d = 0; d < kNumModDests; ++d)
            if (std::abs (p.matrix[s][d]) > 1.0e-4f)
            {
                routes[numRoutes++] = { s, d, p.matrix[s][d] };
                ctx.panModulated = ctx.panModulated || d == dPan;
            }
    ctx.routes = routes;
    ctx.numRoutes = numRoutes;

    const float hz1 = lfoHz (0, p), hz2 = lfoHz (1, p);
    const int w1 = (int) p.lfo1Wave, w2 = (int) p.lfo2Wave;

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    while (num > 0)
    {
        const int n = std::min (num, kChunk);
        for (int i = 0; i < n; ++i)
        {
            lfo1Buf[i] = lfo1.process (hz1, w1, sampleRate);
            lfo2Buf[i] = lfo2.process (hz2, w2, sampleRate);
        }
        for (auto& v : voices)
            v.render (left + start, right + start, n, ctx);
        start += n;
        num -= n;
    }
}
} // namespace mf
