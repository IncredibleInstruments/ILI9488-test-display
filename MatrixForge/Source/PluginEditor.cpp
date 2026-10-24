#include "PluginEditor.h"

using namespace mf;

MainPanel::MainPanel (MatrixForgeProcessor& p)
    : processor (p),
      matrix (p.apvts),
      seq (p.apvts),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    setSize (designWidth, designHeight);

    // ---- Header
    for (auto* b : { &prevButton, &nextButton, &presetsButton, &saveButton, &loadButton, &sizeButton })
        addAndMakeVisible (*b);
    prevButton.setBounds (860, 14, 32, 36);
    nextButton.setBounds (896, 14, 32, 36);
    presetsButton.setBounds (936, 14, 92, 36);
    saveButton.setBounds (1034, 14, 62, 36);
    loadButton.setBounds (1100, 14, 62, 36);
    sizeButton.setBounds (1166, 14, 62, 36);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    presetsButton.onClick = [this] { showPresetMenu(); };
    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPreset(); };
    sizeButton.onClick = [this] { showSizeMenu(); };
    knob ("master", "MASTER", 1450, 4, 40);

    // ---- Row 1: oscillators and mixer
    section ("VCO 1", 10, 72, 360, 230);
    knob ("vco1Coarse", "COARSE", 53, 102);
    knob ("vco1Fine", "FINE", 123, 102);
    knob ("vco1PW", "PULSE W", 193, 102);
    knob ("vco1FM", "FM 3>1", 263, 102);
    knob ("vco1Ultra", "ULTRASAW", 333, 102);
    knob ("vco1Saw", "SAW", 53, 200);
    knob ("vco1Square", "SQUARE", 123, 200);
    knob ("vco1Tri", "TRIANGLE", 193, 200);
    knob ("vco1Sub", "SUB", 263, 200);
    knob ("vco1Metal", "METALIZER", 333, 200);

    section ("VCO 2", 380, 72, 360, 230);
    knob ("vco2Coarse", "COARSE", 423, 102);
    knob ("vco2Fine", "FINE", 493, 102);
    knob ("vco2PW", "PULSE W", 563, 102);
    knob ("vco2Ultra", "ULTRASAW", 633, 102);
    toggle ("vco2Sync", "SYNC", 672, 120, 62, 26);
    knob ("vco2Saw", "SAW", 423, 200);
    knob ("vco2Square", "SQUARE", 493, 200);
    knob ("vco2Tri", "TRIANGLE", 563, 200);
    knob ("vco2Sub", "SUB", 633, 200);
    knob ("vco2Metal", "METALIZER", 703, 200);

    section ("VCO 3 / LFO 3  +  NOISE", 750, 72, 300, 230);
    knob ("vco3Coarse", "COARSE", 793, 102);
    knob ("vco3Fine", "FINE", 863, 102);
    knob ("vco3Rate", "LFO RATE", 933, 102);
    combo ("vco3Wave", 970, 108, 72);
    toggle ("vco3Lfo", "LFO", 970, 140, 72);
    knob ("noiseColor", "NOISE COLOR", 793, 200);

    section ("MIXER", 1060, 72, 430, 230);
    const char* mixIds[] { "mixVco1", "mixVco2", "mixVco3", "mixNoise" };
    const char* routeIds[] { "routeVco1", "routeVco2", "routeVco3", "routeNoise" };
    const char* mixNames[] { "VCO 1", "VCO 2", "VCO 3", "NOISE" };
    for (int c = 0; c < 4; ++c)
    {
        const int cx = 1060 + 54 + c * 107;
        knob (mixIds[c], mixNames[c], cx, 102);
        combo (routeIds[c], cx - 45, 214, 90);
    }

    // ---- Row 2: filters, global, envelopes
    section ("STEINER FILTER", 10, 310, 300, 230);
    combo ("stMode", 20, 360, 68);
    knob ("stCutoff", "CUTOFF", 126, 340);
    knob ("stReso", "RESONANCE", 198, 340);
    knob ("stDrive", "DRIVE", 270, 340);
    knob ("stBrute", "BRUTE", 54, 436);
    knob ("stEnvAmt", "ENV AMT", 126, 436);
    knob ("stKeyTrack", "KEY TRK", 198, 436);
    knob ("stOut", "OUTPUT", 270, 436);

    section ("LADDER FILTER", 320, 310, 300, 230);
    combo ("ldMode", 330, 360, 68);
    knob ("ldCutoff", "CUTOFF", 436, 340);
    knob ("ldReso", "RESONANCE", 508, 340);
    knob ("ldDrive", "DRIVE", 580, 340);
    knob ("ldEnvAmt", "ENV AMT", 364, 436);
    knob ("ldKeyTrack", "KEY TRK", 436, 436);
    knob ("ldOut", "OUTPUT", 508, 436);
    combo ("filterConfig", 546, 456, 70);

    section ("VOICE", 630, 310, 230, 230);
    combo ("voiceMode", 642, 342, 206, 24);
    knob ("glide", "GLIDE", 673, 380);
    knob ("bendRange", "BEND", 745, 380);
    knob ("drift", "DRIFT", 817, 380);

    section ("ENV 1  FILTER", 870, 310, 200, 230);
    section ("ENV 2  VCA", 1080, 310, 200, 230);
    section ("ENV 3  MOD", 1290, 310, 200, 230);
    const char* envLabels[] { "A", "D", "S", "R", "VEL" };
    for (int e = 0; e < 3; ++e)
    {
        const juce::String prefix = "env" + juce::String (e + 1);
        const int x0 = 870 + e * 210;
        for (int k = 0; k < 5; ++k)
        {
            juce::String id, text;
            if (e == 2)
            {
                const char* ids[] { "env3Delay", "env3A", "env3D", "env3S", "env3R" };
                const char* names[] { "DLY", "A", "D", "S", "R" };
                id = ids[k];
                text = names[k];
            }
            else
            {
                const char* suffix[] { "A", "D", "S", "R", "Vel" };
                id = prefix + suffix[k];
                text = envLabels[k];
            }
            fader (id, text, x0 + 20 + k * 40, 340, 172);
        }
    }

    // ---- Row 3: LFOs, matrix, sequencer, effects
    for (int l = 0; l < 2; ++l)
    {
        const int y = 548 + l * 168;
        const juce::String prefix = "lfo" + juce::String (l + 1);
        section ("LFO " + juce::String (l + 1), 10, y, 220, 160);
        combo (prefix + "Wave", 20, y + 30, 104);
        combo (prefix + "Div", 20, y + 60, 104);
        toggle (prefix + "Sync", "SYNC", 20, y + 92, 104);
        toggle (prefix + "Retrig", "RETRIG", 20, y + 120, 104);
        knob (prefix + "Rate", "RATE", 176, y + 50, 64);
    }

    section ("MODULATION MATRIX", 240, 548, 560, 330);
    addAndMakeVisible (matrix);
    matrix.setBounds (248, 574, 544, 296);
    matrix.onHover = [this] (const juce::String& n, const juce::String& v) { showParameter (n, v); };

    section ("ARPEGGIATOR / SEQUENCER", 810, 548, 400, 330);
    combo ("asMode", 820, 578, 90);
    combo ("asDiv", 916, 578, 72);
    combo ("arpPattern", 994, 578, 100);
    knob ("asGate", "GATE", 842, 610, 44);
    knob ("arpOct", "OCTAVES", 896, 610, 44);
    knob ("seqLength", "LENGTH", 950, 610, 44);
    knob ("seqSwing", "SWING", 1004, 610, 44);
    addAndMakeVisible (seq);
    seq.setBounds (820, 676, 380, 194);
    seq.onHover = [this] (const juce::String& n, const juce::String& v) { showParameter (n, v); };

    section ("EFFECTS", 1220, 548, 270, 330);
    combo ("chType", 1230, 592, 70);
    knob ("chRate", "RATE", 1330, 578, 46);
    knob ("chDepth", "DEPTH", 1390, 578, 46);
    knob ("chMix", "MIX", 1450, 578, 46);
    toggle ("dlOn", "DELAY", 1230, 666, 70);
    toggle ("dlSync", "SYNC", 1230, 692, 70);
    toggle ("dlPingPong", "PING", 1230, 718, 70);
    combo ("dlDiv", 1230, 744, 70);
    knob ("dlTime", "TIME", 1330, 672, 46);
    knob ("dlFeedback", "FEEDBK", 1390, 672, 46);
    knob ("dlMix", "MIX", 1450, 672, 46);
    toggle ("rvOn", "REVERB", 1230, 798, 70);
    knob ("rvSize", "SIZE", 1330, 784, 46);
    knob ("rvDamp", "DAMP", 1390, 784, 46);
    knob ("rvMix", "MIX", 1450, 784, 46);

    // ---- Keyboard
    addAndMakeVisible (keyboard);
    keyboard.setBounds (10, 886, 1480, 66);
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (1480.0f / 50.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::accent.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::accent.withAlpha (0.25f));

    refreshPresetName();
    startTimerHz (30);
}

