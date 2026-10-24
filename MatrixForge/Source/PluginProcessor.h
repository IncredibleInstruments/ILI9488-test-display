#pragma once

#include "Params.h"
#include "Presets.h"
#include "dsp/ArpSeq.h"
#include "dsp/FxChain.h"
#include "dsp/Synth.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class MatrixForgeProcessor : public juce::AudioProcessor
{
public:
    MatrixForgeProcessor();
    ~MatrixForgeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MatrixForge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Factory presets are exposed to the host as programs
    int getNumPrograms() override { return mf::kNumFactoryPresets; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override { return mf::getFactoryPresetDisplayName (index); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // User presets
    static juce::File getUserPresetFolder();
    bool saveUserPreset (const juce::File& file);
    bool loadUserPreset (const juce::File& file);

    juce::String getCurrentPresetName() const { return currentPresetName; }
    int getArpSeqStep() const { return arpSeq.currentStep.load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    float editorScale = 1.0f;
    std::function<void()> onPresetChanged; // message thread only

private:
    void resetAllParametersToDefault();
    void notifyPresetChanged();

    mf::ParamRefs refs;
    mf::Snapshot snapshot;
    mf::Synth synth;
    mf::ArpSeq arpSeq;
    mf::FxChain fx;
    juce::MidiBuffer arpMidi;
    float dcX[2] {}, dcY[2] {}, dcCoef = 0.999f;

    int currentProgram = 0;
    juce::String currentPresetName;

    JUCE_DECLARE_WEAK_REFERENCEABLE (MatrixForgeProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatrixForgeProcessor)
};
