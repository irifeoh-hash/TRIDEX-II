#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
// Palette after the original unit: warm charcoal panel, light-grey section header bars with
// dark lettering, orange synthesizer legends, black knobs, role-coloured switches, walnut.
const juce::Colour kPanel     { 0xff3a3935 };
const juce::Colour kPanel2    { 0xff302f2c };
const juce::Colour kLid       { 0xff1f1e1c };
const juce::Colour kWood      { 0xff3e1d0f };
const juce::Colour kInk       { 0xffffffff };
const juce::Colour kHeader    { 0xff5b5953 };
const juce::Colour kHeaderInk { 0xffffffff };
const juce::Colour kCapGrey   { 0xffb7b5ae };
const juce::Colour kCapCream  { 0xffece8db };
const juce::Colour kCapBlue   { 0xff4f8fd8 };   // non-part on/off switches (OUTPUT, effect ON)
const juce::Colour kCapWhite  { 0xfff4f2ec };   // programmer
const juce::Colour kCapTan    { 0xff6e4a30 };
// Part colours for the split indicators: SYNTHE green, BRASS red, STRINGS orange.
const juce::Colour kPartCol[3] = { juce::Colour (0xff5bd34f), juce::Colour (0xffff3b2c), juce::Colour (0xffff9a2e) };
// Switch caps in the part colours (for per-part selectors)
const juce::Colour kPartCap[3] = { juce::Colour (0xff6fbf5c), juce::Colour (0xffd2463a), juce::Colour (0xffe8913a) };
int partIndexForId (const juce::String& id)
{
    if (id.endsWith ("_syn") || id.startsWith ("syn_") || id == "arp1_on" || id == "sg1_on") return 0;
    if (id.endsWith ("_br")  || id.startsWith ("br_")  || id == "arp2_on" || id == "sg2_on") return 1;
    if (id.endsWith ("_st")  || id.startsWith ("st_")  || id == "arp3_on" || id == "sg3_on") return 2;
    return -1;
}
const juce::Colour kLedOn  { 0xffff3a2a };
const juce::Colour kLedOff { 0xff4a1612 };

constexpr int kCol = 66;                 // column pitch
constexpr int kHeadY = 42;               // section header bars (top)
constexpr int kRow1 = 98, kRow2 = 170;   // knob centre rows
constexpr int kModY = 210;               // module names
constexpr int kSwY = 226;                // switch strip top
constexpr int kBarY = 302;               // section name bars (bottom of the switch tier)
constexpr int kTier = 284;               // second row of sections sits this far below the first
constexpr int kLowerY = kBarY + 24 + kTier;   // joystick / keyboard area
constexpr int kRail = 10;                // wooden rail under the keys
constexpr int kKeysX = 312;              // keyboard starts here
constexpr int kStrip = 26;               // split-indicator strip above the keys

// Overlay pages: background, border, title and the grey hint line next to it.
void paintPageFrame (juce::Graphics& g, const juce::Component& page, const juce::String& title = {},
                     const juce::String& hint = {}, int hintX = 0, int hintW = 0)
{
    g.fillAll (kPanel);
    g.setColour (kInk.withAlpha (0.25f));
    g.drawRect (page.getLocalBounds().toFloat(), 1.0f);
    if (title.isEmpty()) return;
    g.setColour (kInk);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title, 18, 8, 300, 18, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (9.0f));
    g.setColour (kInk.withAlpha (0.55f));
    g.drawText (hint, hintX, 8, hintW, 18, juce::Justification::centredLeft);
}

// The rounded, slightly darker card that groups a page's controls.
void paintCard (juce::Graphics& g, juce::Rectangle<float> r, float outlineAlpha = 0.2f)
{
    g.setColour (kPanel2);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (kInk.withAlpha (outlineAlpha));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}
} // namespace

namespace tdxui
{
//==============================================================================
PanelLookAndFeel::PanelLookAndFeel()
{
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0202022));
    setColour (juce::BubbleComponent::outlineColourId, kInk.withAlpha (0.4f));
    setColour (juce::TooltipWindow::textColourId, kInk);
    setColour (juce::ComboBox::backgroundColourId, kPanel2);
    setColour (juce::ComboBox::textColourId, kInk);
    setColour (juce::ComboBox::outlineColourId, kInk.withAlpha (0.3f));
    setColour (juce::PopupMenu::backgroundColourId, kPanel2);
    setColour (juce::PopupMenu::textColourId, kInk);
}

void PanelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    const auto b = juce::Rectangle<int> (x, y, w, h).toFloat();
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 9.0f;
    const bool white = s.getProperties()["white"];
    const auto labels = juce::StringArray::fromTokens (s.getProperties()["labels"].toString(), "|", "");
    const juce::String scale = s.getProperties()["scale"].toString();

    // Scale: fine ticks with sparse numerals, as printed on the panel.
    const auto at = [&] (float t) { return startAngle + t * (endAngle - startAngle); };
    const auto legend = [&] (float t, const juce::String& txt)
    {
        const auto p = c.getPointOnCircumference (r + 7.0f, at (t));
        g.drawText (txt, juce::Rectangle<float> (24, 9).withCentre (p), juce::Justification::centred);
    };
    const auto tick = [&] (float t, float len, float alpha)
    {
        g.setColour (kInk.withAlpha (alpha));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (r + 1.5f, at (t)), c.getPointOnCircumference (r + 1.5f + len, at (t))), 1.0f);
    };
    g.setFont (juce::FontOptions (7.0f).withKerningFactor (0.04f));
    if (labels.size() > 1)
    {
        g.setColour (kInk.withAlpha (0.75f));
        for (int i = 0; i < labels.size(); ++i) legend ((float) i / (float) (labels.size() - 1), labels[i]);
    }
    else if (scale == "bip" || scale == "uni")
    {
        for (int i = 0; i <= 10; ++i) tick ((float) i / 10.0f, i % 5 == 0 ? 3.5f : 2.0f, i % 5 == 0 ? 0.8f : 0.45f);
        g.setColour (kInk.withAlpha (0.7f));
        if (scale == "bip") { legend (0.0f, "-"); legend (0.5f, "0"); legend (1.0f, "+"); }
        else                { legend (0.0f, "0"); legend (1.0f, "10"); }
    }

    // Shadow, black fluted skirt, cap
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - r, c.y - r + 2.0f, r * 2, r * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2c), c.x, c.y - r, juce::Colour (0xff030303), c.x, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);

    const float angle = at (pos);
    // White index line on the skirt (outside the cap)
    g.setColour (kInk);
    g.drawLine (juce::Line<float> (c.getPointOnCircumference (r * 0.78f, angle), c.getPointOnCircumference (r - 0.5f, angle)), 2.0f);

    const float kr = r * 0.74f;
    juce::ignoreUnused (white);
    const auto top = juce::Colour (0xff3a3a3a);   // black knobs, as on the original
    const auto bot = juce::Colour (0xff080808);
    g.setGradientFill (juce::ColourGradient (top, c.x, c.y - kr, bot, c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    // soft top highlight
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillEllipse (c.x - kr * 0.6f, c.y - kr * 0.85f, kr * 1.2f, kr * 0.7f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 0.8f);

    // White index line on the cap
    g.setColour (kInk);
    g.drawLine (juce::Line<float> (c.getPointOnCircumference (kr * 0.25f, angle), c.getPointOnCircumference (kr - 1.5f, angle)), 1.6f);
}

//==============================================================================
void LedButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const bool on = getClickingTogglesState() ? getToggleState() : lit;
    const auto cap = b.reduced (5.0f, 1.5f);

    const auto base = this->cap;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (cap.translated (0.0f, 1.5f).expanded (1.0f), 2.5f);
    g.setGradientFill (juce::ColourGradient (base.brighter (over ? 0.12f : 0.0f), cap.getX(), cap.getY(),
                                             base.darker (down ? 0.45f : 0.18f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 2.0f, cap.getRight() - 2.0f);

    const auto ledC = juce::Point<float> (cap.getCentreX(), cap.getY() + 6.5f);
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (kLedOn.withAlpha (0.55f), ledC.x, ledC.y, kLedOn.withAlpha (0.0f), ledC.x + 8.0f, ledC.y, true));
        g.fillEllipse (juce::Rectangle<float> (16, 16).withCentre (ledC));
    }
    g.setColour (on ? kLedOn : kLedOff);
    g.fillEllipse (juce::Rectangle<float> (5, 5).withCentre (ledC));
}

void ModeSwitch::paintButton (juce::Graphics& g, bool, bool)
{
    const auto b = getLocalBounds().toFloat();
    // Side legends
    g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
    g.setColour (getToggleState() ? accent.withAlpha (0.45f) : accent);
    g.drawText (leftLabel, juce::Rectangle<float> (b.getX(), b.getY(), b.getWidth() * 0.5f, 10.0f), juce::Justification::centred);
    g.setColour (getToggleState() ? accent : accent.withAlpha (0.45f));
    g.drawText (rightLabel, juce::Rectangle<float> (b.getCentreX(), b.getY(), b.getWidth() * 0.5f, 10.0f), juce::Justification::centred);
    // Slot + lever
    const auto slot = juce::Rectangle<float> (b.getCentreX() - 16.0f, b.getY() + 14.0f, 32.0f, 12.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (slot, 3.0f);
    auto lever = slot.withWidth (slot.getWidth() * 0.5f).reduced (1.0f);
    if (getToggleState()) lever.setX (slot.getCentreX() + 1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe2ded4), lever.getX(), lever.getY(), juce::Colour (0xff9e9a90), lever.getX(), lever.getBottom(), false));
    g.fillRoundedRectangle (lever, 2.0f);
}

void SlideSwitch::paintButton (juce::Graphics& g, bool, bool)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    const auto slot = b.withSizeKeepingCentre (12.0f, b.getHeight() - 12.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (slot, 3.0f);
    auto knobR = slot.withHeight (slot.getHeight() * 0.45f).reduced (1.0f);
    if (getToggleState()) knobR.setY (slot.getBottom() - knobR.getHeight() - 1.0f);
    g.setColour (juce::Colour (0xffd8d4ca));
    g.fillRoundedRectangle (knobR, 2.0f);
    g.setColour (kInk);
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("1", juce::Rectangle<float> (slot.getRight() + 2, slot.getY() - 2, 12, 12), juce::Justification::centredLeft);
    g.drawText ("2", juce::Rectangle<float> (slot.getRight() + 2, slot.getBottom() - 10, 12, 12), juce::Justification::centredLeft);
}

//==============================================================================
SplitSelector::SplitSelector (juce::RangedAudioParameter& p)
    : param (p), attachment (p, [this] (float v) { zone = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void SplitSelector::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const bool l = zone != 2, h = zone != 0;
    const auto arrow = [&] (juce::Rectangle<float> r, bool left, bool on)
    {
        juce::Path tri;
        if (left) tri.addTriangle (r.getRight(), r.getY(), r.getRight(), r.getBottom(), r.getX(), r.getCentreY());
        else      tri.addTriangle (r.getX(), r.getY(), r.getX(), r.getBottom(), r.getRight(), r.getCentreY());
        g.setColour (on ? colour : colour.withAlpha (0.18f));
        g.fillPath (tri);
    };
    arrow ({ b.getX() + 4, b.getCentreY() - 5, 10, 10 }, true, l);
    arrow ({ b.getRight() - 14, b.getCentreY() - 5, 10, 10 }, false, h);
}

void SplitSelector::mouseDown (const juce::MouseEvent& e)
{
    // Left arrow toggles the lower 2 octaves, right arrow the upper 3; at least one stays lit.
    bool l = zone != 2, h = zone != 0;
    if (e.x < getWidth() / 2) l = ! l; else h = ! h;
    if (! l && ! h) return;
    attachment.setValueAsCompleteGesture ((float) ((l && h) ? 1 : (l ? 0 : 2)));
}

//==============================================================================
TransposeControl::TransposeControl (juce::RangedAudioParameter& p)
    : attachment (p, [this] (float v) { value = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void TransposeControl::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float bw = b.getHeight();
    const auto drawBtn = [&] (juce::Rectangle<float> r, const juce::String& s)
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r.reduced (1.0f), 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9e4d6), r.getX(), r.getY(),
                                                 juce::Colour (0xffb9b4a6), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r.reduced (2.5f), 2.5f);
        g.setColour (juce::Colour (0xff141414));
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (s, r, juce::Justification::centred);
    };
    drawBtn (b.withWidth (bw), "-");
    drawBtn (b.withTrimmedLeft (b.getWidth() - bw), "+");

    const auto lcdR = b.reduced (bw + 4.0f, 2.0f);
    g.setColour (juce::Colour (0xff1d0705));
    g.fillRoundedRectangle (lcdR, 3.0f);
    g.setColour (value == 0 ? kLedOn.withAlpha (0.55f) : kLedOn);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawText (value > 0 ? "+" + juce::String (value) : juce::String (value), lcdR, juce::Justification::centred);
}

