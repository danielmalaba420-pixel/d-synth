#include "PluginEditor.h"
#include "BinaryData.h"

using namespace dsynth;

const juce::Colour DSynthLookAndFeel::amber     (0xffffb547);
const juce::Colour DSynthLookAndFeel::amberHot  (0xffff8a3d);
const juce::Colour DSynthLookAndFeel::amberDim  (0xffd3913a);
const juce::Colour DSynthLookAndFeel::ink       (0xfff3ede2);
const juce::Colour DSynthLookAndFeel::textSoft  (0xffc9c3b8);
const juce::Colour DSynthLookAndFeel::panelTop  (0xff232428);
const juce::Colour DSynthLookAndFeel::panelBottom (0xff141518);

namespace
{
    constexpr float kRotStart = juce::MathConstants<float>::pi * 1.25f;
    constexpr float kRotEnd   = juce::MathConstants<float>::pi * 2.75f;

    // native layout (matches the validated mock-up)
    const juce::Rectangle<float> kInner   (66, 36, 1188, 698);
    const juce::Rectangle<float> kWheels  (98, 213, 124, 208);
    const juce::Rectangle<float> kBezel   (248, 207, 550, 220);
    const juce::Rectangle<float> kBuild   (824, 222, 132, 132);
    const juce::Rectangle<float> kKeysBox (98, 504, 1124, 204);
    const juce::Rectangle<int>   kKeys    (114, 528, 1092, 180);

    juce::Rectangle<float> knobArea (int i)
    {
        const int col = i % 3, row = i / 3;
        return { 982.0f + (float) col * 80.0f + 9.0f, 222.0f + (float) row * 106.0f, 62.0f, 62.0f };
    }

    // text with letter spacing (tracking), as in the mock-up
    void drawSpaced (juce::Graphics& g, const juce::Font& f, const juce::String& text, float x, float y,
                     float spacing, juce::Justification just = juce::Justification::left, float width = 0)
    {
        float w = 0;
        for (auto ch : text) w += juce::GlyphArrangement::getStringWidth (f, juce::String::charToString (ch)) + spacing;
        w -= spacing;
        float cx = x;
        if (just.testFlags (juce::Justification::horizontallyCentred)) cx = x + (width - w) * 0.5f;
        else if (just.testFlags (juce::Justification::right)) cx = x + width - w;
        juce::GlyphArrangement ga;
        for (auto ch : text)
        {
            const auto c = juce::String::charToString (ch);
            ga.addLineOfText (f, c, cx, y + f.getAscent());
            cx += juce::GlyphArrangement::getStringWidth (f, c) + spacing;
        }
        ga.draw (g);
    }
}

//==============================================================================
DSynthLookAndFeel::DSynthLookAndFeel()
{
    tfLogo      = juce::Typeface::createSystemTypefaceFor (BinaryData::MichromaRegular_ttf, BinaryData::MichromaRegular_ttfSize);
    tfLabel     = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
    tfLabelBold = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedBold_ttf, BinaryData::BarlowCondensedBold_ttfSize);
    tfScreen    = juce::Typeface::createSystemTypefaceFor (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
}

juce::Font DSynthLookAndFeel::logo (float h) const      { return juce::Font (juce::FontOptions (tfLogo).withHeight (h)); }
juce::Font DSynthLookAndFeel::label (float h) const     { return juce::Font (juce::FontOptions (tfLabel).withHeight (h)); }
juce::Font DSynthLookAndFeel::labelBold (float h) const { return juce::Font (juce::FontOptions (tfLabelBold).withHeight (h)); }
juce::Font DSynthLookAndFeel::screen (float h) const    { return juce::Font (juce::FontOptions (tfScreen).withHeight (h)); }

void DSynthLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const bool big = (bool) s.getProperties()["big"];
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    auto box = area.withSizeKeepingCentre (size, size);
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto c = box.getCentre();

    if (big)
    {
        // amber value ring around the BUILD knob
        const float ringR = size * 0.5f - 5.0f;
        juce::Path track; track.addCentredArc (c.x, c.y, ringR, ringR, 0, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff3a3b40));
        g.strokePath (track, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (pos > 0.002f)
        {
            juce::Path val; val.addCentredArc (c.x, c.y, ringR, ringR, 0, startAngle, angle, true);
            g.setGradientFill (juce::ColourGradient (amberHot, box.getX(), box.getBottom(), amber, box.getRight(), box.getY(), false));
            g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (amberHot.withAlpha (0.18f));
            g.strokePath (val, juce::PathStrokeType (16.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        box = box.reduced (14.0f);
    }

    const float r = box.getWidth() * 0.5f;
    // drop shadow + dark collar
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (box.translated (0, 5.0f).expanded (1.0f));
    g.setColour (juce::Colour (0xff0f1012));
    g.fillEllipse (box.expanded (3.0f));
    // body
    juce::ColourGradient body (juce::Colour (0xff4a4d52), c.x - r * 0.3f, c.y - r * 0.4f,
                               juce::Colour (0xff16171a), c.x + r * 0.6f, c.y + r * 0.8f, true);
    body.addColour (0.45, juce::Colour (0xff2a2c30));
    g.setGradientFill (body);
    g.fillEllipse (box);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (box.reduced (0.5f), 1.0f);

    // indicator
    const float inner = r * (big ? 0.42f : 0.38f), outer = r * 0.86f;
    juce::Line<float> line (c.getPointOnCircumference (inner, angle), c.getPointOnCircumference (outer, angle));
    g.setColour (big ? amber : ink);
    g.drawLine (line, big ? 4.0f : 3.0f);
}

void DSynthLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                              bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? juce::Colour (0xff3a2a10) : highlighted ? juce::Colour (0xff2a1f0c) : juce::Colour (0xff1a1408));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colour (0xff3a2a12));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
}

void DSynthLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (amber);
    g.setFont (screen (26.0f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, -2), juce::Justification::centred);
}

//==============================================================================
void Wheel::setValue (float v)
{
    if (std::abs (v - value) > 0.002f) { value = v; repaint(); }
}

void Wheel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::Path clip; clip.addRoundedRectangle (r, 6.0f);
    g.saveState();
    g.reduceClipRegion (clip);

    // ribs scroll with the wheel position
    const float travel = bipolar ? value * 22.0f : (value - 0.5f) * 44.0f;
    const float pitch = 7.0f;
    float off = std::fmod (-travel, pitch); if (off < 0) off += pitch;
    g.setColour (juce::Colour (0xff1a1b1e)); g.fillRect (r);
    g.setColour (juce::Colour (0xff2b2d31));
    for (float yy = r.getY() - pitch + off; yy < r.getBottom(); yy += pitch)
        g.fillRect (r.getX(), yy, r.getWidth(), 4.0f);

    // marker line
    const float centre = r.getCentreY() - (bipolar ? value * r.getHeight() * 0.38f : (value - 0.5f) * r.getHeight() * 0.76f);
    g.setColour (DSynthLookAndFeel::amber.withAlpha (0.9f));
    g.fillRect (r.getX() + 3, centre - 1.5f, r.getWidth() - 6, 3.0f);

    // cylinder shading
    juce::ColourGradient shade (juce::Colours::black.withAlpha (0.75f), 0, r.getY(), juce::Colours::black.withAlpha (0.75f), 0, r.getBottom(), false);
    shade.addColour (0.3, juce::Colours::transparentBlack);
    shade.addColour (0.7, juce::Colours::transparentBlack);
    g.setGradientFill (shade);
    g.fillRect (r);
    g.restoreState();
}

//==============================================================================
LeadScreen::LeadScreen (DSynthLookAndFeel& l) : lnf (l)
{
    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    prev.setTooltip ("Lead précédent");
    next.setTooltip ("Lead suivant");
}

void LeadScreen::setInfo (const Info& i)
{
    if (i != info) { info = i; repaint(); }
}

void LeadScreen::resized()
{
    const int cy = getHeight() / 2 - 4;
    prev.setBounds (20, cy - 22, 44, 44);
    next.setBounds (getWidth() - 64, cy - 22, 44, 44);
}

