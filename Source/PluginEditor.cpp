#include "PluginEditor.h"
#include "SkinRects.h"
#include "BinaryData.h"

using namespace dsynth;

const juce::Colour DSynthLookAndFeel::amber      (0xffef9f27);
const juce::Colour DSynthLookAndFeel::amberHot   (0xffff8a3d);
const juce::Colour DSynthLookAndFeel::amberLight (0xfffac775);
const juce::Colour DSynthLookAndFeel::amberDim   (0xffb07419);
const juce::Colour DSynthLookAndFeel::amberDeep  (0xff9a6416);
const juce::Colour DSynthLookAndFeel::ink        (0xffece6d9);
const juce::Colour DSynthLookAndFeel::textSoft   (0xffa39d92);
const juce::Colour DSynthLookAndFeel::textDim    (0xff6f6a61);

namespace
{
    constexpr float kRotStart = juce::MathConstants<float>::pi * 1.25f;
    constexpr float kRotEnd   = juce::MathConstants<float>::pi * 2.75f;

    juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    // capitals that keep French accents (toUpperCase depends on the system locale)
    juce::String upperFr (const juce::String& s)
    {
        juce::String t = s.toUpperCase();
        const char* pairs[][2] = { { "\xc3\xa9", "\xc3\x89" }, { "\xc3\xa8", "\xc3\x88" }, { "\xc3\xaa", "\xc3\x8a" }, { "\xc3\xa0", "\xc3\x80" },
                                   { "\xc3\xa7", "\xc3\x87" }, { "\xc3\xb4", "\xc3\x94" }, { "\xc3\xae", "\xc3\x8e" }, { "\xc3\xbb", "\xc3\x9b" } };
        for (auto& p : pairs) t = t.replace (juce::String::fromUTF8 (p[0]), juce::String::fromUTF8 (p[1]));
        return t;
    }

    const char* kCats[] = { "TOUS", "FAVORIS", "LEAD", "ARP", "NAPPE", "CLAVIER", "CUIVRE", "BASSE", "FX" };


    // positions measured in the approved mockup (tools/skin)
    juce::Rectangle<float> sk (const float r[4]) { return { r[0], r[1], r[2], r[3] }; }
    juce::Image skinImage (const void* d, int n) { return juce::ImageCache::getFromMemory (d, n); }

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

