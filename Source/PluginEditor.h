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
    void paintButton (juce::Graphics&, bool over, bool down) override;
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
// ARP page: overlays the control section; opened/closed with the ARP button in the top bar.
class ArpPage : public juce::Component
{
public:
    explicit ArpPage (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;

private:
    struct Card
    {
        juce::String name;
        juce::Rectangle<int> area;
        std::unique_ptr<LedButton> on, sync;
        std::unique_ptr<juce::ComboBox> mode, rate, oct;
        std::unique_ptr<juce::Slider> gate;
    };

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
    static constexpr int kW = 1850, kH = 510;

    explicit TridexPanel (TridexProcessor&);
    ~TridexPanel() override;
    void paint (juce::Graphics&) override;

private:
    struct Text { juce::Rectangle<float> r; juce::String s; float size; bool bold; juce::Justification j; };
    struct Bar  { float x1, x2, y; juce::String s; };

    juce::Slider& knob (const char* id, int cx, int cy, const juce::String& label, bool white, int size = 38);
    juce::Slider& switchKnob (const char* id, int cx, int cy, const juce::String& label, bool white);
    tdxui::LedButton& led (const char* id, int cx, int y, const juce::String& label);
    void octave (const char* id, int cx, int y);
    juce::ComboBox& combo (const char* id, int x, int y, int w);
    tdxui::LedButton& progButton (const juce::String& label, int cx, int y, std::function<void()> onClick);
    void text (juce::Rectangle<float> r, const juce::String& s, float size = 9.5f, bool bold = false,
               juce::Justification j = juce::Justification::centred);
    void moduleLabel (float x1, float x2, float y, const juce::String& s) { text ({ x1, y, x2 - x1, 12 }, s, 9.5f, true); }
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
    std::unique_ptr<tdxui::ArpPage> arpPage;
    std::array<tdxui::LedButton*, 4> bankBtns {};
    std::array<tdxui::LedButton*, 8> progBtns {};
    bool writeArmed = false;
    int pendingBank = 0;
    juce::Rectangle<int> lcd;
    juce::String lcdText;

    juce::ComboBox pedalBox;
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
