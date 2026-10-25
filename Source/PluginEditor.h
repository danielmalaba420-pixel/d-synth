#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

//==============================================================================
// Fonts, colours and knob drawing for the D-SYNTH façade
class DSynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DSynthLookAndFeel();

    juce::Font logo (float h) const;     // Michroma
    juce::Font label (float h) const;    // Barlow Condensed SemiBold
    juce::Font labelBold (float h) const;
    juce::Font screen (float h) const;   // Share Tech Mono

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;

    static const juce::Colour amber, amberHot, amberDim, ink, textSoft, panelTop, panelBottom;

private:
    juce::Typeface::Ptr tfLogo, tfLabel, tfLabelBold, tfScreen;
};

//==============================================================================
// Pitch / mod wheel that follows the hardware wheels
class Wheel : public juce::Component
{
public:
    explicit Wheel (bool bipolarIn) : bipolar (bipolarIn) {}
    void setValue (float v);
    void paint (juce::Graphics&) override;

private:
    bool bipolar;
    float value = 0.0f;
};

//==============================================================================
// The amber screen: lead number, name, description, prev/next, delay echoes
class LeadScreen : public juce::Component
{
public:
    explicit LeadScreen (DSynthLookAndFeel&);

    struct Info
    {
        int index = 0;
        juce::String name, description, delayDiv, routing, delayMode;
        float delayMix = 0.0f, delayFb = 0.0f;
        bool operator!= (const Info& o) const
        {
            return index != o.index || name != o.name || delayDiv != o.delayDiv || routing != o.routing
                || delayMode != o.delayMode || std::abs (delayMix - o.delayMix) > 0.005f
                || std::abs (delayFb - o.delayFb) > 0.005f;
        }
    };

    void setInfo (const Info&);
    void paint (juce::Graphics&) override;
    void resized() override;

    juce::TextButton prev { juce::String::fromUTF8 ("\xe2\x80\xb9") }, next { juce::String::fromUTF8 ("\xe2\x80\xba") };

private:
    DSynthLookAndFeel& lnf;
    Info info;
};

//==============================================================================
// White keys with a rainbow strip (Kontakt style), black keys plain
class RainbowKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit RainbowKeyboard (juce::MidiKeyboardState&);
    static juce::Colour zoneColour (int note);

protected:
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;
};

//==============================================================================
// The whole instrument, drawn at its native 1320 x 780 size; the editor scales it
class SynthPanel : public juce::Component
{
public:
    SynthPanel (DSynthProcessor&, DSynthLookAndFeel&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();   // called by the editor's timer

    static constexpr int W = 1320, H = 780;

private:
    void stepProgram (int delta);
    void drawFrame (juce::Graphics&);
    void drawLogo (juce::Graphics&);
    void drawScrew (juce::Graphics&, float cx, float cy);

    DSynthProcessor& proc;
    DSynthLookAndFeel& lnf;
    juce::Image wood;

    struct Knob
    {
        juce::Slider slider;
        juce::String text;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    std::array<Knob, 6> knobs;
    juce::Slider build;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> buildAttach;

    Wheel pitchWheel { true }, modWheel { false };
    LeadScreen screen;
    RainbowKeyboard keyboard;

    int shownProgram = -1;
    bool glideMode = false;
};

//==============================================================================
class DSynthEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DSynthEditor (DSynthProcessor&);
    ~DSynthEditor() override;

    void resized() override;

private:
    void timerCallback() override { panel.refresh(); }

    DSynthLookAndFeel lnf;
    SynthPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSynthEditor)
};