    // glowing amber text, as on a vacuum-fluorescent screen
    void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> box, juce::Font f,
                   juce::Colour col, juce::Justification j, float glow = 0.22f)
    {
        g.setFont (f);
        g.setColour (col.withAlpha (glow));
        for (auto d : { juce::Point<float> (-1.2f, 0), { 1.2f, 0 }, { 0, -1.2f }, { 0, 1.2f } })
            g.drawText (t, box.translated (d.x, d.y), j, false);
        g.setColour (col);
        g.drawText (t, box, j, false);
    }

    // knob value as shown under each knob
    juce::String valueText (juce::RangedAudioParameter* p)
    {
        if (p == nullptr) return {};
        const auto id = p->getParameterID();
        int idx = findParam (id.toRawUTF8());
        if (idx < 0) return p->getCurrentValueAsText();
        const auto& info = paramTable()[(size_t) idx];
        const float v = p->convertFrom0to1 (p->getValue());
        const juce::String unit (info.unit);
        if (info.kind == ParamKind::Choice || info.kind == ParamKind::Bool) return p->getCurrentValueAsText().toUpperCase();
        if (unit == "ms") return juce::String (juce::roundToInt (v)) + " MS";
        if (unit == "dB") return juce::String (v, 1) + " DB";
        if (unit == "st") return juce::String (juce::roundToInt (v)) + " ST";
        if (unit == "Hz") return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " KHZ" : juce::String (juce::roundToInt (v)) + " HZ";
        if (unit == "s")  return juce::String (v, 1) + " S";
        return juce::String (juce::roundToInt (100.0f * (v - info.min) / (info.max - info.min))) + "%";
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

juce::Font DSynthLookAndFeel::logo (float h) const      { return juce::Font (juce::FontOptions (tfLogo).withPointHeight (h)); }
juce::Font DSynthLookAndFeel::label (float h) const     { return juce::Font (juce::FontOptions (tfLabel).withPointHeight (h)); }
juce::Font DSynthLookAndFeel::labelBold (float h) const { return juce::Font (juce::FontOptions (tfLabelBold).withPointHeight (h)); }
juce::Font DSynthLookAndFeel::screen (float h) const    { return juce::Font (juce::FontOptions (tfScreen).withPointHeight (h)); }

// The knob bodies are part of the skin (rendered from the mockup); here only what moves:
// the white pointer, and for BUILD the amber ring (conic amber -> light amber, 9 px, like the mockup)
void DSynthLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const bool big = (bool) s.getProperties()["big"];
    auto box = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = juce::jmin (box.getWidth(), box.getHeight());
    box = box.withSizeKeepingCentre (size, size);
    const auto c = box.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    float knobSize = size;
    if (big)
    {
        knobSize = size * 148.0f / 200.0f;
        if (pos > 0.002f)
        {
            const float ringR = size * 0.5f - 4.5f;
            juce::Path val; val.addCentredArc (c.x, c.y, ringR, ringR, 0, startAngle, angle, true);
            const auto p0 = c.getPointOnCircumference (ringR, startAngle), p1 = c.getPointOnCircumference (ringR, angle);
            g.setGradientFill (juce::ColourGradient (amber, p0.x, p0.y, pos > 0.05f ? amberLight : amber, p1.x, p1.y, false));
            g.strokePath (val, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
            g.setColour (amber.withAlpha (0.12f));
            g.strokePath (val, juce::PathStrokeType (16.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }
    }
    // pointer: 3 px, from 9 % to 33 % of the knob from its edge (as .ptr in the mockup)
    const float outer = knobSize * 0.41f, inner = knobSize * 0.17f;
    juce::Line<float> line (c.getPointOnCircumference (inner, angle), c.getPointOnCircumference (outer, angle));
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawLine (line, 5.0f);
    g.setColour (juce::Colour (0xfff3eee3));
    g.drawLine (line, 3.0f);
}

//==============================================================================
static juce::File favouritesFile()
{
    const auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    return base.getChildFile ("Application Support/Danova/D-SYNTH/favoris.txt");
   #else
    return base.getChildFile ("Danova/D-SYNTH/favoris.txt");
   #endif
}

juce::StringArray Favourites::load()
{
    juce::StringArray a;
    const auto f = favouritesFile();
    if (f.existsAsFile()) a.addLines (f.loadFileAsString());
    a.removeEmptyStrings();
    return a;
}

void Favourites::save (const juce::StringArray& a)
{
    const auto f = favouritesFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText (a.joinIntoString ("\n"));
}

//==============================================================================
void HardButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    // the buttons are drawn in the skin; the selected category is copied from the "pressed" skin by the panel
    juce::ignoreUnused (over);
    auto r = getLocalBounds().toFloat();
    if (down && ! led)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r.reduced (1.0f), 8.0f);
    }
    if (icon == Icon::Star && lit)
    {
        // filled star of the mockup (24 px icon centred in the button)
        const auto cc = r.getCentre();
        juce::Path p;
        const float pts[][2] = { { 12, 3 }, { 14.7f, 8.6f }, { 20.8f, 9.5f }, { 16.4f, 13.8f }, { 17.4f, 19.9f }, { 12, 17 }, { 6.6f, 19.9f }, { 7.6f, 13.8f }, { 3.2f, 9.5f }, { 9.3f, 8.6f } };
        p.startNewSubPath (cc.x + pts[0][0] - 12, cc.y + pts[0][1] - 12);
        for (int i = 1; i < 10; ++i) p.lineTo (cc.x + pts[i][0] - 12, cc.y + pts[i][1] - 12);
        p.closeSubPath();
        g.setColour (juce::Colour (0xff26262a)); g.fillEllipse (juce::Rectangle<float> (30, 30).withCentre (cc));   // hide the outline star of the skin
        g.setColour (DSynthLookAndFeel::amber); g.fillPath (p);
        g.setColour (DSynthLookAndFeel::amberLight); g.strokePath (p, juce::PathStrokeType (1.2f));
    }
}


//==============================================================================
void Wheel::setValue (float v)
{
    if (std::abs (v - value) > 0.002f) { value = v; repaint(); }
}