void TransposeControl::step (int delta)
{
    const int v = juce::jlimit (-12, 12, value + delta);
    if (v != value) attachment.setValueAsCompleteGesture ((float) v);
}

void TransposeControl::mouseDown (const juce::MouseEvent& e)
{
    const int bw = getHeight();
    if (e.x < bw) step (-1);
    else if (e.x > getWidth() - bw) step (+1);
}

void TransposeControl::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int bw = getHeight();
    if (e.x >= bw && e.x <= getWidth() - bw) attachment.setValueAsCompleteGesture (0.0f);
}

//==============================================================================
OctaveControl::OctaveControl (juce::RangedAudioParameter& p)
    : attachment (p, [this] (float v) { value = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void OctaveControl::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto disp = b.withWidth (b.getWidth() * 0.5f).reduced (1.0f, 2.0f);
    g.setColour (juce::Colour (0xff1d0705));
    g.fillRoundedRectangle (disp, 3.0f);
    g.setColour (value == 0 ? colour.withAlpha (0.5f) : colour);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
    g.drawText (value > 0 ? "+" + juce::String (value) : juce::String (value), disp, juce::Justification::centred);

    const auto btnCol = b.withTrimmedLeft (b.getWidth() * 0.5f + 2.0f);
    const auto drawBtn = [&] (juce::Rectangle<float> r, bool up)
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r, 2.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9e4d6), r.getX(), r.getY(),
                                                 juce::Colour (0xffb9b4a6), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r.reduced (1.5f), 2.0f);
        juce::Path tri;
        const auto c = r.getCentre();
        if (up) tri.addTriangle (c.x - 4, c.y + 2.5f, c.x + 4, c.y + 2.5f, c.x, c.y - 3);
        else    tri.addTriangle (c.x - 4, c.y - 2.5f, c.x + 4, c.y - 2.5f, c.x, c.y + 3);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillPath (tri);
    };
    drawBtn (btnCol.withHeight (btnCol.getHeight() * 0.5f).reduced (0.0f, 1.0f), true);
    drawBtn (btnCol.withTrimmedTop (btnCol.getHeight() * 0.5f).reduced (0.0f, 1.0f), false);
}

void OctaveControl::mouseDown (const juce::MouseEvent& e)
{
    if (e.x < getWidth() / 2) return;
    const int v = juce::jlimit (-2, 2, value + (e.y < getHeight() / 2 ? 1 : -1));
    if (v != value) attachment.setValueAsCompleteGesture ((float) v);
}

void OctaveControl::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.x < getWidth() / 2) attachment.setValueAsCompleteGesture (0.0f);
}

//==============================================================================
SplitPointControl::SplitPointControl (juce::RangedAudioParameter& p)
    : attachment (p, [this] (float v) { note = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void SplitPointControl::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1d0705));
    g.fillRoundedRectangle (b, 2.5f);
    g.setColour (colour.withAlpha (0.6f));
    const float cy = b.getCentreY();
    juce::Path l, r;
    l.addTriangle (b.getX() + 3, cy, b.getX() + 7, cy - 3, b.getX() + 7, cy + 3);
    r.addTriangle (b.getRight() - 3, cy, b.getRight() - 7, cy - 3, b.getRight() - 7, cy + 3);
    g.fillPath (l); g.fillPath (r);
    g.setColour (colour);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::bold));
    g.drawText (juce::MidiMessage::getMidiNoteName (note, true, true, 3), b, juce::Justification::centred);
}

void SplitPointControl::setNote (int n)
{
    n = juce::jlimit (24, 108, n);
    if (n != note) attachment.setValueAsPartOfGesture ((float) n);
}

void SplitPointControl::mouseDown (const juce::MouseEvent& e)
{
    dragStartNote = note;
    dragging = false;
    attachment.beginGesture();
    // Arrow zones step immediately, so rapid clicks keep stepping.
    if (e.x < getWidth() / 3) setNote (note - 1);
    else if (e.x > getWidth() * 2 / 3) setNote (note + 1);
}

void SplitPointControl::mouseDrag (const juce::MouseEvent& e)
{
    if (std::abs (e.getDistanceFromDragStartY()) < 3 && ! dragging) return;
    dragging = true;
    setNote (dragStartNote - e.getDistanceFromDragStartY() / 4);
}

void SplitPointControl::mouseUp (const juce::MouseEvent&)
{
    attachment.endGesture();
}

void SplitPointControl::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Reset only from the note name (the middle), not the arrows.
    if (e.x >= getWidth() / 3 && e.x <= getWidth() * 2 / 3) attachment.setValueAsCompleteGesture (60.0f);
}

void SplitPointControl::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    attachment.setValueAsCompleteGesture ((float) juce::jlimit (24, 108, note + (w.deltaY > 0 ? 1 : -1)));
}

//==============================================================================
SplitOverlay::SplitOverlay (juce::AudioProcessorValueTreeState& state, juce::MidiKeyboardComponent& keys)
    : apvts (state), kb (keys)
{
    setInterceptsMouseClicks (false, false);
}

void SplitOverlay::refreshIfChanged()
{
    const char* ids[] = { "syn_zone", "br_zone", "st_zone", "syn_split", "br_split", "st_split", "syn_on", "br_on", "st_on" };
    std::array<float, 9> now {};
    for (size_t i = 0; i < 9; ++i) now[i] = apvts.getRawParameterValue (ids[i])->load();
    if (now != last) { last = now; repaint(); }
}

void SplitOverlay::paint (juce::Graphics& g)
{
    const int lowKey = kb.getRangeStart(), highKey = kb.getRangeEnd();
    const auto keyX = [&] (int note)
    {
        if (note > highKey) return kb.getKeyStartPosition (highKey) + kb.getKeyWidth();
        return kb.getKeyStartPosition (juce::jlimit (lowKey, highKey, note));
    };
    const float right = keyX (highKey + 1), h = (float) getHeight();

    g.setColour (juce::Colour (0xff141412));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f);

    for (int s = 0; s < 3; ++s)
    {
        const int zone = juce::roundToInt (last[(size_t) s]);
        const int split = juce::roundToInt (last[(size_t) s + 3]);
        const bool on = last[(size_t) s + 6] > 0.5f;
        const auto col = kPartCol[s].withAlpha (on ? 1.0f : 0.3f);
        const float xs = keyX (split);

        // Range bar for this part
        const float x0 = zone == 2 ? xs : 0.0f, x1 = zone == 0 ? xs : right;
        const auto bar = juce::Rectangle<float> (x0 + 2.0f, 3.0f + (float) s * 7.0f, std::max (0.0f, x1 - x0 - 4.0f), 5.0f);
        g.setColour (col);
        g.fillRoundedRectangle (bar, 2.0f);

        // Split marker (only meaningful for L or H), offset per part so equal splits stay visible
        if (zone != 1)
        {
            const float x = xs + (float) (s - 1) * 4.0f;
            g.setColour (col);
            g.fillRect (x - 1.0f, 1.0f, 2.0f, h - 2.0f);
            juce::Path tri;
            tri.addTriangle (x - 4.5f, h - 6.0f, x + 4.5f, h - 6.0f, x, h - 0.5f);
            g.fillPath (tri);
        }
    }
}

//==============================================================================
void Joystick::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (kInk.withAlpha (0.25f));
    g.drawRoundedRectangle (b, 6.0f, 1.0f);

    g.setColour (kInk.withAlpha (0.85f));
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText ("VIBRATO", b.withHeight (14).translated (0, 3), juce::Justification::centred);
    g.drawText ("TRILL", b.withTrimmedTop (b.getHeight() - 16), juce::Justification::centred);
    g.drawText ("PITCH", juce::Rectangle<float> (b.getX() + 4, b.getCentreY() - 5, 34, 10), juce::Justification::centredLeft);
    g.drawText ("BEND", juce::Rectangle<float> (b.getRight() - 38, b.getCentreY() - 5, 34, 10), juce::Justification::centredRight);

    const auto c = b.getCentre();
    const float range = juce::jmin (b.getWidth(), b.getHeight()) * 0.32f;
    const auto p = c + pos * range;
    g.setColour (juce::Colour (0xff2a2a2d));
    g.drawLine ({ c, p }, 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a5434), p.x, p.y - 9, juce::Colour (0xff3a1e10), p.x, p.y + 9, false));
    g.fillEllipse (juce::Rectangle<float> (20, 20).withCentre (p));
}

void Joystick::mouseDrag (const juce::MouseEvent& e)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float range = juce::jmin (b.getWidth(), b.getHeight()) * 0.32f;
    const auto d = (e.position - b.getCentre()) / range;
    pos = { juce::jlimit (-1.0f, 1.0f, d.x), juce::jlimit (-1.0f, 1.0f, d.y) };
    jx = pos.x;
    jy = -pos.y;     // up = vibrato (+), down = trill (-)
    repaint();
}

void Joystick::mouseUp (const juce::MouseEvent&)
{
    pos = {};        // spring return to centre
    jx = 0.0f; jy = 0.0f;
    repaint();
}
} // namespace tdxui

//==============================================================================
namespace tdxui
{
//==============================================================================
GateGrid::GateGrid (juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
{
    for (int lane = 0; lane < 2; ++lane)
        for (int k = 0; k < 8; ++k)
        {
            auto& c = cells[(size_t) lane][(size_t) k];
            if (auto* prm = state.getParameter (prefix + (lane == 0 ? "_l" : "_r") + juce::String (k + 1)))
            {
                c.att = std::make_unique<juce::ParameterAttachment> (*prm, [this, lane, k] (float v)
                {
                    cells[(size_t) lane][(size_t) k].on = v > 0.5f;
                    repaint();
                });
                c.att->sendInitialUpdate();
            }
        }
}

juce::Rectangle<float> GateGrid::cellBounds (int lane, int step) const
{
    const float labelW = 16.0f, gap = 4.0f;
    const float w = ((float) getWidth() - labelW - gap * 7.0f) / 8.0f, h = ((float) getHeight() - gap) / 2.0f;
    return { labelW + (float) step * (w + gap), (float) lane * (h + gap), w, h };
}

bool GateGrid::hit (juce::Point<float> pt, int& lane, int& step) const
{
    for (lane = 0; lane < 2; ++lane)
        for (step = 0; step < 8; ++step)
            if (cellBounds (lane, step).expanded (2.0f).contains (pt)) return true;
    return false;
}

void GateGrid::set (int lane, int step, bool v)
{
    auto& c = cells[(size_t) lane][(size_t) step];
    if (c.att && c.on != v) c.att->setValueAsCompleteGesture (v ? 1.0f : 0.0f);
}

void GateGrid::mouseDown (const juce::MouseEvent& e)
{
    int lane, step;
    if (! hit (e.position, lane, step)) return;
    paintValue = ! cells[(size_t) lane][(size_t) step].on;   // drag paints this value
    set (lane, step, paintValue);
}

void GateGrid::mouseDrag (const juce::MouseEvent& e)
{
    int lane, step;
    if (hit (e.position, lane, step)) set (lane, step, paintValue);
}

void GateGrid::paint (juce::Graphics& g)
{
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    for (int lane = 0; lane < 2; ++lane)
    {
        const auto r0 = cellBounds (lane, 0);
        g.setColour (kInk.withAlpha (0.8f));
        g.drawText (lane == 0 ? "L" : "R", juce::Rectangle<float> (0, r0.getY(), 14, r0.getHeight()), juce::Justification::centred);
        for (int k = 0; k < 8; ++k)
        {
            const auto r = cellBounds (lane, k);
            const bool on = cells[(size_t) lane][(size_t) k].on;
            if (on)
            {
                g.setGradientFill (juce::ColourGradient (colour.brighter (0.15f), 0, r.getY(), colour.darker (0.2f), 0, r.getBottom(), false));
                g.fillRoundedRectangle (r, 2.5f);
            }
            else
            {
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.fillRoundedRectangle (r, 2.5f);
                g.setColour (kInk.withAlpha (0.25f));
                g.drawRoundedRectangle (r.reduced (0.5f), 2.5f, 1.0f);
            }
            if (k == currentStep)
            {
                g.setColour (juce::Colours::white.withAlpha (0.9f));
                g.drawRoundedRectangle (r.expanded (1.5f), 3.0f, 1.6f);
            }
        }
    }
}

//==============================================================================
AboutPage::AboutPage (std::function<void()> onClose) : closeFn (std::move (onClose))
{
    const int W = TridexPanel::kW - 48, H = kLowerY - 40;
    setSize (W, H);
    instagram.setFont (juce::FontOptions (16.0f, juce::Font::bold), false, juce::Justification::centredLeft);
    instagram.setColour (juce::HyperlinkButton::textColourId, kInk);
    instagram.setTooltip ("Open in your browser");
    instagram.setBounds (330, 438, 320, 26);
    addAndMakeVisible (instagram);

    close.setColour (juce::TextButton::buttonColourId, kPanel2);
    close.setColour (juce::TextButton::textColourOffId, kInk);
    close.setBounds (W - 110, 12, 90, 26);
    close.onClick = [this] { if (closeFn) closeFn(); };
    addAndMakeVisible (close);
}

void AboutPage::paint (juce::Graphics& g)
{
    paintPageFrame (g, *this);

    // Logo, large, on the left
    static const juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::irife_logo_png, BinaryData::irife_logo_pngSize);
    if (logo.isValid())
    {
        g.setOpacity (1.0f);   // draw at full strength (the border colour above is translucent)
        g.drawImageWithin (logo, 40, 70, 250, 120, juce::RectanglePlacement::centred);
    }

    static const juce::Image wordmark = juce::ImageCache::getFromMemory (BinaryData::tridex_logo_png, BinaryData::tridex_logo_pngSize);
    if (wordmark.isValid())
    {
        g.setOpacity (1.0f);
        g.drawImageWithin (wordmark, 330, 52, 260, 44, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid);
    }
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold).withKerningFactor (0.08f));
    g.setColour (kInk);
    g.drawText ("PERFORMING KEYBOARD  /  BY IRIFE", 330, 98, 500, 18, juce::Justification::centredLeft);

    const juce::String synopsis =
        "Tridex II is a three-part performance synthesizer inspired by the classic early-80s "
        "\"stack of keyboards in one\" polysynths. A polyphonic SYNTHESIZER, a BRASS section and an "
        "ensemble STRINGS section share eight (or sixteen) analog-modelled voices, each part with its own "
        "filter, envelopes, keyboard split and output.\n\n"
        "It started as a faithful study of the original circuits (divider-based oscillators, "
        "paraphonic brass, three-phase string ensemble) and grew into a modern instrument: "
        "two-oscillator synth and brass with glide, true mono and paraphonic or polyphonic brass, "
        "per-part arpeggiators and step gates synced to your DAW, a chorus with a dimension-style "
        "SPACE mode, a tape / VHS processor, per-part octave and split controls, 32 program memories and separate outputs.\n\n"
        "Designed and produced by Irife.";
    g.setColour (kInk);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("Thank you for trying this project!", 330, 132, getWidth() - 380, 22, juce::Justification::centredLeft);

    g.setColour (kInk.withAlpha (0.88f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (synopsis, juce::Rectangle<int> (330, 166, getWidth() - 380, 240), juce::Justification::topLeft, 16, 1.0f);

    g.setColour (kInk.withAlpha (0.7f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold).withKerningFactor (0.1f));
    g.drawText ("FOLLOW", 330, 420, 200, 16, juce::Justification::centredLeft);

    g.setColour (kInk.withAlpha (0.4f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("Korg and Trident are trademarks of Korg Inc. This independent project is not affiliated with or endorsed by Korg.",
                330, getHeight() - 30, getWidth() - 360, 14, juce::Justification::centredLeft);
}

//==============================================================================
ChoiceScreen::ChoiceScreen (juce::RangedAudioParameter& p)
    : param (p), attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); })
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (&p)) names = c->choices;
    attachment.sendInitialUpdate();
}

