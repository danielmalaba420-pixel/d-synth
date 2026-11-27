#include "PluginEditor.h"
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

    // native layout, 1320 x 820 (matches the validated 3D mock-up)
    const juce::Rectangle<float> kPanel  (46, 0, 1228, 820);
    const juce::Rectangle<float> kBezel  (74, 154, 796, 262);
    const juce::Rectangle<float> kBuild  (1016, 160, 200, 200);
    const juce::Rectangle<float> kWheels (74, 568, 132, 230);
    const juce::Rectangle<float> kKeyBox (224, 568, 1022, 230);

    juce::Point<float> knobCentre (int i) { return { 98.0f + (1124.0f / 6.0f) * ((float) i + 0.5f), 476.0f }; }

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

juce::Font DSynthLookAndFeel::logo (float h) const      { return juce::Font (juce::FontOptions (tfLogo).withHeight (h)); }
juce::Font DSynthLookAndFeel::label (float h) const     { return juce::Font (juce::FontOptions (tfLabel).withHeight (h)); }
juce::Font DSynthLookAndFeel::labelBold (float h) const { return juce::Font (juce::FontOptions (tfLabelBold).withHeight (h)); }
juce::Font DSynthLookAndFeel::screen (float h) const    { return juce::Font (juce::FontOptions (tfScreen).withHeight (h)); }