void Wheel::paint (juce::Graphics& g)
{
    // ribbed wheel is in the skin; the amber mark shows the position (pitch: only when bent)
    auto r = getLocalBounds().toFloat();
    if (bipolar && std::abs (value) < 0.01f) return;
    const float y = bipolar ? r.getCentreY() - value * (r.getHeight() * 0.5f - 12.0f)
                            : r.getBottom() - 12.0f - value * (r.getHeight() - 24.0f);
    g.setColour (DSynthLookAndFeel::amber.withAlpha (0.3f));
    g.fillRect (r.getX() + 2.0f, y - 3.5f, r.getWidth() - 4.0f, 7.0f);
    g.setColour (DSynthLookAndFeel::amber);
    g.fillRect (r.getX() + 4.0f, y - 1.5f, r.getWidth() - 8.0f, 3.0f);
}

//==============================================================================
void SoundScreen::setInfo (const Info& i)
{
    if (! (i == info)) { info = i; repaint(); }
}

juce::Rectangle<float> SoundScreen::listArea() const
{
    const auto o = sk (skin::screen[0]).getPosition();
    return sk (skin::row[0]).withBottom (sk (skin::row[7]).getBottom()).translated (-o.x, -o.y);
}

void SoundScreen::mouseDown (const juce::MouseEvent& e)
{
    const auto la = listArea();
    if (! la.contains (e.position)) return;
    const int row = (int) ((e.position.y - la.getY()) / skin::row[0][3]);
    if (row >= 0 && row < (int) info.rows.size() && onPick) onPick (info.rows[(size_t) row].index);
}

void SoundScreen::paint (juce::Graphics& g)
{
    // background, separator, scanlines and glass are in the skin; here only the text
    const auto o = sk (skin::screen[0]).getPosition();
    auto at = [&] (const float r[4]) { return sk (r).translated (-o.x, -o.y); };
    const auto amber = DSynthLookAndFeel::amber;
    const auto rowCol = DSynthLookAndFeel::amberDeep;   // #9a6416
    const auto la = listArea();
    const float rh = skin::row[0][3];
    float y = la.getY();
    for (auto& row : info.rows)
    {
        auto rr = juce::Rectangle<float> (la.getX(), y, la.getWidth(), rh);
        const bool sel = row.index == info.current;
        const juce::String txt = juce::String (row.index + 1).paddedLeft ('0', 2) + "  " + row.name;
        auto tr = rr.reduced (12, 0);
        if (sel)
        {
            g.setColour (amber.withAlpha (0.22f)); g.fillRect (rr.expanded (0, 4));
            g.setColour (amber.withAlpha (0.25f)); g.fillRect (rr.expanded (0, 2));
            g.setColour (amber); g.fillRect (rr);
            g.setColour (juce::Colour (0xff1a1004));
            g.setFont (lnf.screen (15.0f));
            g.drawText (txt, tr, juce::Justification::centredLeft);
            if (row.fav) g.drawText (u8 ("\xe2\x98\x85"), tr, juce::Justification::centredRight);
        }
        else
        {
            glowText (g, txt, tr, lnf.screen (15.0f), rowCol, juce::Justification::centredLeft, 0.18f);
            if (row.fav) glowText (g, u8 ("\xe2\x98\x85"), tr, lnf.screen (15.0f), rowCol, juce::Justification::centredRight, 0.18f);
        }
        y += rh;
    }
    if (info.rows.empty())
        glowText (g, u8 ("AUCUN FAVORI \xe2\x80\x94 APPUIE SUR \xe2\x98\x85"), la.withHeight (rh).reduced (12, 0), lnf.screen (15.0f),
                  rowCol, juce::Justification::centredLeft, 0.18f);

    const auto dim = DSynthLookAndFeel::amberDim;   // #b07419
    auto tag = at (skin::tag[0]);
    g.setColour (dim.withAlpha (0.3f));
    drawSpaced (g, lnf.screen (14.0f), info.category, tag.getX(), tag.getY() - 1.0f, 4.0f);
    g.setColour (dim);
    drawSpaced (g, lnf.screen (14.0f), info.category, tag.getX(), tag.getY(), 4.0f);
    auto num = at (skin::num[0]);
    const juce::String ns = juce::String (info.current + 1).paddedLeft ('0', 2) + " / " + juce::String (info.total);
    g.setColour (dim);
    drawSpaced (g, lnf.screen (14.0f), ns, num.getRight() - 200.0f, num.getY(), 2.0f, juce::Justification::right, 200.0f);

    // name: 46 px, letter-spacing 2, light amber with a double glow
    auto nm = at (skin::name[0]);
    const float nameH = info.name.length() > 13 ? 40.0f : 46.0f;
    const auto nf = lnf.screen (nameH);
    for (auto [d, a] : { std::pair<float, float> { 3.0f, 0.10f }, { 1.5f, 0.22f } })
    {
        g.setColour (amber.withAlpha (a));
        for (auto dd : { juce::Point<float> (-d, 0), { d, 0 }, { 0, -d }, { 0, d } })
            drawSpaced (g, nf, info.name, nm.getX() + dd.x, nm.getCentreY() - nf.getHeight() * 0.5f + dd.y, 2.0f);
    }
    g.setColour (DSynthLookAndFeel::amberLight);
    drawSpaced (g, nf, info.name, nm.getX(), nm.getCentreY() - nf.getHeight() * 0.5f, 2.0f);

    glowText (g, info.description, at (skin::desc[0]).withWidth (420.0f).expanded (0, 2), lnf.screen (15.0f), juce::Colour (0xffc58520), juce::Justification::centredLeft, 0.2f);
    auto lb = at (skin::left[0]); auto rb = at (skin::right[0]);
    glowText (g, info.left, lb.withWidth (240.0f).expanded (0, 2), lnf.screen (13.0f), dim, juce::Justification::centredLeft, 0.2f);
    glowText (g, info.right, rb.withLeft (rb.getRight() - 300.0f).expanded (0, 2), lnf.screen (13.0f), dim, juce::Justification::centredRight, 0.2f);
}

