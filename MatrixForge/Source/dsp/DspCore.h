#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mf
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

// Pade tanh approximation, exact +-1 at |x| >= 3.
inline float fastTanh (float x) noexcept
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float midiToHz (float note) noexcept { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }

struct Rng
{
    explicit Rng (uint32_t seed = 0x9e3779b9u) : state (seed != 0 ? seed : 1u) {}
    float next() noexcept // [0, 1)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) (state >> 8) * (1.0f / 16777216.0f);
    }
    float bipolar() noexcept { return next() * 2.0f - 1.0f; }
    uint32_t state;
};

inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Reflective wavefolder: identity for |x| <= 1, folds back beyond.
inline float fold (float x) noexcept
{
    float m = std::fmod (x + 1.0f, 4.0f);
    if (m < 0.0f)
        m += 4.0f;
    return 1.0f - std::abs (m - 2.0f);
}

//==============================================================================
struct VcoShape
{
    float saw = 0, square = 0, tri = 0, sub = 0, pw = 0.5f, metal = 0, ultra = 0;
};

// VCO1/VCO2: simultaneous saw (with Ultrasaw), pulse, triangle (with Metalizer) and sub.
class BruteVco
{
public:
    void reset (float startPhase) noexcept
    {
        phase = startPhase;
        subPhase = startPhase * 0.5f;
        ultraPhase[0] = std::fmod (startPhase + 0.33f, 1.0f);
        ultraPhase[1] = std::fmod (startPhase + 0.66f, 1.0f);
    }

    float process (float freq, float sampleRate, const VcoShape& s, bool hardSync) noexcept
    {
        const float dt = std::clamp (freq / sampleRate, 0.0f, 0.45f);

        if (hardSync)
            phase = 0.0f;

        phase += dt;
        wrapped = phase >= 1.0f;
        if (wrapped)
            phase -= 1.0f;

        subPhase += dt * 0.5f;
        if (subPhase >= 1.0f)
            subPhase -= 1.0f;

        float out = 0.0f;

        if (s.saw > 0.0f)
        {
            float saw = 2.0f * phase - 1.0f - polyBlep (phase, dt);
            if (s.ultra > 0.001f)
            {
                const float spread = 0.004f + 0.018f * s.ultra;
                float extra = 0.0f;
                for (int i = 0; i < 2; ++i)
                {
                    const float udt = dt * (i == 0 ? 1.0f + spread : 1.0f - spread);
                    ultraPhase[i] += udt;
                    if (ultraPhase[i] >= 1.0f)
                        ultraPhase[i] -= 1.0f;
                    extra += 2.0f * ultraPhase[i] - 1.0f - polyBlep (ultraPhase[i], udt);
                }
                saw = (saw + s.ultra * 0.8f * extra) / (1.0f + s.ultra * 1.1f);
            }
            out += saw * s.saw;
        }

        if (s.square > 0.0f)
        {
            const float pw = std::clamp (s.pw, 0.05f, 0.95f);
            float sq = phase < pw ? 1.0f : -1.0f;
            sq += polyBlep (phase, dt);
            float t2 = phase - pw;
            if (t2 < 0.0f)
                t2 += 1.0f;
            sq -= polyBlep (t2, dt);
            out += sq * s.square;
        }

        if (s.tri > 0.0f)
        {
            const float tri = 1.0f - 4.0f * std::abs (phase - 0.5f);
            out += fold (tri * (1.0f + s.metal * 7.0f)) * s.tri;
        }

        if (s.sub > 0.0f)
        {
            const float sdt = dt * 0.5f;
            float sq = subPhase < 0.5f ? 1.0f : -1.0f;
            sq += polyBlep (subPhase, sdt);
            float t2 = subPhase - 0.5f;
            if (t2 < 0.0f)
                t2 += 1.0f;
            sq -= polyBlep (t2, sdt);
            out += sq * s.sub;
        }

        return out;
    }