// Machined metal knob: knurled skirt, polished cap catching the light, white pointer, amber value ring
void DSynthLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const bool big = (bool) s.getProperties()["big"];
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    auto box = area.withSizeKeepingCentre (size, size);
    const auto c = box.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // value ring
    const float ringW = big ? 9.0f : 3.0f;
    const float ringR = size * 0.5f - ringW * 0.5f - 1.0f;
    {
        juce::Path track; track.addCentredArc (c.x, c.y, ringR, ringR, 0, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff2a1d09));
        g.strokePath (track, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (pos > 0.002f)
        {
            juce::Path val; val.addCentredArc (c.x, c.y, ringR, ringR, 0, startAngle, angle, true);
            if (big)
            {
                g.setColour (amber.withAlpha (0.18f));
                g.strokePath (val, juce::PathStrokeType (ringW + 10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setGradientFill (juce::ColourGradient (amber, box.getX(), box.getBottom(), amberLight, box.getRight(), box.getY(), false));
            g.strokePath (val, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    auto knob = box.reduced (big ? 28.0f : 9.0f);
    const float r = knob.getWidth() * 0.5f;
    if (big)
    {
        g.setColour (juce::Colour (0xff111113));
        g.fillEllipse (box.reduced (ringW + 4.0f));
    }
    // shadow under the knob
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillEllipse (knob.translated (0, r * 0.22f).expanded (r * 0.05f));

    // knurled skirt: alternating light / dark facets
    const int facets = big ? 72 : 40;
    for (int i = 0; i < facets; ++i)
    {
        const float a0 = juce::MathConstants<float>::twoPi * (float) i / (float) facets;
        const float a1 = juce::MathConstants<float>::twoPi * (float) (i + 1) / (float) facets;
        const float light = 0.5f + 0.5f * std::cos (a0 - 2.3f);
        const auto col = juce::Colour (0xff2a2a2d).interpolatedWith (juce::Colour (0xff6e6e73), light * ((i & 1) ? 1.0f : 0.55f));
        juce::Path wedge;
        wedge.addPieSegment (knob, a0, a1 + 0.01f, 0.0f);
        g.setColour (col);
        g.fillPath (wedge);
    }
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (knob.reduced (0.5f), 1.0f);

    // polished cap
    auto cap = knob.reduced (r * 0.18f);
    juce::ColourGradient cg (juce::Colour (0xff9c9ca2), cap.getX() + cap.getWidth() * 0.34f, cap.getY() + cap.getHeight() * 0.28f,
                             juce::Colour (0xff1d1d20), cap.getRight(), cap.getBottom(), true);
    cg.addColour (0.42, juce::Colour (0xff45454a));
    g.setGradientFill (cg);
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (cap, 1.0f);
    // specular highlight
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.fillEllipse (cap.getX() + cap.getWidth() * 0.2f, cap.getY() + cap.getHeight() * 0.12f, cap.getWidth() * 0.35f, cap.getHeight() * 0.22f);

    // pointer
    const float inner = r * 0.30f, outer = r * 0.92f;
    juce::Line<float> line (c.getPointOnCircumference (inner, angle), c.getPointOnCircumference (outer, angle));
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawLine (line, big ? 6.0f : 4.5f);
    g.setColour (juce::Colour (0xfff3eee3));
    g.drawLine (line, big ? 3.5f : 2.6f);
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
    auto all = getLocalBounds().toFloat();
    auto r = all;
    if (led)
    {
        // LED above the button
        auto ledArea = r.removeFromTop (15.0f);
        const auto l = juce::Rectangle<float> (8, 8).withCentre ({ ledArea.getCentreX(), ledArea.getY() + 4.0f });
        if (lit)
        {
            g.setColour (DSynthLookAndFeel::amber.withAlpha (0.35f));
            g.fillEllipse (l.expanded (6));
            juce::ColourGradient lg (juce::Colour (0xfffff3d6), l.getX() + 3, l.getY() + 3, DSynthLookAndFeel::amber, l.getRight(), l.getBottom(), true);
            g.setGradientFill (lg);
        }
        else g.setColour (juce::Colour (0xff3a2a10));
        g.fillEllipse (l);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (l, 0.8f);
    }

    const bool pressed = down || (led && lit);
    if (pressed) r.translate (0, 2.0f);
    const float rad = 6.0f;
    // drop shadow
    if (! pressed)
    {
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (r.translated (0, 4.0f), rad);
    }
    juce::ColourGradient body (pressed ? juce::Colour (0xff2e2e31) : juce::Colour (0xff404044), 0, r.getY(),
                               pressed ? juce::Colour (0xff151517) : juce::Colour (0xff1a1a1c), 0, r.getBottom(), false);
    body.addColour (0.52, pressed ? juce::Colour (0xff1c1c1e) : juce::Colour (0xff242427));
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, rad);
    if (over && ! pressed)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (r, rad);
    }
    // glossy reflection on the top half
    auto gloss = r.reduced (6.0f, 2.0f).withHeight (r.getHeight() * 0.42f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (pressed ? 0.08f : 0.16f), 0, gloss.getY(),
                                             juce::Colours::white.withAlpha (0.0f), 0, gloss.getBottom(), false));
    g.fillRoundedRectangle (gloss, 5.0f);
    // edges
    g.setColour (juce::Colours::white.withAlpha (pressed ? 0.08f : 0.2f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + rad, r.getRight() - rad);
    g.setColour (juce::Colour (0xff070707));
    g.drawRoundedRectangle (r, rad, 1.0f);

    const auto col = (led && lit) ? DSynthLookAndFeel::amber : (icon == Icon::Star ? DSynthLookAndFeel::amber : DSynthLookAndFeel::textSoft);
    if (icon == Icon::None)
    {
        g.setColour (col);
        drawSpaced (g, lnf.labelBold (15.0f), getButtonText(), r.getX(), r.getCentreY() - 9.0f, 2.0f, juce::Justification::horizontallyCentred, r.getWidth());
        return;
    }
    const auto cc = r.getCentre();
    juce::Path p;
    if (icon == Icon::Prev || icon == Icon::Next)
    {
        const float s = icon == Icon::Prev ? -1.0f : 1.0f;
        p.startNewSubPath (cc.x - 4.0f * s, cc.y - 8.0f);
        p.lineTo (cc.x + 4.0f * s, cc.y);
        p.lineTo (cc.x - 4.0f * s, cc.y + 8.0f);
        g.setColour (DSynthLookAndFeel::ink);
        g.strokePath (p, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        p.addStar (cc, 5, 5.0f, 12.0f, 0.0f);
        if (lit)
        {
            g.setColour (DSynthLookAndFeel::amber.withAlpha (0.25f));
            g.strokePath (p, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (DSynthLookAndFeel::amber);
            g.fillPath (p);
            g.setColour (DSynthLookAndFeel::amberLight);
            g.strokePath (p, juce::PathStrokeType (1.2f));
        }
        else
        {
            g.setColour (DSynthLookAndFeel::textSoft);
            g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}


//==============================================================================
void Wheel::setValue (float v)
{
    if (std::abs (v - value) > 0.002f) { value = v; repaint(); }
}

void Wheel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::Path clip; clip.addRoundedRectangle (r, 7.0f);
    g.saveState();
    g.reduceClipRegion (clip);
    const float travel = bipolar ? value * 22.0f : (value - 0.5f) * 44.0f;
    const float pitch = 5.0f;
    float off = std::fmod (-travel, pitch); if (off < 0) off += pitch;
    g.setColour (juce::Colour (0xff2a2a2d)); g.fillRect (r);
    g.setColour (juce::Colour (0xff46464a));
    for (float yy = r.getY() - pitch + off; yy < r.getBottom(); yy += pitch)
        g.fillRect (r.getX(), yy, r.getWidth(), 2.0f);
    const float centre = r.getCentreY() - (bipolar ? value * r.getHeight() * 0.38f : (value - 0.5f) * r.getHeight() * 0.76f);
    g.setColour (DSynthLookAndFeel::amber.withAlpha (0.35f));
    g.fillRect (r.getX() + 2, centre - 4.0f, r.getWidth() - 4, 8.0f);
    g.setColour (DSynthLookAndFeel::amber);
    g.fillRect (r.getX() + 4, centre - 1.5f, r.getWidth() - 8, 3.0f);
    // cylinder shading
    juce::ColourGradient shade (juce::Colours::black.withAlpha (0.78f), 0, r.getY(), juce::Colours::black.withAlpha (0.78f), 0, r.getBottom(), false);
    shade.addColour (0.32, juce::Colours::transparentBlack);
    shade.addColour (0.68, juce::Colours::transparentBlack);
    g.setGradientFill (shade);
    g.fillRect (r);
    g.restoreState();
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 7.0f, 1.0f);
}

//==============================================================================
void SoundScreen::setInfo (const Info& i)
{
    if (! (i == info)) { info = i; repaint(); }
}

juce::Rectangle<float> SoundScreen::listArea() const
{
    return getLocalBounds().toFloat().withWidth ((float) getWidth() * 0.44f).reduced (0, 10);
}

void SoundScreen::mouseDown (const juce::MouseEvent& e)
{
    const auto la = listArea();
    if (! la.contains (e.position)) return;
    const int row = (int) ((e.position.y - la.getY()) / 27.0f);
    if (row >= 0 && row < (int) info.rows.size() && onPick) onPick (info.rows[(size_t) row].index);
}

void SoundScreen::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::Path shape; shape.addRoundedRectangle (r, 6.0f);
    g.saveState();
    g.reduceClipRegion (shape);

    juce::ColourGradient bg (juce::Colour (0xff2c1b05), r.getWidth() * 0.55f, r.getHeight() * 0.45f,
                             juce::Colour (0xff0d0801), r.getRight(), r.getBottom(), true);
    bg.addColour (0.75, juce::Colour (0xff140c02));
    g.setGradientFill (bg);
    g.fillRect (r);

    const auto amber = DSynthLookAndFeel::amber;
    // list
    const auto la = listArea();
    float y = la.getY();
    for (auto& row : info.rows)
    {
        auto rr = juce::Rectangle<float> (la.getX(), y, la.getWidth(), 27.0f);
        const bool sel = row.index == info.current;
        const juce::String txt = juce::String (row.index + 1).paddedLeft ('0', 2) + "  " + row.name;
        if (sel)
        {
            g.setColour (amber.withAlpha (0.35f));
            g.fillRect (rr.expanded (0, 2));
            g.setColour (amber);
            g.fillRect (rr);
            g.setColour (juce::Colour (0xff1a1004));
            g.setFont (lnf.screen (16.0f));
            g.drawText (txt, rr.reduced (12, 0), juce::Justification::centredLeft);
            if (row.fav) g.drawText (u8 ("\xe2\x98\x85"), rr.reduced (12, 0), juce::Justification::centredRight);
        }
        else
        {
            glowText (g, txt, rr.reduced (12, 0), lnf.screen (16.0f), DSynthLookAndFeel::amberDeep, juce::Justification::centredLeft, 0.12f);
            if (row.fav) glowText (g, u8 ("\xe2\x98\x85"), rr.reduced (12, 0), lnf.screen (16.0f), DSynthLookAndFeel::amberDeep, juce::Justification::centredRight, 0.12f);
        }
        y += 27.0f;
    }
    if (info.rows.empty())
        glowText (g, u8 ("AUCUN FAVORI \xe2\x80\x94 APPUIE SUR \xe2\x98\x85"), la.withHeight (27).reduced (12, 0), lnf.screen (15.0f),
                  DSynthLookAndFeel::amberDeep, juce::Justification::centredLeft, 0.12f);

    // separator
    g.setColour (amber.withAlpha (0.18f));
    g.drawVerticalLine ((int) la.getRight(), 10.0f, r.getBottom() - 10.0f);

    // current sound
    auto d = r.withLeft (la.getRight()).reduced (22, 18);
    auto top = d.removeFromTop (20);
    glowText (g, info.category, top, lnf.screen (15.0f), DSynthLookAndFeel::amberDim, juce::Justification::left);
    glowText (g, juce::String (info.current + 1).paddedLeft ('0', 2) + " / " + juce::String (info.total), top, lnf.screen (15.0f),
              DSynthLookAndFeel::amberDim, juce::Justification::right);
    auto bottom = d.removeFromBottom (20);
    glowText (g, info.left, bottom, lnf.screen (14.0f), DSynthLookAndFeel::amberDim, juce::Justification::left);
    glowText (g, info.right, bottom, lnf.screen (14.0f), DSynthLookAndFeel::amberDim, juce::Justification::right);

    auto mid = d.withSizeKeepingCentre (d.getWidth(), 86.0f);
    auto nameBox = mid.removeFromTop (54);
    const float nameH = info.name.length() > 12 ? 38.0f : 46.0f;
    g.setFont (lnf.screen (nameH));
    g.setColour (amber.withAlpha (0.18f));
    for (auto dd : { juce::Point<float> (-2, 0), { 2, 0 }, { 0, -2 }, { 0, 2 } })
        g.drawText (info.name, nameBox.translated (dd.x, dd.y), juce::Justification::centredLeft, false);
    g.setColour (DSynthLookAndFeel::amberLight);
    g.drawText (info.name, nameBox, juce::Justification::centredLeft, false);
    glowText (g, info.description, mid, lnf.screen (15.0f), juce::Colour (0xffc58520), juce::Justification::centredLeft);

    // scanlines
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    for (float yy = 0; yy < r.getBottom(); yy += 3.0f) g.drawHorizontalLine ((int) yy, 0.0f, r.getRight());
    // glass: a diagonal reflection and a soft sheen at the top
    juce::Path refl;
    refl.startNewSubPath (0, 0); refl.lineTo (r.getWidth() * 0.42f, 0); refl.lineTo (r.getWidth() * 0.18f, r.getHeight()); refl.lineTo (0, r.getHeight());
    refl.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), 0, 0, juce::Colours::white.withAlpha (0.02f), r.getWidth() * 0.3f, r.getHeight(), false));
    g.fillPath (refl);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), 0, 0, juce::Colours::white.withAlpha (0.0f), 0, r.getHeight() * 0.2f, false));
    g.fillRect (r);
    g.restoreState();
}

