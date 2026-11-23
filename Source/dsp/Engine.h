#pragma once
// Tridex 80 engine — modelled on the Trident Mk II service manual block diagram:
//
//   KLM-377 key assigner (8 voices, Mode 1/2, per-section split L / L+H / H)
//        |
//   KLM-379 signal generator: per voice VCO1 + VCO2, each a saw core with
//   flip-flop octave dividers (4' -> 8' -> 16'), saws rebuilt by summing the
//   core saw with divided squares. Every voice feeds ALL THREE sections:
//        SY-SIG -> KLM-378: per-voice SSM2044 VCF + VCA, SSM2056 EGs
//        BR-SIG -> KLM-403: per-voice gate (MPX EG) -> ONE VCF/VCA + one EG (paraphonic)
//        ST-SIG -> KLM-384: KBD balance -> per-voice VCA (MPX EG AR)
//                  KLM-404: [compressor] -> vibrato -> ensemble -> [expander] -> bowing -> EQ
//   KLM-401: MG (joystick vibrato / trill, delay vibrato) + flanger (per-section select)

#include "Primitives.h"
#include "Arp.h"
#include <atomic>

namespace tdx
{
enum class Zone { Lower = 0, Both, Upper };          // L, L+H, H
enum class Vco1Wave { Saw = 0, PW, PWM };

inline constexpr float kKbdTrack[] = { 0.0f, 0.25f, 0.5f, 1.0f, 1.5f }; // Off, Quarter, Half, Full, Over

struct Params
{
    // ---- Total control
    float master = 0.8f, tuneCents = 0.0f;
    int splitPoint = 60, assignMode = 0;                // 0 = Mode 1, 1 = Mode 2
    std::array<int, 3> splits { 60, 60, 60 };           // Tridex II: split point per part (Synth, Brass, Strings)
    float mgSpeed = 5.5f, mgIntensity = 0.5f;           // joystick vibrato/trill
    bool delayVib = false; float delayVibInt = 0.3f;     // delay time is fixed on the hardware
    float bendRange = 2.0f;
    int transpose = 0;                                  // Tridex II: -12..+12 semitones (split stays on the physical keys)
    std::array<int, 3> octShift {};                     // Tridex II: per-part octave shift -2..+2 (Synth, Brass, Strings)
    bool flSynth = false, flBrass = false, flStrings = false;
    float flManual = 0.4f, flIntensity = 0.5f, flFeedback = 0.4f, flSpeed = 0.3f;
    bool chOn = false, chSynth = true, chBrass = false, chStrings = false;   // Tridex II chorus / ensemble
    int chMode = 0, chSpace = 1; float chRate = 0.6f, chDepth = 0.5f, chMix = 0.5f, chWidth = 0.7f, chTone = 0.6f;
    bool tpOn = false, tpSynth = true, tpBrass = true, tpStrings = true;      // Tridex II tape / VHS
    int tpType = 1;                                                            // 0 Studio, 1 Cassette, 2 VHS, 3 Worn VHS
    std::array<float, 3> tpDrive { 0.3f, 0.3f, 0.3f };                         // saturation per part
    float tpWow = 0.3f, tpFlutter = 0.2f, tpSpeed = 1.0f, tpDrift = 0.3f, tpAge = 0.3f, tpHiss = 0.1f, tpMix = 1.0f;
    float drift = 0.3f, velSens = 0.0f;
    int pedalMode = 2;                                  // 0 = off, 1 = damper (S-1, Trident), 2 = hold (modern sustain)

    // ---- Synthesizer section
    bool synOn = true; Zone synZone = Zone::Both; float synVolume = 0.7f;
    int v1Oct = 1; Vco1Wave v1Wave = Vco1Wave::Saw; float pwInt = 0.3f, pwmSpeed = 1.0f;
    int v2Scale = 2; float v2Detune = 6.0f;            // v2Scale: 0 Off, 1 16', 2 8', 3 4'
    Vco1Wave v2Wave = Vco1Wave::Saw;                    // VCO2 waveform (shares PW/PWM settings with VCO1)
    float vcoMix = 0.5f;                                // Tridex II: VCO1 <-> VCO2 balance (0.5 = both full)
    float cutoff = 0.6f, res = 0.15f, egInt = 0.3f; int kbdTrack = 2;
    float sfA = 0.01f, sfD = 0.6f, sfS = 0.4f, sfR = 0.4f;
    float saA = 0.005f, saD = 0.5f, saS = 0.8f, saR = 0.4f;
    float vcaAttenDb = 0.0f; bool autoDamp = false;

    // ---- Brass section
    bool brOn = false; Zone brZone = Zone::Both; float brVolume = 0.6f;
    bool br16 = false, br8 = true;                      // OSC 1 footages
    int br1Wave = 0;                                    // OSC 1 wave: 0 Saw, 1 Square, 2 Pulse
    int br2Scale = 0, br2Wave = 0; float br2Detune = 7.0f;   // OSC 2: 0 Off, 1 16', 2 8', 3 4'
    float brOscMix = 0.5f;                              // Tridex II: OSC1 <-> OSC2 balance (0.5 = both full)
    float brCutoff = 0.4f, brRes = 0.1f, brEgInt = 0.5f;
    float brA = 0.08f, brD = 0.5f, brS = 0.7f, brR = 0.3f;
    bool multiTrig = true, trigSelect = false; int silenceNote = 0;   // silenceNote: 0..3 -> 2,4,6,8 keys
    bool brMono = false;                                // Tridex II: monophonic brass (last-note priority)
    bool synMono = false;                               // Tridex II: monophonic synth
    bool synLegato = true, brLegato = true;             // MONO: true = LEGATO (overlapping notes don't retrigger), false = RETRIG
    int numVoices = 8;                                  // Tridex II: polyphony, 8 (classic) or 16
    int notePriority = 0;                               // Tridex II: MONO note priority: 0 Last, 1 First, 2 Low, 3 High
    bool brPolyVcf = false;                             // Tridex II: one brass filter per note instead of one shared
    bool brPoly = false;                                // Tridex II: true polyphonic brass (own EG + VCF + VCA per note) vs paraphonic
    float synGlide = 0.0f, brGlide = 0.0f;              // Tridex II: glide time in seconds (0 = off)
    bool synGliss = false, brGliss = false;             // false = portamento (smooth), true = glissando (semitone steps)

    // ---- Strings section
    bool stOn = false; Zone stZone = Zone::Both; float stVolume = 0.6f;
    bool st16 = false, st8 = true, st4 = true;
    float stAttack = 0.15f, stRelease = 0.8f, kbdBalance = 0.0f;   // kbdBalance 0..1 (key-tracked filter amount)
    float stCutoff = 1.0f, stRes = 0.0f;               // Tridex II: strings VCF (1.0 = fully open)
    float eqLow = 0.0f, eqHigh = 0.0f;                 // panel scale -5..+5
    bool bowOn = false; float bowLevel = 0.5f, bowTone = 0.5f;
    bool vibOn = true; float vibDelay = 0.5f, vibIntensity = 0.4f, vibSpeed = 5.5f;
    bool ensembleOn = true;

    // ---- Arpeggiators (one per section: 0 Synth, 1 Brass, 2 Strings)
    std::array<ArpParams, 3> arp {};
    float arpTempo = 120.0f;     // used when an arp's SYNC is off