    bool wrapped = false;

private:
    float phase = 0.0f, subPhase = 0.0f;
    float ultraPhase[2] { 0.33f, 0.66f };
};

// VCO3: single waveform, usable as audio oscillator or as an LFO (mod source).
class SimpleVco
{
public:
    void reset (float p) noexcept { phase = p; }

    float process (float freq, float sampleRate, int wave) noexcept
    {
        const float dt = std::clamp (freq / sampleRate, 0.0f, 0.45f);
        phase += dt;
        if (phase >= 1.0f)
            phase -= 1.0f;

        switch (wave)
        {
            case 0: return std::sin (kTwoPi * phase);
            case 1: return 1.0f - 4.0f * std::abs (phase - 0.5f);
            case 2: return 2.0f * phase - 1.0f - polyBlep (phase, dt);
            default:
            {
                float sq = phase < 0.5f ? 1.0f : -1.0f;
                sq += polyBlep (phase, dt);
                float t2 = phase - 0.5f;
                if (t2 < 0.0f)
                    t2 += 1.0f;
                return sq - polyBlep (t2, dt);
            }
        }
    }

private:
    float phase = 0.0f;
};

class NoiseSource
{
public:
    explicit NoiseSource (uint32_t seed = 12345u) : rng (seed) {}

    // color < 0 darkens towards red noise, > 0 brightens towards blue noise
    float process (float color) noexcept
    {
        const float white = rng.bipolar();
        lp += 0.04f * (white - lp);
        if (color < 0.0f)
            return white + (-color) * (lp * 3.5f - white);
        const float hp = white - lp;
        return white + color * (hp * 1.3f - white);
    }

private:
    Rng rng;
    float lp = 0.0f;
};

//==============================================================================
// Steiner-Parker flavoured multimode: TPT state-variable core with input drive
// and a "Brute Factor" output-to-input feedback path.
class SteinerFilter
{
public:
    void reset() noexcept { ic1 = ic2 = last = 0.0f; }

    float process (float x, float cutoff, float reso, float drive, float brute, int mode, float sampleRate) noexcept
    {
        const float fc = std::clamp (cutoff, 20.0f, sampleRate * 0.45f);
        const float g = std::tan (kPi * fc / sampleRate);
        const float k = 2.0f - 1.97f * std::clamp (reso, 0.0f, 1.0f);

        x = fastTanh ((x + brute * 1.6f * last) * (1.0f + drive * 3.0f));

        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = fastTanh (2.0f * v1 - ic1);
        ic2 = 2.0f * v2 - ic2;

        float out;
        switch (mode)
        {
            case 0:  out = v2; break;
            case 1:  out = v1 * k; break;
            case 2:  out = x - k * v1 - v2; break;
            default: out = x - k * v1; break;
        }
        last = out;
        return out;
    }

private:
    float ic1 = 0.0f, ic2 = 0.0f, last = 0.0f;
};

// Four-pole transistor ladder with saturating feedback and pole mixing for
// LP24 / LP12 / BP / HP responses.
class LadderFilter
{
public:
    void reset() noexcept
    {
        for (auto& s : state)
            s = 0.0f;
        lastOut = 0.0f;
    }

    float process (float x, float cutoff, float reso, float drive, int mode, float sampleRate) noexcept
    {
        const float fc = std::clamp (cutoff, 20.0f, sampleRate * 0.42f);
        const float g = std::tan (kPi * fc / sampleRate);
        const float G = g / (1.0f + g);
        const float k = 3.9f * std::clamp (reso, 0.0f, 1.0f);
        const float driveGain = 1.0f + drive * 4.0f;

        const float in = x * driveGain * (1.0f + 0.5f * k);
        float y[5];
        y[0] = fastTanh (in - k * lastOut);
        for (int i = 0; i < 4; ++i)
        {
            const float v = (y[i] - state[i]) * G;
            y[i + 1] = v + state[i];
            state[i] = y[i + 1] + v;
        }
        lastOut = y[4];

        float out;
        switch (mode)
        {
            case 0:  out = y[4]; break;
            case 1:  out = y[2]; break;
            case 2:  out = 4.0f * y[2] - 8.0f * y[3] + 4.0f * y[4]; break;
            default: out = y[0] - 4.0f * y[1] + 6.0f * y[2] - 4.0f * y[3] + y[4]; break;
        }
        return out / driveGain * (1.0f + drive);
    }

private:
    float state[4] {};
    float lastOut = 0.0f;
};

