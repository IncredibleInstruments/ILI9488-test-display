#include "PluginProcessor.h"
#include "PluginEditor.h"

MatrixForgeProcessor::MatrixForgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MatrixForge", mf::createParameterLayout())
{
    refs.init (apvts);
    setCurrentProgram (0);
}

void MatrixForgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare (sampleRate);
    arpSeq.prepare (sampleRate);
    fx.prepare (sampleRate, std::max (samplesPerBlock, 32));
    arpMidi.ensureSize (4096);
    dcCoef = (float) std::exp (-mf::kTwoPi * 8.0 / sampleRate); // ~8 Hz AC coupling
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;
    keyboardState.reset();
}

bool MatrixForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void MatrixForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    refs.load (snapshot);

    double bpm = 120.0;
    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = juce::jlimit (20.0, 400.0, *hostBpm);
    synth.setTempo (bpm);

    arpMidi.clear();
    arpSeq.process (midi, arpMidi, numSamples, snapshot, bpm);

    int pos = 0;
    for (const auto meta : arpMidi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos)
        {
            synth.render (buffer, pos, evPos - pos, snapshot);
            pos = evPos;
        }
        synth.handleMidi (meta.getMessage(), snapshot);
    }
    if (pos < numSamples)
        synth.render (buffer, pos, numSamples - pos, snapshot);

    // Remove DC (e.g. from narrow pulse widths) before it builds up in the delay/reverb
    for (int ch = 0; ch < std::min (2, buffer.getNumChannels()); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float y = d[i] - dcX[ch] + dcCoef * dcY[ch];
            dcX[ch] = d[i];
            dcY[ch] = y;
            d[i] = y;
        }
    }

    fx.process (buffer, snapshot, bpm);

    const float master = snapshot.master * snapshot.master * 1.5f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            d[i] = mf::fastTanh (d[i] * master);
    }

    midi.clear();
}

juce::AudioProcessorEditor* MatrixForgeProcessor::createEditor()
{
    return new MatrixForgeEditor (*this);
}

//==============================================================================
void MatrixForgeProcessor::resetAllParametersToDefault()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

void MatrixForgeProcessor::notifyPresetChanged()
{
    juce::MessageManager::callAsync ([safe = juce::WeakReference<MatrixForgeProcessor> (this)]
    {
        if (auto* self = safe.get())
            if (self->onPresetChanged)
                self->onPresetChanged();
    });
}

void MatrixForgeProcessor::setCurrentProgram (int index)
{
    const auto& presets = mf::getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram = index;
    resetAllParametersToDefault();
    for (const auto& [id, value] : presets[(size_t) index].values)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
        else
            jassertfalse; // unknown parameter ID in a factory preset
    }
    currentPresetName = mf::getFactoryPresetDisplayName (index);
    notifyPresetChanged();
}

void MatrixForgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("editorScale", editorScale, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MatrixForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    currentProgram = state.getProperty ("program", 0);
    currentPresetName = state.getProperty ("presetName", mf::getFactoryPresetDisplayName (currentProgram));
    editorScale = (float) (double) state.getProperty ("editorScale", 1.0);
    apvts.replaceState (state);
    notifyPresetChanged();
}

//==============================================================================
juce::File MatrixForgeProcessor::getUserPresetFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("MatrixForge").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

bool MatrixForgeProcessor::saveUserPreset (const juce::File& file)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
    auto xml = state.createXml();
    if (xml == nullptr || ! xml->writeTo (file))
        return false;
    currentPresetName = "USER " + file.getFileNameWithoutExtension();
    notifyPresetChanged();
    return true;
}

bool MatrixForgeProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    resetAllParametersToDefault();
    auto state = juce::ValueTree::fromXml (*xml);
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild (i);
        if (auto* param = apvts.getParameter (child.getProperty ("id").toString()))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) (double) child.getProperty ("value")));
    }
    currentPresetName = "USER " + file.getFileNameWithoutExtension();
    notifyPresetChanged();
    return true;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MatrixForgeProcessor();
}
