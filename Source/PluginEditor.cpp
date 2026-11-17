#include "PluginEditor.h"

namespace
{
const juce::Colour kPanel  { 0xff141416 };
const juce::Colour kPanel2 { 0xff1c1c1f };
const juce::Colour kWood   { 0xff5a3620 };
const juce::Colour kInk    { 0xffeae6dc };
const juce::Colour kLedOn  { 0xffff3a2a };
const juce::Colour kLedOff { 0xff4a1612 };

constexpr int kCol = 58;                 // column pitch
constexpr int kRow1 = 82, kRow2 = 154;   // knob centre rows
constexpr int kModY = 194;               // module names
constexpr int kSwY = 210;                // switch strip top
constexpr int kBarY = 272;               // section name bars
constexpr int kLowerY = 296;             // joystick / arp / keyboard area
constexpr int kArpX = 314;               // arpeggiator panel (Tridex II)
constexpr int kKeysX = kArpX + 392;      // keyboard starts here
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

    // Scale legends around the knob, as printed on the panel.
    g.setColour (kInk.withAlpha (0.8f));
    g.setFont (juce::FontOptions (7.5f));
    const auto legend = [&] (float t, const juce::String& txt)
    {
        const float a = startAngle + t * (endAngle - startAngle);
        const auto p = c.getPointOnCircumference (r + 6.5f, a);
        g.drawText (txt, juce::Rectangle<float> (22, 9).withCentre (p), juce::Justification::centred);
    };
    if (labels.size() > 1)
        for (int i = 0; i < labels.size(); ++i) legend ((float) i / (float) (labels.size() - 1), labels[i]);
    else if (scale == "bip")
    {
        legend (0.0f, "-5"); legend (0.5f, "0"); legend (1.0f, "+5");
        for (int i = 1; i < 10; ++i) if (i != 5) legend ((float) i / 10.0f, ".");
    }
    else if (scale == "uni")
        for (int i = 0; i <= 10; ++i) legend ((float) i / 10.0f, (i % 5 == 0) ? juce::String (i) : juce::String ("."));

    // Skirt + cap
    g.setColour (juce::Colour (0xff050505));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    const float kr = r * 0.82f;
    const auto top = white ? juce::Colour (0xfff2f0ea) : juce::Colour (0xff3c3c40);
    const auto bot = white ? juce::Colour (0xffb8b4aa) : juce::Colour (0xff101012);
    g.setGradientFill (juce::ColourGradient (top, c.x, c.y - kr, bot, c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);

    const float angle = startAngle + pos * (endAngle - startAngle);
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.4f, -kr + 1.5f, 2.8f, kr * 0.6f, 1.0f);
    g.setColour (white ? juce::Colour (0xff141414) : kInk);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
void LedButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const bool on = getClickingTogglesState() ? getToggleState() : lit;
    const auto cap = b.reduced (3.0f, 1.0f);

    const auto base = redCap ? juce::Colour (0xffc83a2c) : juce::Colour (0xffe9e4d6);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (cap.expanded (1.5f), 3.0f);
    g.setGradientFill (juce::ColourGradient (base.brighter (over ? 0.15f : 0.05f), cap.getX(), cap.getY(),
                                             base.darker (down ? 0.5f : 0.25f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.5f);

    const auto ledC = juce::Point<float> (cap.getCentreX(), cap.getY() + 7.0f);
    if (on)
    {
        g.setColour (kLedOn.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre (ledC));
    }
    g.setColour (on ? kLedOn : kLedOff);
    g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (ledC));
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
        g.setColour (on ? kLedOn : kLedOff);
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
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff505055), p.x, p.y - 9, juce::Colour (0xff121214), p.x, p.y + 9, false));
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
        text ({ (float) cx - 30, (float) (cy + box / 2 - 4), 60, 11 }, label, 8.5f);
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
    b->onClick = std::move (onClick);
    addAndMakeVisible (*b);
    text ({ (float) cx - 20, (float) y + 38, 40, 10 }, label, 8.0f, true);
    auto& ref = *b;
    owned.push_back (std::move (b));
    return ref;
}

