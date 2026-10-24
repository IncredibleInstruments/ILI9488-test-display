#pragma once

// Renders a preset and reduces it to a small timbre/motion fingerprint, so the
// factory banks can be checked for variety (no near-duplicates) and movement.

#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace analysis
{
enum Feature { brightness, timbreMotion, attack, sustain, release, ampMotion, width, noisiness, pitchMotion, numFeatures };

inline const char* featureName (int f)
{
    static const char* names[] { "bright", "tmotion", "attack", "sustain", "release", "amotion", "width", "noise", "pmotion" };
    return names[f];
}

using Fingerprint = std::array<float, numFeatures>;

inline Fingerprint analysePreset (MatrixForgeProcessor& proc, int presetIndex, int rootNote)
{
    constexpr double sr = 48000.0;
    constexpr int block = 512, fftOrder = 11, fftSize = 1 << fftOrder, hop = 1024;
    const int holdSamples = (int) (2.0 * sr), totalSamples = (int) (3.0 * sr);

    proc.setCurrentProgram (presetIndex);
    proc.prepareToPlay (sr, block);

    std::vector<float> mid ((size_t) totalSamples), side ((size_t) totalSamples);
    juce::AudioBuffer<float> buffer (2, block);
    const int notes[] { rootNote, rootNote + 7, rootNote + 12 };
    const bool poly = proc.apvts.getRawParameterValue ("voiceMode")->load() >= 2.0f;

    for (int pos = 0; pos < totalSamples; pos += block)
    {
        juce::MidiBuffer midi;
        if (pos == 0)
            for (int i = 0; i < (poly ? 3 : 1); ++i)
                midi.addEvent (juce::MidiMessage::noteOn (1, notes[i], (juce::uint8) 100), 0);
        if (pos <= holdSamples && pos + block > holdSamples)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, n), holdSamples - pos);
        proc.processBlock (buffer, midi);
        for (int i = 0; i < block && pos + i < totalSamples; ++i)
        {
            const float l = buffer.getSample (0, i), r = buffer.getSample (1, i);
            mid[(size_t) (pos + i)] = 0.5f * (l + r);
            side[(size_t) (pos + i)] = 0.5f * (l - r);
        }
    }

    juce::dsp::FFT fft (fftOrder);
    juce::dsp::WindowingFunction<float> window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> work ((size_t) fftSize * 2);

    struct Frame { float rms, centroid, flatness, peakBin, time; };
    std::vector<Frame> frames;
    double sideEnergy = 0.0, midEnergy = 0.0;

    for (int start = 0; start + fftSize <= totalSamples; start += hop)
    {
        std::fill (work.begin(), work.end(), 0.0f);
        double energy = 0.0;
        for (int i = 0; i < fftSize; ++i)
        {
            const float s = mid[(size_t) (start + i)];
            work[(size_t) i] = s;
            energy += s * s;
            if (start + fftSize <= holdSamples)
            {
                midEnergy += s * s;
                sideEnergy += side[(size_t) (start + i)] * side[(size_t) (start + i)];
            }
        }
        window.multiplyWithWindowingTable (work.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (work.data());

        double weighted = 0.0, total = 0.0, logSum = 0.0;
        float peakMag = 0.0f;
        int peakBin = 1;
        const int bins = fftSize / 2;
        for (int k = 1; k < bins; ++k)
        {
            const double m = work[(size_t) k] + 1.0e-9;
            weighted += m * k;
            total += m;
            logSum += std::log (m);
            if (work[(size_t) k] > peakMag)
            {
                peakMag = work[(size_t) k];
                peakBin = k;
            }
        }
        const double centroidHz = total > 0.0 ? weighted / total * sr / fftSize : 0.0;
        const double flat = std::exp (logSum / (bins - 1)) / (total / (bins - 1));
        frames.push_back ({ (float) std::sqrt (energy / fftSize), (float) centroidHz, (float) flat, (float) peakBin,
                            (float) (start + fftSize / 2) / (float) sr });
    }

    Fingerprint f {};
    float maxRms = 1.0e-6f;
    for (auto& fr : frames)
        maxRms = std::max (maxRms, fr.rms);

    // Attack: time to reach 70 % of the loudest frame
    float attackTime = 2.0f;
    for (auto& fr : frames)
        if (fr.rms >= 0.7f * maxRms) { attackTime = fr.time; break; }
    f[attack] = std::log2 (std::max (attackTime, 0.02f));

    // Statistics over the audible part of the hold phase
    std::vector<float> logCentroid, rmsHold, peakHold;
    double flatSum = 0.0;
    float lateRms = 0.0f;
    int lateCount = 0;
    for (auto& fr : frames)
    {
        if (fr.time > 2.0f || fr.rms < 0.05f * maxRms)
            continue;
        logCentroid.push_back (std::log2 (std::max (fr.centroid, 30.0f)));
        rmsHold.push_back (fr.rms);
        peakHold.push_back (std::log2 (fr.peakBin));
        flatSum += fr.flatness;
        if (fr.time > 1.5f) { lateRms += fr.rms; ++lateCount; }
    }

    auto meanStd = [] (const std::vector<float>& v, float& mean, float& sd)
    {
        mean = 0.0f;
        sd = 0.0f;
        if (v.empty()) return;
        for (float x : v) mean += x;
        mean /= (float) v.size();
        for (float x : v) sd += (x - mean) * (x - mean);
        sd = std::sqrt (sd / (float) v.size());
    };

    float m, sd;
    meanStd (logCentroid, m, sd);
    f[brightness] = m;
    f[timbreMotion] = sd;
    meanStd (rmsHold, m, sd);
    f[ampMotion] = m > 0.0f ? sd / m : 0.0f;
    meanStd (peakHold, m, sd);
    f[pitchMotion] = sd;
    f[noisiness] = rmsHold.empty() ? 0.0f : (float) (flatSum / (double) rmsHold.size());
    f[sustain] = 20.0f * std::log10 (std::max (lateCount > 0 ? lateRms / (float) lateCount / maxRms : 0.0f, 1.0e-3f)) / 20.0f;

    float tailRms = 0.0f, atRelease = 1.0e-6f;
    for (auto& fr : frames)
    {
        if (std::abs (fr.time - 1.95f) < 0.03f) atRelease = std::max (atRelease, fr.rms);
        if (fr.time > 2.45f && fr.time < 2.6f) tailRms = std::max (tailRms, fr.rms);
    }
    f[release] = std::log10 (std::max (tailRms / atRelease, 1.0e-3f));
    f[width] = (float) std::sqrt (sideEnergy / std::max (midEnergy, 1.0e-12));
    return f;
}