//==============================================================================
KeyBed::KeyBed (DSynthProcessor& p, DSynthLookAndFeel& l) : proc (p), lnf (l) {}

juce::Colour KeyBed::zoneColour (int note)
{
    const float t = juce::jlimit (0.0f, 1.0f, (float) (note - kLow) / (float) (kHigh - kLow));
    return juce::Colour::fromHSV (std::fmod (0.99f + t * 0.76f, 1.0f), 0.68f, 0.95f, 1.0f);
}

void KeyBed::update()
{
    std::vector<int> held; std::array<uint8_t, 128> st {};
    for (int n = 0; n < 128; ++n) { st[(size_t) n] = proc.uiNotes[(size_t) n].load(); if (st[(size_t) n]) held.push_back (n); }

    std::array<uint8_t, 128> s2 {};
    int shift = 0;
    if (! held.empty())
    {
        const int lo = held.front(), hi = held.back();
        if (lo >= kLow && hi <= kHigh) shift = 0;
        else if (hi - lo <= kHigh - kLow)
        {
            // move the whole chord by octaves so its shape stays the same
            while (lo + shift < kLow) shift += 12;
            while (hi + shift > kHigh) shift -= 12;
        }
        for (int n : held)
        {
            int m = n + shift;
            while (m < kLow) m += 12;      // a chord wider than three octaves: fold the outer notes in
            while (m > kHigh) m -= 12;
            s2[(size_t) m] = std::max (s2[(size_t) m], st[(size_t) n] == 1 ? (uint8_t) 1 : (uint8_t) 2);
        }
        for (int n : held) if (st[(size_t) n] == 1) { int m = n + shift; while (m < kLow) m += 12; while (m > kHigh) m -= 12; s2[(size_t) m] = 1; }
    }
    if (s2 != shown || shift != octaveShift) { shown = s2; octaveShift = shift; repaint(); }
}

juce::Rectangle<float> KeyBed::keyRect (int note) const
{
    const auto o = sk (skin::keys[0]).getPosition();
    for (auto& k : skin::keyRects) if (k.note == note) return juce::Rectangle<float> (k.x, k.y, k.w, k.h).translated (-o.x, -o.y);
    return {};
}

int KeyBed::noteAt (juce::Point<float> p) const
{
    for (int n = kLow; n <= kHigh; ++n) if (isBlack (n) && keyRect (n).contains (p)) return n;
    for (int n = kLow; n <= kHigh; ++n) if (! isBlack (n) && keyRect (n).expanded (0.6f, 0).contains (p)) return n;
    return -1;
}

void KeyBed::mouseDown (const juce::MouseEvent& e)
{
    mouseNote = noteAt (e.position);
    if (mouseNote >= 0) proc.getKeyboardState().noteOn (1, mouseNote, 0.8f);
}

void KeyBed::mouseDrag (const juce::MouseEvent& e)
{
    const int n = noteAt (e.position);
    if (n != mouseNote)
    {
        if (mouseNote >= 0) proc.getKeyboardState().noteOff (1, mouseNote, 0.0f);
        mouseNote = n;
        if (mouseNote >= 0) proc.getKeyboardState().noteOn (1, mouseNote, 0.8f);
    }
}