//==============================================================================
KeyBed::KeyBed (DSynthProcessor& p, DSynthLookAndFeel& l) : proc (p), lnf (l) {}

// one colour per octave, like the coloured keys of Kontakt instruments (same palette as D-GTRACK)
juce::Colour KeyBed::zoneColour (int note)
{
    static const juce::Colour z[3] = { juce::Colour (0xff3ccb7f), juce::Colour (0xff3d8bff), juce::Colour (0xffff5e57) };
    return z[juce::jlimit (0, 2, (note - kLow) / 12)];
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

juce::Rectangle<float> KeyBed::whiteRect (int wi) const
{
    const float w = (float) getWidth() / 21.0f;
    return { (float) wi * w, kBand, w, (float) getHeight() - kBand };
}

juce::Rectangle<float> KeyBed::blackRect (int note) const
{
    const float w = (float) getWidth() / 21.0f;
    static const int whiteBefore[12] = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 };
    const int oct = (note - kLow) / 12, k = (note - kLow) % 12;
    const float x = ((float) (oct * 7 + whiteBefore[k])) * w - w * 0.3f;
    return { x, kBand, w * 0.6f, ((float) getHeight() - kBand) * 0.62f };
}

int KeyBed::noteAt (juce::Point<float> p) const
{
    p.y = juce::jmax (p.y, kBand);
    for (int n = kLow; n <= kHigh; ++n) if (isBlack (n) && blackRect (n).contains (p)) return n;
    static const int whiteNotes[7] = { 0, 2, 4, 5, 7, 9, 11 };
    const int wi = juce::jlimit (0, 20, (int) (p.x / ((float) getWidth() / 21.0f)));
    return kLow + (wi / 7) * 12 + whiteNotes[wi % 7];
}

