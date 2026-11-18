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

// Owner's manual: Attack, Decay and Release range from ~1 ms to over 15 s.
NR timeRange (float maxSec) { NR r (0.001f, maxSec); r.setSkewForCentre (0.5f); return r; }

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
    const NR unit (0.0f, 1.0f), bip (-1.0f, 1.0f), eq (-5.0f, 5.0f);

    // ---- Key assign / total control
    addF (l, "master",     "Total Volume", unit, 0.8f);
    addF (l, "tune",       "Total Tune", NR (-100.0f, 100.0f), 0.0f, "ct");
    addB (l, "assign2",    "Assign Mode 2", false);
    addF (l, "mg_speed",   "Joystick Speed", NR (0.5f, 15.0f, 0.0f, 0.5f), 5.5f, "Hz");
    addF (l, "mg_int",     "Joystick Intensity", unit, 0.5f);
    addB (l, "dvib_on",    "Delay Vibrato", false);
    addF (l, "dvib_int",   "Delay Vibrato Intensity", unit, 0.3f);
    addB (l, "fl_syn",     "Flanger Synthe", false);
    addB (l, "fl_br",      "Flanger Brass", false);
    addB (l, "fl_st",      "Flanger Strings", false);
    addF (l, "fl_manual",  "Flanger Manual", unit, 0.4f);
    addF (l, "fl_int",     "Flanger Intensity", unit, 0.5f);
    addF (l, "fl_fb",      "Flanger Feedback", unit, 0.4f);
    addF (l, "fl_speed",   "Flanger Speed", NR (0.05f, 8.0f, 0.0f, 0.4f), 0.3f, "Hz");

    // ---- Chorus / Ensemble (Tridex II)
    addB (l, "ch_on",    "Chorus On", false);
    addB (l, "ch_syn",   "Chorus Synthe", true);
    addB (l, "ch_br",    "Chorus Brass", false);
    addB (l, "ch_st",    "Chorus Strings", false);
    addC (l, "ch_mode",  "Chorus Mode", { "Chorus I", "Chorus II", "Ensemble", "Space" }, 0);
    addC (l, "ch_space", "Chorus Space", { "1", "2", "3", "4" }, 1);
    addF (l, "ch_width", "Chorus Width", unit, 0.7f);
    addF (l, "ch_tone",  "Chorus Tone", unit, 0.6f);
    addF (l, "ch_rate",  "Chorus Rate", NR (0.05f, 5.0f, 0.0f, 0.4f), 0.6f, "Hz");
    addF (l, "ch_depth", "Chorus Depth", unit, 0.5f);
    addF (l, "ch_mix",   "Chorus Mix", unit, 0.5f);

    // ---- Plugin extras (not on the hardware panel)
    addF (l, "split",      "Split Point", NR (36.0f, 96.0f, 1.0f), 60.0f);   // legacy (unused): per-part splits below
    addF (l, "syn_split",  "Synthe Split Point", NR (24.0f, 108.0f, 1.0f), 60.0f);
    addF (l, "br_split",   "Brass Split Point", NR (24.0f, 108.0f, 1.0f), 60.0f);
    addF (l, "st_split",   "Strings Split Point", NR (24.0f, 108.0f, 1.0f), 60.0f);
    addF (l, "bend_range", "Bend Range", NR (0.0f, 12.0f, 1.0f), 2.0f, "st");
    addF (l, "transpose",  "Transpose", NR (-12.0f, 12.0f, 1.0f), 0.0f, "st");
    addF (l, "syn_octsh",  "Synthe Octave", NR (-2.0f, 2.0f, 1.0f), 0.0f, "oct");
    addF (l, "br_octsh",   "Brass Octave", NR (-2.0f, 2.0f, 1.0f), 0.0f, "oct");
    addF (l, "st_octsh",   "Strings Octave", NR (-2.0f, 2.0f, 1.0f), 0.0f, "oct");
    addF (l, "drift",      "Analog Drift", unit, 0.3f);
    addF (l, "vel_sens",   "Velocity Sens", unit, 0.0f);
    addC (l, "pedal_mode", "Pedal Mode", { "Off", "Damper (S-1)", "Hold" }, 2);
    addC (l, "note_prio",  "Note Priority", { "Last", "First", "Low", "High" }, 0);

    // ---- Synthesizer section (programmable)
    addB (l, "syn_on",    "Synthe Output", true);
    addC (l, "syn_zone",  "Synthe Split", kZones, 1);
    addF (l, "syn_vol",   "Synthe Volume", unit, 0.7f);
    addC (l, "v1_oct",    "VCO1 Octave", kFootage, 1);
    addC (l, "v1_wave",   "VCO1 Waveform", { "Saw", "PW", "PWM" }, 0);
    addF (l, "pw_int",    "PW/PWM", unit, 0.3f);
    addF (l, "pwm_speed", "PWM Speed", NR (0.1f, 12.0f, 0.0f, 0.5f), 1.0f, "Hz");
    addC (l, "v2_scale",  "VCO2 Octave", { "Off", "16'", "8'", "4'" }, 2);
    addF (l, "v2_detune", "VCO2 Detune", NR (-50.0f, 50.0f), 6.0f, "ct");
    addC (l, "v2_wave",   "VCO2 Waveform", { "Saw", "PW", "PWM" }, 0);
    addF (l, "cutoff",    "Cutoff Freq", unit, 0.6f);
    addF (l, "res",       "Resonance", unit, 0.15f);
    addF (l, "eg_int",    "EG Intensity", bip, 0.2f);
    addC (l, "kbd_track", "KBD Track", { "Off", "Quarter", "Half", "Full", "Over" }, 2);
    addF (l, "sf_a", "VCF EG Attack",  timeRange (15.0f), 0.01f, "s");
    addF (l, "sf_d", "VCF EG Decay",   timeRange (15.0f), 0.6f,  "s");
    addF (l, "sf_s", "VCF EG Sustain", unit, 0.4f);
    addF (l, "sf_r", "VCF EG Release", timeRange (15.0f), 0.4f,  "s");
    addF (l, "sa_a", "VCA EG Attack",  timeRange (15.0f), 0.005f, "s");
    addF (l, "sa_d", "VCA EG Decay",   timeRange (15.0f), 0.5f,  "s");
    addF (l, "sa_s", "VCA EG Sustain", unit, 0.8f);
    addF (l, "sa_r", "VCA EG Release", timeRange (15.0f), 0.4f,  "s");
    addF (l, "vca_att",   "Attenuator", NR (-10.0f, 10.0f), 0.0f, "dB");
    addB (l, "auto_damp", "Auto Damp", false);
    addB (l, "syn_mono",  "Synthe Mono", false);
    addB (l, "syn_legato","Synthe Legato", true);    // switch: RETRIG (off) / LEGATO (on)
    { NR g (0.0f, 2.0f); g.setSkewForCentre (0.3f); addF (l, "syn_glide", "Synthe Glide Time", g, 0.0f, "s"); }
    addB (l, "syn_gliss", "Synthe Glissando", false);   // switch: PORTA (off) / GLISS (on)

    // ---- Brass section
    addB (l, "br_on",     "Brass Output", false);
    addC (l, "br_zone",   "Brass Split", kZones, 1);
    addF (l, "br_vol",    "Brass Volume", unit, 0.6f);
    addB (l, "br_16",     "Brass 16'", false);
    addB (l, "br_8",      "Brass 8'", true);
    addC (l, "br_wave",   "Brass OSC1 Waveform", { "Saw", "Square", "Pulse" }, 0);
    addC (l, "br2_oct",   "Brass OSC2 Octave", { "Off", "16'", "8'", "4'" }, 0);
    addC (l, "br2_wave",  "Brass OSC2 Waveform", { "Saw", "Square", "Pulse" }, 0);
    addF (l, "br2_detune","Brass OSC2 Detune", NR (-50.0f, 50.0f), 7.0f, "ct");
    addF (l, "br_cutoff", "Brass Cutoff Freq", unit, 0.4f);
    addF (l, "br_res",    "Brass Resonance", unit, 0.1f);
    addF (l, "br_egint",  "Brass EG Intensity", unit, 0.5f);
    addF (l, "br_a", "Brass Attack",  timeRange (15.0f), 0.08f, "s");
    addF (l, "br_d", "Brass Decay",   timeRange (15.0f), 0.5f,  "s");
    addF (l, "br_s", "Brass Sustain", unit, 0.7f);
    addF (l, "br_r", "Brass Release", timeRange (15.0f), 0.3f,  "s");
    addB (l, "br_multi",  "Multiple Trigger", false);
    addB (l, "br_tsel",   "Trigger Select", false);
    addB (l, "br_mono",   "Brass Mono", false);
    addB (l, "br_legato", "Brass Legato", true);
    addB (l, "br_polyvcf","Brass Poly VCF", false);
    { NR g (0.0f, 2.0f); g.setSkewForCentre (0.3f); addF (l, "br_glide", "Brass Glide Time", g, 0.0f, "s"); }
    addB (l, "br_gliss",  "Brass Glissando", false);
    addC (l, "br_silence","Silence Note", { "2", "4", "6", "8" }, 0);

    // ---- Strings section
    addB (l, "st_on",      "Strings Output", false);
    addC (l, "st_zone",    "Strings Split", kZones, 1);
    addF (l, "st_vol",     "Strings Volume", unit, 0.6f);
    addB (l, "st_16",      "Strings 16'", false);
    addB (l, "st_8",       "Strings 8'", true);
    addB (l, "st_4",       "Strings 4'", true);
    addF (l, "st_attack",  "Strings Attack",  timeRange (15.0f), 0.15f, "s");
    addF (l, "st_release", "Strings Release", timeRange (15.0f), 0.8f, "s");
    addF (l, "st_balance", "KBD Balance", unit, 0.0f);
    addF (l, "st_cutoff",  "Strings Cutoff Freq", unit, 1.0f);
    addF (l, "st_res",     "Strings Resonance", unit, 0.0f);
    addF (l, "st_eq_hi",   "Equalizer High", eq, 0.0f);
    addF (l, "st_eq_lo",   "Equalizer Low", eq, 0.0f);
    addB (l, "bow_on",     "Bowing", false);
    addF (l, "bow_level",  "Bowing Level", unit, 0.5f);
    addF (l, "bow_tone",   "Bowing Tone", unit, 0.5f);
    addB (l, "svib_off",   "Vibrato Off", false);
    addF (l, "svib_delay", "Vibrato Delay Time", NR (0.01f, 3.0f, 0.0f, 0.5f), 0.5f, "s");
    addF (l, "svib_int",   "Vibrato Intensity", unit, 0.25f);
    addF (l, "svib_speed", "Vibrato Speed", NR (1.0f, 10.0f), 5.5f, "Hz");
    addB (l, "ens_off",    "Ensemble Off", false);

    // ---- Arpeggiators (Tridex II addition): one per section
    addF (l, "arp_tempo", "Arp Tempo", NR (40.0f, 240.0f, 1.0f), 120.0f, "BPM");
    const char* arpIds[]   = { "arp1", "arp2", "arp3" };
    const char* arpNames[] = { "Synthe Arp", "Brass Arp", "Strings Arp" };
    for (int i = 0; i < 3; ++i)
    {
        const juce::String id (arpIds[i]), nm (arpNames[i]);
        addB (l, (id + "_on").toRawUTF8(),   (nm + " On").toRawUTF8(), false);
        addC (l, (id + "_mode").toRawUTF8(), (nm + " Mode").toRawUTF8(), { "Up", "Down", "Up/Down", "Random", "Played" }, 0);
        addC (l, (id + "_rate").toRawUTF8(), (nm + " Rate").toRawUTF8(), { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3);
        addC (l, (id + "_oct").toRawUTF8(),  (nm + " Octaves").toRawUTF8(), { "1", "2", "3", "4" }, 0);
        addF (l, (id + "_gate").toRawUTF8(), (nm + " Gate").toRawUTF8(), NR (0.05f, 1.0f), 0.5f);
        addB (l, (id + "_sync").toRawUTF8(), (nm + " Sync").toRawUTF8(), true);
    }

    // ---- Step gate (Tridex II): per part, 8 steps x L/R lanes, own rate + sync
    const char* sgNames[] = { "Synthe", "Brass", "Strings" };
    for (int i = 0; i < 3; ++i)
    {
        const juce::String id ("sg" + juce::String (i + 1)), nm (juce::String (sgNames[i]) + " Step Gate");
        addB (l, (id + "_on").toRawUTF8(),   (nm + " On").toRawUTF8(), false);
        addB (l, (id + "_sync").toRawUTF8(), (nm + " Sync").toRawUTF8(), true);
        addC (l, (id + "_rate").toRawUTF8(), (nm + " Rate").toRawUTF8(), { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3);
        for (int k = 0; k < 8; ++k)
        {
            addB (l, (id + "_l" + juce::String (k + 1)).toRawUTF8(), (nm + " L" + juce::String (k + 1)).toRawUTF8(), true);
            addB (l, (id + "_r" + juce::String (k + 1)).toRawUTF8(), (nm + " R" + juce::String (k + 1)).toRawUTF8(), true);
        }
    }

    return l;
}

const juce::StringArray& TridexProcessor::programmableIds()
{
    // Items marked * in the spec sheet: VCO1, VCO2, VCF, VCF EG, VCA attenuator, VCA EG, Auto Damp.
    static const juce::StringArray ids { "v1_oct", "v1_wave", "pw_int", "pwm_speed", "v2_scale", "v2_detune", "v2_wave",
                                         "cutoff", "res", "eg_int", "kbd_track",
                                         "sf_a", "sf_d", "sf_s", "sf_r", "sa_a", "sa_d", "sa_s", "sa_r",
                                         "vca_att", "auto_damp", "syn_glide", "syn_gliss", "syn_mono", "syn_legato" };
    return ids;
}

//==============================================================================
TridexProcessor::TridexProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Main",    juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Synthe",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Brass",   juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Strings", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TRIDEX", createLayout())
{
    for (int i = 0; i < 3; ++i)
    {
        const juce::String id ("arp" + juce::String (i + 1)), gid ("sg" + juce::String (i + 1));
        auto P = [this] (const juce::String& s) { auto* ptr = apvts.getRawParameterValue (s); jassert (ptr != nullptr); return ptr; };
        arpPtr[(size_t) i] = { P (id + "_on"), P (id + "_mode"), P (id + "_rate"), P (id + "_oct"), P (id + "_gate"), P (id + "_sync") };
        auto& g = gatePtr[(size_t) i];
        g.on = P (gid + "_on"); g.sync = P (gid + "_sync"); g.rate = P (gid + "_rate");
        for (int k = 0; k < 8; ++k) { g.l[(size_t) k] = P (gid + "_l" + juce::String (k + 1)); g.r[(size_t) k] = P (gid + "_r" + juce::String (k + 1)); }
    }

    // Fill the 32 slots from the factory set (defaults + overrides), rest = Init.
    const auto& factory = tdx::factoryPresets();
    for (int i = 0; i < kNumPrograms; ++i)
    {
        auto& prog = programs[(size_t) i];
        const bool has = i < (int) factory.size();
        prog.name = has ? juce::String (factory[(size_t) i].name) : "Init";
        for (auto& id : programmableIds())
            if (auto* rp = apvts.getParameter (id))
                prog.values[id] = rp->convertFrom0to1 (rp->getDefaultValue());
        if (has)
            for (const auto& [id, v] : factory[(size_t) i].values)
                prog.values[juce::String (id)] = v;
    }
}

bool TridexProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Main must be stereo; the three separate part outputs may each be stereo or switched off.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    for (int b = 1; b < layouts.outputBuses.size(); ++b)
    {
        const auto& set = layouts.outputBuses.getReference (b);
        if (! set.isDisabled() && set != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
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

    p.master = get ("master"); p.tuneCents = get ("tune"); p.assignMode = on ("assign2") ? 1 : 0;
    p.mgSpeed = get ("mg_speed"); p.mgIntensity = get ("mg_int");
    p.delayVib = on ("dvib_on"); p.delayVibInt = get ("dvib_int");
    p.flSynth = on ("fl_syn"); p.flBrass = on ("fl_br"); p.flStrings = on ("fl_st");
    p.chOn = on ("ch_on"); p.chSynth = on ("ch_syn"); p.chBrass = on ("ch_br"); p.chStrings = on ("ch_st");
    p.chMode = idx ("ch_mode"); p.chSpace = idx ("ch_space"); p.chWidth = get ("ch_width"); p.chTone = get ("ch_tone"); p.chRate = get ("ch_rate"); p.chDepth = get ("ch_depth"); p.chMix = get ("ch_mix");
    p.flManual = get ("fl_manual"); p.flIntensity = get ("fl_int"); p.flFeedback = get ("fl_fb"); p.flSpeed = get ("fl_speed");
    p.splitPoint = idx ("split");
    p.splits = { juce::roundToInt (get ("syn_split")), juce::roundToInt (get ("br_split")), juce::roundToInt (get ("st_split")) }; p.bendRange = get ("bend_range"); p.transpose = juce::roundToInt (get ("transpose"));
    p.octShift = { juce::roundToInt (get ("syn_octsh")), juce::roundToInt (get ("br_octsh")), juce::roundToInt (get ("st_octsh")) }; p.drift = get ("drift");
    p.velSens = get ("vel_sens"); p.pedalMode = idx ("pedal_mode"); p.notePriority = idx ("note_prio");

    p.synOn = on ("syn_on"); p.synZone = zone ("syn_zone"); p.synVolume = get ("syn_vol");
    p.v1Oct = idx ("v1_oct"); p.v1Wave = static_cast<tdx::Vco1Wave> (idx ("v1_wave"));
    p.pwInt = get ("pw_int"); p.pwmSpeed = get ("pwm_speed");
    p.v2Scale = idx ("v2_scale"); p.v2Detune = get ("v2_detune"); p.v2Wave = static_cast<tdx::Vco1Wave> (idx ("v2_wave"));
    p.cutoff = get ("cutoff"); p.res = get ("res"); p.egInt = get ("eg_int"); p.kbdTrack = idx ("kbd_track");
    p.sfA = get ("sf_a"); p.sfD = get ("sf_d"); p.sfS = get ("sf_s"); p.sfR = get ("sf_r");
    p.saA = get ("sa_a"); p.saD = get ("sa_d"); p.saS = get ("sa_s"); p.saR = get ("sa_r");
    p.vcaAttenDb = get ("vca_att"); p.autoDamp = on ("auto_damp");

    p.brOn = on ("br_on"); p.brZone = zone ("br_zone"); p.brVolume = get ("br_vol");
    p.br16 = on ("br_16"); p.br8 = on ("br_8"); p.br1Wave = idx ("br_wave");
    p.br2Scale = idx ("br2_oct"); p.br2Wave = idx ("br2_wave"); p.br2Detune = get ("br2_detune");
    p.brCutoff = get ("br_cutoff"); p.brRes = get ("br_res"); p.brEgInt = get ("br_egint");
    p.brA = get ("br_a"); p.brD = get ("br_d"); p.brS = get ("br_s"); p.brR = get ("br_r");
    p.multiTrig = on ("br_multi"); p.trigSelect = on ("br_tsel"); p.brMono = on ("br_mono"); p.brPolyVcf = on ("br_polyvcf");
    p.synMono = on ("syn_mono"); p.synLegato = on ("syn_legato"); p.brLegato = on ("br_legato");
    p.synGlide = get ("syn_glide"); p.synGliss = on ("syn_gliss");
    p.brGlide = get ("br_glide"); p.brGliss = on ("br_gliss"); p.silenceNote = idx ("br_silence");

    p.stOn = on ("st_on"); p.stZone = zone ("st_zone"); p.stVolume = get ("st_vol");
    p.st16 = on ("st_16"); p.st8 = on ("st_8"); p.st4 = on ("st_4");
    p.stAttack = get ("st_attack"); p.stRelease = get ("st_release"); p.kbdBalance = get ("st_balance");
    p.stCutoff = get ("st_cutoff"); p.stRes = get ("st_res");
    p.eqLow = get ("st_eq_lo"); p.eqHigh = get ("st_eq_hi");
    p.bowOn = on ("bow_on"); p.bowLevel = get ("bow_level"); p.bowTone = get ("bow_tone");
    p.vibOn = ! on ("svib_off"); p.vibDelay = get ("svib_delay"); p.vibIntensity = get ("svib_int"); p.vibSpeed = get ("svib_speed");
    p.ensembleOn = ! on ("ens_off");

    p.arpTempo = get ("arp_tempo");
    for (size_t i = 0; i < 3; ++i)
    {
        const auto& ap = arpPtr[i];
        auto& a = p.arp[i];
        a.on      = ap.on->load() > 0.5f;
        a.mode    = (int) ap.mode->load();
        a.rate    = (int) ap.rate->load();
        a.octaves = (int) ap.oct->load() + 1;
        a.gate    = ap.gate->load();
        a.sync    = ap.sync->load() > 0.5f;

        const auto& gp = gatePtr[i];
        auto& g = p.gate[i];
        g.on   = gp.on->load() > 0.5f;
        g.sync = gp.sync->load() > 0.5f;
        g.rate = (int) gp.rate->load();
        for (size_t k = 0; k < 8; ++k)
        {
            g.left[k]  = gp.l[k]->load() > 0.5f;
            g.right[k] = gp.r[k]->load() > 0.5f;
        }
    }
}

void TridexProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                  engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())            engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())         { midiBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f; applyJoystick(); }
    else if (m.isSustainPedalOn())     engine.setPedal (true);
    else if (m.isSustainPedalOff())    engine.setPedal (false);
    else if (m.isControllerOfType (1)) { midiVib   = (float) m.getControllerValue() / 127.0f; applyJoystick(); }
    else if (m.isControllerOfType (2)) { midiTrill = (float) m.getControllerValue() / 127.0f; applyJoystick(); }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    else if (m.isProgramChange())      recallProgram (m.getProgramChangeNumber() % kNumPrograms);
}