void ChoiceScreen::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1d0705));
    g.fillRoundedRectangle (b, 2.5f);
    g.setColour (kLedOn.withAlpha (0.6f));
    const float cy = b.getCentreY();
    juce::Path l, r;
    l.addTriangle (b.getX() + 4, cy, b.getX() + 9, cy - 4, b.getX() + 9, cy + 4);
    r.addTriangle (b.getRight() - 4, cy, b.getRight() - 9, cy - 4, b.getRight() - 9, cy + 4);
    g.fillPath (l); g.fillPath (r);
    g.setColour (kLedOn);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), fontSize, juce::Font::bold));
    if (labels.size() > index) g.drawText (labels[index], b, juce::Justification::centred);
    else g.drawFittedText (names[index].toUpperCase(), getLocalBounds().reduced (12, 0), juce::Justification::centred, 1, 0.7f);
}

void ChoiceScreen::step (int d)
{
    const int n = juce::jmax (1, names.size());
    attachment.setValueAsCompleteGesture ((float) ((index + d + n) % n));
}

void ChoiceScreen::mouseDown (const juce::MouseEvent& e)
{
    if (cycleOnClick) { step (+1); return; }
    if (e.x < 14) { step (-1); return; }
    if (e.x > getWidth() - 14) { step (+1); return; }
    juce::PopupMenu m;
    if (! sectioned)
        for (int i = 0; i < names.size(); ++i) m.addItem (i + 1, names[i], true, i == index);
    else
    {
        // Group by prefix (SYN / BR / ST / FX), whatever order the entries were added in.
        const auto groupOf = [this] (int i) -> juce::String
        {
            const auto pre = names[i].upToFirstOccurrenceOf (" ", false, false);
            return (pre == "SYN") ? "SYNTHESIZER" : (pre == "BR") ? "BRASS" : (pre == "ST") ? "STRINGS"
                 : (pre == "FL" || pre == "CH" || pre == "TP") ? "EFFECTS" : (i == 0 ? "" : "GLOBAL");
        };
        for (const juce::String grp : { "", "GLOBAL", "SYNTHESIZER", "BRASS", "STRINGS", "EFFECTS" })
        {
            bool header = false;
            for (int i = 0; i < names.size(); ++i)
            {
                if (groupOf (i) != grp) continue;
                if (! header && grp.isNotEmpty()) { m.addSectionHeader (grp); header = true; }
                m.addItem (i + 1, names[i], true, i == index);
            }
        }
    }
    juce::Component::SafePointer<ChoiceScreen> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe] (int r) { if (safe != nullptr && r > 0) safe->attachment.setValueAsCompleteGesture ((float) (r - 1)); });
}

void ChoiceScreen::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (cycleOnClick)
    {
        const int to = juce::jlimit (0, juce::jmax (0, names.size() - 1), index + (w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0)));
        if (to != index) attachment.setValueAsCompleteGesture ((float) to);
        return;
    }
    if (w.deltaY > 0) step (-1); else if (w.deltaY < 0) step (+1);
}

//==============================================================================
juce::Slider& Controls::knob (juce::Component& owner, const juce::String& id, juce::Rectangle<int> r, const juce::String& scale)
{
    auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    s->getProperties().set ("scale", scale);
    s->setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s->setPopupDisplayEnabled (true, true, &owner);
    if (auto* prm = apvts.getParameter (id)) s->setTooltip (prm->getName (64));
    auto& ref = add (owner, std::move (s), r);
    sAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, ref));
    return ref;
}

LedButton& Controls::led (juce::Component& owner, const juce::String& id, juce::Rectangle<int> r)
{
    auto b = std::make_unique<LedButton> (id);
    b->setClickingTogglesState (true);
    if (auto* prm = apvts.getParameter (id)) b->setTooltip (prm->getName (64));
    auto& ref = add (owner, std::move (b), r);
    bAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, ref));
    return ref;
}

ChoiceScreen& Controls::screen (juce::Component& owner, const juce::String& id, juce::Rectangle<int> r)
{
    return add (owner, std::make_unique<ChoiceScreen> (*apvts.getParameter (id)), r);
}

juce::ComboBox& Controls::combo (juce::Component& owner, const juce::String& id, juce::Rectangle<int> r)
{
    auto cb = std::make_unique<juce::ComboBox>();
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
    {
        cb->addItemList (choice->choices, 1);
        cb->setTooltip (choice->getName (64));
    }
    auto& ref = add (owner, std::move (cb), r);
    cAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, ref));
    return ref;
}

//==============================================================================
LfoPanel::LfoPanel (juce::AudioProcessorValueTreeState& state, std::function<float()> phase, std::function<float()> value)
    : apvts (state), getPhase (std::move (phase)), getValue (std::move (value))
{
    setSize (284, 210);
    const auto screen = [&] (const char* id, juce::Rectangle<int> r, bool sectioned, const juce::String& tip)
    {
        auto& s = ctl.screen (*this, id, r);
        s.sectioned = sectioned;
        s.setTooltip (tip);
    };
    const auto knob = [&] (const juce::String& id, juce::Rectangle<int> r, bool bipolar) -> juce::Slider&
    {
        auto& s = ctl.knob (*this, id, r, bipolar ? "bip" : "uni");
        if (bipolar) s.setDoubleClickReturnValue (true, 0.0);
        return s;
    };
    screen ("lfo_wave", { 8, 20, 104, 22 }, false, "LFO waveform: click for the list, or use the arrows");
    screen ("lfo_sync", { 160, 20, 116, 22 }, false, "Sync to the host tempo (OFF = free RATE)");
    knob ("lfo_rate", { 12, 54, 44, 44 }, false).setTooltip ("LFO rate (free, when SYNC is OFF)");
    knob ("lfo_fade", { 66, 54, 44, 44 }, false).setTooltip ("Fade-in time after the first key of a phrase");
    ctl.led (*this, "lfo_retrig", { 123, 56, 30, 36 })
        .setTooltip ("KEY RETRIG: restart the wave on the first key of a phrase (off = free running)");
    scope = { 166, 56, 110, 40 };
    for (int k = 0; k < 2; ++k)
    {
        const juce::String n (k + 1);
        const int y = 124 + k * 38;
        screen (("lfo_dest" + n).toRawUTF8(), { 22, y, 190, 22 }, true, "LFO destination " + n + ": click for the list");
        knob ("lfo_amt" + n, { 232, y - 9, 40, 40 }, true).setTooltip ("Amount " + n + " (centre = none; double-click to reset)");
    }
    screen ("mw_mode", { 86, 189, 126, 18 }, false, "What the MOD WHEEL (CC1) does: VIBRATO (classic) or bring in / speed up the LFO");
}

void LfoPanel::paint (juce::Graphics& g)
{
    g.fillAll (kPanel2.darker (0.15f));
    g.setColour (kInk);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold).withKerningFactor (0.1f));
    g.drawText ("LFO", 8, 4, 60, 14, juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText ("WAVE", 8, 43, 104, 10, juce::Justification::centred);
    g.drawText ("SYNC", 160, 43, 116, 10, juce::Justification::centred);
    g.drawText ("RATE", 4, 98, 60, 10, juce::Justification::centred);
    g.drawText ("FADE", 58, 98, 60, 10, juce::Justification::centred);
    g.drawText ("KEY RETRIG", 108, 93, 60, 10, juce::Justification::centred);
    g.setColour (kInk.withAlpha (0.6f));
    g.drawText ("DESTINATION", 22, 111, 190, 10, juce::Justification::centredLeft);
    g.drawText ("AMOUNT", 222, 111, 60, 10, juce::Justification::centred);
    g.setColour (kInk);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("1", 6, 124, 14, 22, juce::Justification::centred);
    g.drawText ("2", 6, 162, 14, 22, juce::Justification::centred);
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText ("MOD WHEEL", 20, 189, 64, 18, juce::Justification::centredLeft);

    // Scope: the selected wave with a dot at the current position
    const auto r = scope.toFloat();
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (kInk.withAlpha (0.15f));
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 4, r.getRight() - 4);
    const int wave = (int) apvts.getRawParameterValue ("lfo_wave")->load();
    // The engine's own wave function; S&H and Random are shown as six example random steps.
    const auto shape = [wave] (float x) -> float
    {
        static const float h[] = { 0.6f, -0.3f, 0.9f, -0.8f, 0.1f, -0.5f, 0.6f };
        if (wave < 5) return tdx::lfoShape (wave, x, 0.0f, 0.0f);
        const float t = x * 6.0f;
        const int i = juce::jlimit (0, 5, (int) t);
        return wave == 5 ? tdx::lfoShape (5, 0.0, 0.0f, h[i]) : tdx::lfoShape (6, t - (float) i, h[i], h[i + 1]);
    };
    const auto in = r.reduced (6.0f, 7.0f);
    juce::Path path;
    for (int i = 0; i <= 100; ++i)
    {
        const float x = (float) i / 100.0f;
        const float px = in.getX() + x * in.getWidth(), py = in.getCentreY() - shape (x) * in.getHeight() * 0.5f;
        if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
    }
    g.setColour (kLedOn.withAlpha (0.45f));
    g.strokePath (path, juce::PathStrokeType (1.4f));
    const float ph = juce::jlimit (0.0f, 1.0f, getPhase()), val = juce::jlimit (-1.0f, 1.0f, getValue());
    const juce::Point<float> dot (in.getX() + ph * in.getWidth(), in.getCentreY() - val * in.getHeight() * 0.5f);
    g.setColour (kLedOn);
    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (dot));
}

//==============================================================================
namespace
{
    const char* kFxNames[] = { "GATE", "CHORUS", "FLANGER", "TAPE" };
    const char* kFxOrderIds[] = { "fx_order_syn", "fx_order_br", "fx_order_st" };
    const char* kLaneNames[] = { "SYNTHE", "BRASS", "STRINGS" };
}