MainPanel::~MainPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MainPanel::section (const juce::String& title, int x, int y, int w, int h)
{
    sections.push_back ({ title, { x, y, w, h } });
}

void MainPanel::knob (const juce::String& id, const juce::String& text, int centreX, int y, int size)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->slider.setBounds (centreX - size / 2, y, size, size);
    k->slider.setPopupMenuEnabled (false);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k->slider);

    auto* param = processor.apvts.getParameter (id);
    k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    k->slider.onValueChange = [this, param, s = &k->slider]
    {
        if (s->isMouseOverOrDragging())
            showParameter (param->getName (64), param->getCurrentValueAsText());
    };
    k->slider.onDragStart = k->slider.onValueChange;

    k->label.setText (text, juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (size < 50 ? 9.5f : 10.5f, juce::Font::bold)));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setBounds (centreX - 40, y + size - 2, 80, 14);
    k->label.setInterceptsMouseClicks (false, false);

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    knobs.push_back (std::move (k));
}

void MainPanel::fader (const juce::String& id, const juce::String& text, int centreX, int y, int height)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::LinearVertical);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->slider.setBounds (centreX - 16, y, 32, height);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k->slider);

    auto* param = processor.apvts.getParameter (id);
    k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    k->slider.onValueChange = [this, param, s = &k->slider]
    {
        if (s->isMouseOverOrDragging())
            showParameter (param->getName (64), param->getCurrentValueAsText());
    };
    k->slider.onDragStart = k->slider.onValueChange;

    k->label.setText (text, juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setBounds (centreX - 20, y + height + 2, 40, 14);

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    knobs.push_back (std::move (k));
}