void KeyBed::mouseDown (const juce::MouseEvent& e)
{
    mouseNote = noteAt (e.position);
    proc.getKeyboardState().noteOn (1, mouseNote, 0.8f);
}

void KeyBed::mouseDrag (const juce::MouseEvent& e)
{
    const int n = noteAt (e.position);
    if (n != mouseNote)
    {
        if (mouseNote >= 0) proc.getKeyboardState().noteOff (1, mouseNote, 0.0f);
        mouseNote = n;
        proc.getKeyboardState().noteOn (1, mouseNote, 0.8f);
    }
}

void KeyBed::mouseUp (const juce::MouseEvent&)
{
    if (mouseNote >= 0) proc.getKeyboardState().noteOff (1, mouseNote, 0.0f);
    mouseNote = -1;
}

void KeyBed::paint (juce::Graphics& g)
{
    static const int whiteNotes[7] = { 0, 2, 4, 5, 7, 9, 11 };
    // colour band above the keys: one zone per octave, with its name and range
    {
        const float w = (float) getWidth() / 3.0f;
        static const char* names[3] = { "GRAVE", "M\xc3\x89" "DIUM", "AIGU" };
        for (int o = 0; o < 3; ++o)
        {
            auto r = juce::Rectangle<float> ((float) o * w + 1.0f, 0.0f, w - 2.0f, kBand + 4.0f);
            const auto c = zoneColour (kLow + 12 * o);
            g.setColour (c);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (juce::Colour (0xff0f1113));
            g.setFont (lnf.label (12.0f));
            auto tr = r.withTrimmedBottom (4.0f).reduced (8.0f, 0.0f);
            g.drawText (juce::String::fromUTF8 (names[o]), tr, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff0f1113).withAlpha (0.7f));
            g.drawText ("C" + juce::String (kLow / 12 - 1 + o) + juce::String::fromUTF8 ("\xe2\x80\x93") + "B" + juce::String (kLow / 12 - 1 + o),
                        tr, juce::Justification::centredRight);
        }
    }
    for (int wi = 0; wi < 21; ++wi)
    {
        const int note = kLow + (wi / 7) * 12 + whiteNotes[wi % 7];
        auto k = whiteRect (wi).reduced (0.5f, 0).withTrimmedRight (1.0f);
        const uint8_t s = shown[(size_t) note];
        if (s == 1) k = k.withTrimmedTop (-0.0f).translated (0, 1.5f);
        juce::Path p; p.addRoundedRectangle (k.getX(), k.getY(), k.getWidth(), k.getHeight(), 7.0f, 7.0f, false, false, true, true);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillPath (p, juce::AffineTransform::translation (0, 3.0f));
        juce::ColourGradient body (juce::Colours::white, 0, k.getY(), juce::Colour (0xffdcd7cc), 0, k.getBottom(), false);
        body.addColour (0.7, juce::Colour (0xfff1eee6));
        g.setGradientFill (body);
        g.fillPath (p);
        {
            // transparent colour over the key, like Kontakt; the played key takes the full colour
            const auto z = zoneColour (note);
            g.setColour (z.withAlpha (s == 1 ? 0.85f : (s == 2 ? 0.55f : 0.26f)));
            g.fillPath (p);
            if (s > 0) { g.setColour (z.brighter (0.3f).withAlpha (0.95f)); g.fillRect (k.withHeight (6.0f)); }
        }
        // front lip and top shading
        g.setColour (juce::Colour (0xffcdc7bb));
        g.fillRect (k.withTop (k.getBottom() - 8.0f).reduced (1.0f, 0));
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.25f), 0, k.getY(), juce::Colours::transparentBlack, 0, k.getY() + 10.0f, false));
        g.fillRect (k.withHeight (10.0f));
        if (note % 12 == 0)
        {
            g.setColour (juce::Colour (0xff9b968c));
            g.setFont (lnf.label (13.0f));
            g.drawText ("C" + juce::String (note / 12 - 1), k.withTrimmedBottom (14).removeFromBottom (16), juce::Justification::centred);
        }
    }
    for (int n = kLow; n <= kHigh; ++n)
    {
        if (! isBlack (n)) continue;
        auto k = blackRect (n);
        const uint8_t s = shown[(size_t) n];
        juce::Path p; p.addRoundedRectangle (k.getX(), k.getY(), k.getWidth(), k.getHeight(), 5.0f, 5.0f, false, false, true, true);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillPath (p, juce::AffineTransform::translation (0, 4.0f));
        juce::ColourGradient body (juce::Colour (0xff0b0b0c), k.getX(), 0, juce::Colour (0xff0b0b0c), k.getRight(), 0, false);
        body.addColour (0.45, juce::Colour (0xff2c2c2f));
        g.setGradientFill (body);
        g.fillPath (p);
        g.setColour (juce::Colours::black);
        g.fillRect (k.withTop (k.getBottom() - 7.0f).reduced (1, 0));
        {
            const auto z = zoneColour (n);
            g.setColour (s == 1 ? z.withAlpha (0.95f) : (s == 2 ? z.withAlpha (0.7f) : z.darker (0.5f).withAlpha (0.55f)));
            g.fillPath (p);
        }
        // glossy streak
        auto gl = k.reduced (k.getWidth() * 0.25f, 4.0f).withHeight (k.getHeight() * 0.6f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), 0, gl.getY(), juce::Colours::white.withAlpha (0.02f), 0, gl.getBottom(), false));
        g.fillRoundedRectangle (gl, 3.0f);
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
    wood = juce::ImageCache::getFromMemory (BinaryData::walnut_jpg, BinaryData::walnut_jpgSize);
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
    if (name == "GLORY BRASS")             set = { { { "smpStac", "STABS" }, { "padLevel", "SYNTH" }, { "ampR", "RELEASE" }, { "revMix", "REVERB" }, { "width", "WIDTH" }, { "output", "VOLUME" } } };
    if (name == "STRING DINSEI")           set = { { { "glide", "CHORD GLIDE" }, { "ampA", "ATTAQUE" }, { "padLevel", "PAD" }, { "revMix", "REVERB" }, { "width", "WIDTH" }, { "output", "VOLUME" } } };
    if (name == "STRING GLIDE")            set[0] = { "glide", "CHORD GLIDE" };
    if (name == "THE PRAYER")              set = { { { "cutoff", "TONE" }, { "ampA", "ATTAQUE" }, { "chorus", "CHORUS" }, { "lfoCut", "MOUVEMENT" }, { "revMix", "REVERB" }, { "output", "VOLUME" } } };
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
    info.description = upperFr (u8 (pr.description));
    info.category = u8 (pr.category);
    info.fav = favs.contains (pr.name);
    info.left = "BUILD " + juce::String (juce::roundToInt (val ("build") * 100.0f)) + "%";

    const juce::String nm (pr.name);
    const bool sampled = val ("smpSet") > 0.5f && val ("smpLevel") > 0.0f;
    if (sampled && ! proc.samplesReady())
        info.right = "CHARGEMENT DES SAMPLES...";
    else if (val ("arp") > 0.5f)
        info.right = "ARP " + juce::String (kArpRateNames[juce::jlimit (0, 4, (int) std::lround (val ("arpRate")))]) + u8 (" \xc2\xb7 ")
                   + juce::String (kArpModeNames[juce::jlimit (0, 3, (int) std::lround (val ("arpMode")))]).toUpperCase();
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

    for (int i = 0; i < catButtons.size(); ++i)
        catButtons[i]->setLit (proc.uiCategory == kCats[i]);
    starBtn.setLit (info.fav);

    // header tempo + knob readouts
    const juce::String hr = u8 ("SYNC \xe2\x99\xa9 ") + juce::String (juce::roundToInt (proc.tempo())) + " BPM";
    juce::StringArray vals;
    for (auto& k : knobs) vals.add (valueText (st.getParameter (k.id)));
    if (hr != headerRight) { headerRight = hr; repaint (juce::Rectangle<int> (980, 18, 200, 50)); }
    if (vals != knobVals) { knobVals = vals; repaint (juce::Rectangle<int> (60, 505, 1200, 60)); }
}

