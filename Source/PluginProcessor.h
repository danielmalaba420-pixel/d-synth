#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine/DSynthEngine.h"
#include "Engine/Presets.h"

class DSynthProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    DSynthProcessor();
    ~DSynthProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "D-SYNTH"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    // The 12 factory leads are exposed as programs, so every DAW can browse them
    int getNumPrograms() override { return (int) dsynth::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() { return apvts; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Live performance values for the editor (wheels on screen follow the keyboard)
    std::atomic<float> uiPitchBend { 0.0f }, uiModWheel { 0.0f };
    // what the on-screen keyboard shows: 0 = up, 1 = key down, 2 = held by the sustain pedal
    std::array<std::atomic<uint8_t>, 128> uiNotes {};
    int lastFx() const { return engine.lastFx(); }
    float tempo() const { return engine.getTempo(); }
    bool leslieFast() const { return engine.leslieFast(); }
    bool samplesReady() const { return engine.samplesReady(); }
    // front-panel browsing (not saved with the song)
    juce::String uiCategory { "TOUS" };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParamsToEngine();
    void handleMidi (const juce::MidiMessage& m);
    void timerCallback() override;

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, dsynth::kNumParams> rawParams {};
    dsynth::Engine engine;
    juce::MidiKeyboardState keyboardState;
    int currentProgram = 0;
    bool pedalDown = false;
    std::shared_ptr<const dsynth::SampleBank> bankRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSynthProcessor)
};