void KeyBed::mouseUp (const juce::MouseEvent&)
{
    if (mouseNote >= 0) proc.getKeyboardState().noteOff (1, mouseNote, 0.0f);
    mouseNote = -1;
}

void KeyBed::paint (juce::Graphics& g)
{
    // the white and black keys are in the skin; played notes light up in their colour
    for (int pass = 0; pass < 2; ++pass)
        for (int n = kLow; n <= kHigh; ++n)
        {
            if (isBlack (n) != (pass == 1)) continue;
            const uint8_t s = shown[(size_t) n];
            if (s == 0) continue;
            const auto k = keyRect (n);
            const bool blk = pass == 1;
            juce::Path p; p.addRoundedRectangle (k.getX(), k.getY(), k.getWidth(), k.getHeight(), blk ? 5.0f : 7.0f, blk ? 5.0f : 7.0f, false, false, true, true);
            const auto z = zoneColour (n);
            g.saveState();
            if (! blk)   // the colour of a white key stays behind its black neighbours
                for (int b = n - 1; b <= n + 1; b += 2)
                    if (b >= kLow && b <= kHigh && isBlack (b)) g.excludeClipRegion (keyRect (b).expanded (1.0f, 0.0f).withTrimmedBottom (-3.0f).toNearestInt());
            g.setColour (z.withAlpha (blk ? (s == 1 ? 0.7f : 0.35f) : (s == 1 ? 0.55f : 0.25f)));
            g.fillPath (p);
            if (! blk) { g.setColour (z.withAlpha (s == 1 ? 0.9f : 0.45f)); g.fillRect (k.withHeight (6.0f)); }
            g.restoreState();
        }
    if (octaveShift != 0)
    {
        // where you really play: e.g. "OCT +1" when the chord was shown one octave lower
        const int o = -octaveShift / 12;
        const juce::String t = "OCT " + juce::String (o > 0 ? "+" : "") + juce::String (o);
        auto b = juce::Rectangle<float> ((float) getWidth() - 84.0f, (float) getHeight() - 34.0f, 76.0f, 24.0f);
        g.setColour (juce::Colour (0xe0140c02));
        g.fillRoundedRectangle (b, 5.0f);
        glowText (g, t, b, lnf.screen (15.0f), DSynthLookAndFeel::amber, juce::Justification::centred);
    }
}

//==============================================================================
SynthPanel::SynthPanel (DSynthProcessor& p, DSynthLookAndFeel& l)
    : proc (p), lnf (l),
      prevBtn (l, "prev", HardButton::Icon::Prev), starBtn (l, "fav", HardButton::Icon::Star), nextBtn (l, "next", HardButton::Icon::Next),
      screen (l), keys (p, l)
{
    skinBg    = skinImage (BinaryData::skin_bg_png, BinaryData::skin_bg_pngSize);
    skinOn    = skinImage (BinaryData::skin_on_png, BinaryData::skin_on_pngSize);
    setLookAndFeel (&lnf);
    favs = Favourites::load();

    for (auto& k : knobs)
    {
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setRotaryParameters (kRotStart, kRotEnd, true);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (k.slider);
    }
    build.getProperties().set ("big", true);
    build.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    build.setRotaryParameters (kRotStart, kRotEnd, true);
    build.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (build);
    buildAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.getState(), "build", build);

    for (auto* c : kCats)
    {
        auto* b = catButtons.add (new HardButton (lnf, juce::String (c) == "FAVORIS" ? u8 ("\xe2\x98\x85 FAVORIS") : juce::String (c), HardButton::Icon::None, true));
        const juce::String cat (c);
        b->onClick = [this, cat] { selectCategory (cat); };
        addAndMakeVisible (b);
    }
    prevBtn.onClick = [this] { stepProgram (-1); };
    nextBtn.onClick = [this] { stepProgram (1); };
    starBtn.onClick = [this] { toggleFavourite(); };
    prevBtn.setTooltip (u8 ("Son pr\xc3\xa9" "c\xc3\xa9" "dent"));
    nextBtn.setTooltip ("Son suivant");
    starBtn.setTooltip ("Ajouter / retirer des favoris");
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (starBtn);
    addAndMakeVisible (nextBtn);

    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);
    addAndMakeVisible (screen);
    addAndMakeVisible (keys);
    screen.onPick = [this] (int idx) { proc.setCurrentProgram (idx); refresh(); };

    setSize (W, H);
    refresh();
}