struct BankReport
{
    float meanDistance = 0.0f, minDistance = 0.0f;
    int closestA = 0, closestB = 0;
};

// Distances use each feature divided by its spread across all presets.
inline std::vector<BankReport> bankReports (const std::vector<Fingerprint>& prints, int banks, int perBank, Fingerprint& spread)
{
    for (int k = 0; k < numFeatures; ++k)
    {
        double mean = 0.0, var = 0.0;
        for (auto& p : prints) mean += p[(size_t) k];
        mean /= (double) prints.size();
        for (auto& p : prints) var += (p[(size_t) k] - mean) * (p[(size_t) k] - mean);
        spread[(size_t) k] = (float) std::max (std::sqrt (var / (double) prints.size()), 1.0e-4);
    }

    auto dist = [&spread] (const Fingerprint& a, const Fingerprint& b)
    {
        double d = 0.0;
        for (int k = 0; k < numFeatures; ++k)
        {
            const double x = (a[(size_t) k] - b[(size_t) k]) / spread[(size_t) k];
            d += x * x;
        }
        return (float) std::sqrt (d);
    };

    std::vector<BankReport> reports;
    for (int b = 0; b < banks; ++b)
    {
        BankReport r;
        r.minDistance = 1.0e9f;
        int pairs = 0;
        for (int i = 0; i < perBank; ++i)
            for (int j = i + 1; j < perBank; ++j)
            {
                const float d = dist (prints[(size_t) (b * perBank + i)], prints[(size_t) (b * perBank + j)]);
                r.meanDistance += d;
                ++pairs;
                if (d < r.minDistance) { r.minDistance = d; r.closestA = i; r.closestB = j; }
            }
        r.meanDistance /= (float) pairs;
        reports.push_back (r);
    }
    return reports;
}
} // namespace analysis