    // ---- Step gate (Tridex II): per section, 8 steps x L/R lanes, own rate and sync
    struct StepGate
    {
        bool on = false, sync = true;
        int rate = 3;                                   // index into kArpRates (default 1/16)
        std::array<bool, 8> left { true, true, true, true, true, true, true, true };
        std::array<bool, 8> right { true, true, true, true, true, true, true, true };
    };
    std::array<StepGate, 3> gate {};
};

//==============================================================================
// One octave-divider VCO: phase is the 16' (lowest) counter, everything above
// is derived from it so all footages stay phase-locked like the 4013 dividers.
struct DividerVco
{
    double phase = 0.0;
    float mis8 = 0.0f, mis16 = 0.0f;   // summing-resistor tolerance (makes the "staircase" slightly audible)

    struct Out { float saw4, saw8, saw16; double ph4, ph8, ph16, dt4, dt8, dt16; };

    static float saw (double ph, double dt) { return float (2.0 * ph - 1.0) - polyBlep (ph, dt); }
    // Square that is -1 in the first half, +1 in the second (a divider stage).
    static float sq (double ph, double dt)
    {
        float v = ph < 0.5 ? 1.0f : -1.0f;
        v += polyBlep (ph, dt);
        double t2 = ph - 0.5; if (t2 < 0.0) t2 += 1.0;
        v -= polyBlep (t2, dt);
        return -v;
    }

    Out process (double inc16)
    {
        Out o;
        o.dt16 = std::min (inc16, 0.12); o.dt8 = o.dt16 * 2.0; o.dt4 = o.dt16 * 4.0;
        o.ph16 = phase;
        o.ph8 = phase * 2.0; o.ph8 -= std::floor (o.ph8);
        o.ph4 = phase * 4.0; o.ph4 -= std::floor (o.ph4);

        o.saw4  = saw (o.ph4, o.dt4);
        o.saw8  = 0.5f * o.saw4 * (1.0f + mis8)  + 0.5f * sq (o.ph8,  o.dt8);
        o.saw16 = 0.5f * o.saw8 * (1.0f + mis16) + 0.5f * sq (o.ph16, o.dt16);

        phase += o.dt16; if (phase >= 1.0) phase -= 1.0;
        return o;
    }
};

struct Voice
{
    int note = -1;
    uint32_t age = 0;
    bool keyDown = false, synGate = false, brGate = false, stGate = false, damped = false;
    float velocity = 1.0f, driftCents = 0.0f, wander = 0.0f, v2Offset = 0.0f;

    DividerVco vco1, vco2, brVco2;   // brVco2: brass section OSC 2
    Ladder vcf;
    Ladder brVcf;                // Tridex II: per-note brass filter (POLY VCF mode)
    Adsr fenv, aenv;
    Ar brGateEnv, stEnv, bowEnv;
    float bowSmooth = 0.0f;      // bow accent, slewed so a retrigger on a sounding note can't click
    Adsr brEnv;                  // per-note brass EG (POLY voicing)
    bool brCut = false;          // brass gate being cut quickly (mono note change)
    int pitchNote = -1;          // note the oscillators are actually playing
    float glidePitch = 60.0f, glideTarget = 60.0f, glideCoef = 1.0f;   // portamento / glissando state
    bool gliss = false, glideInit = false;
    float vcaSlew = 0.0f;        // analog-style VCA slew (~0.7 ms): softens envelope edges without dulling attacks
    float fadeGain = 1.0f;       // declick: a stolen voice fades out before taking its new note
    bool swapping = false;
    OnePoleLP balLp1, balLp2;   // strings KBD BALANCE filter (12 dB, key tracked)

    bool isActive() const { return aenv.isActive() || brGateEnv.isActive() || brEnv.isActive() || stEnv.isActive(); }
};

//==============================================================================
class Engine
{
public:
    static constexpr int kVoices = 16;   // pool size; p.numVoices (8 or 16) of them are used
    Params p;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        Rng r (0xC0FFEEu);
        for (auto& v : voices)
        {
            v = Voice {};
            v.vcf.prepare (sr); v.vcf.compensation = 0.6f;   // mostly flat passband (manual's graph), less boom
            v.brVcf.prepare (sr); v.brEnv.prepare (sr);
            v.fenv.prepare (sr); v.aenv.prepare (sr);
            v.driftCents = r.bipolar() * 3.0f;
            v.v2Offset = r.bipolar() * 2.0f;
            v.vco1.phase = 0.5 * (r.bipolar() + 1.0); v.vco2.phase = 0.5 * (r.bipolar() + 1.0);
            v.vco1.mis8 = r.bipolar() * 0.02f; v.vco1.mis16 = r.bipolar() * 0.02f;
            v.vco2.mis8 = r.bipolar() * 0.02f; v.vco2.mis16 = r.bipolar() * 0.02f;
            v.brVco2.phase = 0.5 * (r.bipolar() + 1.0);
            v.brVco2.mis8 = r.bipolar() * 0.02f; v.brVco2.mis16 = r.bipolar() * 0.02f;
        }
        brassVcf.prepare (sr); brassEnv.prepare (sr);
        stringsVcfL.prepare (sr); stringsVcfR.prepare (sr);
        for (auto* f : { &stSmoothL1, &stSmoothL2, &stSmoothR1, &stSmoothR2 }) { *f = {}; f->set (9000.0f, sr); }
        stVib.prepare (0.03f, sr);
        ensemble.prepare (sr);
        for (auto& f : flangers) f.prepare (sr);
        for (auto& c : choruses) c.prepare (sr);
        for (auto& t : tapes) t.prepare (sr);
        for (auto& d : dcPL) d = {};
        for (auto& d : dcPR) d = {};
        for (auto* d : { &dcL, &dcR }) *d = {};
        brassHeld = stringsHeld = 0; nextVoice = 0;
        held.fill (false); sustained.fill (false); keyMask.fill (0); arpMask.fill (0); sounding = makeSounding();
        for (auto& a : arps) a.clear ([] (int) {});
    }

    //--------------------------------------------------------------------------
    enum : int { kSyn = 1, kBr = 2, kSt = 4, kAll = 7 };

    // A key went down: route it per section, either straight to the voices or to
    // that section's arpeggiator.
    void noteOn (int note, float vel)
    {
        int mask = 0;
        if (p.synOn && inZone (p.synZone, note, 0)) mask |= kSyn;
        if (p.brOn  && inZone (p.brZone, note, 1))  mask |= kBr;
        if (p.stOn  && inZone (p.stZone, note, 2))  mask |= kSt;
        if (mask == 0) return;

        if (held[(size_t) note]) releaseKey (note);
        held[(size_t) note] = true; sustained[(size_t) note] = false;
        if (heldCount++ == 0) { delayVibEnv = 0.0f; for (auto& q : gateFreeQn) q = 0.0; }   // free-running gates restart at step 1

        int direct = 0, arped = 0;
        for (int s = 0; s < 3; ++s)
            if (mask & (1 << s)) (p.arp[(size_t) s].on ? arped : direct) |= (1 << s);
        for (int s = 0; s < 3; ++s)
            if (arped & (1 << s)) arps[(size_t) s].keyOn (note, vel);

        keyMask[(size_t) note] = (uint8_t) direct;
        arpMask[(size_t) note] = (uint8_t) arped;
        if (direct) voiceOn (note, vel, direct);
    }

    void noteOff (int note)
    {
        if (! held[(size_t) note]) return;
        held[(size_t) note] = false;
        heldCount = std::max (0, heldCount - 1);
        if (p.pedalMode == 2 && pedal) { sustained[(size_t) note] = true; return; }
        releaseKey (note);
    }