SynthPanel::~SynthPanel() { setLookAndFeel (nullptr); }

std::vector<int> SynthPanel::visibleSounds() const
{
    std::vector<int> v;
    const auto& pr = factoryPresets();
    for (int i = 0; i < (int) pr.size(); ++i)
    {
        const auto& s = pr[(size_t) i];
        if (proc.uiCategory == "TOUS"
            || (proc.uiCategory == "FAVORIS" && favs.contains (s.name))
            || proc.uiCategory == s.category)
            v.push_back (i);
    }
    return v;
}

void SynthPanel::selectCategory (const juce::String& c)
{
    proc.uiCategory = c;
    const auto v = visibleSounds();
    if (! v.empty() && std::find (v.begin(), v.end(), proc.getCurrentProgram()) == v.end())
        proc.setCurrentProgram (v.front());
    refresh();
}

void SynthPanel::stepProgram (int delta)
{
    auto v = visibleSounds();
    if (v.empty()) return;
    auto it = std::find (v.begin(), v.end(), proc.getCurrentProgram());
    int pos = it == v.end() ? 0 : (int) (it - v.begin()) + delta;
    pos = (pos % (int) v.size() + (int) v.size()) % (int) v.size();
    proc.setCurrentProgram (v[(size_t) pos]);
    refresh();
}

void SynthPanel::toggleFavourite()
{
    const juce::String name = factoryPresets()[(size_t) proc.getCurrentProgram()].name;
    favs = Favourites::load();                         // another D-SYNTH window may have changed them
    if (favs.contains (name)) favs.removeString (name); else favs.add (name);
    Favourites::save (favs);
    refresh();
}

// the six knobs follow the sound: each one controls what matters most for it
void SynthPanel::bindKnobs()
{
    const juce::String name = factoryPresets()[(size_t) proc.getCurrentProgram()].name;
    struct K { juce::String id, text; };
    std::array<K, 6> set = { { { "glide", "GLIDE" }, { "cutoff", "TONE" }, { "dlyMix", "DELAY" }, { "revMix", "REVERB" }, { "width", "WIDTH" }, { "output", "VOLUME" } } };
    if (name.startsWith ("ARP"))           set = { { { "arpRate", "VITESSE" }, { "arpMode", "MOTIF" }, { "cutoff", "TONE" }, { "dlyMix", "DELAY" }, { "revMix", "REVERB" }, { "output", "VOLUME" } } };
    if (name == "ARP CRYSTAL")             set[2] = { "epBark", "BRILLANCE" };
    if (name == "HOLY G")                  set = { { { "orgReg", "TIRETTES" }, { "orgPerc", "PERCU" }, { "orgClick", "CLIC" }, { "orgDrive", "DRIVE" }, { "leslie", "ROTARY" }, { "output", "VOLUME" } } };
    if (name == "FIRE KEYS")               set = { { { "smpLevel", "PIANO" }, { "padLevel", "SUPER SAW" }, { "orgLevel", "ORGUE" }, { "dlyMix", "DELAY" }, { "revMix", "REVERB" }, { "output", "VOLUME" } } };
    if (name == "GLORY BRASS")             set = { { { "ampA", "ATTAQUE" }, { "padLevel", "SYNTH" }, { "ampR", "RELEASE" }, { "revMix", "REVERB" }, { "width", "WIDTH" }, { "output", "VOLUME" } } };
    if (name == "STRING DINSEI")           set = { { { "glide", "CHORD GLIDE" }, { "ampA", "ATTAQUE" }, { "padLevel", "PAD" }, { "revMix", "REVERB" }, { "width", "WIDTH" }, { "output", "VOLUME" } } };
    if (name == "STRING GLIDE")            set[0] = { "glide", "CHORD GLIDE" };
    if (name == "THE PRAYER")              set = { { { "cutoff", "CHALEUR" }, { "ampA", "ATTAQUE" }, { "ampR", "RELEASE" }, { "revMix", "REVERB" }, { "velSens", u8 ("V\xc3\x89LOCIT\xc3\x89") }, { "output", "VOLUME" } } };
    if (name == "SIGNLY")                  set = { { { "glide", "GLIDE" }, { "dlyFb", u8 ("R\xc3\x89P\xc3\x89TITIONS") }, { "dlyMix", "DELAY" }, { "dlyBloom", "BLOOM" }, { "revMix", "REVERB" }, { "output", "VOLUME" } } };
    if (name == "RISER")                   set = { { { "riserBars", u8 ("DUR\xc3\x89" "E") }, { "riserPitch", u8 ("MONT\xc3\x89" "E") }, { "cutoff", "TONE" }, { "dlyMix", "DELAY" }, { "revMix", "REVERB" }, { "output", "VOLUME" } } };
    if (name == "FX KIT")                  set = { { { "revSize", "ESPACE" }, { "revMix", "REVERB" }, { "dlyMix", "DELAY" }, { "comp", "PUNCH" }, { "width", "WIDTH" }, { "output", "VOLUME" } } };
    if (name == "FINGER BASS")             set = { { { "bassTone", "TONE" }, { "bassPickup", "MICRO" }, { "bassThump", "ATTAQUE" }, { "bassSustain", "SUSTAIN" }, { "comp", "PUNCH" }, { "output", "VOLUME" } } };
    if (name == "SYNTH BASS" || name == "SUB BASS")
                                           set = { { { "glide", "GLIDE" }, { "cutoff", "TONE" }, { "drive", "DRIVE" }, { "ampR", "RELEASE" }, { "comp", "PUNCH" }, { "output", "VOLUME" } } };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.text = set[i].text;
        if (k.id != set[i].id)
        {
            k.attach.reset();
            k.id = set[i].id;
            k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.getState(), k.id, k.slider);
        }
    }
}

