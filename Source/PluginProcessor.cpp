#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <mutex>
#include <thread>

using namespace dsynth;

namespace
{
    // The sample bank (strings, brass, piano) is shared by every D-SYNTH instance and loaded once,
    // on a background thread, from the folder the installer fills.
    struct SharedBank
    {
        std::mutex lock;
        std::shared_ptr<SampleBank> bank;
        std::atomic<bool> loading { false }, done { false };

        static juce::Array<juce::File> candidates()
        {
            const auto common = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory);
            const auto user = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
           #if JUCE_MAC
            return { common.getChildFile ("Application Support/Danova/D-SYNTH/Samples"),
                     user.getChildFile ("Application Support/Danova/D-SYNTH/Samples") };
           #else
            return { common.getChildFile ("Danova/D-SYNTH/Samples"), user.getChildFile ("Danova/D-SYNTH/Samples") };
           #endif
        }

        void startLoading()
        {
            if (loading.exchange (true)) return;
            std::thread ([this]
            {
                auto b = std::make_shared<SampleBank>();
                // FLAC is lossless: the 16-bit samples come back exactly as recorded
                auto decode = [] (const std::string& path, int16_t* dest, int frames, int channels) -> bool
                {
                    juce::FlacAudioFormat flac;
                    std::unique_ptr<juce::AudioFormatReader> r (flac.createReaderFor (new juce::FileInputStream (juce::File (path)), true));
                    if (r == nullptr || (int) r->numChannels != channels || r->lengthInSamples < frames) return false;
                    std::vector<int> a ((size_t) frames), c ((size_t) frames);
                    int* chans[2] = { a.data(), channels == 2 ? c.data() : nullptr };
                    if (! r->read (chans, channels, 0, frames, false)) return false;
                    for (int i = 0; i < frames; ++i)
                        for (int ch = 0; ch < channels; ++ch)
                            dest[(size_t) i * (size_t) channels + (size_t) ch] = (int16_t) ((ch == 0 ? a : c)[(size_t) i] >> 16);
                    return true;
                };
                for (auto& dir : candidates())
                    if (dir.getChildFile ("samples.txt").existsAsFile() && b->load (dir.getFullPathName().toStdString(), decode))
                        break;
                { std::lock_guard<std::mutex> g (lock); bank = b; }
                done = true;
            }).detach();
        }
    };
    SharedBank& sharedBank() { static SharedBank s; return s; }
}

DSynthProcessor::DSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DSYNTH", createLayout())
{
    const auto& table = paramTable();
    for (int i = 0; i < kNumParams; ++i)
        rawParams[(size_t) i] = apvts.getRawParameterValue (table[(size_t) i].id);

    for (auto& n : uiNotes) n = 0;
    setCurrentProgram (0);
    sharedBank().startLoading();
    startTimer (250);
}

DSynthProcessor::~DSynthProcessor() { stopTimer(); }

// hands the sample bank to the engine as soon as the loader has finished
void DSynthProcessor::timerCallback()
{
    auto& sb = sharedBank();
    if (! sb.done) return;
    std::lock_guard<std::mutex> g (sb.lock);
    bankRef = sb.bank;
    engine.setSampleBank (bankRef.get());
    stopTimer();
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
                else if (std::strcmp (p.id, "arpRate") == 0)  for (auto* n : kArpRateNames) choices.add (n);
                else if (std::strcmp (p.id, "arpMode") == 0)  for (auto* n : kArpModeNames) choices.add (n);
                else if (std::strcmp (p.id, "orgReg") == 0)   for (auto* n : kOrgRegNames) choices.add (n);
                else if (std::strcmp (p.id, "orgVib") == 0)   for (auto* n : kOrgVibNames) choices.add (n);
                else if (std::strcmp (p.id, "smpSet") == 0)   for (auto* n : kSmpSetNames) choices.add (n);
                else if (std::strcmp (p.id, "riserBars") == 0) for (auto* n : kRiserBarNames) choices.add (n);
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
    if (m.isNoteOn())
    {
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        uiNotes[(size_t) m.getNoteNumber()] = 1;
    }
    else if (m.isNoteOff())
    {
        engine.noteOff (m.getNoteNumber());
        uiNotes[(size_t) m.getNoteNumber()] = pedalDown ? 2 : 0;
    }
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
    else if (m.isSustainPedalOn())  { engine.sustainPedal (true); pedalDown = true; }
    else if (m.isSustainPedalOff())
    {
        engine.sustainPedal (false); pedalDown = false;
        for (auto& n : uiNotes) if (n.load() == 2) n = 0;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        engine.allNotesOff();
        for (auto& n : uiNotes) n = 0;
    }
}

void DSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) engine.setTempo (*bpm);
            if (auto ppq = pos->getPpqPosition()) engine.setTransport (*ppq, pos->getIsPlaying());
        }

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