//==============================================================================
// Analog-style exponential envelope with optional delay stage (DADSR).
class Envelope
{
public:
    enum class Stage { Idle, Delay, Attack, Decay, Sustain, Release };

    void setSampleRate (float sr) noexcept { sampleRate = sr; }

    void setParams (float delay, float attack, float decay, float sustain, float release) noexcept
    {
        delaySamples = (int) (delay * sampleRate);
        aCoef = std::exp (-1.466f / (std::max (attack, 0.0005f) * sampleRate));
        dCoef = std::exp (-6.9f / (std::max (decay, 0.001f) * sampleRate));
        rCoef = std::exp (-6.9f / (std::max (release, 0.001f) * sampleRate));
        sustainLevel = sustain;
    }

    void noteOn() noexcept
    {
        delayCount = delaySamples;
        stage = delayCount > 0 ? Stage::Delay : Stage::Attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    void reset() noexcept
    {
        stage = Stage::Idle;
        value = 0.0f;
    }

    bool isIdle() const noexcept { return stage == Stage::Idle; }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Delay:
                if (--delayCount <= 0)
                    stage = Stage::Attack;
                break;
            case Stage::Attack:
                value = 1.3f + (value - 1.3f) * aCoef;
                if (value >= 1.0f)
                {
                    value = 1.0f;
                    stage = Stage::Decay;
                }
                break;
            case Stage::Decay:
                value = sustainLevel + (value - sustainLevel) * dCoef;
                if (value - sustainLevel < 1.0e-4f)
                {
                    value = sustainLevel;
                    stage = Stage::Sustain;
                }
                break;
            case Stage::Sustain:
                value += (sustainLevel - value) * 0.002f;
                break;
            case Stage::Release:
                value *= rCoef;
                if (value < 1.0e-5f)
                {
                    value = 0.0f;
                    stage = Stage::Idle;
                }
                break;
            case Stage::Idle:
                break;
        }
        return value;
    }

private:
    Stage stage = Stage::Idle;
    float sampleRate = 44100.0f;
    float value = 0.0f, aCoef = 0.0f, dCoef = 0.0f, rCoef = 0.0f, sustainLevel = 1.0f;
    int delaySamples = 0, delayCount = 0;
};

class Lfo
{
public:
    explicit Lfo (uint32_t seed = 777u) : rng (seed) {}

    void reset() noexcept { phase = 0.0f; }

    float process (float hz, int wave, float sampleRate) noexcept
    {
        phase += hz / sampleRate;
        if (phase >= 1.0f)
        {
            phase -= std::floor (phase);
            held = rng.bipolar();
            smoothFrom = smoothTo;
            smoothTo = rng.bipolar();
        }

        switch (wave)
        {
            case 0: return std::sin (kTwoPi * phase);
            case 1: return 1.0f - 4.0f * std::abs (phase - 0.5f);
            case 2: return 2.0f * phase - 1.0f;
            case 3: return 1.0f - 2.0f * phase;
            case 4: return phase < 0.5f ? 1.0f : -1.0f;
            case 5: return held;
            default:
            {
                const float t = phase * phase * (3.0f - 2.0f * phase);
                return smoothFrom + (smoothTo - smoothFrom) * t;
            }
        }
    }

private:
    Rng rng;
    float phase = 0.0f, held = 0.0f, smoothFrom = 0.0f, smoothTo = 0.0f;
};
} // namespace mf