void LeadScreen::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient bg (juce::Colour (0xff1d1608), r.getCentreX(), r.getHeight() * 0.4f,
                             juce::Colour (0xff0c0904), r.getRight(), r.getBottom(), true);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);

    auto glowText = [&] (const juce::String& t, juce::Rectangle<float> box, juce::Font f, juce::Colour col, juce::Justification j)
    {
        g.setFont (f);
        g.setColour (col.withAlpha (0.18f));
        for (auto d : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
            g.drawText (t, box.translated (d.x, d.y), j, false);
        g.setColour (col);
        g.drawText (t, box, j, false);
    };

    const auto amber = DSynthLookAndFeel::amber, dim = DSynthLookAndFeel::amberDim;
    auto top = r.reduced (20, 16).removeFromTop (20);
    glowText ("LEAD " + juce::String (info.index + 1).paddedLeft ('0', 2) + " / 12", top, lnf.screen (16), dim, juce::Justification::left);
    glowText ("BPM SYNC " + juce::String::fromUTF8 ("\xc2\xb7") + " " + info.delayDiv, top, lnf.screen (16), dim, juce::Justification::right);

    auto mid = r.withSizeKeepingCentre (r.getWidth() - 160, 80).translated (0, -6);
    glowText (info.name, mid.removeFromTop (48), lnf.screen (40), juce::Colour (0xffffc266), juce::Justification::centred);
    glowText (info.description, mid, lnf.screen (16), dim, juce::Justification::centred);

    // delay echoes: each bar is one repeat, shaped by feedback and level
    auto bottom = r.reduced (20, 16);
    const float baseY = bottom.getBottom();
    float h = 34.0f;
    for (int n = 0; n < 8; ++n)
    {
        const float alpha = juce::jlimit (0.08f, 1.0f, (0.35f + info.delayMix) * (1.0f - (float) n * 0.1f));
        g.setColour (amber.withAlpha (alpha));
        g.fillRoundedRectangle (bottom.getX() + (float) n * 16.0f, baseY - h, 10.0f, h, 2.0f);
        h = juce::jmax (4.0f, h * (0.62f + 0.35f * info.delayFb));
    }
    auto br = bottom.removeFromBottom (40).removeFromRight (260);
    glowText (info.routing, br.removeFromTop (20), lnf.screen (15), dim, juce::Justification::right);
    glowText (info.delayMode, br, lnf.screen (15), dim, juce::Justification::right);
}

//==============================================================================
RainbowKeyboard::RainbowKeyboard (juce::MidiKeyboardState& s)
    : juce::MidiKeyboardComponent (s, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setAvailableRange (36, 71);           // C2 .. B4, three octaves
    setScrollButtonsVisible (false);
    setKeyWidth (52.0f);
    setBlackNoteWidthProportion (32.0f / 52.0f);
    setBlackNoteLengthProportion (112.0f / 180.0f);
    setColour (shadowColourId, juce::Colours::transparentBlack);
    setColour (upDownButtonBackgroundColourId, juce::Colours::transparentBlack);
    setOpaque (false);
}

juce::Colour RainbowKeyboard::zoneColour (int note)
{
    // red at the bottom of the keyboard to violet at the top
    const float t = juce::jlimit (0.0f, 1.0f, (float) (note - 36) / 35.0f);
    return juce::Colour::fromHSV (std::fmod (0.99f + t * 0.76f, 1.0f), 0.68f, 0.92f, 1.0f);
}

void RainbowKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                     juce::Colour, juce::Colour)
{
    auto key = area.reduced (1.0f, 0.0f).withTrimmedBottom (2.0f);
    juce::Path p; p.addRoundedRectangle (key.getX(), key.getY(), key.getWidth(), key.getHeight(), 6.0f, 6.0f, false, false, true, true);

    juce::ColourGradient body (juce::Colour (0xffe6e2da), 0, key.getY(), juce::Colour (0xffecebe7), 0, key.getBottom(), false);
    body.addColour (0.12, juce::Colour (0xfffbfaf7));
    body.addColour (0.70, juce::Colours::white);
    g.setGradientFill (body);
    g.fillPath (p);

    const auto zone = zoneColour (note);
    if (isDown)       { g.setColour (zone.withAlpha (0.35f)); g.fillPath (p); }
    else if (isOver)  { g.setColour (zone.withAlpha (0.10f)); g.fillPath (p); }

    g.setColour (zone);
    g.fillRect (key.withHeight (5.0f));
    g.setColour (juce::Colours::black.withAlpha (0.12f));
    g.fillRect (key.withTop (key.getBottom() - 6.0f));
    g.setColour (juce::Colour (0xff070708));
    g.drawLine (area.getRight(), area.getY(), area.getRight(), area.getBottom(), 1.0f);

    if (note % 12 == 0)
    {
        g.setColour (juce::Colour (0xff6b6660));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("C" + juce::String (note / 12 - 1), key.withTrimmedBottom (10).removeFromBottom (16), juce::Justification::centred);
    }
}