juce::ComboBox& TridexPanel::combo (const char* id, int x, int y, int w)
{
    auto c = std::make_unique<juce::ComboBox>();
    auto* choice = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id));
    jassert (choice != nullptr);
    c->addItemList (choice->choices, 1);
    c->setBounds (x, y, w, 22);
    c->setTooltip (choice->getName (64));
    addAndMakeVisible (*c);
    cAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *c));
    auto& ref = *c;
    owned.push_back (std::move (c));
    return ref;
}

void TridexPanel::text (juce::Rectangle<float> r, const juce::String& s, float size, bool bold, juce::Justification j)
{
    texts.push_back ({ r, s, size, bold, j });
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
            sel->setBounds (col (1) - 22, y, 44, 22);
            sel->setTooltip (juce::String (names[i]) + " split: lower 2 / upper 3 octaves");
            addAndMakeVisible (*sel);
            owned.push_back (std::move (sel));
            text ({ (float) col (1) - 28, (float) y + 22, 56, 10 }, names[i], 8.0f);
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
        moduleLabel ((float) s0, (float) s0 + 2 * kCol, kModY, "VCO 1");

        switchKnob ("v2_scale", col (2), kRow1, "OCTAVE", true);
        switchKnob ("v2_wave", col (3), kRow1, "WAVEFORM", true);
        knob ("v2_detune", col (2), kRow2, "DETUNE", true);
        moduleLabel ((float) s0 + 2 * kCol, (float) s0 + 4 * kCol, kModY, "VCO 2");

        knob ("cutoff", col (4), kRow1, "CUTOFF FREQ", true);
        knob ("res", col (5), kRow1, "RESONANCE", true);
        switchKnob ("kbd_track", col (4), kRow2, "KBD TRACK", true);
        knob ("eg_int", col (5), kRow2, "EG INTENSITY", true);
        moduleLabel ((float) s0 + 4 * kCol, (float) s0 + 6 * kCol, kModY, "VCF");

        const char* fe[] = { "sf_a", "sf_d", "sf_s", "sf_r" };
        const char* ae[] = { "sa_a", "sa_d", "sa_s", "sa_r" };
        const char* nm[] = { "ATTACK", "DECAY", "SUSTAIN", "RELEASE" };
        for (int i = 0; i < 4; ++i)
        {
            knob (fe[i], col (6 + i), kRow1, nm[i], true);
            knob (ae[i], col (6 + i), kRow2, nm[i], true);
        }
        text ({ (float) s0 + 6 * kCol, (float) kRow1 - 41, 4.0f * kCol, 11 }, "VCF EG", 9.5f, true);
        moduleLabel ((float) s0 + 6 * kCol, (float) s0 + 10 * kCol, kModY, "VCA EG");

        knob ("vca_att", col (10), kRow1, "ATTENUATOR", true);
        led ("auto_damp", col (10), kRow2 - 22, "AUTO DAMP");
        moduleLabel ((float) s0 + 10 * kCol, (float) s0 + 11 * kCol, kModY, "VCA");

        // Programmer strip: WRITE  MANUAL | A B C D | 1..8, then MIXER
        int bx = s0 + 16;
        writeBtn = &progButton ("WRITE", bx, kSwY, [this] { writeArmed = ! writeArmed; pendingBank = proc.getCurrentProgram() / 8; });
        writeBtn->redCap = true;
        manualBtn = &progButton ("MANUAL", bx += 32, kSwY, [this] { proc.setManual(); writeArmed = false; });
        bx += 10;
        for (int i = 0; i < 4; ++i)
            bankBtns[(size_t) i] = &progButton (juce::String::charToString ((juce::juce_wchar) ('A' + i)), bx += 30, kSwY, [this, i] { bankPressed (i); });
        bx += 8;
        for (int i = 0; i < 8; ++i)
            progBtns[(size_t) i] = &progButton (juce::String (i + 1), bx += 30, kSwY, [this, i] { programPressed (i); });
        text ({ (float) s0 + 80, (float) kSwY + 50, 360, 10 }, "PROGRAMMER", 8.0f, true);

        knob ("syn_vol", s0 + 11 * kCol - 62, kSwY + 18, "VOLUME", false, 30);
        led ("syn_on", s0 + 11 * kCol - 20, kSwY, "OUTPUT");
        auto [a, b] = sectionEnd (11);
        sectionBar ((float) a, (float) b, kBarY, "SYNTHESIZER");
    }

    // ---------------- BRASS
    {
        const int s0 = x;
        // OSC 1 (footage on the 16'/8' buttons below) and OSC 2
        switchKnob ("br_wave", col (0), kRow1, "OSC 1 WAVE", false);
        switchKnob ("br2_oct", col (0), kRow2, "OSC 2 OCTAVE", false);
        switchKnob ("br2_wave", col (1), kRow1, "OSC 2 WAVE", false);
        knob ("br2_detune", col (1), kRow2, "DETUNE", false);
        moduleLabel ((float) s0, (float) s0 + 2 * kCol, kModY, "OSCILLATORS");

        knob ("br_cutoff", col (2), kRow1, "CUTOFF FREQ", false);
        knob ("br_res", col (3), kRow1, "RESONANCE", false);
        knob ("br_a", col (4), kRow1, "ATTACK", false);
        knob ("br_d", col (5), kRow1, "DECAY", false);
        switchKnob ("br_silence", col (2), kRow2, "SILENCE NOTE", false);
        knob ("br_egint", col (3), kRow2, "EG INTENSITY", false);
        knob ("br_s", col (4), kRow2, "SUSTAIN", false);
        knob ("br_r", col (5), kRow2, "RELEASE", false);
        moduleLabel ((float) s0 + 2 * kCol, (float) s0 + 4 * kCol, kModY, "VCF");
        moduleLabel ((float) s0 + 4 * kCol, (float) s0 + 6 * kCol, kModY, "EG");

        knob ("br_vol", s0 + 20, kSwY + 18, "VOLUME", false, 30);
        int bx = s0 + 54;
        led ("br_on", bx, kSwY, "OUTPUT");
        led ("br_16", bx += 40, kSwY, "OSC 1 16'");
        led ("br_8", bx += 38, kSwY, "OSC 1 8'");
        led ("br_multi", bx += 46, kSwY, "MULTIPLE TRIGGER");
        led ("br_tsel", bx += 42, kSwY, "TRIGGER SELECT");
        led ("br_mono", bx += 42, kSwY, "MONO");
        auto [a, b] = sectionEnd (6);
        sectionBar ((float) a, (float) b, kBarY, "BRASS");
    }

    // ---------------- STRINGS
    {
        const int s0 = x;
        knob ("st_attack", col (0), kRow1, "ATTACK", false);
        knob ("st_release", col (0), kRow2, "RELEASE", false);
        knob ("st_eq_hi", col (1), kRow1, "HIGH", false);
        knob ("st_eq_lo", col (1), kRow2, "LOW", false);
        knob ("bow_level", col (2), kRow1, "LEVEL", false);
        knob ("bow_tone", col (2), kRow2, "TONE", false);
        knob ("svib_delay", col (3), kRow1, "DELAY TIME", false);
        knob ("svib_int", col (3), kRow2, "INTENSITY", false);
        knob ("svib_speed", col (4), kRow1, "SPEED", false);
        knob ("st_balance", col (4), kRow2, "KBD BALANCE", false);
        moduleLabel ((float) s0, (float) s0 + kCol, kModY, "ENVELOPE");
        moduleLabel ((float) s0 + kCol, (float) s0 + 2 * kCol, kModY, "EQUALIZER");
        moduleLabel ((float) s0 + 2 * kCol, (float) s0 + 3 * kCol, kModY, "BOWING");
        moduleLabel ((float) s0 + 3 * kCol, (float) s0 + 4 * kCol, kModY, "VIBRATO");
        moduleLabel ((float) s0 + 4 * kCol, (float) s0 + 5 * kCol, kModY, "FILTER");

        knob ("st_vol", s0 + 20, kSwY + 18, "VOLUME", false, 30);
        int bx = s0 + 54;
        led ("st_on", bx, kSwY, "OUTPUT");
        led ("st_16", bx += 36, kSwY, "16'");
        led ("st_8", bx += 32, kSwY, "8'");
        led ("st_4", bx += 32, kSwY, "4'");
        led ("bow_on", bx += 38, kSwY, "BOWING");
        led ("svib_off", bx += 36, kSwY, "VIBRATO OFF");
        led ("ens_off", bx += 36, kSwY, "ENSEMBLE OFF");
        auto [a, b] = sectionEnd (5);
        sectionBar ((float) a, (float) b, kBarY, "STRINGS");
    }

    // ---------------- OUTPUT
    {
        knob ("master", col (0), kRow2, "TOTAL VOLUME", false);
        text ({ (float) x, (float) kRow1 - 30, (float) kCol, 11 }, "POWER ON", 8.0f, true);
        auto [a, b] = sectionEnd (1);
        sectionBar ((float) a, (float) b, kBarY, "VOLUME");
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
        js->setBounds (170, y0 + 40, 128, 138);

        // TRANSPOSE (Tridex II), above the joystick
        auto tr = std::make_unique<tdxui::TransposeControl> (*proc.apvts.getParameter ("transpose"));
        tr->setBounds (176, y0 + 14, 116, 24);
        tr->setTooltip ("Transpose -12..+12 semitones (double-click the value to reset)");
        addAndMakeVisible (*tr);
        owned.push_back (std::move (tr));
        text ({ 176.0f, (float) y0 + 1, 116.0f, 11 }, "TRANSPOSE", 8.0f, true);
        addAndMakeVisible (*js);
        owned.push_back (std::move (js));
        sectionBar (40, 298, (float) y0 + 188, "JOYSTICK");
    }

    // ---------------- ARPEGGIATOR (Tridex II): one per part, like the Flanger's per-part switches
    {
        const int ax = kArpX, y0 = kLowerY + 6;
        const char* ids[] = { "arp1", "arp2", "arp3" };
        const char* names[] = { "SYNTHE", "BRASS", "STRINGS" };
        const auto hdr = [&] (int x, int w, const char* s) { text ({ (float) x, (float) y0, (float) w, 10 }, s, 7.5f, true); };
        hdr (ax + 60, 32, "ON"); hdr (ax + 98, 72, "MODE"); hdr (ax + 174, 70, "RATE");
        hdr (ax + 248, 50, "OCT"); hdr (ax + 298, 36, "GATE"); hdr (ax + 336, 36, "SYNC");
        for (int r = 0; r < 3; ++r)
        {
            const juce::String id (ids[r]);
            const int y = y0 + 12 + r * 50;
            text ({ (float) ax, (float) y + 11, 58, 14 }, names[r], 8.5f, true, juce::Justification::centredLeft);
            led ((id + "_on").toRawUTF8(), ax + 76, y, "");
            combo ((id + "_mode").toRawUTF8(), ax + 98, y + 7, 72);
            combo ((id + "_rate").toRawUTF8(), ax + 174, y + 7, 70);
            combo ((id + "_oct").toRawUTF8(), ax + 248, y + 7, 50);
            knob ((id + "_gate").toRawUTF8(), ax + 316, y + 18, "", false, 22);
            led ((id + "_sync").toRawUTF8(), ax + 354, y, "");
        }
        knob ("arp_tempo", ax + 14, y0 + 176, "", false, 22).setTooltip ("Arp tempo (used when SYNC is off)");
        text ({ (float) ax + 30, (float) y0 + 170, 120, 12 }, "TEMPO (SYNC OFF)", 7.5f, false, juce::Justification::centredLeft);
        sectionBar ((float) ax + 140, (float) ax + 378, (float) y0 + 178, "ARPEGGIATOR");
    }

    // ---------------- Plugin setup strip (not on the hardware)
    {
        const int y = 22;
        int sx = kW - 430;
        const auto mini = [&] (const char* id, const char* name, const char* tip)
        {
            knob (id, sx, y, "", false, 18).setTooltip (tip);
            text ({ (float) sx + 15, 15, 44, 12 }, name, 8.0f, false, juce::Justification::centredLeft);
            sx += 68;
        };
        mini ("split", "SPLIT", "Split point (hardware: fixed, lower 2 / upper 3 octaves)");
        mini ("bend_range", "BEND", "Joystick pitch-bend range");
        mini ("drift", "DRIFT", "Analog drift (per-voice tuning tolerance)");
        mini ("vel_sens", "VELO", "Velocity sensitivity (not on the hardware)");
        pedalBox.addItemList ({ "Pedal: Off", "Pedal: Damper", "Pedal: Hold" }, 1);
        pedalBox.setBounds (sx - 10, 10, 118, 22);
        addAndMakeVisible (pedalBox);
        pedalAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "pedal_mode", pedalBox);
    }

    lcd = { 330, 9, 320, 24 };

    // ---------------- Keyboard (61 keys, C-C)
    keyboard.setAvailableRange (36, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setBounds (kKeysX + 2, kLowerY + 6, kW - kKeysX - 32, kH - kLowerY - 12);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kLedOn.withAlpha (0.45f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.35f));
    addAndMakeVisible (keyboard);

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
    for (auto* b : bankBtns) b->repaint();
    for (auto* b : progBtns) b->repaint();

    const auto t = writeArmed ? juce::String ("WRITE: PICK BANK + PROGRAM")
                              : (manual ? juce::String ("MANUAL") : proc.getProgramName (cur));
    if (t != lcdText) { lcdText = t; repaint (lcd); }
}