void MainPanel::combo (const juce::String& id, int x, int y, int w, int h)
{
    auto c = std::make_unique<Combo>();
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (id)))
        c->box.addItemList (choice->choices, 1);
    c->box.setBounds (x, y, w, h);
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, id, c->box);
    addAndMakeVisible (c->box);
    combos.push_back (std::move (c));
}

void MainPanel::toggle (const juce::String& id, const juce::String& text, int x, int y, int w, int h)
{
    auto t = std::make_unique<Toggle>();
    t->button.setButtonText (text);
    t->button.setBounds (x, y, w, h);
    t->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, id, t->button);
    addAndMakeVisible (t->button);
    toggles.push_back (std::move (t));
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    // Brushed header strip
    juce::ColourGradient header (juce::Colour (0xff23282f), 0, 0, juce::Colour (0xff15181c), 0, 64, false);
    g.setGradientFill (header);
    g.fillRect (0, 0, designWidth, 64);
    g.setColour (colours::panelEdge);
    g.drawHorizontalLine (64, 0.0f, (float) designWidth);

    g.setColour (colours::text);
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("MATRIX", 18, 8, 120, 34, juce::Justification::centredLeft);
    g.setColour (colours::accent);
    g.drawText ("FORGE", 132, 8, 110, 34, juce::Justification::centredLeft);
    g.setColour (colours::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("ANALOG MATRIX SYNTHESIZER", 20, 40, 260, 14, juce::Justification::centredLeft);

    // LCD
    auto lcd = lcdArea.toFloat();
    g.setColour (colours::lcdBack);
    g.fillRoundedRectangle (lcd, 5.0f);
    g.setColour (colours::panelEdge.brighter (0.2f));
    g.drawRoundedRectangle (lcd, 5.0f, 1.5f);
    g.setColour (colours::lcdText);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::bold)));
    g.drawText (presetLine, lcdArea.reduced (12, 4).removeFromTop (22), juce::Justification::centredLeft, true);
    g.setColour (colours::lcdText.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain)));
    g.drawText (paramLine, lcdArea.reduced (12, 4).removeFromBottom (18), juce::Justification::centredLeft, true);

    // Sections
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        g.setColour (colours::panel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (colours::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (colours::accent);
        g.fillRect (r.getX() + 10.0f, r.getY() + 9.0f, 3.0f, 10.0f);
        g.setColour (colours::text);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (s.title, (int) r.getX() + 18, (int) r.getY() + 5, (int) r.getWidth() - 24, 18, juce::Justification::centredLeft);
    }

    g.setColour (colours::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("FILTER ROUTING", 1070, 190, 410, 16, juce::Justification::centred);
    g.drawText ("MODE", 20, 344, 68, 14, juce::Justification::centredLeft);
    g.drawText ("MODE", 330, 344, 68, 14, juce::Justification::centredLeft);
    g.drawText ("CONFIG", 546, 440, 70, 14, juce::Justification::centredLeft);
    g.drawText ("WAVE", 970, 92, 72, 14, juce::Justification::centredLeft);
}

void MainPanel::showParameter (const juce::String& name, const juce::String& value)
{
    paramLine = name + ": " + value;
    paramShownAt = juce::Time::getMillisecondCounterHiRes();
    repaint (lcdArea);
}

void MainPanel::refreshPresetName()
{
    presetLine = processor.getCurrentPresetName();
    repaint (lcdArea);
}

void MainPanel::timerCallback()
{
    seq.setPlayingStep (processor.getArpSeqStep());

    if (paramLine.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() - paramShownAt > 2500.0)
    {
        paramLine = {};
        repaint (lcdArea);
    }
}

void MainPanel::stepPreset (int delta)
{
    const int n = processor.getNumPrograms();
    processor.setCurrentProgram ((processor.getCurrentProgram() + delta + n) % n);
}

void MainPanel::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& presets = getFactoryPresets();
    for (int b = 0; b < kNumBanks; ++b)
    {
        juce::PopupMenu bank;
        for (int i = 0; i < kPresetsPerBank; ++i)
        {
            const int index = b * kPresetsPerBank + i;
            bank.addItem (index + 1, juce::String (i + 1).paddedLeft ('0', 2) + "  " + presets[(size_t) index].name, true,
                          index == processor.getCurrentProgram());
        }
        menu.addSubMenu (juce::String::charToString ((juce::juce_wchar) ('A' + b)) + "  " + getBankNames()[b], bank);
    }

    auto userFiles = MatrixForgeProcessor::getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.mfpreset");
    userFiles.sort();
    juce::PopupMenu user;
    for (int i = 0; i < userFiles.size(); ++i)
        user.addItem (10000 + i, userFiles[i].getFileNameWithoutExtension());
    menu.addSeparator();
    menu.addSubMenu ("User Presets", user, ! userFiles.isEmpty());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetsButton),
                        [this, userFiles] (int result)
                        {
                            if (result >= 10000)
                                processor.loadUserPreset (userFiles[result - 10000]);
                            else if (result > 0)
                                processor.setCurrentProgram (result - 1);
                        });
}

