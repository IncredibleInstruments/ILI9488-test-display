// Offline checks: every factory preset must load cleanly and produce finite,
// audible, non-clipping audio from held notes (including arp/seq presets).

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetAnalysis.h"
#include <juce_events/juce_events.h>
#include <iostream>

namespace
{
struct RenderResult
{
    float peak = 0.0f;
    bool finite = true;
};

RenderResult render (MatrixForgeProcessor& proc, double sampleRate, int blockSize, double seconds, std::initializer_list<int> notes)
{
    RenderResult r;
    juce::AudioBuffer<float> buffer (2, blockSize);
    const int totalBlocks = (int) (seconds * sampleRate / blockSize);
    const int releaseBlock = totalBlocks * 2 / 3;

    for (int b = 0; b < totalBlocks; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
        if (b == releaseBlock)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, n), 7);

        proc.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* d = buffer.getReadPointer (ch);
            for (int i = 0; i < blockSize; ++i)
            {
                if (! std::isfinite (d[i]))
                    r.finite = false;
                r.peak = std::max (r.peak, std::abs (d[i]));
            }
        }
    }
    return r;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    int failures = 0;

    MatrixForgeProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    // Analysis mode: MatrixForgeTests --features prints a CSV fingerprint per preset
    if (argc > 1 && juce::String (argv[1]) == "--features")
    {
        std::cout << "index,name";
        for (int k = 0; k < analysis::numFeatures; ++k)
            std::cout << "," << analysis::featureName (k);
        std::cout << "\n";
        for (int i = 0; i < mf::kNumFactoryPresets; ++i)
        {
            const auto bank = mf::getBankNames()[i / mf::kPresetsPerBank];
            const auto f = analysis::analysePreset (proc, i, bank == "Bass" || bank == "Percussion" ? 36 : 60);
            std::cout << i << ",\"" << mf::getFactoryPresetDisplayName (i) << "\"";
            for (float x : f)
                std::cout << "," << x;
            std::cout << "\n";
        }
        return 0;
    }

    // Diagnostic mode: MatrixForgeTests <presetIndex> prints the peak of every 0.1 s
    if (argc > 1)
    {
        proc.setCurrentProgram (juce::String (argv[1]).getIntValue());
        proc.prepareToPlay (48000.0, 480);
        for (int b = 0; b < 30; ++b)
        {
            const auto r = render (proc, 48000.0, 480, 0.1, {});
            std::cout << b * 0.1 << "s peak=" << r.peak << "\n";
            if (b == 0)
            {
                juce::MidiBuffer midi;
                for (int n : { 60, 64, 67 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                juce::AudioBuffer<float> buf (2, 480);
                proc.processBlock (buf, midi);
            }
        }
        return 0;
    }

    if (proc.getNumPrograms() != mf::kNumFactoryPresets || mf::kNumFactoryPresets != 256)
    {
        std::cout << "FAIL: expected 256 factory presets\n";
        ++failures;
    }

    // Every preset parameter ID must exist
    for (int i = 0; i < mf::kNumFactoryPresets; ++i)
        for (auto& [id, value] : mf::getFactoryPresets()[(size_t) i].values)
            if (proc.apvts.getParameter (id) == nullptr)
            {
                std::cout << "FAIL: preset " << mf::getFactoryPresetDisplayName (i) << " uses unknown parameter " << id << "\n";
                ++failures;
            }

    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        proc.setCurrentProgram (i);
        proc.prepareToPlay (48000.0, 512); // clear voices/effects between presets
        const auto bankName = mf::getBankNames()[i / mf::kPresetsPerBank];
        const int root = bankName == "Bass" || bankName == "Percussion" ? 36 : 60;
        const auto r = render (proc, 48000.0, 512, 3.0, { root, root + 4, root + 7 });

        const bool ok = r.finite && r.peak > 0.005f && r.peak <= 1.0f;
        if (! ok)
            ++failures;
        std::cout << (ok ? "ok   " : "FAIL ") << proc.getProgramName (i) << "  peak=" << r.peak << (r.finite ? "" : "  NaN/Inf") << "\n";
    }

    // Odd host block sizes / sample rates
    for (double sr : { 44100.0, 96000.0 })
        for (int bs : { 1, 33, 2048 })
        {
            proc.setCurrentProgram (16 * 7 + 2); // an arp preset
            proc.prepareToPlay (sr, bs);
            const auto r = render (proc, sr, bs, 1.0, { 60, 64 });
            const bool ok = r.finite && r.peak > 0.005f && r.peak <= 1.0f;
            failures += ok ? 0 : 1;
            std::cout << (ok ? "ok   " : "FAIL ") << "sr=" << sr << " block=" << bs << " peak=" << r.peak << "\n";
        }

    // State round trip
    {
        proc.setCurrentProgram (42);
        juce::MemoryBlock state;
        proc.getStateInformation (state);
        proc.setCurrentProgram (0);
        proc.setStateInformation (state.getData(), (int) state.getSize());
        const bool ok = proc.getCurrentProgram() == 42 && proc.apvts.getRawParameterValue ("voiceMode")->load() == (float) mf::modePoly;
        failures += ok ? 0 : 1;
        std::cout << (ok ? "ok   " : "FAIL ") << "state round trip\n";
    }

    // User preset round trip
    {
        auto file = juce::File::createTempFile (".mfpreset");
        proc.setCurrentProgram (5);
        const float cutoff = proc.apvts.getRawParameterValue ("ldCutoff")->load();
        const bool saved = proc.saveUserPreset (file);
        proc.setCurrentProgram (200);
        const bool loaded = proc.loadUserPreset (file);
        const bool ok = saved && loaded && std::abs (proc.apvts.getRawParameterValue ("ldCutoff")->load() - cutoff) < 0.01f;
        failures += ok ? 0 : 1;
        std::cout << (ok ? "ok   " : "FAIL ") << "user preset round trip\n";
        file.deleteFile();
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
