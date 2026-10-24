#pragma once

#include "PluginProcessor.h"
#include "ui/ForgeLookAndFeel.h"
#include "ui/ModMatrixComponent.h"
#include "ui/SeqComponent.h"

// The whole front panel, laid out at a fixed design size. The editor scales it
// with an affine transform so every window size stays crisp and proportional.
class MainPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int designWidth = 1500;
    static constexpr int designHeight = 960;

    explicit MainPanel (MatrixForgeProcessor&);
    ~MainPanel() override;

    void paint (juce::Graphics&) override;
    void refreshPresetName();

    std::function<void (float)> onScaleChosen;

private:
    struct Section
    {
        juce::String title;
        juce::Rectangle<int> bounds;
    };

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Combo
    {
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void section (const juce::String& title, int x, int y, int w, int h);
    void knob (const juce::String& id, const juce::String& text, int centreX, int y, int size = 56);
    void fader (const juce::String& id, const juce::String& text, int centreX, int y, int height);
    void combo (const juce::String& id, int x, int y, int w, int h = 22);
    void toggle (const juce::String& id, const juce::String& text, int x, int y, int w, int h = 22);
    void showParameter (const juce::String& name, const juce::String& value);
    void showPresetMenu();
    void showSizeMenu();
    void stepPreset (int delta);
    void savePreset();
    void loadPreset();
    void timerCallback() override;

    MatrixForgeProcessor& processor;
    mf::ForgeLookAndFeel lnf;

    std::vector<Section> sections;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Combo>> combos;
    std::vector<std::unique_ptr<Toggle>> toggles;

    mf::ModMatrixComponent matrix;
    mf::SeqComponent seq;
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, presetsButton { "PRESETS" },
        saveButton { "SAVE" }, loadButton { "LOAD" }, sizeButton { "SIZE" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Rectangle<int> lcdArea { 330, 8, 520, 48 };
    juce::String presetLine, paramLine;
    double paramShownAt = 0.0;
};

class MatrixForgeEditor : public juce::AudioProcessorEditor
{
public:
    explicit MatrixForgeEditor (MatrixForgeProcessor&);
    ~MatrixForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MatrixForgeProcessor& processor;
    MainPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatrixForgeEditor)
};