void MainPanel::showSizeMenu()
{
    juce::PopupMenu menu;
    const float scales[] { 0.5f, 0.625f, 0.75f, 0.875f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
    for (int i = 0; i < 9; ++i)
        menu.addItem (i + 1, juce::String (juce::roundToInt (scales[i] * 100.0f)) + " %", true,
                      std::abs (processor.editorScale - scales[i]) < 0.01f);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (sizeButton), [this, scales] (int result)
    {
        if (result > 0 && onScaleChosen)
            onScaleChosen (scales[result - 1]);
    });
}

void MainPanel::savePreset()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Save preset", MatrixForgeProcessor::getUserPresetFolder(), "*.mfpreset");
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto file = fc.getResult();
                                  if (file != juce::File())
                                      processor.saveUserPreset (file.withFileExtension ("mfpreset"));
                              });
}

void MainPanel::loadPreset()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Load preset", MatrixForgeProcessor::getUserPresetFolder(), "*.mfpreset");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto file = fc.getResult();
                                  if (file.existsAsFile())
                                      processor.loadUserPreset (file);
                              });
}

//==============================================================================
MatrixForgeEditor::MatrixForgeEditor (MatrixForgeProcessor& p)
    : AudioProcessorEditor (p), processor (p), panel (p)
{
    addAndMakeVisible (panel);

    const double aspect = (double) MainPanel::designWidth / (double) MainPanel::designHeight;
    setResizable (true, true);
    setResizeLimits (MainPanel::designWidth / 2, MainPanel::designHeight / 2, MainPanel::designWidth * 2, MainPanel::designHeight * 2);
    getConstrainer()->setFixedAspectRatio (aspect);

    float scale = processor.editorScale;
    if (scale <= 0.0f || scale > 2.0f)
        scale = 1.0f;
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto area = display->userArea;
        const float fit = juce::jmin ((float) (area.getHeight() - 80) / (float) MainPanel::designHeight,
                                      (float) (area.getWidth() - 40) / (float) MainPanel::designWidth);
        scale = juce::jmax (0.5f, juce::jmin (scale, fit));
    }

    panel.onScaleChosen = [this] (float s)
    {
        setSize (juce::roundToInt (MainPanel::designWidth * s), juce::roundToInt (MainPanel::designHeight * s));
    };
    processor.onPresetChanged = [this] { panel.refreshPresetName(); };

    setSize (juce::roundToInt (MainPanel::designWidth * scale), juce::roundToInt (MainPanel::designHeight * scale));
}

MatrixForgeEditor::~MatrixForgeEditor()
{
    processor.onPresetChanged = nullptr;
}

void MatrixForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
}

void MatrixForgeEditor::resized()
{
    const float scale = (float) getWidth() / (float) MainPanel::designWidth;
    panel.setTransform (juce::AffineTransform::scale (scale));
    panel.setBounds (0, 0, MainPanel::designWidth, MainPanel::designHeight);
    processor.editorScale = scale;
}