    void setPedal (bool down)
    {
        pedal = down;
        if (! down && p.pedalMode == 2)
            for (int n = 0; n < 128; ++n)
                if (sustained[(size_t) n]) { sustained[(size_t) n] = false; releaseKey (n); }
    }

    void pitchBend (float bipolar) { bend = bipolar; }
    void joystickY (float vibrato01, float trill01) { joyVib = vibrato01; joyTrill = trill01; }

    // Host transport for arp sync: tempo and position (quarter notes) at the start of the block.
    void setTransport (double bpm, double ppq, bool playing)
    {
        hostBpm = bpm > 1.0 ? bpm : 120.0;
        hostPpq = ppq;
        hostPlaying = playing;
    }

    void allNotesOff()
    {
        for (int s = 0; s < 3; ++s) arps[(size_t) s].clear ([this, s] (int n) { voiceOff (n, 1 << s); });
        monoCurrent = -1; monoCount = 0; sCurrent = -1; sCount = 0; sVoice = bVoice = -1;
        for (auto& v : voices) if (v.keyDown) releaseSections (v, kAll, false);
        held.fill (false); sustained.fill (false); keyMask.fill (0); arpMask.fill (0); sounding = makeSounding();
        heldCount = 0;
    }

    //--------------------------------------------------------------------------
    // partL / partR (optional): per-part outputs [0] Synth, [1] Brass, [2] Strings; null entries are skipped.
    void render (float* outL, float* outR, int numSamples, float* const* partL = nullptr, float* const* partR = nullptr)
    {
        updateCoefficients();
        const double invSr = 1.0 / sr;
        const float kbd = kKbdTrack[std::clamp (p.kbdTrack, 0, 4)];
        const float attenGain = std::pow (10.0f, p.vcaAttenDb / 20.0f);
        const int v1Oct = 2 - std::clamp (p.v1Oct, 0, 2);   // footage index -> divider stage (0 = 4' core)

        const float fadeStep = 1.0f / (0.003f * sr);   // 3 ms steal fade
        // Oscillator MIX: centre = both oscillators at full level; towards an end the other one fades out.
        const float vcoG1 = std::min (1.0f, 2.0f * (1.0f - p.vcoMix)), vcoG2 = std::min (1.0f, 2.0f * p.vcoMix);
        const float brG1 = std::min (1.0f, 2.0f * (1.0f - p.brOscMix)), brG2 = std::min (1.0f, 2.0f * p.brOscMix);
        const float gateSmooth = 1.0f - std::exp (-1.0f / (0.002f * sr));
        const float vcaSlewCoef = 1.0f - std::exp (-1.0f / (0.0007f * sr));
        const float sendSmooth = 1.0f - std::exp (-1.0f / (0.010f * sr));
        const float bowSlew = 1.0f - std::exp (-1.0f / (0.003f * sr));
        const int brassOscs = std::max (1, (p.br16 ? 1 : 0) + (p.br8 ? 1 : 0) + (p.br2Scale > 0 ? 1 : 0));
        const int stringFeet = std::max (1, (p.st16 ? 1 : 0) + (p.st8 ? 1 : 0) + (p.st4 ? 1 : 0));
        const float stMixNorm = 1.0f / std::sqrt ((float) stringFeet);
        const float brassMixNorm = 1.0f / std::sqrt ((float) brassOscs);
        const double hostQnPerSample = hostBpm / 60.0 / sr;
        const double freeQnPerSample = std::max (20.0f, p.arpTempo) / 60.0 / sr;

        // Leaving MONO releases the mono brass note (keys are re-read on the next press).
        if (lastBrMono && ! p.brMono) monoReset();
        if (lastSynMono && ! p.synMono) { if (sCurrent >= 0) voiceOffRaw (sCurrent, kSyn); sCurrent = -1; sCount = 0; }
        lastSynMono = p.synMono;
        lastBrMono = p.brMono;

        // An arp that was switched off releases whatever it was playing.
        for (int s = 0; s < 3; ++s)
            if (! p.arp[(size_t) s].on && ! arps[(size_t) s].isIdle())
                arps[(size_t) s].clear ([this, s] (int n) { voiceOff (n, 1 << s); });

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- Arpeggiators (one per section)
            for (int s = 0; s < 3; ++s)
            {
                const auto& ap = p.arp[(size_t) s];
                if (! ap.on) continue;
                const int bit = 1 << s;
                arps[(size_t) s].tick (ap, ap.sync && hostPlaying, hostPpq,
                                       ap.sync ? hostQnPerSample : freeQnPerSample,
                                       [this, bit] (int n, float vel) { voiceOn (n, vel, bit); },
                                       [this, bit] (int n) { voiceOff (n, bit); });
            }
            hostPpq += hostQnPerSample;

            // ---- MG / joystick (KLM-401) — modulates all VCOs via the anti-log amp
            mgPhase += p.mgSpeed * invSr; if (mgPhase >= 1.0) mgPhase -= 1.0;
            const float tri = float (mgPhase < 0.5 ? 4.0 * mgPhase - 1.0 : 3.0 - 4.0 * mgPhase);
            const float trillSq = mgPhase < 0.5 ? 1.0f : 0.0f;
            delayVibEnv = std::min (1.0f, delayVibEnv + float (invSr / kDelayVibTime));
            // With DELAY VIBRATO on, joystick up/down (vibrato & trill) has no effect.
            const float jv = p.delayVib ? 0.0f : joyVib, jt = p.delayVib ? 0.0f : joyTrill;
            const float vibSemis = tri * (jv * p.mgIntensity + (p.delayVib ? delayVibEnv * p.delayVibInt * 0.6f : 0.0f));
            const float trillSemis = trillSq * jt * p.mgIntensity * 2.0f;
            const float globalSemis = p.tuneCents * 0.01f + (float) p.transpose + bend * p.bendRange + vibSemis + trillSemis;

            pwmPhase += p.pwmSpeed * invSr; if (pwmPhase >= 1.0) pwmPhase -= 1.0;
            const float pwmTri = float (pwmPhase < 0.5 ? 4.0 * pwmPhase - 1.0 : 3.0 - 4.0 * pwmPhase);
            // PW: 0 = square, narrowing until the pulse collapses (silence) at 10.
            // PWM: 0 = no modulation; from ~8 upward the pulse collapses for part of each cycle.
            float pw = 0.5f;
            if (p.v1Wave == Vco1Wave::PW)  pw = 0.5f * (1.0f - p.pwInt);
            if (p.v1Wave == Vco1Wave::PWM) pw = 0.5f * (1.0f - p.pwInt * 1.25f * (0.5f + 0.5f * pwmTri));
            pw = std::max (0.0f, pw);

            float synSum = 0.0f, brSum = 0.0f, stSum = 0.0f, bowSum = 0.0f, brWeight = 0.0f, stWeight = 0.0f;
            bool brassGateOpen = false;

            for (auto& v : voices)
            {
                if (! v.isActive()) { if (v.note >= 0 && ! v.keyDown) v.note = -1; continue; }

                if (v.swapping)
                {
                    v.fadeGain -= fadeStep;
                    if (v.fadeGain <= 0.0f) swapVoice (v);
                }
                // Smooth (S-curve) fade so the declick itself has no corners.
                const float fg = v.swapping ? v.fadeGain * v.fadeGain * (3.0f - 2.0f * v.fadeGain) : 1.0f;
                const float vg = fg * v.velocity;

                v.wander += rng.bipolar() * 0.02f - v.wander * 0.0005f;
                // Glide: exponential slide towards the target; glissando quantises it to semitones.
                v.glidePitch += (v.glideTarget - v.glidePitch) * v.glideCoef;
                const float basePitch = v.gliss ? std::round (v.glidePitch) : v.glidePitch;
                const float semis = basePitch + globalSemis + p.drift * (v.driftCents + v.wander) * 0.01f;
                // 16' counter frequency = note - 12 semitones (8' = concert pitch)
                const auto o1 = v.vco1.process (midiToHz (semis - 12.0f) * invSr);

                // ---- Synth (KLM-378)
                if (v.aenv.isActive())
                {
                    const float osc1 = footageWave (o1, v1Oct, p.v1Wave == Vco1Wave::Saw ? -1.0f : pw);

                    float osc2 = 0.0f;
                    if (p.v2Scale > 0)
                    {
                        const float s2 = semis + (p.v2Detune + v.v2Offset * p.drift) * 0.01f - 12.0f;
                        const auto o2 = v.vco2.process (midiToHz (s2) * invSr);
                        const int stage = 3 - p.v2Scale;          // 16' -> 2, 8' -> 1, 4' -> 0
                        osc2 = footageWave (o2, stage, p.v2Wave == Vco1Wave::Saw ? -1.0f : pw);
                    }

                    const float fe = v.fenv.process();
                    const float aeRaw = v.aenv.process();
                    v.vcaSlew += (aeRaw - v.vcaSlew) * vcaSlewCoef;
                    const float ae = v.vcaSlew;
                    const float cutOct = p.cutoff * 10.0f + p.egInt * fe * 10.0f + kbd * ((float) v.pitchNote - 60.0f) / 12.0f;
                    v.vcf.setCutoff (20.0f * std::exp2 (cutOct), p.res);
                    synSum += v.vcf.process ((osc1 * vcoG1 + osc2 * vcoG2) * 0.35f) * ae * attenGain * vg;
                    if (! v.aenv.isActive()) v.vcf.reset();
                }

                // ---- Brass per-voice gate (KLM-403 MPX EG)
                if (v.brGateEnv.isActive())
                {
                    const float g = v.brGateEnv.process();
                    brassGateOpen = true;
                    brWeight += g * fg;
                    const float bpw = brassPulseWidth (p.br1Wave);
                    float b = ((p.br16 ? footageWave (o1, 2, bpw) : 0.0f) + (p.br8 ? footageWave (o1, 1, bpw) : 0.0f)) * brG1;
                    if (p.br2Scale > 0)
                    {
                        const float s2 = semis + (p.br2Detune + v.v2Offset * p.drift) * 0.01f - 12.0f;
                        const auto ob = v.brVco2.process (midiToHz (s2) * invSr);
                        b += footageWave (ob, 3 - p.br2Scale, brassPulseWidth (p.br2Wave)) * brG2;
                    }
                    // Normalise the oscillator stack (16' + 8' + OSC 2) so it doesn't overdrive the filter.
                    b *= brassMixNorm;
                    if (p.brPoly)
                    {
                        // POLY voicing: this note's own EG drives its own filter and amplifier.
                        const float e = v.brEnv.process();
                        const float cutOct = p.brCutoff * 10.0f + p.brEgInt * e * 8.0f + 0.5f * ((float) v.pitchNote - 60.0f) / 12.0f;
                        v.brVcf.setCutoff (20.0f * std::exp2 (cutOct), p.brRes);
                        b = v.brVcf.process (b * kBrassIn) * kBrassMakeup * e;
                    }
                    else if (p.brPolyVcf)
                    {
                        // PARA voicing, own filter per note, half key tracking, swept by the shared brass EG.
                        const float cutOct = p.brCutoff * 10.0f + p.brEgInt * brassEnv.value * 8.0f + 0.5f * ((float) v.pitchNote - 60.0f) / 12.0f;
                        v.brVcf.setCutoff (20.0f * std::exp2 (cutOct), p.brRes);
                        b = v.brVcf.process (b * kBrassIn) * kBrassMakeup;
                    }
                    brSum += b * g * vg;
                    if (! v.brGateEnv.isActive()) v.brVcf.reset();
                }

                // ---- Strings per-voice (KLM-384): KBD balance + AR VCA
                if (v.stEnv.isActive())
                {
                    const float e = v.stEnv.process();
                    const float raw = ((p.st16 ? o1.saw16 : 0.0f) + (p.st8 ? o1.saw8 : 0.0f) + (p.st4 ? o1.saw4 : 0.0f)) * stMixNorm;
                    stWeight += e * fg;
                    const float sig = v.balLp2.process (v.balLp1.process (raw));
                    stSum += sig * e * vg;
                    v.bowSmooth += (v.bowEnv.value - v.bowSmooth) * bowSlew;
                    if (p.bowOn) bowSum += sig * v.bowSmooth * e * fg;
                    v.bowEnv.value *= bowDecay;
                }
            }

            // ---- Brass: single VCF + VCA driven by one EG (paraphonic)
            float brass = 0.0f;
            if (p.brPoly)
            {
                // POLY voicing: notes were enveloped and filtered individually above.
                brassEnv.process();   // keep the shared EG ticking so switching back is seamless
                brass = brSum * (1.0f / std::sqrt (std::max (1.0f, brWeight))) * p.brVolume;
            }
            else if (brassEnv.isActive() || brassGateOpen)
            {
                const float e = brassEnv.process();
                // Voice-count compensation: 1/sqrt(notes sounding), following the gate fades,
                // so one note and a full chord sit at about the same loudness.
                const float comp = 1.0f / std::sqrt (std::max (1.0f, brWeight));
                if (p.brPolyVcf)
                    brass = brSum * comp * e * p.brVolume;        // already filtered per note
                else
                {
                    brassVcf.setCutoff (20.0f * std::exp2 (p.brCutoff * 10.0f + p.brEgInt * e * 8.0f), p.brRes);
                    brass = brassVcf.process (brSum * comp * kBrassIn) * kBrassMakeup * e * p.brVolume;
                }
            }

            // ---- Strings chain (KLM-404)
            stVibEnv = std::min (1.0f, stVibEnv + float (invSr / std::max (0.01f, p.vibDelay)));
            // Voice-count compensation (1/sqrt(notes sounding), following the envelopes) keeps chords
            // from piling up into the ensemble and filter; makeup gain comes after (kStTrim).
            float s = stSum * 0.22f / std::sqrt (std::max (1.0f, stWeight));
            // Vibrato and ensemble always run and are crossfaded (~10 ms), so VIBRATO OFF /
            // ENSEMBLE OFF don't click.
            {
                stVibPhase += p.vibSpeed * invSr; if (stVibPhase >= 1.0) stVibPhase -= 1.0;
                const float mod = std::sin (kTwoPi * (float) stVibPhase) * p.vibIntensity * stVibEnv;
                stVib.push (s);
                const float vib = stVib.read ((2.0f + 1.6f * mod) * 0.001f * sr);
                vibMix += ((p.vibOn ? 1.0f : 0.0f) - vibMix) * sendSmooth;
                s += (vib - s) * vibMix;
            }
            float sL = s, sR = s;
            {
                float eL, eR;
                ensemble.process (s, 1.0f, eL, eR);
                ensMix += ((p.ensembleOn ? 1.0f : 0.0f) - ensMix) * sendSmooth;
                sL += (eL - sL) * ensMix; sR += (eR - sR) * ensMix;
            }
            if (p.bowOn)
            {
                const float b = bowBp.process (bowSum * 0.22f) * p.bowLevel * 2.0f;
                sL += b; sR += b;
            }
            // Gentle string-machine smoothing (~9 kHz, 12 dB/oct) takes the raw sawtooth fizz off.
            sL = stSmoothL2.process (stSmoothL1.process (sL));
            sR = stSmoothR2.process (stSmoothR1.process (sR));
            if (p.stCutoff < 0.999f || p.stRes > 0.001f)
            {
                // Strings VCF (Tridex II), after the ensemble so it shapes the whole section.
                const float hz = 20.0f * std::exp2 (p.stCutoff * 10.0f);
                stringsVcfL.setCutoff (hz, p.stRes); stringsVcfR.setCutoff (hz, p.stRes);
                sL = stringsVcfL.process (sL); sR = stringsVcfR.process (sR);
            }
            sL = eqHiL.process (eqLoL.process (sL)) * p.stVolume * kStTrim;
            sR = eqHiR.process (eqLoR.process (sR)) * p.stVolume * kStTrim;

            const float synth = synSum * p.synVolume * kSynTrim;

            // ---- Step gates: per section L/R gains, smoothed (~2 ms) so the chopping never clicks
            float gl[3], gr[3];
            for (int s = 0; s < 3; ++s)
            {
                const auto& sg = p.gate[(size_t) s];
                float tl = 1.0f, tr = 1.0f;
                if (sg.on)
                {
                    const double len = kArpRates[std::clamp (sg.rate, 0, 5)];
                    const double pos = (sg.sync && hostPlaying) ? hostPpq : gateFreeQn[(size_t) s];
                    const int step = (int) (((long long) std::floor (pos / len)) & 7);
                    gateStepUi[(size_t) s].store (step, std::memory_order_relaxed);
                    tl = sg.left[(size_t) step] ? 1.0f : 0.0f;
                    tr = sg.right[(size_t) step] ? 1.0f : 0.0f;
                }
                else gateStepUi[(size_t) s].store (-1, std::memory_order_relaxed);
                gateFreeQn[(size_t) s] += sg.sync ? hostQnPerSample : freeQnPerSample;
                gateL[(size_t) s] += (tl - gateL[(size_t) s]) * gateSmooth;
                gateR[(size_t) s] += (tr - gateR[(size_t) s]) * gateSmooth;
                gl[s] = gateL[(size_t) s]; gr[s] = gateR[(size_t) s];
            }

            // ---- Flanger on selected sections
            // Routing: chorus-selected parts go through the chorus first; flanger-selected parts
            // (and the chorus output, if any chorused part is also flanger-selected) through the flanger.
            // Each part runs through its own chorus and flanger (same settings, LFOs in step), so it
            // can also be sent to its own output. Sends are crossfaded (~10 ms): no clicks when switching.
            const bool chSel[3] = { p.chSynth, p.chBrass, p.chStrings };
            const bool flSel[3] = { p.flSynth, p.flBrass, p.flStrings };
            const float secL[3] = { synth * gl[0], brass * gl[1], sL * gl[2] };
            const float secR[3] = { synth * gr[0], brass * gr[1], sR * gr[2] };
            float mixL = 0.0f, mixR = 0.0f;
            for (int s = 0; s < 3; ++s)
            {
                chSend[(size_t) s] += (((p.chOn && chSel[s]) ? 1.0f : 0.0f) - chSend[(size_t) s]) * sendSmooth;
                flSend[(size_t) s] += ((flSel[s] ? 1.0f : 0.0f) - flSend[(size_t) s]) * sendSmooth;
                const float c = chSend[(size_t) s], f = flSend[(size_t) s];

                float xl = secL[s], xr = secR[s];
                float cl = xl, cr = xr;      // effects always run so they're ready when switched in
                choruses[(size_t) s].process (cl, cr, p.chMode, p.chRate, p.chDepth, p.chMix, p.chSpace, p.chWidth, p.chTone);
                xl += (cl - xl) * c;  xr += (cr - xr) * c;
                float fl = xl, fr = xr;
                flangers[(size_t) s].process (fl, fr, p.flManual, p.flIntensity, p.flFeedback, p.flSpeed);
                xl += (fl - xl) * f;  xr += (fr - xr) * f;
                // Tape at the end of the part's chain (crossfaded in like the other effects)
                const bool tpSel[3] = { p.tpSynth, p.tpBrass, p.tpStrings };
                tpSend[(size_t) s] += (((p.tpOn && tpSel[s]) ? 1.0f : 0.0f) - tpSend[(size_t) s]) * sendSmooth;
                float tl = xl, tr = xr;
                tapes[(size_t) s].process (tl, tr, p.tpType, p.tpDrive[(size_t) s], p.tpWow, p.tpFlutter, p.tpSpeed,
                                           p.tpDrift, p.tpAge, p.tpHiss, p.tpMix);
                xl += (tl - xl) * tpSend[(size_t) s];  xr += (tr - xr) * tpSend[(size_t) s];

                mixL += xl; mixR += xr;
                if (partL != nullptr && partL[s] != nullptr)
                {
                    partL[s][i] = outputStage (dcPL[(size_t) s].process (xl) * p.master);
                    partR[s][i] = outputStage (dcPR[(size_t) s].process (xr) * p.master);
                }
            }

            outL[i] = outputStage (dcL.process (mixL) * p.master);
            outR[i] = outputStage (dcR.process (mixR) * p.master);
        }
    }