void SynthPanel::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].slider.setBounds (juce::Rectangle<int> (82, 82).withCentre (knobCentre ((int) i).toInt()));
    build.setBounds (kBuild.toNearestInt());

    const float bw = 108.0f, gap = 10.0f;
    for (int i = 0; i < catButtons.size(); ++i)
        catButtons[i]->setBounds (juce::Rectangle<float> (194.0f + (float) i * (bw + gap), 82.0f, bw, 57.0f).toNearestInt());
    prevBtn.setBounds (897, 162, 62, 54);
    starBtn.setBounds (897, 258, 62, 54);
    nextBtn.setBounds (897, 354, 62, 54);

    screen.setBounds (kBezel.reduced (11).toNearestInt());
    pitchWheel.setBounds (90, 590, 42, 168);
    modWheel.setBounds (148, 590, 42, 168);
    keys.setBounds (kKeyBox.reduced (6).withTrimmedTop (8).toNearestInt());
}

void SynthPanel::drawScrew (juce::Graphics& g, float cx, float cy, float r)
{
    juce::Rectangle<float> b (cx - r, cy - r, r * 2, r * 2);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillEllipse (b.translated (0, 1));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d6cf), cx - r * 0.3f, cy - r * 0.4f, juce::Colour (0xff3d3b38), cx + r, cy + r, true));
    g.fillEllipse (b);
    g.setColour (juce::Colour (0xff2a2826));
    g.drawLine (juce::Line<float> (juce::Point<float> (cx, cy).getPointOnCircumference (r * 0.7f, 2.2f),
                                   juce::Point<float> (cx, cy).getPointOnCircumference (r * 0.7f, 2.2f + juce::MathConstants<float>::pi)), 2.0f);
}