void TridexPanel::paint (juce::Graphics& g)
{
    // Wooden end cheeks + panel
    g.fillAll (kWood);
    g.setGradientFill (juce::ColourGradient (kWood.brighter (0.15f), 0, 0, kWood.darker (0.3f), 24, 0, false));
    g.fillRect (0, 0, 24, kH);
    g.setGradientFill (juce::ColourGradient (kWood.darker (0.3f), (float) kW - 24, 0, kWood.brighter (0.15f), (float) kW, 0, false));
    g.fillRect (kW - 24, 0, 24, kH);
    g.setColour (kPanel);
    g.fillRect (24, 0, kW - 48, kLowerY);
    g.setColour (kPanel2);
    g.fillRect (24, kLowerY, kKeysX - 24, kH - kLowerY);
    g.setColour (juce::Colours::black);
    g.fillRect (kKeysX, kLowerY, kW - kKeysX - 24, kH - kLowerY);

    // Header: original wordmark + program display
    g.setColour (kInk);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold).withHorizontalScale (1.15f));
    g.drawText ("TRIDEX II", 36, 6, 200, 30, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (9.0f));
    g.setColour (kInk.withAlpha (0.6f));
    g.drawText ("PERFORMING KEYBOARD", 176, 14, 140, 14, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff1d0705));
    g.fillRoundedRectangle (lcd.toFloat(), 3.0f);
    g.setColour (kLedOn);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawText (lcdText.toUpperCase(), lcd.reduced (8, 0), juce::Justification::centredLeft);

    g.setColour (kInk.withAlpha (0.15f));
    g.drawHorizontalLine (38, 24.0f, (float) kW - 24);

    for (const auto& t : texts)
    {
        g.setColour (kInk.withAlpha (0.92f));
        g.setFont (juce::FontOptions (t.size, t.bold ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (t.s, t.r.toNearestInt(), t.j, 2, 0.75f);
    }

    // Section bars:  ━━━ NAME ━━━
    const auto barFont = juce::Font (juce::FontOptions (9.5f, juce::Font::bold));
    g.setFont (barFont);
    for (const auto& b : bars)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (barFont, b.s) + 14.0f;
        const float mid = (b.x1 + b.x2) * 0.5f;
        g.setColour (kInk.withAlpha (0.85f));
        g.fillRect (b.x1, b.y + 4.0f, mid - tw * 0.5f - b.x1, 3.0f);
        g.fillRect (mid + tw * 0.5f, b.y + 4.0f, b.x2 - (mid + tw * 0.5f), 3.0f);
        g.setColour (kInk);
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
