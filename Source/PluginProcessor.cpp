#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using NR  = juce::NormalisableRange<float>;

namespace
{
const juce::StringArray kFootage { "16'", "8'", "4'" };
const juce::StringArray kZones   { "L", "L+H", "H" };

NR timeRange (float maxSec) { NR r (0.001f, maxSec); r.setSkewForCentre (0.4f); return r; }

void addF (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name, NR r, float def, const char* unit = "")
{
    const juce::String u (unit);
    const int decimals = (r.interval >= 1.0f || u == "ct") ? 0 : (r.end > 9.0f ? 1 : 2);
    auto attrs = juce::AudioParameterFloatAttributes().withLabel (u)
                     .withStringFromValueFunction ([decimals, u] (float v, int)
                     {
                         if (u == "s" && v < 1.0f) return juce::String (juce::roundToInt (v * 1000.0f)) + " ms";
                         return juce::String (v, decimals) + (u.isEmpty() ? "" : " " + u);
                     });
    l.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def, attrs));
}
void addC (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name, const juce::StringArray& c, int def)
{
    l.add (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, c, def));
}
void addB (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name, bool def)
{
    l.add (std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def));
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout TridexProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    const NR unit (0.0f, 1.0f);
    const NR dB10 (-10.0f, 10.0f), dB12 (-12.0f, 12.0f);

    // ---- Total control
    addF (l, "master",      "Total Volume", unit, 0.8f);
    addF (l, "tune",        "Tune", NR (-100.0f, 100.0f), 0.0f, "ct");
    addF (l, "split",       "Split Point", NR (36.0f, 96.0f, 1.0f), 60.0f);
    addC (l, "assign",      "Assign Mode", { "1", "2" }, 0);
    addF (l, "mg_speed",    "Joystick Speed", NR (0.5f, 15.0f, 0.0f, 0.5f), 5.5f, "Hz");
    addF (l, "mg_int",      "Joystick Intensity", unit, 0.5f);
    addB (l, "dvib_on",     "Delay Vibrato", false);
    addF (l, "dvib_int",    "Delay Vib Int", unit, 0.3f);
    addF (l, "dvib_time",   "Delay Vib Time", NR (0.05f, 4.0f, 0.0f, 0.5f), 0.8f, "s");
    addF (l, "bend_range",  "Bend Range", NR (0.0f, 12.0f, 1.0f), 2.0f, "st");
    addB (l, "fl_syn",      "Flanger Synth", false);
    addB (l, "fl_br",       "Flanger Brass", false);
    addB (l, "fl_st",       "Flanger Strings", false);
    addF (l, "fl_manual",   "Flanger Manual", unit, 0.4f);
    addF (l, "fl_int",      "Flanger Intensity", unit, 0.5f);
    addF (l, "fl_fb",       "Flanger Feedback", unit, 0.4f);
    addF (l, "fl_speed",    "Flanger Speed", NR (0.05f, 8.0f, 0.0f, 0.4f), 0.3f, "Hz");
    addF (l, "drift",       "Drift", unit, 0.3f);
    addF (l, "vel_sens",    "Velocity", unit, 0.0f);
    addC (l, "pedal_mode",  "Pedal Mode", { "Hold", "Rel SW" }, 0);

    // ---- Synthesizer section
    addB (l, "syn_on",     "Synth Output", true);
    addC (l, "syn_zone",   "Synth Split", kZones, 1);
    addF (l, "syn_vol",    "Synth Volume", unit, 0.7f);
    addC (l, "v1_oct",     "VCO1 Octave", kFootage, 1);
    addC (l, "v1_wave",    "VCO1 Wave", { "Saw", "PW", "PWM" }, 0);
    addF (l, "pw_int",     "PW/PWM Int", unit, 0.3f);
    addF (l, "pwm_speed",  "PWM Speed", NR (0.1f, 12.0f, 0.0f, 0.5f), 1.0f, "Hz");
    addC (l, "v2_scale",   "VCO2 Scale", { "Off", "16'", "8'", "4'" }, 2);
    addF (l, "v2_detune",  "VCO2 Detune", NR (-50.0f, 50.0f), 6.0f, "ct");
    addF (l, "cutoff",     "Synth Cutoff", unit, 0.6f);
    addF (l, "res",        "Synth Resonance", unit, 0.15f);
    addF (l, "eg_int",     "Synth EG Intensity", NR (-1.0f, 1.0f), 0.3f);
    addC (l, "kbd_track",  "KBD Track", { "Off", "Quarter", "Half", "Full", "Over" }, 2);
    addF (l, "sf_a", "VCF Attack",  timeRange (10.0f), 0.01f, "s");
    addF (l, "sf_d", "VCF Decay",   timeRange (10.0f), 0.6f,  "s");
    addF (l, "sf_s", "VCF Sustain", unit, 0.4f);
    addF (l, "sf_r", "VCF Release", timeRange (10.0f), 0.4f,  "s");
    addF (l, "sa_a", "VCA Attack",  timeRange (10.0f), 0.005f, "s");
    addF (l, "sa_d", "VCA Decay",   timeRange (10.0f), 0.5f,  "s");
    addF (l, "sa_s", "VCA Sustain", unit, 0.8f);
    addF (l, "sa_r", "VCA Release", timeRange (10.0f), 0.4f,  "s");
    addF (l, "vca_att",    "VCA Attenuator", dB10, 0.0f, "dB");
    addB (l, "auto_damp",  "Auto Damp", false);

    // ---- Brass section
    addB (l, "br_on",     "Brass Output", false);
    addC (l, "br_zone",   "Brass Split", kZones, 1);
    addF (l, "br_vol",    "Brass Volume", unit, 0.6f);
    addB (l, "br_16",     "Brass 16'", false);
    addB (l, "br_8",      "Brass 8'", true);
    addF (l, "br_cutoff", "Brass Cutoff", unit, 0.4f);
    addF (l, "br_res",    "Brass Resonance", unit, 0.1f);
    addF (l, "br_egint",  "Brass EG Intensity", NR (-1.0f, 1.0f), 0.5f);
    addF (l, "br_a", "Brass Attack",  timeRange (10.0f), 0.08f, "s");
    addF (l, "br_d", "Brass Decay",   timeRange (10.0f), 0.5f,  "s");
    addF (l, "br_s", "Brass Sustain", unit, 0.7f);
    addF (l, "br_r", "Brass Release", timeRange (10.0f), 0.3f,  "s");
    addB (l, "br_multi",  "Multi Trig", true);

    // ---- Strings section
    addB (l, "st_on",      "Strings Output", false);
    addC (l, "st_zone",    "Strings Split", kZones, 1);
    addF (l, "st_vol",     "Strings Volume", unit, 0.6f);
    addB (l, "st_16",      "Strings 16'", false);
    addB (l, "st_8",       "Strings 8'", true);
    addB (l, "st_4",       "Strings 4'", true);
    addF (l, "st_attack",  "Strings Attack",  timeRange (5.0f), 0.15f, "s");
    addF (l, "st_release", "Strings Release", timeRange (10.0f), 0.8f, "s");
    addF (l, "st_balance", "KBD Balance", NR (-1.0f, 1.0f), 0.0f);
    addF (l, "st_eq_lo",   "Strings EQ Low", dB12, 0.0f, "dB");
    addF (l, "st_eq_hi",   "Strings EQ High", dB12, 0.0f, "dB");
    addB (l, "bow_on",     "Bowing", false);
    addF (l, "bow_level",  "Bowing Level", unit, 0.5f);
    addF (l, "bow_tone",   "Bowing Tone", unit, 0.5f);
    addB (l, "svib_on",    "Strings Vibrato", false);
    addF (l, "svib_delay", "Vib Delay", NR (0.01f, 3.0f, 0.0f, 0.5f), 0.5f, "s");
    addF (l, "svib_int",   "Vib Intensity", unit, 0.4f);
    addF (l, "svib_speed", "Vib Speed", NR (1.0f, 10.0f), 5.5f, "Hz");
    addB (l, "ensemble",   "Ensemble", true);

    return l;
}