void RainbowKeyboard::drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                     juce::Colour)
{
    auto key = area.withTrimmedTop (-1.0f);
    juce::Path p; p.addRoundedRectangle (key.getX(), key.getY(), key.getWidth(), key.getHeight(), 5.0f, 5.0f, false, false, true, true);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillPath (p, juce::AffineTransform::translation (0, 4.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2b2f), 0, key.getY(), juce::Colour (0xff0d0d0f), 0, key.getBottom(), false));
    g.fillPath (p);
    if (isDown || isOver)
    {
        g.setColour (zoneColour (note).withAlpha (isDown ? 0.45f : 0.15f));
        g.fillPath (p);
    }
}

//==============================================================================
SynthPanel::SynthPanel (DSynthProcessor& p, DSynthLookAndFeel& l)
    : proc (p), lnf (l), screen (l), keyboard (p.getKeyboardState())
{
    wood = juce::ImageCache::getFromMemory (BinaryData::walnut_jpg, BinaryData::walnut_jpgSize);
    setLookAndFeel (&lnf);

    const char* ids[] = { "glide", "cutoff", "drive", "dlyMix", "revMix", "width" };
    const char* names[] = { "GLIDE", "FILTER", "DRIVE", "DELAY", "REVERB", "WIDTH" };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.text = names[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setRotaryParameters (kRotStart, kRotEnd, true);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setPopupDisplayEnabled (true, true, this);
        k.slider.setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff1a1408));
        k.slider.setColour (juce::BubbleComponent::outlineColourId, juce::Colour (0xff3a2a12));
        k.slider.setColour (juce::TooltipWindow::textColourId, DSynthLookAndFeel::amber);
        addAndMakeVisible (k.slider);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.getState(), ids[i], k.slider);
    }

    build.getProperties().set ("big", true);
    build.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    build.setRotaryParameters (kRotStart, kRotEnd, true);
    build.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    build.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (build);
    buildAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.getState(), "build", build);

    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);
    addAndMakeVisible (screen);
    addAndMakeVisible (keyboard);

    screen.prev.onClick = [this] { stepProgram (-1); };
    screen.next.onClick = [this] { stepProgram (1); };

    setSize (W, H);
    refresh();
}

void SynthPanel::stepProgram (int delta)
{
    const int n = proc.getNumPrograms();
    proc.setCurrentProgram ((proc.getCurrentProgram() + delta + n) % n);
    refresh();
}

void SynthPanel::refresh()
{
    pitchWheel.setValue (proc.uiPitchBend.load());
    modWheel.setValue (proc.uiModWheel.load());

    auto& st = proc.getState();
    auto val = [&] (const char* id) { return st.getRawParameterValue (id)->load(); };

    const int prog = proc.getCurrentProgram();
    const auto& pr = factoryPresets()[(size_t) prog];

    LeadScreen::Info info;
    info.index = prog;
    info.name = pr.name;
    info.description = juce::String::fromUTF8 (pr.description);
    info.delayDiv = kDelayNames[juce::jlimit (0, 6, (int) std::lround (val ("dlyTime")))];
    const juce::String arrow = juce::String::fromUTF8 ("\xe2\x86\x92");
    const juce::String dest = val ("mwBuild") > 0.3f ? "BUILD"
                            : val ("mwTrem") > 0.3f ? "TREMOLO"
                            : val ("mwFilter") > 0.3f ? "FILTER" : "VIBRATO";
    info.routing = "PB " + juce::String::fromUTF8 ("\xc2\xb1") + juce::String ((int) val ("pbRange")) + " " + juce::String::fromUTF8 ("\xc2\xb7") + " MOD " + arrow + " " + dest;
    info.delayMode = val ("dlyDuo") > 0.01f ? "DELAY DUO" : val ("dlyPingPong") > 0.5f ? "DELAY PING-PONG" : "DELAY STEREO";
    info.delayMix = val ("dlyMix");
    info.delayFb = val ("dlyFb");
    screen.setInfo (info);

    // On The Glide, the Glide knob sets the chord glide: relabel it and light it up
    const bool isGlide = juce::String (pr.name).endsWith ("GLIDE");
    if (isGlide != glideMode || prog != shownProgram)
    {
        glideMode = isGlide;
        shownProgram = prog;
        knobs[0].text = isGlide ? "CHORD GLIDE" : "GLIDE";
        repaint();
    }
}

