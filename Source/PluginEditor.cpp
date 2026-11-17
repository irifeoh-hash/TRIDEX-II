#include "PluginEditor.h"
#include "Presets.h"

namespace
{
constexpr int kCellW = 74, kCellH = 84, kTitleH = 26, kPad = 10, kTopH = 46, kKeysH = 72, kCols = 7;

const juce::Colour kPanel   { 0xff1b1a1c };
const juce::Colour kWood    { 0xff4a2e1c };
const juce::Colour kText    { 0xffe8e2d6 };
const juce::Colour kSynth   { 0xffe0a030 };
const juce::Colour kBrass   { 0xffd2553a };
const juce::Colour kStrings { 0xff4f9fd0 };
const juce::Colour kGlobal  { 0xffa8a8a0 };

juce::String shortLabel (juce::String name)
{
    for (auto prefix : { "Synth ", "Brass ", "Strings ", "Flanger ", "Joystick " })
        if (name.startsWith (prefix) && name.length() > juce::String (prefix).length() + 2)
            name = name.fromFirstOccurrenceOf (prefix, false, false);
    return name;
}
} // namespace

//==============================================================================
TridexLookAndFeel::TridexLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a292c));
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff4a494c));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2a292c));
    setColour (juce::ToggleButton::textColourId, kText);
    setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffff4030));
}

void TridexLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Track
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
    g.setColour (juce::Colour (0xff3a393c));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob cap
    const float kr = r * 0.72f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4b4a4e), c.x, c.y - kr,
                                             juce::Colour (0xff141315), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    // Pointer
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.5f, -kr, 3.0f, kr * 0.55f, 1.0f);
    g.setColour (kText);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
juce::Component* TridexEditor::Control::comp() const
{
    if (slider) return slider.get();
    if (combo)  return combo.get();
    return toggle.get();
}

void TridexEditor::addSection (const juce::String& title, juce::Colour colour, int columns, std::initializer_list<const char*> ids)
{
    Section sec { title, colour, columns, {}, {} };

    for (auto* id : ids)
    {
        auto* param = proc.apvts.getParameter (id);
        jassert (param != nullptr);
        auto ctl = std::make_unique<Control>();
        ctl->id = id;
        ctl->label = shortLabel (param->getName (32));

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            ctl->combo = std::make_unique<juce::ComboBox>();
            ctl->combo->addItemList (choice->choices, 1);
            addAndMakeVisible (*ctl->combo);
            ctl->cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *ctl->combo);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            ctl->toggle = std::make_unique<juce::ToggleButton> ("ON");
            addAndMakeVisible (*ctl->toggle);
            ctl->bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *ctl->toggle);
        }
        else
        {
            ctl->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
            ctl->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, kCellW - 6, 16);
            ctl->slider->setColour (juce::Slider::rotarySliderFillColourId, colour);
            addAndMakeVisible (*ctl->slider);
            ctl->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *ctl->slider);
        }
        sec.controls.push_back (std::move (ctl));
    }
    sections.push_back (std::move (sec));
}

TridexEditor::TridexEditor (TridexProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // Left column
    addSection ("SYNTHESIZER", kSynth, kCols, {
        "syn_on", "syn_zone", "syn_vol", "v1_oct", "v1_wave", "pw_int", "pwm_speed",
        "v2_scale", "v2_detune", "cutoff", "res", "eg_int", "kbd_track", "vca_att",
        "sf_a", "sf_d", "sf_s", "sf_r", "sa_a", "sa_d", "sa_s", "sa_r", "auto_damp" });
    addSection ("BRASS", kBrass, kCols, {
        "br_on", "br_zone", "br_vol", "br_16", "br_8", "br_multi", "br_cutoff",
        "br_res", "br_egint", "br_a", "br_d", "br_s", "br_r" });
    // Right column
    addSection ("STRINGS", kStrings, kCols, {
        "st_on", "st_zone", "st_vol", "st_16", "st_8", "st_4", "ensemble",
        "st_attack", "st_release", "st_balance", "st_eq_lo", "st_eq_hi", "bow_on", "bow_level",
        "bow_tone", "svib_on", "svib_delay", "svib_int", "svib_speed" });
    addSection ("TOTAL CONTROL", kGlobal, kCols, {
        "master", "tune", "split", "assign", "bend_range", "mg_speed", "mg_int",
        "dvib_on", "dvib_int", "dvib_time", "fl_syn", "fl_br", "fl_st", "fl_manual",
        "fl_int", "fl_fb", "fl_speed", "drift", "vel_sens", "pedal_mode" });

    const auto& presets = tdx::factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        programBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + presets[(size_t) i].name, i + 1);
    programBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    programBox.onChange = [this] { proc.setCurrentProgram (programBox.getSelectedId() - 1); };
    addAndMakeVisible (programBox);

    keyboard.setAvailableRange (36, 96);   // 61 keys, C-C
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    const int colW = kCols * kCellW + kPad;
    setSize (colW * 2 + kPad * 3, kTopH + kTitleH * 2 + 6 * kCellH + kKeysH + kPad * 4);
}