//==============================================================================
TridexProcessor::TridexProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TRIDEX", createLayout())
{
}

bool TridexProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TridexProcessor::prepareToPlay (double sampleRate, int)
{
    syncParams();
    engine.prepare (sampleRate);
}

void TridexProcessor::syncParams()
{
    auto& p = engine.p;
    const auto on   = [this] (const char* id) { return get (id) > 0.5f; };
    const auto idx  = [this] (const char* id) { return (int) get (id); };
    const auto zone = [&] (const char* id) { return static_cast<tdx::Zone> (idx (id)); };

    p.master = get ("master"); p.tuneCents = get ("tune"); p.splitPoint = idx ("split"); p.assignMode = idx ("assign");
    p.mgSpeed = get ("mg_speed"); p.mgIntensity = get ("mg_int");
    p.delayVib = on ("dvib_on"); p.delayVibInt = get ("dvib_int"); p.delayVibTime = get ("dvib_time");
    p.bendRange = get ("bend_range");
    p.flSynth = on ("fl_syn"); p.flBrass = on ("fl_br"); p.flStrings = on ("fl_st");
    p.flManual = get ("fl_manual"); p.flIntensity = get ("fl_int"); p.flFeedback = get ("fl_fb"); p.flSpeed = get ("fl_speed");
    p.drift = get ("drift"); p.velSens = get ("vel_sens"); p.pedalMode = idx ("pedal_mode");

    p.synOn = on ("syn_on"); p.synZone = zone ("syn_zone"); p.synVolume = get ("syn_vol");
    p.v1Oct = idx ("v1_oct"); p.v1Wave = static_cast<tdx::Vco1Wave> (idx ("v1_wave"));
    p.pwInt = get ("pw_int"); p.pwmSpeed = get ("pwm_speed");
    p.v2Scale = idx ("v2_scale"); p.v2Detune = get ("v2_detune");
    p.cutoff = get ("cutoff"); p.res = get ("res"); p.egInt = get ("eg_int"); p.kbdTrack = idx ("kbd_track");
    p.sfA = get ("sf_a"); p.sfD = get ("sf_d"); p.sfS = get ("sf_s"); p.sfR = get ("sf_r");
    p.saA = get ("sa_a"); p.saD = get ("sa_d"); p.saS = get ("sa_s"); p.saR = get ("sa_r");
    p.vcaAttenDb = get ("vca_att"); p.autoDamp = on ("auto_damp");

    p.brOn = on ("br_on"); p.brZone = zone ("br_zone"); p.brVolume = get ("br_vol");
    p.br16 = on ("br_16"); p.br8 = on ("br_8");
    p.brCutoff = get ("br_cutoff"); p.brRes = get ("br_res"); p.brEgInt = get ("br_egint");
    p.brA = get ("br_a"); p.brD = get ("br_d"); p.brS = get ("br_s"); p.brR = get ("br_r");
    p.multiTrig = on ("br_multi");

    p.stOn = on ("st_on"); p.stZone = zone ("st_zone"); p.stVolume = get ("st_vol");
    p.st16 = on ("st_16"); p.st8 = on ("st_8"); p.st4 = on ("st_4");
    p.stAttack = get ("st_attack"); p.stRelease = get ("st_release"); p.kbdBalance = get ("st_balance");
    p.eqLowDb = get ("st_eq_lo"); p.eqHighDb = get ("st_eq_hi");
    p.bowOn = on ("bow_on"); p.bowLevel = get ("bow_level"); p.bowTone = get ("bow_tone");
    p.vibOn = on ("svib_on"); p.vibDelay = get ("svib_delay"); p.vibIntensity = get ("svib_int"); p.vibSpeed = get ("svib_speed");
    p.ensembleOn = on ("ensemble");
}

void TridexProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())           engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())        engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isSustainPedalOn())    engine.setPedal (true);
    else if (m.isSustainPedalOff())   engine.setPedal (false);
    else if (m.isControllerOfType (1)) { joyVib = (float) m.getControllerValue() / 127.0f;   engine.joystickY (joyVib, joyTrill); }
    else if (m.isControllerOfType (2)) { joyTrill = (float) m.getControllerValue() / 127.0f; engine.joystickY (joyVib, joyTrill); }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
}

void TridexProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    syncParams();
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    buffer.clear();

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    int pos = 0;

    // Sample-accurate MIDI: render up to each event, then apply it.
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, buffer.getNumSamples(), meta.samplePosition);
        if (t > pos) { engine.render (L + pos, R + pos, t - pos); pos = t; }
        handleMidi (meta.getMessage());
    }
    if (pos < buffer.getNumSamples())
        engine.render (L + pos, R + pos, buffer.getNumSamples() - pos);
}

//==============================================================================
int TridexProcessor::getNumPrograms() { return (int) tdx::factoryPresets().size(); }

const juce::String TridexProcessor::getProgramName (int index)
{
    const auto& presets = tdx::factoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? juce::String (presets[(size_t) index].name) : juce::String();
}

void TridexProcessor::setCurrentProgram (int index)
{
    const auto& presets = tdx::factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;

    // Reset everything to defaults, then apply the preset's overrides.
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : presets[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

void TridexProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void TridexProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TridexProcessor::createEditor() { return new TridexEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TridexProcessor(); }