void SynthPanel::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].slider.setBounds (knobArea ((int) i).expanded (6).toNearestInt());
    build.setBounds (kBuild.toNearestInt());
    pitchWheel.setBounds (116, 231, 32, 142);
    modWheel.setBounds (172, 231, 32, 142);
    screen.setBounds (kBezel.reduced (10).toNearestInt());
    keyboard.setBounds (kKeys);
}

void SynthPanel::drawScrew (juce::Graphics& g, float cx, float cy)
{
    juce::Rectangle<float> r (cx - 6, cy - 6, 12, 12);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillEllipse (r.translated (0, 1));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d6cf), cx - 2, cy - 3, juce::Colour (0xff4a4844), cx + 4, cy + 5, true));
    g.fillEllipse (r);
    g.setColour (juce::Colour (0xff3a3834));
    g.drawLine (juce::Line<float> (juce::Point<float> (cx, cy).getPointOnCircumference (4.5f, 2.18f),
                                   juce::Point<float> (cx, cy).getPointOnCircumference (4.5f, 2.18f + juce::MathConstants<float>::pi)), 2.0f);
}

void SynthPanel::drawFrame (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    juce::Path frame; frame.addRoundedRectangle (all, 28.0f);

    g.saveState();
    g.reduceClipRegion (frame);
    if (wood.isValid())
        g.drawImage (wood, all, juce::RectanglePlacement::fillDestination);
    else
        g.fillAll (juce::Colour (0xff4e2f17));

    // varnish light: highlight on top, shade at the bottom, darker edges
    juce::ColourGradient vert (juce::Colour (0x29ffe2ba), 0, 0, juce::Colour (0x59000000), 0, all.getBottom(), false);
    vert.addColour (0.09, juce::Colours::transparentBlack);
    vert.addColour (0.85, juce::Colours::transparentBlack);
    g.setGradientFill (vert); g.fillRect (all);
    juce::ColourGradient vig (juce::Colour (0x1affc88c), all.getWidth() * 0.45f, all.getHeight() * 0.35f,
                              juce::Colour (0x52000000), 0, all.getBottom(), true);
    vig.addColour (0.6, juce::Colours::transparentBlack);
    g.setGradientFill (vig); g.fillRect (all);
    g.restoreState();

    // bevelled edge
    g.setColour (juce::Colour (0x59ffdeb4));
    g.drawRoundedRectangle (all.reduced (1.5f).translated (0.5f, 0.5f), 27.0f, 1.5f);
    g.setColour (juce::Colour (0x8c000000));
    juce::Path bottomEdge; bottomEdge.addRoundedRectangle (all.reduced (1.0f), 27.0f);
    g.saveState(); g.reduceClipRegion (all.withTrimmedTop (all.getHeight() * 0.5f).toNearestInt());
    g.strokePath (bottomEdge, juce::PathStrokeType (2.5f)); g.restoreState();

    // façade sunk into the wood: shadow, groove, light lip
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 18, { 0, 10 }).drawForRectangle (g, kInner.toNearestInt());
    g.setColour (juce::Colour (0xb3140a04)); g.drawRoundedRectangle (kInner.expanded (4.5f), 11.0f, 1.0f);
    g.setColour (juce::Colour (0x29ffd6aa)); g.drawRoundedRectangle (kInner.expanded (3.0f), 10.0f, 2.0f);
    g.setColour (juce::Colour (0xff0d0703)); g.drawRoundedRectangle (kInner.expanded (1.0f), 9.0f, 2.0f);

    juce::ColourGradient face (DSynthLookAndFeel::panelTop, 0, kInner.getY(), DSynthLookAndFeel::panelBottom, 0, kInner.getBottom(), false);
    face.addColour (0.6, juce::Colour (0xff18191c));
    g.setGradientFill (face);
    g.fillRoundedRectangle (kInner, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) kInner.getY() + 1, kInner.getX() + 8, kInner.getRight() - 8);

    drawScrew (g, kInner.getX() + 16, kInner.getY() + 16);
    drawScrew (g, kInner.getRight() - 16, kInner.getY() + 16);
    drawScrew (g, kInner.getX() + 16, kInner.getBottom() - 16);
    drawScrew (g, kInner.getRight() - 16, kInner.getBottom() - 16);
}

