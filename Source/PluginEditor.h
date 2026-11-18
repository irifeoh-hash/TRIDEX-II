#pragma once
// Hardware-style panel. Control placement follows the Trident Mk II front
// panel (owner's manual pp. 4-42): left-to-right Key Assign, Flanger,
// Synthesizer (with Programmer and Mixer strip), Brass, Strings, Output; the
// Delay Vibrato and Joystick panel sit to the left of the 61-key keyboard.
// Artwork and branding are original.

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace tdxui
{
class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getPopupMenuFont() override { return juce::FontOptions (13.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.0f); }
};

// Cream push button with a red LED above the cap (as on the hardware switch strips).
class LedButton : public juce::Button
{
public:
    explicit LedButton (const juce::String& name) : juce::Button (name) {}
    bool lit = false;            // used when not a toggle (programmer buttons)
    bool redCap = false;         // WRITE button
    juce::Colour cap { 0xffb7b5ae };   // cap colour (grey, cream, green, orange, tan...)
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Horizontal two-position switch: PORTA (left) / GLISS (right).
class ModeSwitch : public juce::ToggleButton
{
public:
    juce::Colour accent { 0xffece8de };
    juce::String leftLabel { "PORTA" }, rightLabel { "GLISS" };
    void paintButton (juce::Graphics&, bool, bool) override;
};

// Two-position slide switch (ASSIGN MODE 1/2).
class SlideSwitch : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool, bool) override;
};

// KBD SPLIT selector: two arrow LEDs, L / L+H / H.
class SplitSelector : public juce::Component, public juce::SettableTooltipClient
{
public:
    SplitSelector (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Colour colour { 0xffff3a2a };   // part colour (green / red / orange)

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int zone = 1; // 0 L, 1 L+H, 2 H
};

// TRANSPOSE: - / + buttons with the current value (-12..+12). Double-click the value to reset.
class TransposeControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    TransposeControl (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void step (int delta);
    juce::ParameterAttachment attachment;
    int value = 0;
};

// Per-part OCTAVE: small up/down buttons beside a value display (-2..+2).
class OctaveControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    OctaveControl (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::ParameterAttachment attachment;
    int value = 0;
};

// Per-part split point: note name display. Click the left/right edge for -/+ a semitone,
// drag up/down for bigger moves, double-click to reset to C3 (middle C).
class SplitPointControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    SplitPointControl (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    juce::Colour colour { 0xffff3a2a };   // part colour
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void setNote (int n);
    juce::ParameterAttachment attachment;
    int note = 60, dragStartNote = 60;
    bool dragging = false;
};

// Draws each part's keyboard range (coloured bars) and split point (coloured line) over the keys.
class SplitOverlay : public juce::Component
{
public:
    SplitOverlay (juce::AudioProcessorValueTreeState& state, juce::MidiKeyboardComponent& keys);
    void paint (juce::Graphics&) override;
    void refreshIfChanged();

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::MidiKeyboardComponent& kb;
    std::array<float, 9> last {};
};

// Spring-loaded joystick: X = pitch bend, Y+ = vibrato, Y- = trill.
class Joystick : public juce::Component, public juce::SettableTooltipClient
{
public:
    Joystick (std::atomic<float>& x, std::atomic<float>& y) : jx (x), jy (y) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    std::atomic<float>& jx;
    std::atomic<float>& jy;
    juce::Point<float> pos;
};
// SPACE 1-4 push buttons (radio) for the chorus page.
class SpaceButtons : public juce::Component, public juce::SettableTooltipClient
{
public:
    SpaceButtons (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::ParameterAttachment attachment;
    int value = 1;
};

// ABOUT page: opened by clicking the I Rife logo; synopsis + Instagram link.
class AboutPage : public juce::Component
{
public:
    explicit AboutPage (std::function<void()> onClose);
    void paint (juce::Graphics&) override;

private:
    juce::HyperlinkButton instagram { "www.instagram.com/irife", juce::URL ("https://www.instagram.com/irife") };
    juce::TextButton close { "CLOSE" };
    std::function<void()> closeFn;
};

// Clickable area over the logo in the top bar.
class LogoHotspot : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onClick;
    LogoHotspot() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void mouseUp (const juce::MouseEvent& e) override { if (onClick && getLocalBounds().contains (e.getPosition())) onClick(); }
};

// CHORUS page: overlays the control section; opened/closed with the CHORUS button in the top bar.
class ChorusPage : public juce::Component
{
public:
    explicit ChorusPage (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;

private:
    juce::Slider& knob (const juce::String& id, juce::Rectangle<int> r);
    LedButton& led (const juce::String& id, juce::Rectangle<int> r);
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<std::unique_ptr<juce::Component>> comps;
    std::vector<std::pair<juce::Component*, juce::String>> labels;
    juce::Rectangle<int> card;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> bAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> cAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sAtt;
};

// STEP GATE grid: 8 steps x L/R lanes, click to toggle, drag to paint; current step highlighted.
class GateGrid : public juce::Component
{
public:
    GateGrid (juce::AudioProcessorValueTreeState& state, const juce::String& prefix);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    int currentStep = -1;

private:
    struct Cell { std::unique_ptr<juce::ParameterAttachment> att; bool on = true; };
    juce::Rectangle<float> cellBounds (int lane, int step) const;
    bool hit (juce::Point<float>, int& lane, int& step) const;
    void set (int lane, int step, bool v);
    std::array<std::array<Cell, 8>, 2> cells;
    bool paintValue = true;
};

// ARP page: overlays the control section; opened/closed with the ARP button in the top bar.
class ArpPage : public juce::Component, private juce::Timer
{
public:
    ArpPage (juce::AudioProcessorValueTreeState& state, std::function<int (int)> gateStep);
    void paint (juce::Graphics&) override;

private:
    struct Card
    {
        juce::String name;
        juce::Rectangle<int> area;
        std::unique_ptr<LedButton> on, sync;
        std::unique_ptr<juce::ComboBox> mode, rate, oct;
        std::unique_ptr<juce::Slider> gate;
        std::unique_ptr<LedButton> sgOn, sgSync;
        std::unique_ptr<juce::ComboBox> sgRate;
        std::unique_ptr<GateGrid> grid;
    };
    void timerCallback() override;
    std::function<int (int)> gateStepFn;