FxPage::FxPage (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    const int W = TridexPanel::kW - 48, H = kLowerY - 40;
    setSize (W, H);
    link = std::make_unique<LedButton> ("LINK");
    link->setClickingTogglesState (true);
    link->cap = kCapBlue;
    link->setBounds (40, 430, 34, 42);
    link->setTooltip ("LINK ALL PARTS: moving a block in one lane moves it in all three");
    addAndMakeVisible (*link);
    linkAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "fx_link", *link);
    link->onClick = [this] { if (link->getToggleState()) setOrder (0, (int) apvts.getRawParameterValue (kFxOrderIds[0])->load()); repaint(); };

    reset = std::make_unique<LedButton> ("RESET");
    reset->setBounds (150, 430, 34, 42);
    reset->setTooltip ("Back to the classic order: GATE > CHORUS > FLANGER > TAPE");
    reset->onClick = [this] { for (int l = 0; l < 3; ++l) setOrder (l, 0); };
    addAndMakeVisible (*reset);
}

juce::Rectangle<float> FxPage::blockRect (int lane, int slot) const
{
    return { 190.0f + (float) slot * 220.0f, 78.0f + (float) lane * 112.0f, 170.0f, 62.0f };
}

bool FxPage::fxActive (int lane, int fx) const
{
    const auto on = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load() > 0.5f; };
    static const char* sel[3][3] = { { "ch_syn", "fl_syn", "tp_syn" }, { "ch_br", "fl_br", "tp_br" }, { "ch_st", "fl_st", "tp_st" } };
    switch (fx)
    {
        case 0:  return on ("sg" + juce::String (lane + 1) + "_on");
        case 1:  return on ("ch_on") && on (sel[lane][0]);
        case 2:  return on (sel[lane][1]);
        default: return on ("tp_on") && on (sel[lane][2]);
    }
}

void FxPage::setOrder (int lane, int index)
{
    const bool linked = apvts.getRawParameterValue ("fx_link")->load() > 0.5f;
    for (int l = 0; l < 3; ++l)
    {
        if (! linked && l != lane) continue;
        if (auto* prm = apvts.getParameter (kFxOrderIds[l]))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) index));
            prm->endChangeGesture();
        }
    }
    repaint();
}

void FxPage::timerCallback() { repaint(); }

void FxPage::mouseDown (const juce::MouseEvent& e)
{
    for (int lane = 0; lane < 3; ++lane)
        for (int slot = 0; slot < 4; ++slot)
        {
            const auto r = blockRect (lane, slot);
            if (! r.contains (e.position)) continue;
            int dir = 0;
            if (e.position.x < r.getX() + 30.0f) dir = -1;
            else if (e.position.x > r.getRight() - 30.0f) dir = +1;
            if (dir == 0 || slot + dir < 0 || slot + dir > 3) return;
            auto order = tdx::fxOrders()[(size_t) juce::jlimit (0, 23, (int) apvts.getRawParameterValue (kFxOrderIds[lane])->load())];
            std::swap (order[(size_t) slot], order[(size_t) (slot + dir)]);
            setOrder (lane, tdx::fxOrderIndex (order));
            return;
        }
}

void FxPage::paint (juce::Graphics& g)
{
    paintPageFrame (g, *this, "FX ROUTING", "EACH PART'S EFFECT ORDER  /  USE THE ARROWS ON A BLOCK TO MOVE IT  /  PRESS FX AGAIN TO CLOSE", 130, 800);

    for (int lane = 0; lane < 3; ++lane)
    {
        const auto col = kPartCol[lane];
        const float cy = blockRect (lane, 0).getCentreY();
        paintCard (g, { 18.0f, cy - 48.0f, (float) getWidth() - 36.0f, 96.0f }, 0.15f);

        g.setColour (col);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (kLaneNames[lane], 40, (int) cy - 10, 110, 20, juce::Justification::centredLeft);

        // signal line through the lane
        g.setColour (col.withAlpha (0.8f));
        g.fillRect (130.0f, cy - 1.5f, (float) getWidth() - 220.0f, 3.0f);
        juce::Path arrow; arrow.addTriangle ((float) getWidth() - 92.0f, cy - 7.0f, (float) getWidth() - 92.0f, cy + 7.0f, (float) getWidth() - 80.0f, cy);
        g.fillPath (arrow);
        g.setColour (kInk);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("OUT", getWidth() - 76, (int) cy - 8, 40, 16, juce::Justification::centredLeft);

        const auto& order = tdx::fxOrders()[(size_t) juce::jlimit (0, 23, (int) apvts.getRawParameterValue (kFxOrderIds[lane])->load())];
        for (int slot = 0; slot < 4; ++slot)
        {
            const int fx = order[(size_t) slot];
            const bool act = fxActive (lane, fx);
            const auto r = blockRect (lane, slot);
            g.setColour (act ? col.withAlpha (0.22f).overlaidWith (juce::Colour (0xff1d1c1a).withAlpha (0.55f)) : juce::Colour (0xff1d1c1a));
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (act ? col : kInk.withAlpha (0.3f));
            g.drawRoundedRectangle (r, 5.0f, act ? 2.0f : 1.0f);
            g.setColour (act ? kInk : kInk.withAlpha (0.45f));
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold).withKerningFactor (0.08f));
            g.drawText (kFxNames[fx], r.reduced (30.0f, 0.0f).withTrimmedBottom (14.0f), juce::Justification::centred);
            g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
            g.setColour (act ? col : kInk.withAlpha (0.35f));
            g.drawText (act ? "ON" : "OFF", r.reduced (30.0f, 0.0f).withTrimmedTop (34.0f), juce::Justification::centred);
            // move arrows
            g.setColour (kInk.withAlpha (slot > 0 ? 0.8f : 0.15f));
            juce::Path l; l.addTriangle (r.getX() + 10, r.getCentreY(), r.getX() + 20, r.getCentreY() - 7, r.getX() + 20, r.getCentreY() + 7);
            g.fillPath (l);
            g.setColour (kInk.withAlpha (slot < 3 ? 0.8f : 0.15f));
            juce::Path rr; rr.addTriangle (r.getRight() - 10, r.getCentreY(), r.getRight() - 20, r.getCentreY() - 7, r.getRight() - 20, r.getCentreY() + 7);
            g.fillPath (rr);
        }
    }

    g.setColour (kInk);
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText ("LINK ALL PARTS", 12, 474, 90, 12, juce::Justification::centred);
    g.drawText ("RESET", 122, 474, 90, 12, juce::Justification::centred);
    g.setColour (kInk.withAlpha (0.6f));
    g.setFont (juce::FontOptions (9.5f));
    g.drawFittedText ("A block shows ON when that effect is active for the part (STEP GATE on the ARP page, CHORUS and TAPE need their ON switch "
                      "and part selector, FLANGER its part selector). Switching order fades the part out and back in over a few milliseconds, so it never clicks.\n"
                      "TAPE LAST: wobble and saturation over everything (classic).   TAPE FIRST: the chorus and flanger sweep a wobbly, saturated tape sound.   "
                      "GATE LAST: the gate also chops effect tails and tape hiss, for a harder rhythm.",
                      240, 424, getWidth() - 270, 60, juce::Justification::centredLeft, 4);
}

//==============================================================================
SettingsPage::SettingsPage (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    const int W = TridexPanel::kW - 48, H = kLowerY - 40;
    setSize (W, H);
    card = { 18, 40, W - 36, 300 };
    const int x0 = card.getX(), y = card.getY() + 56;

    // MIDI & PLAYING
    addScreen ("pedal_mode", { x0 + 30, y, 140, 24 }, "PEDAL", "Sustain pedal (CC64): OFF / DAMPER (Trident release switch, works with AUTO DAMP) / HOLD");
    addScreen ("note_prio", { x0 + 190, y, 120, 24 }, "NOTE PRIORITY", "Which held note a MONO part plays: LAST / FIRST / LOW / HIGH");
    addKnob ("bend_range", { x0 + 330, y - 16, 56, 56 }, "BEND RANGE");
    addKnob ("vel_sens", { x0 + 400, y - 16, 56, 56 }, "VELOCITY");
    // TUNING
    addKnob ("drift", { x0 + 520, y - 16, 56, 56 }, "ANALOG DRIFT");
    // SOUND / CPU
    quality = &ctl.screen (*this, "quality", { x0 + 650, y, 110, 24 });
    quality->cycleOnClick = true; quality->fontSize = 12.0f; quality->labels = { "NORMAL", "ECO" };
    quality->setTooltip ("Filter quality: NORMAL = 2x oversampled filters, ECO = lower CPU. Click to switch.");
    labels.push_back ({ quality->getBounds(), "QUALITY" });
    // OUTPUTS
    addScreen ("out_mode", { x0 + 830, y, 130, 24 }, "OUTPUTS",
               "MAIN MIX: everything on output 1. SEPARATE: Synthe / Brass / Strings on outputs 2 / 3 / 4 only (Main goes silent)");
}

void SettingsPage::addKnob (const char* id, juce::Rectangle<int> r, const juce::String& label)
{
    ctl.knob (*this, id, r, juce::String (id) == "bend_range" ? "none" : "uni");   // bend is 0-12 st, not 0-10
    labels.push_back ({ r, label });
}

void SettingsPage::addScreen (const char* id, juce::Rectangle<int> r, const juce::String& label, const juce::String& tip)
{
    ctl.screen (*this, id, r).setTooltip (tip);
    labels.push_back ({ r, label });
}

