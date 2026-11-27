#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

//==============================================================================
// Fonts, colours and the 3D knob drawing of the D-SYNTH 2 façade
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

    static const juce::Colour amber, amberHot, amberLight, amberDim, amberDeep, ink, textSoft, textDim;

private:
    juce::Typeface::Ptr tfLogo, tfLabel, tfLabelBold, tfScreen;
};

//==============================================================================
// Favourites, kept on the computer (shared by every project and every D-SYNTH instance)
struct Favourites
{
    static juce::StringArray load();
    static void save (const juce::StringArray&);
};

//==============================================================================
// A raised hardware button with a reflection; optional LED above (category buttons)
class HardButton : public juce::Button
{
public:
    enum class Icon { None, Prev, Next, Star };
    HardButton (DSynthLookAndFeel& l, const juce::String& text, Icon i = Icon::None, bool withLed = false)
        : juce::Button (text), lnf (l), icon (i), led (withLed) {}
    void setLit (bool b) { if (b != lit) { lit = b; repaint(); } }
    void paintButton (juce::Graphics&, bool over, bool down) override;

private:
    DSynthLookAndFeel& lnf;
    Icon icon;
    bool led, lit = false;
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
// The amber screen under glass: sound list on the left, current sound on the right
class SoundScreen : public juce::Component
{
public:
    explicit SoundScreen (DSynthLookAndFeel& l) : lnf (l) {}

    struct Row { int index; juce::String name; bool fav; };
    struct Info
    {
        std::vector<Row> rows;
        int current = 0, total = 20;
        juce::String name, description, category, left, right;
        bool fav = false;
        bool operator== (const Info& o) const
        {
            if (rows.size() != o.rows.size()) return false;
            for (size_t i = 0; i < rows.size(); ++i)
                if (rows[i].index != o.rows[i].index || rows[i].fav != o.rows[i].fav) return false;
            return current == o.current && name == o.name && fav == o.fav && left == o.left && right == o.right && category == o.category;
        }
    };

    void setInfo (const Info&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void (int)> onPick;

private:
    DSynthLookAndFeel& lnf;
    Info info;
    juce::Rectangle<float> listArea() const;
};

//==============================================================================
// Three octaves (C2..B4) that light up with every note you play, from any octave:
// a chord played outside the range is moved by whole octaves so its shape stays readable.
class KeyBed : public juce::Component
{
public:
    KeyBed (DSynthProcessor&, DSynthLookAndFeel&);
    void update();   // from the panel timer
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    static constexpr int kLow = 36, kHigh = 71;
    static constexpr float kBand = 18.0f;   // height of the colour band above the keys
    static juce::Colour zoneColour (int note);

private:
    int noteAt (juce::Point<float>) const;
    juce::Rectangle<float> whiteRect (int whiteIndex) const;
    juce::Rectangle<float> blackRect (int note) const;
    static bool isBlack (int n) { const int k = n % 12; return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }

    DSynthProcessor& proc;
    DSynthLookAndFeel& lnf;
    std::array<uint8_t, 128> shown {};   // display state per key, after octave folding
    int octaveShift = 0;                 // how many octaves the chord was moved to fit
    int mouseNote = -1;
};

//==============================================================================
// The whole instrument, drawn at its native 1320 x 820 size; the editor scales it
class SynthPanel : public juce::Component
{
public:
    SynthPanel (DSynthProcessor&, DSynthLookAndFeel&);
    ~SynthPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();   // called by the editor's timer

    static constexpr int W = 1320, H = 820;

private:
    void stepProgram (int delta);
    std::vector<int> visibleSounds() const;
    void selectCategory (const juce::String&);
    void toggleFavourite();
    void bindKnobs();
    void drawScrew (juce::Graphics&, float cx, float cy, float r = 7.0f);

    DSynthProcessor& proc;
    DSynthLookAndFeel& lnf;
    juce::Image wood;

    struct Knob
    {
        juce::Slider slider;
        juce::String text, id;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    std::array<Knob, 6> knobs;
    juce::Slider build;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> buildAttach;

    juce::OwnedArray<HardButton> catButtons;
    HardButton prevBtn, starBtn, nextBtn;
    Wheel pitchWheel { true }, modWheel { false };
    SoundScreen screen;
    KeyBed keys;

    juce::StringArray favs;
    int shownProgram = -1;
    juce::String headerRight;
    juce::StringArray knobVals;
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
