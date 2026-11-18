#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Engine.h"

class TridexProcessor : public juce::AudioProcessor
{
public:
    TridexProcessor();
    ~TridexProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    // Host program list = the 32 programmer slots (A1..D8).
    int getNumPrograms() override { return kNumPrograms; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override { recallProgram (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& name) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Programmer (KLM-380). As on the hardware, only the synthesizer section's
    // white-knob controls are stored in a program.
    static constexpr int kNumPrograms = 32;
    static const juce::StringArray& programmableIds();

    void recallProgram (int index);
    void writeProgram (int index);
    bool isManual() const { return manualMode; }
    void setManual() { manualMode = true; }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;                   // fed by the editor's on-screen keyboard
    std::atomic<float> guiJoyX { 0.0f }, guiJoyY { 0.0f };   // editor joystick, -1..1

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    int getGateStep (int section) const { return engine.gateStepUi[(size_t) juce::jlimit (0, 2, section)].load (std::memory_order_relaxed); }

private:
    struct Program { juce::String name; std::map<juce::String, float> values; };

    void syncParams();
    void handleMidi (const juce::MidiMessage& m);
    void applyJoystick();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    // Cached parameter pointers for the per-part arp / step-gate settings (looked up once, so
    // nothing is allocated on the audio thread).
    struct ArpPtrs  { std::atomic<float> *on, *mode, *rate, *oct, *gate, *sync; };
    struct GatePtrs { std::atomic<float> *on, *sync, *rate; std::array<std::atomic<float>*, 8> l, r; };
    std::array<ArpPtrs, 3> arpPtr {};
    std::array<GatePtrs, 3> gatePtr {};

    tdx::Engine engine;
    std::array<Program, kNumPrograms> programs;
    int currentProgram = 0;
    bool manualMode = true;
    float midiBend = 0.0f, midiVib = 0.0f, midiTrill = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TridexProcessor)
};