void SettingsPage::paint (juce::Graphics& g)
{
    paintPageFrame (g, *this, "SETTINGS", "SAVED WITH YOUR PROJECT  /  PRESS SETTINGS AGAIN TO CLOSE", 110, 600);
    paintCard (g, card.toFloat());

    // group headers and dividers
    const int x0 = card.getX();
    g.setColour (kInk.withAlpha (0.7f));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold).withKerningFactor (0.1f));
    g.drawText ("MIDI & PLAYING", x0 + 24, card.getY() + 18, 300, 14, juce::Justification::centredLeft);
    g.drawText ("TUNING", x0 + 500, card.getY() + 18, 200, 14, juce::Justification::centredLeft);
    g.drawText ("SOUND / CPU", x0 + 630, card.getY() + 18, 200, 14, juce::Justification::centredLeft);
    g.drawText ("OUTPUTS", x0 + 810, card.getY() + 18, 200, 14, juce::Justification::centredLeft);
    g.setColour (kInk.withAlpha (0.12f));
    g.drawVerticalLine (x0 + 482, (float) card.getY() + 16, (float) card.getY() + 120);
    g.drawVerticalLine (x0 + 612, (float) card.getY() + 16, (float) card.getY() + 120);
    g.drawVerticalLine (x0 + 792, (float) card.getY() + 16, (float) card.getY() + 120);
    g.drawHorizontalLine (card.getY() + 132, (float) x0 + 20, (float) card.getRight() - 20);

    g.setColour (kInk);
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    for (const auto& [b, text] : labels)
        g.drawText (text, b.getX() - 20, b.getBottom() + 4, b.getWidth() + 40, 12, juce::Justification::centred);

    g.setColour (kInk.withAlpha (0.75f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawFittedText ("PEDAL: OFF ignores the sustain pedal. DAMPER is the Trident release switch: with AUTO DAMP on, holding it lets release tails ring. "
                      "HOLD is a normal sustain pedal.\n"
                      "NOTE PRIORITY: which held note a MONO part (synth or brass) plays: the LAST pressed, the FIRST, the LOWEST or the HIGHEST.\n"
                      "BEND RANGE: joystick / pitch-wheel bend in semitones (0-12).   VELOCITY: 0 = no velocity response, as on the original.\n"
                      "ANALOG DRIFT: small random tuning differences and slow wander per voice (0 = perfectly in tune).\n"
                      "QUALITY: NORMAL runs the filters 2x oversampled (cleanest). ECO runs them at the host rate for roughly 20-40% less CPU; "
                      "at very high cutoff with high resonance it is a little brighter.\n"
                      "OUTPUTS: MAIN MIX sends everything to output 1 (outputs 2-4 are silent). SEPARATE sends SYNTHE / BRASS / STRINGS "
                      "to outputs 2 / 3 / 4 only, and Main goes silent, so nothing is doubled in your mixer.",
                      x0 + 24, card.getY() + 144, card.getWidth() - 48, 130, juce::Justification::topLeft, 9);
}

//==============================================================================
TapePage::TapePage (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    const int W = TridexPanel::kW - 48, H = kLowerY - 40;
    setSize (W, H);
    card = { 18, 40, W - 36, 340 };
    const int x = card.getX(), y1 = card.getY() + 64, y2 = card.getY() + 214;

    const auto led = [&] (const char* id, juce::Rectangle<int> r, const juce::String& label, juce::Colour cap)
    {
        auto& b = ctl.led (*this, id, r);
        b.cap = cap;
        labels.push_back ({ &b, label });
    };
    const auto knob = [&] (const char* id, juce::Rectangle<int> r, const juce::String& label)
    {
        labels.push_back ({ &ctl.knob (*this, id, r), label });
    };
    led ("tp_on", { x + 26, y1 + 6, 34, 42 }, "TAPE ON", kCapBlue);

    auto& type = ctl.combo (*this, "tp_type", { x + 90, y1 + 14, 130, 26 });
    type.setTooltip ("Tape type: Studio / Cassette / VHS / Worn VHS");
    labels.push_back ({ &type, "TAPE TYPE" });

    const char* ids[] = { "tp_wow", "tp_flutter", "tp_speed", "tp_drift", "tp_age", "tp_hiss", "tp_mix" };
    const char* names[] = { "WOW", "FLUTTER", "SPEED", "DRIFT", "AGE", "HISS", "MIX" };
    for (int i = 0; i < 7; ++i) knob (ids[i], { x + 272 + i * 116, y1 - 6, 70, 70 }, names[i]);

    // Per-part: select + saturation
    const char* selIds[] = { "tp_syn", "tp_br", "tp_st" };
    const char* drvIds[] = { "tp_drv_syn", "tp_drv_br", "tp_drv_st" };
    const char* parts[] = { "SYNTHE", "BRASS", "STRINGS" };
    for (int i = 0; i < 3; ++i)
    {
        const int px = x + 110 + i * 380;
        led (selIds[i], { px, y2 + 6, 34, 42 }, parts[i], kPartCap[i]);
        knob (drvIds[i], { px + 56, y2 - 6, 70, 70 }, juce::String (parts[i]) + " DRIVE");
    }
}

void TapePage::paint (juce::Graphics& g)
{
    paintPageFrame (g, *this, "TAPE", "WOW / FLUTTER / SATURATION AT THE END OF EACH PART'S CHAIN  /  PRESS TAPE AGAIN TO CLOSE", 80, 760);
    const auto r = card.toFloat();
    paintCard (g, r);
    g.setColour (kInk.withAlpha (0.12f));
    g.drawVerticalLine (card.getX() + 240, r.getY() + 20, r.getY() + 140);
    g.drawHorizontalLine (card.getY() + 160, r.getX() + 20, r.getRight() - 20);

    g.setColour (kInk.withAlpha (0.7f));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold).withKerningFactor (0.1f));
    g.drawText ("SATURATION PER PART", card.getX() + 24, card.getY() + 172, 300, 14, juce::Justification::centredLeft);

    g.setColour (kInk.withAlpha (0.85f));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    for (const auto& [c, text] : labels)
    {
        if (text.isEmpty()) continue;
        const auto cb = c->getBounds().toFloat();
        g.drawText (text, juce::Rectangle<float> (cb.getX() - 24, cb.getBottom() + 4, cb.getWidth() + 48, 12), juce::Justification::centred);
    }

    g.setColour (kInk.withAlpha (0.6f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawFittedText ("STUDIO: clean reel, little wobble.   CASSETTE: more flutter, softer top, a little hiss.   "
                      "VHS: strong wow, darker, noisier.   WORN VHS: very wavy and unstable, with dropouts.\n"
                      "WOW: slow pitch sway.   FLUTTER: fast shimmer.   SPEED: how fast the tape wobbles.   DRIFT: random, unsteady wobble.   "
                      "AGE: darker, worn tape.   HISS: tape noise.   DRIVE: tape saturation for each part.",
                      card.withY (card.getBottom() + 14).withHeight (34), juce::Justification::centredLeft, 3);
}

//==============================================================================
SpaceButtons::SpaceButtons (juce::RangedAudioParameter& p)
    : attachment (p, [this] (float v) { value = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void SpaceButtons::paint (juce::Graphics& g)
{
    const float w = (float) getWidth() / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        const auto r = juce::Rectangle<float> ((float) i * w, 0.0f, w, (float) getHeight()).reduced (3.0f, 1.0f);
        const bool on = i == value;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f).expanded (1.0f), 2.5f);
        g.setGradientFill (juce::ColourGradient (kCapGrey.brighter (0.05f), 0, r.getY(), kCapGrey.darker (on ? 0.3f : 0.15f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 2.0f);
        const auto ledC = juce::Point<float> (r.getCentreX(), r.getY() + 6.5f);
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (kLedOn.withAlpha (0.55f), ledC.x, ledC.y, kLedOn.withAlpha (0.0f), ledC.x + 8.0f, ledC.y, true));
            g.fillEllipse (juce::Rectangle<float> (16, 16).withCentre (ledC));
        }
        g.setColour (on ? kLedOn : kLedOff);
        g.fillEllipse (juce::Rectangle<float> (5, 5).withCentre (ledC));
        g.setColour (juce::Colour (0xff202020));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (juce::String (i + 1), r.withTrimmedTop (12.0f), juce::Justification::centred);
    }
}

void SpaceButtons::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, 3, (int) (e.position.x / ((float) getWidth() / 4.0f)));
    attachment.setValueAsCompleteGesture ((float) i);
}

ChorusPage::ChorusPage (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    const int W = TridexPanel::kW - 48, H = kLowerY - 40;
    setSize (W, H);
    card = { 18, 40, W - 36, 230 };
    const int y = card.getY() + 70;

    const auto led = [&] (const char* id, juce::Rectangle<int> r, const juce::String& label, juce::Colour cap)
    {
        auto& b = ctl.led (*this, id, r);
        b.cap = cap;
        labels.push_back ({ &b, label });
    };
    led ("ch_on", { card.getX() + 26, y, 34, 42 }, "CHORUS ON", kCapBlue);

    auto& mode = ctl.combo (*this, "ch_mode", { card.getX() + 90, y + 8, 120, 26 });
    labels.push_back ({ &mode, "MODE" });

    auto& sp = ctl.add (*this, std::make_unique<SpaceButtons> (*apvts.getParameter ("ch_space")), { card.getX() + 232, y, 150, 42 });
    sp.setTooltip ("SPACE 1-4: depth of the dimension-style mode (SPACE)");
    labels.push_back ({ &sp, "SPACE (MODE: SPACE)" });

    const char* kIds[] = { "ch_rate", "ch_depth", "ch_width", "ch_tone", "ch_mix" };
    const char* kNames[] = { "RATE", "DEPTH", "WIDTH", "TONE", "MIX" };
    for (int i = 0; i < 5; ++i)
        labels.push_back ({ &ctl.knob (*this, kIds[i], { card.getX() + 410 + i * 82, y - 14, 70, 70 }), kNames[i] });

    const char* pIds[] = { "ch_syn", "ch_br", "ch_st" };
    const char* pNames[] = { "SYNTHE", "BRASS", "STRINGS" };
    for (int i = 0; i < 3; ++i)
        led (pIds[i], { card.getX() + 840 + i * 52, y, 34, 42 }, pNames[i], kPartCap[i]);
}

void ChorusPage::paint (juce::Graphics& g)
{
    paintPageFrame (g, *this, "CHORUS / ENSEMBLE",
                    "PICK THE PARTS IT APPLIES TO, LIKE THE FLANGER  /  RUNS BEFORE THE FLANGER  /  PRESS CHORUS AGAIN TO CLOSE", 190, 760);
    const auto r = card.toFloat();
    paintCard (g, r);
    g.setColour (kInk.withAlpha (0.12f));
    for (const int x : { 220, 396, 820 })
        g.drawVerticalLine (card.getX() + x, r.getY() + 20, r.getBottom() - 20);

    g.setColour (kInk.withAlpha (0.85f));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    for (const auto& [c, text] : labels)
    {
        if (text.isEmpty()) continue;
        const auto cb = c->getBounds().toFloat();
        g.drawText (text, juce::Rectangle<float> (cb.getX() - 20, cb.getBottom() + 6, cb.getWidth() + 40, 12), juce::Justification::centred);
    }
    g.setColour (kInk.withAlpha (0.6f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawFittedText ("CHORUS I: gentle stereo chorus.   CHORUS II: faster and deeper.   ENSEMBLE: lush three-phase string ensemble.\n"
                      "SPACE: dimension-style widening, very smooth with almost no pitch wobble; SPACE 1-4 set its depth.   "
                      "WIDTH: stereo spread.   TONE: darker (analog delay) to clearer.",
                      card.withY (card.getBottom() + 14).withHeight (34), juce::Justification::centredLeft, 3);
}

ArpPage::ArpPage (juce::AudioProcessorValueTreeState& state, std::function<int (int)> gateStep)
    : apvts (state), gateStepFn (std::move (gateStep))
{
    const int W = TridexPanel::kW - 48, H = kLowerY - 40;    // panel minus cheeks, below the header
    setSize (W, H);
    const char* ids[] = { "arp1", "arp2", "arp3" };
    const char* names[] = { "SYNTHE", "BRASS", "STRINGS" };
    const int pad = 18, gap = 12, cardY = 36, cardH = (H - cardY - 12 - 2 * gap) / 3, cardW = W - 2 * pad;

    for (int i = 0; i < 3; ++i)
    {
        auto& c = cards[(size_t) i];
        const juce::String id (ids[i]), sg ("sg" + juce::String (i + 1));
        c.name = names[i];
        c.area = { pad, cardY + i * (cardH + gap), cardW, cardH };
        const int x = c.area.getX() + 70, y = c.area.getY() + (cardH - 110) / 2;   // title sits at the left

        // Arpeggiator block
        c.on   = &ctl.led (*this, id + "_on",   { x + 0,   y + 30, 34, 42 });
        c.on->cap = kPartCap[i];
        c.mode = &ctl.combo (*this, id + "_mode", { x + 50,  y + 40, 104, 26 });
        c.rate = &ctl.combo (*this, id + "_rate", { x + 164, y + 40, 78, 26 });
        c.oct  = &ctl.combo (*this, id + "_oct",  { x + 252, y + 40, 52, 26 });
        c.gate = &ctl.knob (*this, id + "_gate", { x + 310, y + 20, 64, 64 });
        c.sync = &ctl.led (*this, id + "_sync", { x + 384, y + 30, 34, 42 });

        // Step gate block, to the right
        const int gx = x + 448;
        c.sgOn   = &ctl.led (*this, sg + "_on", { gx, y + 30, 34, 42 });
        c.sgOn->cap = kPartCap[i];
        c.grid   = &ctl.add (*this, std::make_unique<GateGrid> (apvts, sg), { gx + 46, y + 32, 270, 44 });
        c.grid->colour = kPartCap[i];
        c.sgRate = &ctl.combo (*this, sg + "_rate", { gx + 326, y + 40, 70, 26 });
        c.sgSync = &ctl.led (*this, sg + "_sync", { gx + 404, y + 30, 34, 42 });
    }
    tempo = &ctl.knob (*this, "arp_tempo", { W - 52, 2, 30, 30 }, "none");
    startTimerHz (30);
}

void ArpPage::timerCallback()
{
    if (! isVisible()) return;
    for (int i = 0; i < 3; ++i)
    {
        const int st = gateStepFn ? gateStepFn (i) : -1;
        auto& gr = *cards[(size_t) i].grid;
        if (gr.currentStep != st) { gr.currentStep = st; gr.repaint(); }
    }
}

void ArpPage::paint (juce::Graphics& g)
{
    paintPageFrame (g, *this, "ARPEGGIATOR", "ONE ARP + STEP GATE PER PART  /  SYNC LOCKS TO THE HOST CLOCK  /  PRESS ARP AGAIN TO CLOSE", 150, 700);

    for (const auto& c : cards)
    {
        const auto r = c.area.toFloat();
        paintCard (g, r);

        g.setColour (kInk);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (c.name, juce::Rectangle<float> (r.getX() + 12, r.getY(), 60, r.getHeight()), juce::Justification::centredLeft);
        g.setColour (kInk.withAlpha (0.15f));
        g.drawVerticalLine ((int) r.getX() + 64, r.getY() + 10, r.getBottom() - 10);

        g.setColour (kInk.withAlpha (0.85f));
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        const auto lbl = [&] (juce::Component& comp, const juce::String& s)
        {
            const auto cb = comp.getBounds().toFloat();
            g.drawText (s, juce::Rectangle<float> (cb.getX() - 10, cb.getBottom() + 6, cb.getWidth() + 20, 12), juce::Justification::centred);
        };
        lbl (*c.on, "ARP ON"); lbl (*c.mode, "MODE"); lbl (*c.rate, "RATE"); lbl (*c.oct, "OCTAVES"); lbl (*c.sync, "SYNC");
        const auto gb = c.gate->getBounds().toFloat();
        g.drawText ("NOTE LEN", juce::Rectangle<float> (gb.getX() - 6, gb.getBottom() - 2, gb.getWidth() + 12, 12), juce::Justification::centred);
        lbl (*c.sgOn, "STEP GATE"); lbl (*c.sgRate, "RATE"); lbl (*c.sgSync, "SYNC");

        // divider between the arp block and the step gate block
        g.setColour (kInk.withAlpha (0.12f));
        g.drawVerticalLine (c.sgOn->getX() - 14, r.getY() + 10, r.getBottom() - 10);
        g.setColour (kInk.withAlpha (0.85f));
    }

    g.setColour (kInk.withAlpha (0.85f));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    const auto tb = tempo->getBounds();
    g.drawText ("TEMPO (WHEN SYNC IS OFF)", tb.getX() - 206, tb.getCentreY() - 6, 200, 12, juce::Justification::centredRight);
}
} // namespace tdxui

//==============================================================================
juce::Slider& TridexPanel::knob (const char* id, int cx, int cy, const juce::String& label, bool white, int size)
{
    auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    auto* param = proc.apvts.getParameter (id);
    jassert (param != nullptr);
    s->getProperties().set ("white", white);
    s->getProperties().set ("scale", size < 30 ? "none" : (param->getNormalisableRange().start < 0.0f ? "bip" : "uni"));
    s->setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s->setPopupDisplayEnabled (true, true, this);
    s->setTooltip (param->getName (64));
    const int box = size + 22;
    s->setBounds (cx - box / 2, cy - box / 2, box, box);
    addAndMakeVisible (*s);
    sAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *s));
    if (label.isNotEmpty())
        text ({ (float) cx - 30, (float) (cy + box / 2 - 4), 60, 11 }, label, 8.5f, false, juce::Justification::centred, white ? kInk : juce::Colour());
    auto& ref = *s;
    owned.push_back (std::move (s));
    return ref;
}