// Joystick X = pitch bend, Y+ = vibrato, Y- = trill. MIDI and the on-screen stick add.
void TridexProcessor::applyJoystick()
{
    const float gx = guiJoyX.load(), gy = guiJoyY.load();
    engine.pitchBend (juce::jlimit (-1.0f, 1.0f, midiBend + gx));
    engine.joystickY (juce::jlimit (0.0f, 1.0f, midiVib + juce::jmax (0.0f, gy)),
                      juce::jlimit (0.0f, 1.0f, midiTrill + juce::jmax (0.0f, -gy)));
}

void TridexProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    syncParams();
    applyJoystick();

    // Host clock for the arpeggiators.
    {
        double bpm = 120.0, ppq = 0.0; bool playing = false;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) bpm = *b;
                if (auto q = pos->getPpqPosition()) ppq = *q;
                playing = pos->getIsPlaying();
            }
        engine.setTransport (bpm, ppq, playing);
    }
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    buffer.clear();

    // Main mix + optional separate outputs (Synthe / Brass / Strings), like the hardware's SEPARATE OUT.
    auto main = getBusBuffer (buffer, false, 0);
    float* L = main.getWritePointer (0);
    float* R = main.getWritePointer (1);
    float* pl[3] { nullptr, nullptr, nullptr };
    float* pr[3] { nullptr, nullptr, nullptr };
    for (int s = 0; s < 3; ++s)
        if (s + 1 < getBusCount (false) && getBus (false, s + 1)->isEnabled())
        {
            auto bus = getBusBuffer (buffer, false, s + 1);
            if (bus.getNumChannels() >= 2) { pl[s] = bus.getWritePointer (0); pr[s] = bus.getWritePointer (1); }
        }
    int pos = 0;
    const auto renderTo = [&] (int start, int count)
    {
        float* opl[3]; float* opr[3];
        for (int s = 0; s < 3; ++s) { opl[s] = pl[s] ? pl[s] + start : nullptr; opr[s] = pr[s] ? pr[s] + start : nullptr; }
        engine.render (L + start, R + start, count, opl, opr);
    };

    // Sample-accurate MIDI: render up to each event, then apply it.
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, buffer.getNumSamples(), meta.samplePosition);
        if (t > pos) { renderTo (pos, t - pos); pos = t; }
        handleMidi (meta.getMessage());
    }
    if (pos < buffer.getNumSamples())
        renderTo (pos, buffer.getNumSamples() - pos);
}

