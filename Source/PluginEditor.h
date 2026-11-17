#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class TridexLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TridexLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

class TridexEditor : public juce::AudioProcessorEditor
{
public:
    explicit TridexEditor (TridexProcessor&);
    ~TridexEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Control
    {
        juce::String id, label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
        juce::Component* comp() const;
    };

    struct Section
    {
        juce::String title;
        juce::Colour colour;
        int columns;
        std::vector<std::unique_ptr<Control>> controls;
        juce::Rectangle<int> bounds;
    };

    void addSection (const juce::String& title, juce::Colour c, int columns, std::initializer_list<const char*> ids);

    TridexProcessor& proc;
    TridexLookAndFeel lnf;
    std::vector<Section> sections;
    juce::ComboBox programBox;
    juce::MidiKeyboardComponent keyboard { proc.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TridexEditor)
};