juce::Slider& TridexPanel::switchKnob (const char* id, int cx, int cy, const juce::String& label, bool white)
{
    auto& s = knob (id, cx, cy, label, white);
    auto* choice = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id));
    jassert (choice != nullptr);
    const int n = choice->choices.size();
    const float half = juce::degreesToRadians (20.0f * (float) (n - 1));
    s.setRotaryParameters (juce::MathConstants<float>::twoPi - half, juce::MathConstants<float>::twoPi + half, true);
    juce::StringArray shortNames;
    for (auto c : choice->choices) shortNames.add (c.length() > 5 ? c.substring (0, 3).toUpperCase() : c.toUpperCase());
    s.getProperties().set ("labels", shortNames.joinIntoString ("|"));
    return s;
}

tdxui::LedButton& TridexPanel::led (const char* id, int cx, int y, const juce::String& label)
{
    auto b = std::make_unique<tdxui::LedButton> (label);
    b->setClickingTogglesState (true);
    b->setBounds (cx - 15, y, 30, 36);
    b->setTooltip (proc.apvts.getParameter (id)->getName (64));
    {
        // Cap colours by role, as on the original: blue OUTPUT, cream footage, tan delay vibrato, grey otherwise.
        const juce::String sid (id);
        if (sid == "syn_on" || sid == "br_on" || sid == "st_on") b->cap = kCapBlue;
        else if (sid == "br_16" || sid == "br_8" || sid == "st_16" || sid == "st_8" || sid == "st_4") b->cap = kCapCream;
        else if (sid == "dvib_on") b->cap = kCapTan;
        else if (sid == "fl_syn") b->cap = kPartCap[0];
        else if (sid == "fl_br")  b->cap = kPartCap[1];
        else if (sid == "fl_st")  b->cap = kPartCap[2];
        else b->cap = kCapGrey;
    }
    addAndMakeVisible (*b);
    bAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b));
    text ({ (float) cx - 18, (float) y + 38, 36, 20 }, label, 7.0f, false, juce::Justification::centredTop);
    auto& ref = *b;
    owned.push_back (std::move (b));
    return ref;
}

tdxui::LedButton& TridexPanel::progButton (const juce::String& label, int cx, int y, std::function<void()> onClick)
{
    auto b = std::make_unique<tdxui::LedButton> (label);
    b->setBounds (cx - 14, y, 28, 36);
    b->cap = kCapWhite;
    b->onClick = std::move (onClick);
    addAndMakeVisible (*b);
    text ({ (float) cx - 20, (float) y + 38, 40, 10 }, label, 8.0f, true);
    auto& ref = *b;
    owned.push_back (std::move (b));
    return ref;
}

void TridexPanel::octave (const char* id, int cx, int y)
{
    auto o = std::make_unique<tdxui::OctaveControl> (*proc.apvts.getParameter (id));
    if (const int pi = partIndexForId (id); pi >= 0) o->colour = kPartCol[pi];
    o->setBounds (cx - 22, y, 44, 36);
    o->setTooltip (proc.apvts.getParameter (id)->getName (64) + " (double-click the display to reset)");
    addAndMakeVisible (*o);
    owned.push_back (std::move (o));
    text ({ (float) cx - 24, (float) y + 38, 48, 10 }, "OCTAVE", 7.0f);
}

void TridexPanel::modeSwitch (const char* id, int cx, int cy, bool orangeLegend, const juce::String& left, const juce::String& right)
{
    auto sw = std::make_unique<tdxui::ModeSwitch>();
    sw->accent = orangeLegend ? kInk : kInk;
    sw->leftLabel = left; sw->rightLabel = right;
    sw->setBounds (cx - 28, cy - 18, 56, 30);
    sw->setTooltip (left == "PORTA" ? "Glide mode: PORTA (smooth slide) / GLISS (semitone steps)"
                                    : left == "PARA" ? "Brass voicing: PARA (shared EG/VCA, like the original) / POLY (own EG, VCF and VCA per note)"
                                    : "MONO mode: RETRIG (every key restarts the envelopes) / LEGATO (no restart while a key is held)");
    addAndMakeVisible (*sw);
    bAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *sw));
    owned.push_back (std::move (sw));
    text ({ (float) cx - 27, (float) cy + 16, 54, 11 }, left + " / " + right, 7.0f, false, juce::Justification::centred, orangeLegend ? kInk : juce::Colour());
}

void TridexPanel::text (juce::Rectangle<float> r, const juce::String& s, float size, bool bold, juce::Justification j, juce::Colour col)
{
    texts.push_back ({ r, s, size, bold, j, col });
}