void SynthPanel::refresh()
{
    pitchWheel.setValue (proc.uiPitchBend.load());
    modWheel.setValue (proc.uiModWheel.load());
    keys.update();

    const int prog = proc.getCurrentProgram();
    const auto& all = factoryPresets();
    const auto& pr = all[(size_t) prog];
    if (prog != shownProgram)
    {
        shownProgram = prog;
        bindKnobs();
        repaint();
    }

    auto& st = proc.getState();
    auto val = [&] (const char* id) { return st.getRawParameterValue (id)->load(); };

    // the list: up to 8 rows around the current sound
    const auto vis = visibleSounds();
    SoundScreen::Info info;
    const int w = (int) (std::find (vis.begin(), vis.end(), prog) - vis.begin());
    const int start = juce::jmax (0, juce::jmin (w - 3, (int) vis.size() - 8));
    for (int i = start; i < juce::jmin ((int) vis.size(), start + 8); ++i)
    {
        const auto& s = all[(size_t) vis[(size_t) i]];
        info.rows.push_back ({ vis[(size_t) i], s.name, favs.contains (s.name) });
    }
    info.current = prog;
    info.total = (int) all.size();
    info.name = pr.name;
    info.description = u8 (pr.description);
    info.category = u8 (pr.category);
    info.fav = favs.contains (pr.name);
    info.left = "BUILD " + juce::String (juce::roundToInt (val ("build") * 100.0f)) + "%";

    const juce::String nm (pr.name);
    const bool sampled = val ("smpSet") > 0.5f && val ("smpLevel") > 0.0f;
    if (sampled && ! proc.samplesReady())
        info.right = "CHARGEMENT DES SAMPLES...";
    else if (val ("arp") > 0.5f)
        info.right = "ARP " + juce::String (kArpRateNames[juce::jlimit (0, 4, (int) std::lround (val ("arpRate")))]) + u8 (" \xc2\xb7 ")
                   + juce::String (kArpModeNames[juce::jlimit (0, 5, (int) std::lround (val ("arpMode")))]).toUpperCase();
    else if (val ("orgLevel") > 0.5f)
        info.right = juce::String ("LESLIE ") + (proc.leslieFast() ? "RAPIDE" : "LENT") + u8 (" \xc2\xb7 MOLETTE");
    else if (nm == "FX KIT")
    {
        const int fx = proc.lastFx();
        info.right = fx >= 0 ? juce::String (kFxNames[fx]) : u8 ("DO3 \xe2\x86\x92 SOL4");
    }
    else if (val ("riser") > 0.5f)
        info.right = juce::String (kRiserBarNames[juce::jlimit (0, 3, (int) std::lround (val ("riserBars")))]).toUpperCase().replace ("BARS", "MESURES").replace ("BAR", "MESURE") + " AU TEMPO";
    else
        info.right = val ("poly") > 0.5f ? "POLY 16" : "MONO";
    screen.setInfo (info);

    if (proc.uiCategory != shownCategory) { shownCategory = proc.uiCategory; repaint (juce::Rectangle<int> (150, 70, 1120, 80)); }
    for (int i = 0; i < catButtons.size(); ++i)
        catButtons[i]->setLit (proc.uiCategory == kCats[i]);
    starBtn.setLit (info.fav);

    // header tempo + knob readouts
    const juce::String hr = u8 ("SYNC \xe2\x99\xa9 ") + juce::String (juce::roundToInt (proc.tempo())) + " BPM";
    juce::StringArray vals;
    for (auto& k : knobs) vals.add (valueText (st.getParameter (k.id)));
    if (hr != headerRight) { headerRight = hr; repaint (sk (skin::bpmlcd[0]).expanded (4).toNearestInt()); }
    if (vals != knobVals) { knobVals = vals; repaint (juce::Rectangle<int> (60, (int) skin::klab[0][1] - 4, 1200, 50)); }
}