void SynthPanel::drawLogo (juce::Graphics& g)
{
    const juce::Rectangle<float> plate (98, 58, 352, 62);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1d20), 0, plate.getY(), juce::Colour (0xff101113), 0, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 8.0f);
    g.setColour (juce::Colour (0xff3a2a12));
    g.drawRoundedRectangle (plate, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine ((int) plate.getY() + 1, plate.getX() + 6, plate.getRight() - 6);

    // "D" emblem
    const juce::Rectangle<float> d (106, 66, 46, 46);
    g.setColour (DSynthLookAndFeel::amberHot.withAlpha (0.25f));
    g.fillRoundedRectangle (d.expanded (4), 8.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffc266), d.getX(), d.getY(), DSynthLookAndFeel::amberHot, d.getRight(), d.getBottom(), false));
    g.fillRoundedRectangle (d, 6.0f);
    g.setColour (juce::Colour (0xff1a0f06));
    g.setFont (lnf.logo (26.0f));
    for (float dx : { 0.4f, 1.0f, 1.6f })   // embolden the emblem letter
        g.drawText ("D", d.translated (dx, 0), juce::Justification::centred);

    // SYNTH + waveform
    g.setColour (juce::Colours::black);
    drawSpaced (g, lnf.logo (30.0f), "SYNTH", 170, 66, 6.0f);
    g.setColour (DSynthLookAndFeel::ink);
    drawSpaced (g, lnf.logo (30.0f), "SYNTH", 170, 65, 6.0f);

    juce::Path wave;
    const float wx = 378, wy = 70;
    wave.startNewSubPath (wx + 1, wy + 10);
    wave.cubicTo (wx + 5, wy + 1, wx + 9, wy + 1, wx + 13, wy + 10);
    wave.cubicTo (wx + 17, wy + 19, wx + 21, wy + 19, wx + 25, wy + 10);
    wave.cubicTo (wx + 29, wy + 1, wx + 33, wy + 1, wx + 37, wy + 10);
    wave.lineTo (wx + 41, wy + 4); wave.lineTo (wx + 45, wy + 16); wave.lineTo (wx + 49, wy + 4); wave.lineTo (wx + 53, wy + 10);
    g.setColour (DSynthLookAndFeel::amber);
    g.strokePath (wave, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // BY DANOVA between two gold rules
    g.setColour (DSynthLookAndFeel::amber.withAlpha (0.7f));
    g.fillRect (170.0f, 107.0f, 22.0f, 1.0f);
    g.setColour (juce::Colour (0xffc9a46a));
    drawSpaced (g, lnf.label (13.0f), "BY DANOVA", 202, 99, 6.0f);
    g.setGradientFill (juce::ColourGradient (DSynthLookAndFeel::amber.withAlpha (0.7f), 318, 0, DSynthLookAndFeel::amber.withAlpha (0.0f), 436, 0, false));
    g.fillRect (318.0f, 107.0f, 118.0f, 1.0f);
}