//==============================================================================
TridexPanel::TridexPanel (TridexProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    setSize (kW, kH);

    int x = 34;   // just inside the left wood cheek
    const auto col = [&] (int i) { return x + kCol / 2 + i * kCol; };
    const auto sectionEnd = [&] (int cols) { const int start = x; x += cols * kCol + 12; return std::pair<int, int> { start, x - 12 }; };

    // ---------------- KEY ASSIGN
    {
        auto sw = std::make_unique<tdxui::SlideSwitch>();
        sw->setBounds (col (0) - 14, kRow1 - 22, 30, 46);
        sw->setTooltip ("Assign Mode (1: each note its own voice / 2: same voice reused)");
        addAndMakeVisible (*sw);
        bAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "assign2", *sw));
        owned.push_back (std::move (sw));
        text ({ (float) col (0) - 30, (float) kRow1 + 26, 60, 11 }, "ASSIGN MODE");
        knob ("tune", col (0), kRow2, "TOTAL TUNE", false);

        const char* ids[] = { "syn_zone", "br_zone", "st_zone" };
        const char* names[] = { "SYNTHE", "BRASS", "STRINGS" };
        for (int i = 0; i < 3; ++i)
        {
            const int y = kRow1 - 30 + i * 46;
            auto sel = std::make_unique<tdxui::SplitSelector> (*proc.apvts.getParameter (ids[i]));
            sel->colour = kPartCol[i];
            sel->setBounds (col (1) - 22, y, 44, 22);
            sel->setTooltip (juce::String (names[i]) + " split: lower 2 / upper 3 octaves");
            addAndMakeVisible (*sel);
            owned.push_back (std::move (sel));
            text ({ (float) col (1) - 28, (float) y + 22, 56, 10 }, names[i], 8.0f, true, juce::Justification::centred, kInk);

            // Tridex II: this part's own split point
            const char* splitIds[] = { "syn_split", "br_split", "st_split" };
            auto sp = std::make_unique<tdxui::SplitPointControl> (*proc.apvts.getParameter (splitIds[i]));
            sp->colour = kPartCol[i];
            sp->setBounds (col (1) - 22, y + 32, 44, 13);
            sp->setTooltip (juce::String (names[i]) + " split point: click the arrows, drag, or scroll; double-click for C3");
            addAndMakeVisible (*sp);
            owned.push_back (std::move (sp));
        }
        // Tridex II: polyphony, 8 (classic) or 16 voices shared by all parts
        {
            auto pd = std::make_unique<tdxui::ChoiceScreen> (*proc.apvts.getParameter ("polyphony"));
            pd->cycleOnClick = true; pd->fontSize = 12.0f; pd->labels = { " 8 VOICE", "16 VOICE" };
            pd->setBounds (x + 14, kSwY + 14, 2 * kCol - 28, 24);
            pd->setTooltip ("Polyphony: 8 voices (classic Trident) or 16 voices, shared by all three parts. Click to switch.");
            addAndMakeVisible (*pd);
            owned.push_back (std::move (pd));
            text ({ (float) x, (float) kSwY + 42, 2.0f * kCol, 11 }, "POLYPHONY", 8.0f, true);
        }
        auto [a, b] = sectionEnd (2);
        moduleLabel ((float) a + kCol, (float) b, kModY, "KBD SPLIT");
        sectionBar ((float) a, (float) b, kBarY, "KEY ASSIGN");
    }

    // ---------------- FLANGER
    {
        knob ("fl_speed", col (0), kRow1, "SPEED", false);
        knob ("fl_fb", col (1), kRow1, "FEEDBACK", false);
        knob ("fl_int", col (0), kRow2, "INTENSITY", false);
        knob ("fl_manual", col (1), kRow2, "MANUAL", false);
        const int c0 = col (0) - 6, step = 32;
        led ("fl_syn", c0, kSwY, "SYNTHE");
        led ("fl_br", c0 + step, kSwY, "BRASS");
        led ("fl_st", c0 + step * 2, kSwY, "STRINGS");
        auto [a, b] = sectionEnd (2);
        sectionBar ((float) a, (float) b, kBarY, "FLANGER");
    }

    // ---------------- SYNTHESIZER (white knobs = programmable)
    {
        const int s0 = x;
        switchKnob ("v1_oct", col (0), kRow1, "OCTAVE", true);
        switchKnob ("v1_wave", col (1), kRow1, "WAVEFORM", true);
        knob ("pw_int", col (0), kRow2, "PW/PWM", true);
        knob ("pwm_speed", col (1), kRow2, "PWM SPEED", true);
        moduleLabel ((float) s0, (float) s0 + 2 * kCol, kModY, "VCO 1", kInk);

        switchKnob ("v2_scale", col (2), kRow1, "OCTAVE", true);
        switchKnob ("v2_wave", col (3), kRow1, "WAVEFORM", true);
        knob ("v2_detune", col (2), kRow2, "DETUNE", true);
        knob ("vco_mix", col (3), kRow2, "VCO MIX", true);
        moduleLabel ((float) s0 + 2 * kCol, (float) s0 + 4 * kCol, kModY, "VCO 2", kInk);

        knob ("cutoff", col (4), kRow1, "CUTOFF FREQ", true);
        knob ("res", col (5), kRow1, "RESONANCE", true);
        switchKnob ("kbd_track", col (4), kRow2, "KBD TRACK", true);
        knob ("eg_int", col (5), kRow2, "EG INTENSITY", true);
        moduleLabel ((float) s0 + 4 * kCol, (float) s0 + 6 * kCol, kModY, "VCF", kInk);

        const char* fe[] = { "sf_a", "sf_d", "sf_s", "sf_r" };
        const char* ae[] = { "sa_a", "sa_d", "sa_s", "sa_r" };
        const char* nm[] = { "ATTACK", "DECAY", "SUSTAIN", "RELEASE" };
        for (int i = 0; i < 4; ++i)
        {
            knob (fe[i], col (6 + i), kRow1, nm[i], true);
            knob (ae[i], col (6 + i), kRow2, nm[i], true);
        }
        text ({ (float) s0 + 6 * kCol, (float) kRow1 - 41, 4.0f * kCol, 11 }, "VCF EG", 9.5f, true, juce::Justification::centred, kInk);
        moduleLabel ((float) s0 + 6 * kCol, (float) s0 + 10 * kCol, kModY, "VCA EG", kInk);

        knob ("vca_att", col (10), kRow1, "ATTENUATOR", true);
        led ("auto_damp", col (10), kRow2 - 22, "AUTO DAMP");
        moduleLabel ((float) s0 + 10 * kCol, (float) s0 + 11 * kCol, kModY, "VCA", kInk);
        // Tridex II: glide (portamento / glissando)
        knob ("syn_glide", col (11), kRow1, "GLIDE TIME", true);
        modeSwitch ("syn_gliss", col (11), kRow2, true);
        // Tridex II: MONO column (MONO on/off + RETRIG / LEGATO)
        led ("syn_mono", col (12), kRow1 - 22, "MONO");
        modeSwitch ("syn_legato", col (12), kRow2, true, "RETRIG", "LEGATO");
        moduleLabel ((float) s0 + 12 * kCol, (float) s0 + 13 * kCol, kModY, "MONO", kInk);
        moduleLabel ((float) s0 + 11 * kCol, (float) s0 + 12 * kCol, kModY, "GLIDE", kInk);

        // Programmer strip: WRITE  MANUAL | A B C D | 1..8, then MIXER
        int bx = s0 + 16;
        writeBtn = &progButton ("WRITE", bx, kSwY, [this] { writeArmed = ! writeArmed; pendingBank = proc.getCurrentProgram() / 8; });
        writeBtn->cap = kCapGrey;   // the LED still shows red while armed
        manualBtn = &progButton ("MANUAL", bx += 32, kSwY, [this] { proc.setManual(); writeArmed = false; });
        bx += 10;
        for (int i = 0; i < 4; ++i)
            bankBtns[(size_t) i] = &progButton (juce::String::charToString ((juce::juce_wchar) ('A' + i)), bx += 30, kSwY, [this, i] { bankPressed (i); });
        bx += 8;
        for (int i = 0; i < 8; ++i)
            progBtns[(size_t) i] = &progButton (juce::String (i + 1), bx += 30, kSwY, [this, i] { programPressed (i); });
        text ({ (float) s0 + 80, (float) kSwY + 50, 360, 10 }, "PROGRAMMER", 8.0f, true);

        octave ("syn_octsh", s0 + 13 * kCol - 118, kSwY);
        knob ("syn_vol", s0 + 13 * kCol - 62, kSwY + 18, "VOLUME", false, 30);
        led ("syn_on", s0 + 13 * kCol - 20, kSwY, "OUTPUT");
        auto [a, b] = sectionEnd (13);
        sectionBar ((float) a, (float) b, kBarY, "SYNTHESIZER");
    }

    // ---- Second row of sections (Brass, Strings, Output), stacked under the first
    x = 34;

    // ---------------- BRASS
    {
        const int s0 = x;
        // OSC 1 (footage on the 16'/8' buttons below) and OSC 2
        switchKnob ("br_wave", col (0), (kRow1 + kTier), "OSC 1 WAVE", false);
        switchKnob ("br2_oct", col (0), (kRow2 + kTier), "OSC 2 OCTAVE", false);
        switchKnob ("br2_wave", col (1), (kRow1 + kTier), "OSC 2 WAVE", false);
        knob ("br2_detune", col (1), (kRow2 + kTier), "DETUNE", false);
        knob ("br_oscmix", col (2), (kRow1 + kTier), "OSC MIX", false);
        moduleLabel ((float) s0, (float) s0 + 3 * kCol, (kModY + kTier), "OSCILLATORS");

        knob ("br_cutoff", col (3), (kRow1 + kTier), "CUTOFF FREQ", false);
        knob ("br_res", col (4), (kRow1 + kTier), "RESONANCE", false);
        knob ("br_a", col (5), (kRow1 + kTier), "ATTACK", false);
        knob ("br_d", col (6), (kRow1 + kTier), "DECAY", false);
        switchKnob ("br_silence", col (3), (kRow2 + kTier), "SILENCE NOTE", false);
        knob ("br_egint", col (4), (kRow2 + kTier), "EG INTENSITY", false);
        knob ("br_s", col (5), (kRow2 + kTier), "SUSTAIN", false);
        knob ("br_r", col (6), (kRow2 + kTier), "RELEASE", false);
        moduleLabel ((float) s0 + 3 * kCol, (float) s0 + 5 * kCol, (kModY + kTier), "VCF");
        moduleLabel ((float) s0 + 5 * kCol, (float) s0 + 7 * kCol, (kModY + kTier), "EG");

        knob ("br_vol", s0 + 20, (kSwY + kTier) + 18, "VOLUME", false, 30);
        int bx = s0 + 54;
        led ("br_on", bx, (kSwY + kTier), "OUTPUT");
        led ("br_16", bx += 40, (kSwY + kTier), "OSC 1 16'");
        led ("br_8", bx += 38, (kSwY + kTier), "OSC 1 8'");
        led ("br_multi", bx += 46, (kSwY + kTier), "MULTIPLE TRIGGER");
        led ("br_tsel", bx += 42, (kSwY + kTier), "TRIGGER SELECT");
        octave ("br_octsh", bx += 46, (kSwY + kTier));
        // Tridex II: per-note brass filter switch
        moduleLabel ((float) s0 + 7 * kCol, (float) s0 + 8 * kCol, (kModY + kTier), "VOICING");
        {
            // VOICING (Tridex II): PARA (original, one shared filter + EG) / PARA+VCF (own filter per note) / POLY
            auto vs = std::make_unique<tdxui::ChoiceScreen> (*proc.apvts.getParameter ("br_voicing"));
            vs->fontSize = 9.5f;
            vs->setBounds (col (7) - 32, (kRow2 + kTier) - 12, 64, 22);
            vs->setTooltip ("Brass voicing: PARA = one filter + envelope for the section (original) / "
                            "PARA+VCF = own filter per note, shared envelope / POLY = own filter + envelope per note");
            addAndMakeVisible (*vs);
            owned.push_back (std::move (vs));
            text ({ (float) col (7) - 33, (float) (kRow2 + kTier) + 12, 66, 11 }, "PARA / +VCF / POLY", 7.0f, false, juce::Justification::centred);
        }
        // Tridex II: glide (portamento / glissando)
        knob ("br_glide", col (8), (kRow1 + kTier), "GLIDE TIME", false);
        modeSwitch ("br_gliss", col (8), (kRow2 + kTier), false);
        // Tridex II: MONO column (MONO on/off + RETRIG / LEGATO)
        led ("br_mono", col (9), (kRow1 + kTier) - 22, "MONO");
        modeSwitch ("br_legato", col (9), (kRow2 + kTier), false, "RETRIG", "LEGATO");
        moduleLabel ((float) s0 + 9 * kCol, (float) s0 + 10 * kCol, (kModY + kTier), "MONO");
        moduleLabel ((float) s0 + 8 * kCol, (float) s0 + 9 * kCol, (kModY + kTier), "GLIDE");
        auto [a, b] = sectionEnd (10);
        sectionBar ((float) a, (float) b, (kBarY + kTier), "BRASS");
    }

    // ---------------- STRINGS
    {
        const int s0 = x;
        knob ("st_attack", col (0), (kRow1 + kTier), "ATTACK", false);
        knob ("st_release", col (0), (kRow2 + kTier), "RELEASE", false);
        knob ("st_eq_hi", col (1), (kRow1 + kTier), "HIGH", false);
        knob ("st_eq_lo", col (1), (kRow2 + kTier), "LOW", false);
        knob ("bow_level", col (2), (kRow1 + kTier), "LEVEL", false);
        knob ("bow_tone", col (2), (kRow2 + kTier), "TONE", false);
        knob ("svib_delay", col (3), (kRow1 + kTier), "DELAY TIME", false);
        knob ("svib_int", col (3), (kRow2 + kTier), "INTENSITY", false);
        knob ("svib_speed", col (4), (kRow1 + kTier), "SPEED", false);
        knob ("st_balance", col (4), (kRow2 + kTier), "KBD BALANCE", false);
        moduleLabel ((float) s0, (float) s0 + kCol, (kModY + kTier), "ENVELOPE");
        moduleLabel ((float) s0 + kCol, (float) s0 + 2 * kCol, (kModY + kTier), "EQUALIZER");
        moduleLabel ((float) s0 + 2 * kCol, (float) s0 + 3 * kCol, (kModY + kTier), "BOWING");
        moduleLabel ((float) s0 + 3 * kCol, (float) s0 + 4 * kCol, (kModY + kTier), "VIBRATO");
        moduleLabel ((float) s0 + 4 * kCol, (float) s0 + 5 * kCol, (kModY + kTier), "FILTER");
        // Tridex II: strings VCF
        knob ("st_cutoff", col (5), (kRow1 + kTier), "CUTOFF FREQ", false);
        knob ("st_res", col (5), (kRow2 + kTier), "RESONANCE", false);
        moduleLabel ((float) s0 + 5 * kCol, (float) s0 + 6 * kCol, (kModY + kTier), "VCF");

        knob ("st_vol", s0 + 20, (kSwY + kTier) + 18, "VOLUME", false, 30);
        int bx = s0 + 54;
        led ("st_on", bx, (kSwY + kTier), "OUTPUT");
        // Footages, each with a small level knob underneath (Tridex II)
        {
            const char* feet[] = { "st_16", "st_8", "st_4" };
            const char* lvls[] = { "st_lvl16", "st_lvl8", "st_lvl4" };
            const char* names[] = { "16'", "8'", "4'" };
            for (int k = 0; k < 3; ++k)
            {
                led (feet[k], bx += (k == 0 ? 38 : 34), (kSwY + kTier), names[k]);
                knob (lvls[k], bx, (kSwY + kTier) + 63, "", false, 21).setTooltip (juce::String ("Strings ") + names[k] + " level");
            }
        }
        led ("bow_on", bx += 38, (kSwY + kTier), "BOWING");
        led ("svib_off", bx += 36, (kSwY + kTier), "VIBRATO\nOFF");
        led ("ens_off", bx += 36, (kSwY + kTier), "ENSEMBLE\nOFF");
        led ("st_divide", bx += 36, (kSwY + kTier), "DIVIDE\nDOWN").setTooltip (
            "DIVIDE-DOWN: all strings notes come from one top-octave bank, so octaves are phase-locked like a string machine");
        octave ("st_octsh", bx += 42, (kSwY + kTier));
        auto [a, b] = sectionEnd (6);
        sectionBar ((float) a, (float) b, (kBarY + kTier), "STRINGS");
    }

    // ---------------- OUTPUT
    {
        knob ("master", col (0), (kRow2 + kTier), "TOTAL VOLUME", false);
        text ({ (float) x, (float) (kRow1 + kTier) - 30, (float) kCol, 11 }, "POWER ON", 8.0f, true);
        auto [a, b] = sectionEnd (1);
        sectionBar ((float) a, (float) b, (kBarY + kTier), "VOLUME");
    }

    // ---------------- Lower left: DELAY VIBRATO + JOYSTICK
    {
        const int y0 = kLowerY + 8;
        knob ("dvib_int", 70, y0 + 34, "INTENSITY", false);
        led ("dvib_on", 128, y0 + 14, "DELAY VIBRATO");
        sectionBar (40, 160, (float) y0 + 92, "DELAY VIBRATO");

        knob ("mg_int", 70, y0 + 136, "INTENSITY", false);
        knob ("mg_speed", 128, y0 + 136, "SPEED", false);
        auto js = std::make_unique<tdxui::Joystick> (proc.guiJoyX, proc.guiJoyY);
        js->setBounds (178, y0 + 40, 128, 138);

        // TRANSPOSE (Tridex II), above the joystick
        auto tr = std::make_unique<tdxui::TransposeControl> (*proc.apvts.getParameter ("transpose"));
        tr->setBounds (184, y0 + 14, 116, 24);
        tr->setTooltip ("Transpose -12..+12 semitones (double-click the value to reset)");
        addAndMakeVisible (*tr);
        owned.push_back (std::move (tr));
        text ({ 184.0f, (float) y0 + 1, 116.0f, 11 }, "TRANSPOSE", 8.0f, true);
        addAndMakeVisible (*js);
        owned.push_back (std::move (js));
        sectionBar (40, 306, (float) y0 + 188, "JOYSTICK");

        // Assignable LFO (Tridex II): its panel takes the joystick area; the LFO button stays on top.
        lfoPanel = std::make_unique<tdxui::LfoPanel> (proc.apvts, [this] { return proc.getLfoPhase(); }, [this] { return proc.getLfoValue(); });
        lfoPanel->setBounds (26, kLowerY + 2, kKeysX - 28, kH - kLowerY - kRail - 4);
        addChildComponent (*lfoPanel);
        auto lb = std::make_unique<tdxui::LedButton> ("LFO");
        lb->setBounds (146, y0 + 14, 30, 36);
        lb->setTooltip ("Show the LFO (in place of the joystick); press again for the joystick");
        lb->onClick = [this] { lfoPanel->setVisible (! lfoPanel->isVisible()); lfoBtn->toFront (false); };
        addAndMakeVisible (*lb);
        lfoBtn = lb.get();
        owned.push_back (std::move (lb));
        text ({ 136.0f, (float) y0 + 52, 50.0f, 10.0f }, "LFO", 7.0f, false, juce::Justification::centredTop);
    }


    lcd = { 300, 9, 130, 24 };

    // ---------------- Overlay pages and their top-bar buttons (Tridex II)
    arpPage = std::make_unique<tdxui::ArpPage> (proc.apvts, [this] (int sec) { return proc.getGateStep (sec); });
    addPage (*arpPage, "ARP", 10, 34, "Show / hide the arpeggiator page");

    // ABOUT page: click the Irife logo
    aboutPage = std::make_unique<tdxui::AboutPage> ([this] { aboutPage->setVisible (false); });
    addPage (*aboutPage);
    logoHotspot = std::make_unique<tdxui::LogoHotspot>();
    logoHotspot->setBounds (188, 2, 100, 34);
    logoHotspot->setTooltip ("About Tridex II");
    logoHotspot->onClick = [this] { togglePage (aboutPage.get()); };
    addAndMakeVisible (*logoHotspot);

    chorusPage = std::make_unique<tdxui::ChorusPage> (proc.apvts);
    addPage (*chorusPage, "CHORUS", 80, 56, "Show / hide the chorus page");
    tapePage = std::make_unique<tdxui::TapePage> (proc.apvts);
    addPage (*tapePage, "TAPE", 176, 40, "Show / hide the tape page");
    fxPage = std::make_unique<tdxui::FxPage> (proc.apvts);
    addPage (*fxPage, "FX", 254, 24, "Show / hide the FX routing page (effect order per part)");
    settingsPage = std::make_unique<tdxui::SettingsPage> (proc.apvts);
    addPage (*settingsPage, "SETTINGS", 316, 64, "Show / hide the settings page");

    // ---------------- Keyboard (61 keys, C-C)
    keyboard.setAvailableRange (36, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setBounds (kKeysX + 2, kLowerY + 6 + kStrip, kW - kKeysX - 32, kH - kLowerY - 12 - kRail - kStrip);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff2ecdc));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff101010));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff8f8a7e));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x22e8893a));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kLedOn.withAlpha (0.45f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.35f));
    addAndMakeVisible (keyboard);

    // Split indicators in their own strip above the keys
    splitOverlay = std::make_unique<tdxui::SplitOverlay> (proc.apvts, keyboard);
    splitOverlay->setBounds (keyboard.getX(), kLowerY + 4, keyboard.getWidth(), kStrip);
    addAndMakeVisible (*splitOverlay);
    splitOverlay->refreshIfChanged();

    startTimerHz (15);
}