void SynthPanel::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].slider.setBounds (sk (skin::knob[i]).toNearestInt());
    build.setBounds (sk (skin::build[0]).toNearestInt());
    for (int i = 0; i < catButtons.size(); ++i)
        catButtons[i]->setBounds (sk (skin::cat[i]).toNearestInt());
    prevBtn.setBounds (sk (skin::nav[0]).toNearestInt());
    starBtn.setBounds (sk (skin::nav[1]).toNearestInt());
    nextBtn.setBounds (sk (skin::nav[2]).toNearestInt());
    screen.setBounds (sk (skin::screen[0]).toNearestInt());
    pitchWheel.setBounds (sk (skin::wheel[0]).toNearestInt());
    modWheel.setBounds (sk (skin::wheel[1]).toNearestInt());
    keys.setBounds (sk (skin::keys[0]).toNearestInt());
}

// The face is the approved mockup itself (rendered by tools/skin/make_skin.py); the panel only adds what changes
void SynthPanel::paint (juce::Graphics& g)
{
    const juce::Rectangle<float> all (0, 0, (float) W, (float) H);
    if (skinBg.isValid()) g.drawImage (skinBg, all, juce::RectanglePlacement::stretchToFit);
    else g.fillAll (juce::Colour (0xff131315));

    // selected category: pressed button + lit LED, copied from the "pressed" skin
    const float sx = skinOn.isValid() ? (float) skinOn.getWidth() / (float) W : 1.0f;
    for (int i = 0; i < catButtons.size(); ++i)
        if (proc.uiCategory == kCats[i] && skinOn.isValid())
        {
            const auto r = sk (skin::cat[i]).expanded (8.0f, 0.0f).withTrimmedTop (-4.0f).withBottom (skin::cat[i][1] + skin::cat[i][3] + 12.0f);
            g.drawImage (skinOn, (int) r.getX(), (int) r.getY(), (int) r.getWidth(), (int) r.getHeight(),
                         (int) (r.getX() * sx), (int) (r.getY() * sx), (int) (r.getWidth() * sx), (int) (r.getHeight() * sx));
        }

    // tempo readout
    glowText (g, headerRight, sk (skin::bpmlcd[0]), lnf.screen (14.0f), DSynthLookAndFeel::amber, juce::Justification::centred, 0.25f);

    // knob names and values
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto lab = sk (skin::klab[i]), val = sk (skin::kval[i]);
        const float cx = lab.getCentreX();
        g.setColour (DSynthLookAndFeel::textSoft);
        drawSpaced (g, lnf.label (14.0f), knobs[i].text, cx - 100.0f, lab.getY() + 1.0f, 3.0f, juce::Justification::horizontallyCentred, 200.0f);
        glowText (g, knobVals[(int) i], juce::Rectangle<float> (cx - 100.0f, val.getY() - 1.0f, 200.0f, val.getHeight() + 2.0f), lnf.screen (13.0f),
                  DSynthLookAndFeel::amber, juce::Justification::centred, 0.25f);
    }
}

void SynthPanel::paintOverChildren (juce::Graphics& g)
{
    juce::ignoreUnused (g);   // (the screen glass is under the text in the mockup: it is part of the background)
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
}

void DSynthEditor::resized()
{
    const float s = (float) getWidth() / (float) SynthPanel::W;
    panel.setBounds (0, 0, SynthPanel::W, SynthPanel::H);
    panel.setTransform (juce::AffineTransform::scale (s));
}