void SynthPanel::paint (juce::Graphics& g)
{
    drawFrame (g);
    drawLogo (g);

    // header right: 12 LEADS · power LED
    g.setColour (juce::Colour (0xffa39a8c));
    drawSpaced (g, lnf.label (15.0f), "12 LEADS", 1000, 80, 2.0f);
    g.setColour (DSynthLookAndFeel::amberHot.withAlpha (0.35f));
    g.fillEllipse (1093, 82, 14, 14);
    g.setColour (DSynthLookAndFeel::amberHot);
    g.fillEllipse (1095, 84, 10, 10);
    g.setColour (juce::Colour (0xffa39a8c));
    drawSpaced (g, lnf.label (15.0f), "POWER", 1114, 80, 2.0f);

    // gold separator
    juce::ColourGradient sep (DSynthLookAndFeel::amber.withAlpha (0.0f), 98, 0, DSynthLookAndFeel::amber.withAlpha (0.0f), 1222, 0, false);
    sep.addColour (0.5, DSynthLookAndFeel::amber.withAlpha (0.45f));
    g.setGradientFill (sep);
    g.fillRect (98.0f, 130.0f, 1124.0f, 1.0f);

    // wheel housing
    g.setColour (juce::Colour (0xff101113));
    g.fillRoundedRectangle (kWheels, 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (kWheels.reduced (1), 10.0f, 2.0f);
    for (float x : { 112.0f, 168.0f })
    {
        g.setColour (juce::Colour (0xff050506));
        g.fillRoundedRectangle (x, 227, 40, 150, 8.0f);
    }
    g.setColour (DSynthLookAndFeel::textSoft);
    g.setFont (lnf.label (14.0f));
    drawSpaced (g, lnf.label (14.0f), "PITCH", 112, 386, 2.0f, juce::Justification::horizontallyCentred, 40);
    drawSpaced (g, lnf.label (14.0f), "MOD", 168, 386, 2.0f, juce::Justification::horizontallyCentred, 40);

    // screen bezel
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (kBezel, 12.0f);
    g.setColour (juce::Colour (0xff2b2c30));
    g.drawRoundedRectangle (kBezel.expanded (1), 12.0f, 2.0f);

    // knob labels
    g.setColour (DSynthLookAndFeel::amber);
    drawSpaced (g, lnf.labelBold (20.0f), "BUILD", kBuild.getX(), kBuild.getBottom() + 12, 4.0f,
                juce::Justification::horizontallyCentred, kBuild.getWidth());
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto a = knobArea ((int) i);
        const bool hot = i == 0 && glideMode;
        g.setColour (hot ? DSynthLookAndFeel::amber : DSynthLookAndFeel::textSoft);
        drawSpaced (g, lnf.label (14.0f), knobs[i].text, a.getX() - 30, a.getBottom() + 10, 2.0f,
                    juce::Justification::horizontallyCentred, a.getWidth() + 60);
    }

    // keyboard housing + rainbow bar
    juce::Path kb;
    kb.addRoundedRectangle (kKeysBox.getX(), kKeysBox.getY(), kKeysBox.getWidth(), kKeysBox.getHeight(), 10.0f, 10.0f, true, true, false, false);
    g.setColour (juce::Colour (0xff070708));
    g.fillPath (kb);
    juce::ColourGradient rb (RainbowKeyboard::zoneColour (36), 114, 0, RainbowKeyboard::zoneColour (71), 1206, 0, false);
    for (int i = 1; i < 6; ++i) rb.addColour (i / 6.0, RainbowKeyboard::zoneColour (36 + i * 6));
    g.setColour (juce::Colour (0x40ffb478));
    g.fillRoundedRectangle (113, 513, 1094, 10, 5.0f);
    g.setGradientFill (rb);
    g.fillRoundedRectangle (114, 514, 1092, 8, 4.0f);
}

//==============================================================================
DSynthEditor::DSynthEditor (DSynthProcessor& p)
    : AudioProcessorEditor (&p), panel (p, lnf)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) SynthPanel::W / (double) SynthPanel::H);
    setResizeLimits (SynthPanel::W * 55 / 100, SynthPanel::H * 55 / 100, SynthPanel::W * 13 / 10, SynthPanel::H * 13 / 10);
    setSize (SynthPanel::W * 8 / 10, SynthPanel::H * 8 / 10);
    startTimerHz (30);
}

DSynthEditor::~DSynthEditor()
{
    stopTimer();
    panel.setLookAndFeel (nullptr);
}

void DSynthEditor::resized()
{
    const float s = (float) getWidth() / (float) SynthPanel::W;
    panel.setBounds (0, 0, SynthPanel::W, SynthPanel::H);
    panel.setTransform (juce::AffineTransform::scale (s));
}