TridexPanel::~TridexPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TridexPanel::bankPressed (int bank)
{
    if (writeArmed) { pendingBank = bank; return; }
    // As on the hardware: a bank press recalls the same program number in that bank.
    const int num = proc.isManual() ? 0 : proc.getCurrentProgram() % 8;
    pendingBank = bank;
    proc.recallProgram (bank * 8 + num);
}

void TridexPanel::programPressed (int number)
{
    if (writeArmed)
    {
        proc.writeProgram (pendingBank * 8 + number);
        writeArmed = false;
        return;
    }
    const int bank = proc.isManual() ? pendingBank : proc.getCurrentProgram() / 8;
    proc.recallProgram (bank * 8 + number);
}

void TridexPanel::addPage (juce::Component& page, const char* button, int x, int labelW, const juce::String& tip)
{
    page.setBounds (24, 40, kW - 48, kLowerY - 40);
    addChildComponent (page);
    pages.push_back (&page);
    if (button == nullptr) return;

    auto b = std::make_unique<tdxui::LedButton> (button);
    b->setBounds (lcd.getRight() + x, 4, 30, 32);
    b->setTooltip (tip);
    b->onClick = [this, &page] { togglePage (&page); };
    addAndMakeVisible (*b);
    pageButtons.push_back ({ b.get(), &page });
    owned.push_back (std::move (b));
    text ({ (float) lcd.getRight() + x + 34, 13, (float) labelW, 14 }, button, 10.0f, true, juce::Justification::centredLeft);
}

void TridexPanel::togglePage (juce::Component* page)
{
    if (page == nullptr) return;
    const bool show = ! page->isVisible();
    for (auto* p : pages) p->setVisible (false);
    page->setVisible (show);
    if (show) page->toFront (false);
}

void TridexPanel::timerCallback()
{
    const bool manual = proc.isManual();
    const int cur = proc.getCurrentProgram();
    const bool blink = (juce::Time::getMillisecondCounter() / 250) % 2 == 0;

    writeBtn->lit = writeArmed && blink;
    manualBtn->lit = manual;
    for (int i = 0; i < 4; ++i)
        bankBtns[(size_t) i]->lit = writeArmed ? (i == pendingBank) : (! manual && cur / 8 == i);
    for (int i = 0; i < 8; ++i)
        progBtns[(size_t) i]->lit = ! manual && ! writeArmed && cur % 8 == i;
    writeBtn->repaint(); manualBtn->repaint();
    if (splitOverlay) splitOverlay->refreshIfChanged();
    for (auto& [btn, page] : pageButtons)   // a page button is lit while its page is open
        if (btn->lit != page->isVisible()) { btn->lit = page->isVisible(); btn->repaint(); }
    if (lfoBtn != nullptr)
    {
        // Lit while the LFO panel is open; otherwise it flashes with the LFO when a destination is in use.
        const bool used = (proc.apvts.getRawParameterValue ("lfo_dest1")->load() > 0.5f && std::abs (proc.apvts.getRawParameterValue ("lfo_amt1")->load()) > 0.001f)
                       || (proc.apvts.getRawParameterValue ("lfo_dest2")->load() > 0.5f && std::abs (proc.apvts.getRawParameterValue ("lfo_amt2")->load()) > 0.001f);
        const bool lit = lfoPanel->isVisible() || (used && proc.getLfoValue() > 0.0f);
        if (lfoBtn->lit != lit) { lfoBtn->lit = lit; lfoBtn->repaint(); }
    }
    for (auto* b : bankBtns) b->repaint();
    for (auto* b : progBtns) b->repaint();

    const auto t = writeArmed ? juce::String ("WRITE: PICK BANK + PROGRAM")
                              : (manual ? juce::String ("MANUAL") : proc.getProgramName (cur));
    if (t != lcdText) { lcdText = t; repaint (lcd); }
}

void TridexPanel::paint (juce::Graphics& g)
{
    // Wooden end cheeks + panel
    // Walnut end cheeks with a little grain
    g.fillAll (kWood);
    g.setGradientFill (juce::ColourGradient (kWood.brighter (0.2f), 0, 0, kWood.darker (0.35f), 24, 0, false));
    g.fillRect (0, 0, 24, kH);
    g.setGradientFill (juce::ColourGradient (kWood.darker (0.35f), (float) kW - 24, 0, kWood.brighter (0.2f), (float) kW, 0, false));
    g.fillRect (kW - 24, 0, 24, kH);
    for (int k = 0; k < 9; ++k)
        for (int side = 0; side < 2; ++side)
        {
            const float x0 = (side == 0 ? 2.0f : (float) kW - 22.0f) + (float) k * 2.3f;
            juce::Path grain;
            grain.startNewSubPath (x0, 0.0f);
            for (int y = 0; y <= kH; y += 20)
                grain.lineTo (x0 + 0.9f * std::sin ((float) y * 0.021f + (float) (k * 1.7 + side)), (float) y);
            g.setColour (juce::Colours::black.withAlpha (0.10f + 0.03f * (float) (k % 3)));
            g.strokePath (grain, juce::PathStrokeType (0.7f));
        }

    // Top lid strip (header) and the main charcoal panel
    g.setColour (kLid);
    g.fillRect (24, 0, kW - 48, 38);
    g.setGradientFill (juce::ColourGradient (kPanel.brighter (0.05f), 0, 38, kPanel.darker (0.12f), 0, (float) kLowerY, false));
    g.fillRect (24, 38, kW - 48, kLowerY - 38);

    for (const int ty : { 0, kTier })
    {
        // Switch tier: a second panel strip under the knobs, as on the hardware
        g.setColour (kPanel2);
        g.fillRect (24, kSwY + ty - 12, kW - 48, kBarY + 18 - (kSwY - 12));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (24, kSwY + ty - 13, kW - 48, 1);
        g.setColour (kInk.withAlpha (0.10f));
        g.fillRect (24, kSwY + ty - 12, kW - 48, 1);

        // Section header bars across the top of each row (light grey, dark lettering) + dividers
        const auto headFont = juce::Font (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.10f));
        std::vector<float> ends;
        for (const auto& bar : bars)
        {
            if (! juce::approximatelyEqual (bar.y, (float) (kBarY + ty))) continue;
            ends.push_back (bar.x2);
            const auto r = juce::Rectangle<float> (bar.x1 - 2.0f, (float) (kHeadY + ty), bar.x2 - bar.x1 + 4.0f, 15.0f);
            g.setGradientFill (juce::ColourGradient (kHeader.brighter (0.06f), 0, r.getY(), kHeader.darker (0.08f), 0, r.getBottom(), false));
            g.fillRect (r);
            g.setColour (kHeaderInk);
            g.setFont (headFont);
            g.drawText (bar.s, r, juce::Justification::centred);
        }
        std::sort (ends.begin(), ends.end());
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        for (size_t i = 0; i + 1 < ends.size(); ++i)
            g.fillRect (ends[i] + 5.5f, (float) (kHeadY + ty), 1.0f, (float) (kBarY + 16 - kHeadY));
    }
    // Seam between the two rows
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (24, kBarY + 22, kW - 48, 2);

    // Lower area: joystick panel and the keybed, wooden rail under the keys
    g.setColour (kPanel2.darker (0.15f));
    g.fillRect (24, kLowerY, kKeysX - 24, kH - kLowerY - kRail);
    g.setColour (juce::Colours::black);
    g.fillRect (kKeysX, kLowerY, kW - kKeysX - 24, kH - kLowerY - kRail);
    g.setGradientFill (juce::ColourGradient (kWood.brighter (0.25f), 0, (float) (kH - kRail), kWood.darker (0.3f), 0, (float) kH, false));
    g.fillRect (0, kH - kRail, kW, kRail);

    // Header: original wordmark + program display
    static const juce::Image wordmark = juce::ImageCache::getFromMemory (BinaryData::tridex_logo_png, BinaryData::tridex_logo_pngSize);
    if (wordmark.isValid())
        g.drawImageWithin (wordmark, 34, 5, 150, 29, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid);
    // I Rife logo, small, beside the wordmark
    static const juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::irife_logo_png, BinaryData::irife_logo_pngSize);
    if (logo.isValid())
        g.drawImageWithin (logo, 190, 3, 96, 32, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::onlyReduceInSize);

    g.setColour (juce::Colour (0xff1d0705));
    g.fillRoundedRectangle (lcd.toFloat(), 3.0f);
    g.setColour (kLedOn);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawText (lcdText.toUpperCase(), lcd.reduced (8, 0), juce::Justification::centredLeft);

    g.setColour (kInk.withAlpha (0.15f));
    g.drawHorizontalLine (38, 24.0f, (float) kW - 24);

    for (const auto& t : texts)
    {
        g.setColour (t.col.isTransparent() ? kInk.withAlpha (0.92f) : t.col);
        g.setFont (juce::FontOptions (t.size * 0.96f, t.bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (0.05f));
        g.drawFittedText (t.s, t.r.toNearestInt(), t.j, 2, 0.75f);
    }

    // Section bars:  ━━━ NAME ━━━
    const auto barFont = juce::Font (juce::FontOptions (9.0f, juce::Font::bold).withKerningFactor (0.18f));
    g.setFont (barFont);
    for (const auto& b : bars)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (barFont, b.s) + 14.0f;
        const float mid = (b.x1 + b.x2) * 0.5f;
        const int pi = b.s == "SYNTHESIZER" ? 0 : (b.s == "BRASS" ? 1 : (b.s == "STRINGS" ? 2 : -1));
        const auto lineCol = pi >= 0 ? kPartCol[pi] : kInk.withAlpha (0.7f);
        g.setColour (lineCol);
        g.fillRect (b.x1, b.y + 5.0f, mid - tw * 0.5f - b.x1, pi >= 0 ? 2.0f : 1.5f);
        g.fillRect (mid + tw * 0.5f, b.y + 5.0f, b.x2 - (mid + tw * 0.5f), pi >= 0 ? 2.0f : 1.5f);
        g.setColour (pi >= 0 ? kPartCol[pi] : kInk);
        g.drawText (b.s, juce::Rectangle<float> (mid - tw * 0.5f, b.y - 2.0f, tw, 14.0f), juce::Justification::centred);
    }
}

//==============================================================================
TridexEditor::TridexEditor (TridexProcessor& p) : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (TridexPanel::kW / 2, TridexPanel::kH / 2, TridexPanel::kW * 2, TridexPanel::kH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) TridexPanel::kW / TridexPanel::kH);
    setSize (TridexPanel::kW, TridexPanel::kH);
}

void TridexEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) TridexPanel::kW));
}
