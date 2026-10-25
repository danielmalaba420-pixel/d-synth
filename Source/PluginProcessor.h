#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine/DSynthEngine.h"
#include "Engine/Presets.h"

class DSynthProcessor : public juce::AudioProcessor
{
public:
    DSynthProcessor();
    ~DSynthProcessor() override = default;

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParamsToEngine();
    void handleMidi (const juce::MidiMessage& m);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, dsynth::kNumParams> rawParams {};
    dsynth::Engine engine;
    juce::MidiKeyboardState keyboardState;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSynthProcessor)
};