void SynthPanel::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    juce::Path outer; outer.addRoundedRectangle (all, 16.0f);
    g.saveState();
    g.reduceClipRegion (outer);

    // walnut cheeks
    for (auto cheek : { juce::Rectangle<float> (0, 0, 46, (float) H), juce::Rectangle<float> ((float) W - 46, 0, 46, (float) H) })
    {
        if (wood.isValid()) g.drawImage (wood, cheek, juce::RectanglePlacement::fillDestination);
        else { g.setColour (juce::Colour (0xff5a361f)); g.fillRect (cheek); }
        juce::ColourGradient sh (juce::Colours::black.withAlpha (0.55f), cheek.getX(), 0, juce::Colours::black.withAlpha (0.5f), cheek.getRight(), 0, false);
        sh.addColour (0.3, juce::Colours::transparentBlack);
        sh.addColour (0.6, juce::Colour (0x12ffdcb4));
        g.setGradientFill (sh); g.fillRect (cheek);
        drawScrew (g, cheek.getCentreX(), 26, 7);
        drawScrew (g, cheek.getCentreX(), (float) H - 26, 7);
    }

    // brushed black panel
    juce::ColourGradient face (juce::Colour (0xff1e1e21), 0, 0, juce::Colour (0xff0e0e10), 0, (float) H, false);
    face.addColour (0.38, juce::Colour (0xff131315));
    g.setGradientFill (face);
    g.fillRect (kPanel);
    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (float x = kPanel.getX(); x < kPanel.getRight(); x += 3.0f) g.drawVerticalLine ((int) x, 0.0f, (float) H);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawVerticalLine ((int) kPanel.getX(), 0.0f, (float) H);
    g.drawVerticalLine ((int) kPanel.getRight() - 1, 0.0f, (float) H);
    drawScrew (g, kPanel.getX() + 14, 14, 5);
    drawScrew (g, kPanel.getRight() - 14, 14, 5);
    g.restoreState();

    // header: logo + credits
    g.setColour (juce::Colours::black);
    drawSpaced (g, lnf.logo (32.0f), "D-SYNTH", 74, 24, 9.0f);
    g.setColour (DSynthLookAndFeel::ink);
    drawSpaced (g, lnf.logo (32.0f), "D-SYNTH", 74, 22, 9.0f);
    g.setColour (juce::Colour (0xff8d877c));
    drawSpaced (g, lnf.label (14.0f), u8 ("CR\xc3\x89\xc3\x89 ET D\xc3\x89VELOPP\xc3\x89 PAR STUDIO DANOVA & DANIEL MALABA"), 360, 36, 3.0f);

    // header right: tempo readout + version plate
    {
        const juce::Rectangle<float> rd (992, 26, 178, 32);
        g.setColour (juce::Colour (0xff050505)); g.fillRoundedRectangle (rd.expanded (2), 6.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c1b05), rd.getCentreX(), rd.getCentreY(), juce::Colour (0xff0d0801), rd.getRight(), rd.getBottom(), true));
        g.fillRoundedRectangle (rd, 5.0f);
        glowText (g, headerRight, rd, lnf.screen (15.0f),
                  DSynthLookAndFeel::amber, juce::Justification::centred);
        const juce::Rectangle<float> plate (1182, 28, 60, 28);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d4c8), 0, plate.getY(), juce::Colour (0xffa9a398), 0, plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.6f)); g.drawHorizontalLine ((int) plate.getY() + 1, plate.getX() + 3, plate.getRight() - 3);
        g.setColour (juce::Colour (0xff1a1a1c));
        g.setFont (lnf.logo (12.0f));
        g.drawText ("V2.0", plate, juce::Justification::centred);
    }

    // category label
    g.setColour (DSynthLookAndFeel::textDim);
    drawSpaced (g, lnf.label (15.0f), u8 ("CAT\xc3\x89GORIE"), 74, 110, 3.0f);

    // screen bezel (recessed)
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (kBezel.expanded (1).translated (0, 1), 12.0f, 1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050505), 0, kBezel.getY(), juce::Colour (0xff121213), 0, kBezel.getBottom(), false));
    g.fillRoundedRectangle (kBezel, 12.0f);
    juce::DropShadow (juce::Colours::black, 10, { 0, 3 }).drawForRectangle (g, kBezel.reduced (11).toNearestInt());

    // BUILD label
    g.setColour (DSynthLookAndFeel::ink);
    drawSpaced (g, lnf.logo (18.0f), "BUILD", kBuild.getX(), kBuild.getBottom() + 14, 7.0f, juce::Justification::horizontallyCentred, kBuild.getWidth());

    // knob labels + values
    {
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const auto c = knobCentre ((int) i);
            g.setColour (DSynthLookAndFeel::textSoft);
            drawSpaced (g, lnf.label (15.0f), knobs[i].text, c.x - 80, c.y + 44, 3.0f, juce::Justification::horizontallyCentred, 160);
            const juce::String v = knobVals[(int) i];
            glowText (g, v, { c.x - 80, c.y + 64, 160, 18 }, lnf.screen (14.0f), DSynthLookAndFeel::amber, juce::Justification::centred, 0.25f);
        }
    }

    // wheel housing
    g.setColour (juce::Colour (0xff0a0a0b));
    g.fillRoundedRectangle (kWheels, 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (kWheels.expanded (1).translated (0, 1), 10.0f, 1.0f);
    g.setColour (DSynthLookAndFeel::textSoft);
    drawSpaced (g, lnf.label (13.0f), "PITCH", 90, 766, 2.0f, juce::Justification::horizontallyCentred, 42);
    drawSpaced (g, lnf.label (13.0f), "MOD", 148, 766, 2.0f, juce::Justification::horizontallyCentred, 42);

    // key bed with the red felt strip
    g.setColour (juce::Colour (0xff070707));
    g.fillRoundedRectangle (kKeyBox, 6.0f);
    const auto felt = kKeyBox.reduced (6, 0).withHeight (10).translated (0, 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5c1116), 0, felt.getY(), juce::Colour (0xff3a0a0e), 0, felt.getBottom(), false));
    g.fillRect (felt);
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