private:
    // One footage of a divider VCO. stage: 0 = 4', 1 = 8', 2 = 16'.
    // pw < 0 selects the sawtooth; otherwise a pulse of that width (collapses to silence near 0).
    static float footageWave (const DividerVco::Out& o, int stage, float pw)
    {
        stage = std::clamp (stage, 0, 2);
        if (pw < 0.0f) return stage == 0 ? o.saw4 : (stage == 1 ? o.saw8 : o.saw16);
        if (pw < 0.004f) return 0.0f;
        const double ph = stage == 0 ? o.ph4 : (stage == 1 ? o.ph8 : o.ph16);
        const double dt = stage == 0 ? o.dt4 : (stage == 1 ? o.dt8 : o.dt16);
        float out = ph < pw ? 1.0f : -1.0f;
        out += polyBlep (ph, dt);
        double t2 = ph - pw; if (t2 < 0.0) t2 += 1.0;
        out -= polyBlep (t2, dt);
        return out;
    }

    // Brass waveforms: Saw, Square (50%), Pulse (25%).
    static float brassPulseWidth (int wave) { return wave == 1 ? 0.5f : (wave == 2 ? 0.25f : -1.0f); }

    bool inZone (Zone z, int note, int section) const
    {
        if (z == Zone::Both) return true;
        const int split = p.splits[(size_t) std::clamp (section, 0, 2)];
        return z == Zone::Lower ? note < split : note >= split;
    }

    // KLM-377 key assigner (owner's manual p.4/42):
    //  Mode 1: a different unit for each note, so every note keeps its own release.
    //  Mode 2: the same unit plays each new note; the previous note's release is cut.
    Voice& allocate (int note)
    {
        for (auto& v : voices) if (v.note == note && v.isActive()) return stamp (v);

        // Only the first n voices are assignable (8 = classic, 16 = Tridex II). Voices above n that
        // are still sounding after a switch from 16 to 8 just finish their notes.
        const int n = std::clamp (p.numVoices, 1, kVoices);
        if (p.assignMode == 0)
        {
            if (nextVoice >= n) nextVoice = 0;
            for (int k = 0; k < n; ++k)
            {
                auto& v = voices[(size_t) ((nextVoice + k) % n)];
                if (! v.keyDown && ! v.isActive()) { nextVoice = (nextVoice + k + 1) % n; return stamp (v); }
            }
        }
        else
        {
            for (int i = 0; i < n; ++i) if (! voices[(size_t) i].keyDown) return stamp (voices[(size_t) i]);   // lowest unit not held
        }
        // Steal: oldest released voice, else oldest held.
        Voice* best = nullptr;
        for (int i = 0; i < n; ++i) { auto& v = voices[(size_t) i]; if (! v.keyDown && (! best || v.age < best->age)) best = &v; }
        if (! best) for (int i = 0; i < n; ++i) { auto& v = voices[(size_t) i]; if (! best || v.age < best->age) best = &v; }
        return stamp (*best);
    }

    static std::array<std::array<int16_t, 128>, 3> makeSounding()
    {
        std::array<std::array<int16_t, 128>, 3> a {};
        for (auto& r : a) r.fill (-1);
        return a;
    }

    // Completes a declick swap: the voice starts its new note from silence.
    void swapVoice (Voice& v)
    {
        v.swapping = false;
        v.fadeGain = 1.0f;
        v.pitchNote = v.note;
        v.glidePitch = v.glideTarget = (float) v.note;
        v.glideCoef = 1.0f; v.gliss = false;
        v.fenv.reset(); v.aenv.reset();
        if (v.synGate) { v.fenv.gateOn(); v.aenv.gateOn(); }
        v.brGateEnv.value = 0.0f;
        v.brEnv.reset(); if (v.brGate) v.brEnv.gateOn();
        v.stEnv.value = 0.0f;
        v.bowEnv.value = v.stGate ? 1.0f : 0.0f;
        v.vcf.reset(); v.brVcf.reset();
    }

    // Clean up to -2 dBFS, then a soft knee towards 0 dBFS (only touches real overs).
    static float outputStage (float x)
    {
        const float ax = std::abs (x);
        if (ax < 0.8f) return x;
        return std::copysign (0.8f + 0.2f * std::tanh ((ax - 0.8f) / 0.2f), x);
    }

    int brassThreshold() const { return p.trigSelect ? 2 * (std::clamp (p.silenceNote, 0, 3) + 1) : 1; }

    Voice& stamp (Voice& v) { v.age = ++ageCounter; return v; }

    // Section note entry points: brass is diverted through the mono handler when MONO is on.
    // Each section plays the note shifted by its OCTAVE setting. The sounding note is
    // remembered per section so a release always matches, even if the octave changed.
    void voiceOn (int note, float vel, int mask)
    {
        for (int s = 0; s < 3; ++s)
        {
            const int bit = 1 << s;
            if (! (mask & bit)) continue;
            const int sn = std::clamp (note + 12 * std::clamp (p.octShift[(size_t) s], -2, 2), 0, 127);
            sounding[(size_t) s][(size_t) note] = (int16_t) sn;
            if (bit == kBr && p.brMono) brassMonoOn (sn, vel);
            else if (bit == kSyn && p.synMono) synthMonoOn (sn, vel);
            else voiceOnRaw (sn, vel, bit);
        }
    }

    void voiceOff (int note, int mask)
    {
        for (int s = 0; s < 3; ++s)
        {
            const int bit = 1 << s;
            if (! (mask & bit)) continue;
            int sn = sounding[(size_t) s][(size_t) note];
            if (sn < 0) sn = note;
            sounding[(size_t) s][(size_t) note] = -1;
            if (bit == kBr && p.brMono) brassMonoOff (sn);
            else if (bit == kSyn && p.synMono) synthMonoOff (sn);
            else voiceOffRaw (sn, bit);
        }
    }

    // ---- Monophonic brass: one note at a time, last-note priority.
    // ---- Monophonic synth: one note at a time, last-note priority; legato when notes overlap
    // (the new note picks up the envelopes where the old one was, so nothing restarts).
    // Index of the held note that should sound, by NOTE PRIORITY (Last / First / Low / High).
    int pickByPriority (const std::array<int, 128>& stack, int count) const
    {
        if (count <= 0) return -1;
        int k = count - 1;                                         // Last
        if (p.notePriority == 1) k = 0;                            // First
        else if (p.notePriority == 2 || p.notePriority == 3)       // Low / High
            for (int i = 0; i < count; ++i)
                if (p.notePriority == 2 ? stack[(size_t) i] < stack[(size_t) k] : stack[(size_t) i] > stack[(size_t) k]) k = i;
        return k;
    }

    // ---- True monophonic behaviour (Minimoog / Pro-One style) --------------------------
    // Each mono part owns ONE voice that keeps running: a new key retunes it in place, so the
    // oscillator phase and filter state carry straight through (no voice switch, no click).
    // LEGATO: with a key held nothing restarts. RETRIG: the envelope re-attacks from its current level.

    // The part's mono voice, if it is still ours and not shared with another section.
    Voice* monoVoice (int idx, int expectedNote, int section)
    {
        if (idx < 0) return nullptr;
        auto& v = voices[(size_t) idx];
        if (! v.isActive() || v.note != expectedNote) return nullptr;
        if (section == 0 && (v.brGate || v.stGate)) return nullptr;
        if (section == 1 && (v.synGate || v.stGate)) return nullptr;
        return &v;
    }

    int findVoice (int note, int bit) const
    {
        for (int i = 0; i < kVoices; ++i)
        {
            const auto& v = voices[(size_t) i];
            if (v.keyDown && v.note == note && ((bit == kSyn && v.synGate) || (bit == kBr && v.brGate))) return i;
        }
        return -1;
    }

    // Retune a mono voice in place (glide if the part's GLIDE TIME is up).
    void retuneMono (Voice& v, int note, float vel, int section)
    {
        v.note = note; v.pitchNote = note; v.keyDown = true;
        v.velocity = 1.0f - p.velSens * (1.0f - vel);
        v.swapping = false; v.fadeGain = 1.0f; v.glideInit = true;
        const float gt = section == 0 ? p.synGlide : p.brGlide;
        if (gt > 0.001f)
        {
            v.glideTarget = (float) note;
            v.glideCoef = 1.0f - std::exp (-1.0f / (std::max (0.001f, gt * 0.25f) * sr));
            v.gliss = section == 0 ? p.synGliss : p.brGliss;
        }
        else { v.glidePitch = v.glideTarget = (float) note; v.glideCoef = 1.0f; v.gliss = false; }
        lastNote[(size_t) section] = note;
    }

    void synthMonoOn (int note, float vel)
    {
        sRemove (note);
        if (sCount < (int) sStack.size()) { sStack[(size_t) sCount] = note; sVel[(size_t) sCount] = vel; ++sCount; }
        const int k = pickByPriority (sStack, sCount);
        if (k >= 0) synthMonoSwitchTo (sStack[(size_t) k], sVel[(size_t) k], sStack[(size_t) k] == note);
    }

    void synthMonoOff (int note)
    {
        sRemove (note);
        if (note != sCurrent) { voiceOffRaw (note, kSyn); return; }
        if (sCount > 0) { const int k = pickByPriority (sStack, sCount); synthMonoSwitchTo (sStack[(size_t) k], sVel[(size_t) k], false); return; }
        if (auto* mv = monoVoice (sVoice, sCurrent, 0))
        {
            mv->synGate = false; mv->fenv.gateOff(); mv->aenv.gateOff();
            mv->keyDown = mv->brGate || mv->stGate;
        }
        else voiceOffRaw (sCurrent, kSyn);
        sCurrent = -1;
    }

    // press = a key was just pressed (vs. falling back to a still-held key on release).
    void synthMonoSwitchTo (int note, float vel, bool press)
    {
        if (auto* mv = monoVoice (sVoice, sCurrent >= 0 ? sCurrent : sVoiceNote, 0))
        {
            const bool keyWasHeld = mv->synGate;
            retuneMono (*mv, note, vel, 0);
            mv->damped = false;
            if (! keyWasHeld) { mv->synGate = true; mv->fenv.gateOn(); mv->aenv.gateOn(); }   // new press: re-attack from current level
            else if (press && ! p.synLegato) { mv->fenv.gateOn(); mv->aenv.gateOn(); }      // RETRIG: re-attack from current level
            sVoiceNote = note;
        }
        else
        {
            if (sCurrent >= 0) voiceOffRaw (sCurrent, kSyn);
            voiceOnRaw (note, vel, kSyn);
            sVoice = findVoice (note, kSyn);
            sVoiceNote = note;
        }
        sCurrent = note;
    }

    void sRemove (int note)
    {
        for (int i = 0; i < sCount; ++i)
            if (sStack[(size_t) i] == note)
            {
                for (int j = i; j < sCount - 1; ++j) { sStack[(size_t) j] = sStack[(size_t) j + 1]; sVel[(size_t) j] = sVel[(size_t) j + 1]; }
                --sCount;
                return;
            }
    }

    void brassMonoOn (int note, float vel)
    {
        monoRemove (note);
        if (monoCount < (int) monoStack.size()) { monoStack[(size_t) monoCount] = note; monoVel[(size_t) monoCount] = vel; ++monoCount; }
        const int k = pickByPriority (monoStack, monoCount);
        if (k >= 0) monoSwitchTo (monoStack[(size_t) k], monoVel[(size_t) k], monoStack[(size_t) k] == note);
    }

    void brassMonoOff (int note)
    {
        monoRemove (note);
        if (note != monoCurrent) { voiceOffRaw (note, kBr); return; }   // e.g. held from before MONO was switched on
        if (monoCount > 0) { const int k = pickByPriority (monoStack, monoCount); monoSwitchTo (monoStack[(size_t) k], monoVel[(size_t) k], false); return; }
        if (auto* mv = monoVoice (bVoice, monoCurrent, 1))
        {
            mv->brGate = false; mv->brGateEnv.gate = false; mv->brEnv.gateOff();
            mv->keyDown = mv->synGate || mv->stGate;
            const bool wasOn = brassHeld >= brassThreshold();
            brassHeld = std::max (0, brassHeld - 1);
            if (wasOn && brassHeld < brassThreshold()) brassEnv.gateOff();
        }
        else voiceOffRaw (monoCurrent, kBr);
        monoCurrent = -1;
    }

    void monoSwitchTo (int note, float vel, bool press)
    {
        if (auto* mv = monoVoice (bVoice, monoCurrent >= 0 ? monoCurrent : bVoiceNote, 1))
        {
            const bool keyWasHeld = mv->brGate;
            retuneMono (*mv, note, vel, 1);
            if (! keyWasHeld)
            {
                // New press while the mono voice is releasing: reopen it; the brass EG re-attacks from its level.
                mv->brGate = true; mv->brGateEnv.gate = true; mv->brCut = false; mv->brEnv.gateOn();
                ++brassHeld;
                if (brassHeld >= brassThreshold()) brassEnv.gateOn();
            }
            else if (press && ! p.brLegato) { brassEnv.gateOn(); mv->brEnv.gateOn(); }      // RETRIG: re-attack from current level
            bVoiceNote = note;
        }
        else
        {
            if (monoCurrent >= 0) voiceOffRaw (monoCurrent, kBr);
            voiceOnRaw (note, vel, kBr);
            bVoice = findVoice (note, kBr);
            bVoiceNote = note;
        }
        monoCurrent = note;
    }

    void monoRemove (int note)
    {
        for (int i = 0; i < monoCount; ++i)
            if (monoStack[(size_t) i] == note)
            {
                for (int j = i; j < monoCount - 1; ++j) { monoStack[(size_t) j] = monoStack[(size_t) j + 1]; monoVel[(size_t) j] = monoVel[(size_t) j + 1]; }
                --monoCount;
                return;
            }
    }

    void monoReset()
    {
        if (monoCurrent >= 0) voiceOffRaw (monoCurrent, kBr);
        monoCurrent = -1; monoCount = 0;
    }

    // A key came up: release its direct sections and take it out of the arps.
    void releaseKey (int note)
    {
        const int direct = keyMask[(size_t) note], arped = arpMask[(size_t) note];
        keyMask[(size_t) note] = arpMask[(size_t) note] = 0;
        for (int s = 0; s < 3; ++s)
            if (arped & (1 << s)) arps[(size_t) s].keyOff (note);
        if (direct) voiceOff (note, direct);
    }

    // Start a note on the given sections. Sections share the 8 voices, as on the
    // hardware; a voice already holding this note just gains the extra sections.
    void voiceOnRaw (int note, float vel, int mask)
    {
        if ((mask & kSyn) && p.autoDamp)
            for (auto& o : voices) if (! o.keyDown && o.aenv.isActive()) o.damped = true;

        Voice* vp = nullptr;
        for (auto& o : voices) if (o.keyDown && o.note == note) { vp = &o; break; }
        if (vp == nullptr)
        {
            vp = &allocate (note);
            if (vp->keyDown) releaseSections (*vp, kAll, true);   // stolen while held: keep section counters right
        }
        Voice& v = *vp;
        // Glide (Tridex II): Synth or Brass portamento/glissando from the previous note.
        const int gsec = (mask & kSyn) ? 0 : ((mask & kBr) ? 1 : -1);
        const float gtime = gsec == 0 ? p.synGlide : (gsec == 1 ? p.brGlide : 0.0f);
        if (gsec >= 0 && gtime > 0.001f)
        {
            float start = (float) note;
            if (v.isActive() && v.glideInit) start = v.glidePitch;                // carry on from where this voice is
            else if (lastNote[(size_t) gsec] >= 0) start = (float) lastNote[(size_t) gsec];
            // A continuous slide never clicks, so no steal fade is needed.
            v.swapping = false; v.fadeGain = 1.0f; v.pitchNote = note;
            v.glidePitch = start; v.glideTarget = (float) note; v.glideInit = true;
            v.glideCoef = 1.0f - std::exp (-1.0f / (std::max (0.001f, gtime * 0.25f) * sr));   // ~98 % of the way in GLIDE TIME
            v.gliss = gsec == 0 ? p.synGliss : p.brGliss;
        }
        // Otherwise: a voice still sounding another pitch fades out first (no pop), then takes the note.
        else if (v.isActive() && v.pitchNote >= 0 && v.pitchNote != note) { v.swapping = true; }
        else if (! v.swapping)
        {
            v.pitchNote = note; v.fadeGain = 1.0f;
            v.glidePitch = v.glideTarget = (float) note; v.glideCoef = 1.0f; v.gliss = false; v.glideInit = true;
        }
        if (mask & kSyn) lastNote[0] = note;
        if (mask & kBr)  lastNote[1] = note;
        v.note = note; v.keyDown = true;
        v.velocity = 1.0f - p.velSens * (1.0f - vel);

        // KBD BALANCE filter tracks this note right away (also refreshed every block).
        const float noteHz = midiToHz ((float) note);
        const float hz = std::exp2 (std::log2 (18000.0f) + p.kbdBalance * (std::log2 (noteHz * 3.0f) - std::log2 (18000.0f)));
        v.balLp1.set (hz, sr); v.balLp2.set (hz, sr);

        if (mask & kSyn)
        {
            v.damped = false;
            v.synGate = true;
            v.fenv.gateOn(); v.aenv.gateOn();
        }
        if ((mask & kBr) && ! v.brGate)
        {
            v.brCut = false;
            // TRIGGER: neither switch = single trigger (first note only); MULTIPLE TRIGGER =
            // every note; TRIGGER SELECT = brass only sounds once SILENCE NOTE keys are held.
            v.brGate = true;
            v.brGateEnv.gate = true;
            v.brEnv.gateOn();                     // per-note EG (used in POLY voicing)
            ++brassHeld;
            const int threshold = brassThreshold();
            if (! suppressBrassTrigger && (brassHeld == threshold || (brassHeld > threshold && p.multiTrig))) brassEnv.gateOn();
        }
        if ((mask & kSt) && ! v.stGate)
        {
            v.stGate = true;
            v.stEnv.gate = true;
            v.bowEnv.value = 1.0f;                       // bowing transient fires on each new string note
            if (stringsHeld++ == 0) stVibEnv = 0.0f;
        }
    }

    void voiceOffRaw (int note, int mask)
    {
        for (auto& v : voices)
            if (v.keyDown && v.note == note) releaseSections (v, mask, false);
    }

    void releaseSections (Voice& v, int mask, bool stolen)
    {
        if ((mask & kSyn) && v.synGate) { v.synGate = false; v.fenv.gateOff(); v.aenv.gateOff(); }
        if ((mask & kBr) && v.brGate)
        {
            v.brGate = false;
            v.brGateEnv.gate = false;
            v.brEnv.gateOff();
            const bool wasOn = brassHeld >= brassThreshold();
            brassHeld = std::max (0, brassHeld - 1);
            if (wasOn && brassHeld < brassThreshold() && ! stolen) brassEnv.gateOff();
        }
        if ((mask & kSt) && v.stGate) { v.stGate = false; v.stEnv.gate = false; stringsHeld = std::max (0, stringsHeld - 1); }
        if (! v.synGate && ! v.brGate && ! v.stGate) v.keyDown = false;
    }

    void updateCoefficients()
    {
        // DAMPER jack (S-1): switch UP = programmed release ignored (release 0), DOWN = programmed release.
        const bool damperUp = p.pedalMode == 1 && ! pedal;
        for (auto& v : voices)
        {
            const bool cut = damperUp || v.damped;
            v.fenv.set (p.sfA, p.sfD, p.sfS, cut ? 0.006f : p.sfR);
            v.aenv.set (p.saA, p.saD, p.saS, cut ? 0.006f : p.saR);
            if (v.note >= 0)
            {
                // KBD BALANCE: low notes mellower, high notes brighter.
                const float noteHz = midiToHz ((float) v.note);
                const float hz = std::exp2 (std::log2 (18000.0f) + p.kbdBalance * (std::log2 (noteHz * 3.0f) - std::log2 (18000.0f)));
                v.balLp1.set (hz, sr); v.balLp2.set (hz, sr);
            }
            if (v.brCut && ! v.brGateEnv.isActive()) v.brCut = false;
            v.brGateEnv.set (0.008f, v.brCut ? 0.010f : std::max (0.01f, p.brR), sr);   // gate stays open for the brass EG release
            v.stEnv.set (std::max (p.stAttack, 0.004f), std::max (p.stRelease, 0.006f), sr);   // no snapping
        }
        brassEnv.set (p.brA, p.brD, p.brS, p.brR);
        for (auto& v : voices) v.brEnv.set (p.brA, p.brD, p.brS, p.brR);
        bowDecay = std::exp (-1.0f / (0.12f * sr));
        bowBp.bandPass (400.0f * std::exp2 (p.bowTone * 3.0f), 1.8f, sr);
        eqLoL.lowShelf (180.0f, p.eqLow * 2.0f, sr);   eqLoR.lowShelf (180.0f, p.eqLow * 2.0f, sr);
        eqHiL.highShelf (3500.0f, p.eqHigh * 2.0f, sr); eqHiR.highShelf (3500.0f, p.eqHigh * 2.0f, sr);
    }

    static constexpr float kDelayVibTime = 0.6f;   // fixed on the hardware
    static constexpr float kSynTrim = 0.35f, kStTrim = 1.25f;   // gain staging: sections sit level with brass, with headroom
    // Brass filter staging: feed the (shared, paraphonic) filter gently, make the level up afterwards.
    static constexpr float kBrassIn = 0.20f, kBrassMakeup = 2.4f;
    float sr = 44100.0f;
    std::array<Voice, kVoices> voices;
    uint32_t ageCounter = 0;
    int nextVoice = 0;

    Ladder brassVcf, stringsVcfL, stringsVcfR;
    OnePoleLP stSmoothL1, stSmoothL2, stSmoothR1, stSmoothR2;
    Adsr brassEnv;
    int brassHeld = 0, stringsHeld = 0;
    std::array<int, 128> monoStack {};
    std::array<float, 128> monoVel {};
    int monoCount = 0, monoCurrent = -1;
    bool suppressBrassTrigger = false, lastBrMono = false, lastSynMono = false;
    std::array<int, 128> sStack {};
    std::array<float, 128> sVel {};
    int sCount = 0, sCurrent = -1, sVoice = -1, sVoiceNote = -1, bVoice = -1, bVoiceNote = -1;

    DelayLine stVib;
    double stVibPhase = 0.0;
    float stVibEnv = 1.0f, bowDecay = 0.999f;
    Biquad bowBp, eqLoL, eqLoR, eqHiL, eqHiR;
    Ensemble ensemble;
    std::array<Flanger, 3> flangers;          // one per part (same settings), for separate outputs
    std::array<StereoChorus, 3> choruses;
    std::array<TapeFx, 3> tapes;

    std::array<bool, 128> held {}, sustained {};
    std::array<uint8_t, 128> keyMask {}, arpMask {};   // per held key: sections played directly / via arp
    std::array<std::array<int16_t, 128>, 3> sounding = makeSounding();   // per section: input note -> sounding note
    std::array<Arp, 3> arps;
    double hostBpm = 120.0, hostPpq = 0.0;
    std::array<double, 3> gateFreeQn {};
    std::array<float, 3> chSend {}, flSend {}, tpSend {};   // crossfaded effect sends per section
    float vibMix = 1.0f, ensMix = 1.0f;          // crossfaded strings vibrato / ensemble
    std::array<int, 2> lastNote { -1, -1 };   // last note started in Synth / Brass (glide source)
    std::array<float, 3> gateL { 1.0f, 1.0f, 1.0f }, gateR { 1.0f, 1.0f, 1.0f };
public:
    std::array<std::atomic<int>, 3> gateStepUi { -1, -1, -1 };   // current step per section, for the editor
private:
    bool hostPlaying = false;
    int heldCount = 0;
    bool pedal = false;

    double mgPhase = 0.0, pwmPhase = 0.0;
    float delayVibEnv = 1.0f, bend = 0.0f, joyVib = 0.0f, joyTrill = 0.0f;
    Rng rng { 0xBADA55u };
    DcBlocker dcL, dcR;
    std::array<DcBlocker, 3> dcPL {}, dcPR {};
};

} // namespace tdx