TridexEditor::~TridexEditor() { setLookAndFeel (nullptr); }

void TridexEditor::paint (juce::Graphics& g)
{
    // Wooden end cheeks + dark panel
    g.fillAll (kWood);
    auto panel = getLocalBounds().reduced (kPad / 2, 0);
    g.setColour (kPanel);
    g.fillRect (panel);

    g.setColour (kText);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("TRIDEX 80", kPad * 2, 0, 260, kTopH, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (12.0f));
    g.setColour (kText.withAlpha (0.6f));
    g.drawText ("POLYPHONIC SYNTHESIZER  |  BRASS  |  STRINGS  /  8 VOICES", 190, 0, 400, kTopH, juce::Justification::centredLeft);

    for (const auto& sec : sections)
    {
        g.setColour (juce::Colour (0xff232225));
        g.fillRoundedRectangle (sec.bounds.toFloat(), 6.0f);
        g.setColour (sec.colour);
        g.fillRoundedRectangle (sec.bounds.withHeight (kTitleH - 4).toFloat(), 6.0f);
        g.setColour (kPanel);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (sec.title, sec.bounds.withHeight (kTitleH - 4).reduced (8, 0), juce::Justification::centredLeft);

        g.setFont (juce::FontOptions (11.0f));
        for (const auto& c : sec.controls)
        {
            const auto cb = c->comp()->getBounds();
            g.setColour (kText.withAlpha (0.85f));
            g.drawFittedText (c->label.toUpperCase(), cb.getX() - 2, cb.getY() - 16, cb.getWidth() + 4, 14,
                              juce::Justification::centred, 1, 0.7f);
        }
    }
}

void TridexEditor::resized()
{
    auto area = getLocalBounds().reduced (kPad);
    auto top = area.removeFromTop (kTopH - kPad);
    programBox.setBounds (top.removeFromRight (280).reduced (0, 6));

    keyboard.setBounds (area.removeFromBottom (kKeysH));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);  // 36 white keys
    area.removeFromBottom (kPad);

    const auto layoutSection = [] (Section& sec, juce::Rectangle<int> r)
    {
        sec.bounds = r;
        auto inner = r.withTrimmedTop (kTitleH);
        for (size_t i = 0; i < sec.controls.size(); ++i)
        {
            const int col = (int) i % sec.columns, row = (int) i / sec.columns;
            auto cell = juce::Rectangle<int> (inner.getX() + col * kCellW, inner.getY() + row * kCellH, kCellW, kCellH)
                            .withTrimmedTop (18).reduced (3, 2);
            auto* comp = sec.controls[i]->comp();
            if (sec.controls[i]->slider) comp->setBounds (cell);
            else                         comp->setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 26));
        }
    };

    const auto rowsFor = [] (const Section& sec) { return ((int) sec.controls.size() + sec.columns - 1) / sec.columns; };
    const int colW = kCols * kCellW + kPad;
    auto left = area.removeFromLeft (colW);
    area.removeFromLeft (kPad);
    auto right = area.removeFromLeft (colW);

    for (int c = 0; c < 2; ++c)
    {
        auto& col = c == 0 ? left : right;
        for (int k = 0; k < 2; ++k)
        {
            auto& sec = sections[(size_t) (c * 2 + k)];
            layoutSection (sec, col.removeFromTop (kTitleH + rowsFor (sec) * kCellH));
            col.removeFromTop (kPad / 2);
        }
    }
}
