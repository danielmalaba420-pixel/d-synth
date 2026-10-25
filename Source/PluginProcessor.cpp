#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace dsynth;

DSynthProcessor::DSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DSYNTH", createLayout())
{
    const auto& table = paramTable();
    for (int i = 0; i < kNumParams; ++i)
        rawParams[(size_t) i] = apvts.getRawParameterValue (table[(size_t) i].id);

    setCurrentProgram (0);
}

juce::AudioProcessorValueTreeState::ParameterLayout DSynthProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto& table = paramTable();

    for (const auto& p : table)
    {
        const juce::ParameterID pid { p.id, 1 };
        switch (p.kind)
        {
            case ParamKind::Float:
            {
                juce::NormalisableRange<float> range (p.min, p.max);
                if (p.skewCentre > p.min && p.skewCentre < p.max)
                    range.setSkewForCentre (p.skewCentre);
                layout.add (std::make_unique<juce::AudioParameterFloat> (
                    pid, p.name, range, p.def,
                    juce::AudioParameterFloatAttributes().withLabel (p.unit)));
                break;
            }
            case ParamKind::Int:
                layout.add (std::make_unique<juce::AudioParameterInt> (
                    pid, p.name, (int) p.min, (int) p.max, (int) p.def,
                    juce::AudioParameterIntAttributes().withLabel (p.unit)));
                break;
            case ParamKind::Choice:
            {
                juce::StringArray choices;
                if (std::strcmp (p.id, "dlyTime") == 0)       for (auto* n : kDelayNames) choices.add (n);
                else if (std::strcmp (p.id, "subOct") == 0)   choices = { "-1 Oct", "-2 Oct" };
                else if (std::strcmp (p.id, "engine") == 0)   for (auto* n : kEngineNames) choices.add (n);
                else if (std::strcmp (p.id, "epModel") == 0)  for (auto* n : kEpModelNames) choices.add (n);
                else                                          for (auto* n : kWaveNames) choices.add (n);
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, p.name, choices, (int) p.def));
                break;
            }
            case ParamKind::Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, p.name, p.def > 0.5f));
                break;
        }
    }
    return layout;
}

bool DSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void DSynthProcessor::prepareToPlay (double sampleRate, int)
{
    pushParamsToEngine();
    engine.prepare (sampleRate);
    keyboardState.reset();
}

void DSynthProcessor::pushParamsToEngine()
{
    for (int i = 0; i < kNumParams; ++i)
        engine.setParam (i, rawParams[(size_t) i]->load());
}

void DSynthProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                      engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())                engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
    {
        const float v = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        engine.pitchBend (v); uiPitchBend = v;
    }
    else if (m.isControllerOfType (1))
    {
        const float v = (float) m.getControllerValue() / 127.0f;
        engine.modWheel (v); uiModWheel = v;
    }
    else if (m.isSustainPedalOn())         engine.sustainPedal (true);
    else if (m.isSustainPedalOff())        engine.sustainPedal (false);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
}

void DSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                engine.setTempo (*bpm);

    pushParamsToEngine();
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    buffer.clear();
    const bool stereo = buffer.getNumChannels() > 1;
    float* L = buffer.getWritePointer (0);
    float* R = stereo ? buffer.getWritePointer (1) : nullptr;

    // render between MIDI events so every note lands on its exact sample
    juce::HeapBlock<float> monoScratch;
    if (! stereo) { monoScratch.allocate ((size_t) numSamples, true); R = monoScratch.get(); }

    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, numSamples, meta.samplePosition);
        if (at > pos) { engine.process (L + pos, R + pos, at - pos); pos = at; }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples) engine.process (L + pos, R + pos, numSamples - pos);

    if (! stereo)
        for (int i = 0; i < numSamples; ++i) L[i] = 0.5f * (L[i] + R[i]);
}

void DSynthProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;

    const auto values = presets[(size_t) index].values();
    const auto& table = paramTable();
    for (int i = 0; i < kNumParams; ++i)
        if (auto* param = apvts.getParameter (table[(size_t) i].id))
            param->setValueNotifyingHost (param->convertTo0to1 (values[(size_t) i]));
}

const juce::String DSynthProcessor::getProgramName (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return {};
    return presets[(size_t) index].name;
}

void DSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) state.getProperty ("program", 0);
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* DSynthProcessor::createEditor()
{
    return new DSynthEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DSynthProcessor();
}