//==============================================================================
const juce::String TridexProcessor::getProgramName (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPrograms)) return {};
    return juce::String::charToString ((juce::juce_wchar) ('A' + index / 8)) + juce::String (index % 8 + 1)
           + " " + programs[(size_t) index].name;
}

void TridexProcessor::changeProgramName (int index, const juce::String& name)
{
    if (juce::isPositiveAndBelow (index, kNumPrograms)) programs[(size_t) index].name = name;
}

void TridexProcessor::recallProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPrograms)) return;
    currentProgram = index;
    manualMode = false;
    for (const auto& [id, value] : programs[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

void TridexProcessor::writeProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPrograms)) return;
    auto& prog = programs[(size_t) index];
    for (auto& id : programmableIds())
        prog.values[id] = apvts.getRawParameterValue (id)->load();
    if (prog.name == "Init") prog.name = "User";
    currentProgram = index;
    manualMode = false;
}

void TridexProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::ValueTree progs ("PROGRAMS");
    progs.setProperty ("current", currentProgram, nullptr);
    progs.setProperty ("manual", manualMode, nullptr);
    for (int i = 0; i < kNumPrograms; ++i)
    {
        juce::ValueTree pt ("P");
        pt.setProperty ("name", programs[(size_t) i].name, nullptr);
        for (const auto& [id, v] : programs[(size_t) i].values) pt.setProperty (id, v, nullptr);
        progs.appendChild (pt, nullptr);
    }
    state.removeChild (state.getChildWithName ("PROGRAMS"), nullptr);
    state.appendChild (progs, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void TridexProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto state = juce::ValueTree::fromXml (*xml);

    if (auto progs = state.getChildWithName ("PROGRAMS"); progs.isValid())
    {
        currentProgram = juce::jlimit (0, kNumPrograms - 1, (int) progs.getProperty ("current", 0));
        manualMode = (bool) progs.getProperty ("manual", true);
        for (int i = 0; i < juce::jmin (kNumPrograms, progs.getNumChildren()); ++i)
        {
            auto pt = progs.getChild (i);
            auto& prog = programs[(size_t) i];
            prog.name = pt.getProperty ("name", prog.name).toString();
            for (auto& id : programmableIds())
                if (pt.hasProperty (id)) prog.values[id] = (float) pt.getProperty (id);
        }
        state.removeChild (progs, nullptr);
    }
    apvts.replaceState (state);
}

juce::AudioProcessorEditor* TridexProcessor::createEditor() { return new TridexEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TridexProcessor(); }