    juce::Slider& makeKnob (const juce::String& id, juce::Rectangle<int> r);
    juce::ComboBox& makeCombo (const juce::String& id, juce::Rectangle<int> r);
    LedButton& makeLed (const juce::String& id, juce::Rectangle<int> r);

    juce::AudioProcessorValueTreeState& apvts;
    std::array<Card, 3> cards;
    std::unique_ptr<juce::Slider> tempo;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> bAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> cAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sAtt;
};
} // namespace tdxui

//==============================================================================
class TridexPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int kW = 1104, kH = 806;

    explicit TridexPanel (TridexProcessor&);
    ~TridexPanel() override;
    void paint (juce::Graphics&) override;

private:
    struct Text { juce::Rectangle<float> r; juce::String s; float size; bool bold; juce::Justification j; juce::Colour col; };
    struct Bar  { float x1, x2, y; juce::String s; };

    juce::Slider& knob (const char* id, int cx, int cy, const juce::String& label, bool white, int size = 38);
    juce::Slider& switchKnob (const char* id, int cx, int cy, const juce::String& label, bool white);
    tdxui::LedButton& led (const char* id, int cx, int y, const juce::String& label);
    void octave (const char* id, int cx, int y);
    void modeSwitch (const char* id, int cx, int cy, bool orangeLegend, const juce::String& left = "PORTA", const juce::String& right = "GLISS");
    juce::ComboBox& combo (const char* id, int x, int y, int w);
    tdxui::LedButton& progButton (const juce::String& label, int cx, int y, std::function<void()> onClick);
    void text (juce::Rectangle<float> r, const juce::String& s, float size = 9.5f, bool bold = false,
               juce::Justification j = juce::Justification::centred, juce::Colour col = {});
    void moduleLabel (float x1, float x2, float y, const juce::String& s, juce::Colour c = {}) { text ({ x1, y, x2 - x1, 12 }, s, 9.5f, true, juce::Justification::centred, c); }
    void sectionBar (float x1, float x2, float y, const juce::String& s) { bars.push_back ({ x1, x2, y, s }); }

    void timerCallback() override;
    void bankPressed (int bank);
    void programPressed (int number);

    TridexProcessor& proc;
    tdxui::PanelLookAndFeel lnf;

    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> bAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pedalAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> cAtt;
    std::vector<Text> texts;
    std::vector<Bar> bars;

    tdxui::LedButton* writeBtn = nullptr;
    tdxui::LedButton* manualBtn = nullptr;
    tdxui::LedButton* arpBtn = nullptr;
    tdxui::LedButton* chorusBtn = nullptr;
    std::unique_ptr<tdxui::ChorusPage> chorusPage;
    std::unique_ptr<tdxui::AboutPage> aboutPage;
    std::unique_ptr<tdxui::LogoHotspot> logoHotspot;
    std::unique_ptr<tdxui::ArpPage> arpPage;
    std::unique_ptr<tdxui::SplitOverlay> splitOverlay;
    std::array<tdxui::LedButton*, 4> bankBtns {};
    std::array<tdxui::LedButton*, 8> progBtns {};
    bool writeArmed = false;
    int pendingBank = 0;
    juce::Rectangle<int> lcd;
    juce::String lcdText;

    juce::ComboBox pedalBox, prioBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> prioAtt;
    juce::MidiKeyboardComponent keyboard { proc.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };
};

class TridexEditor : public juce::AudioProcessorEditor
{
public:
    explicit TridexEditor (TridexProcessor&);
    void resized() override;

private:
    TridexPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TridexEditor)
};
