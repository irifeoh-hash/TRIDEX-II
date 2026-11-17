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
    float mgSpeed = 5.5f, mgIntensity = 0.5f;           // joystick vibrato/trill
    bool delayVib = false; float delayVibInt = 0.3f;     // delay time is fixed on the hardware
    float bendRange = 2.0f;
    bool flSynth = false, flBrass = false, flStrings = false;
    float flManual = 0.4f, flIntensity = 0.5f, flFeedback = 0.4f, flSpeed = 0.3f;
    float drift = 0.3f, velSens = 0.0f;
    int pedalMode = 2;                                  // 0 = off, 1 = damper (S-1, Trident), 2 = hold (modern sustain)

    // ---- Synthesizer section
    bool synOn = true; Zone synZone = Zone::Both; float synVolume = 0.7f;
    int v1Oct = 1; Vco1Wave v1Wave = Vco1Wave::Saw; float pwInt = 0.3f, pwmSpeed = 1.0f;
    int v2Scale = 2; float v2Detune = 6.0f;            // v2Scale: 0 Off, 1 16', 2 8', 3 4'
    float cutoff = 0.6f, res = 0.15f, egInt = 0.3f; int kbdTrack = 2;
    float sfA = 0.01f, sfD = 0.6f, sfS = 0.4f, sfR = 0.4f;
    float saA = 0.005f, saD = 0.5f, saS = 0.8f, saR = 0.4f;
    float vcaAttenDb = 0.0f; bool autoDamp = false;

    // ---- Brass section
    bool brOn = false; Zone brZone = Zone::Both; float brVolume = 0.6f;
    bool br16 = false, br8 = true;
    float brCutoff = 0.4f, brRes = 0.1f, brEgInt = 0.5f;
    float brA = 0.08f, brD = 0.5f, brS = 0.7f, brR = 0.3f;
    bool multiTrig = true, trigSelect = false; int silenceNote = 0;   // silenceNote: 0..3 -> 2,4,6,8 keys

    // ---- Strings section
    bool stOn = false; Zone stZone = Zone::Both; float stVolume = 0.6f;
    bool st16 = false, st8 = true, st4 = true;
    float stAttack = 0.15f, stRelease = 0.8f, kbdBalance = 0.0f;   // kbdBalance 0..1 (key-tracked filter amount)
    float eqLow = 0.0f, eqHigh = 0.0f;                 // panel scale -5..+5
    bool bowOn = false; float bowLevel = 0.5f, bowTone = 0.5f;
    bool vibOn = true; float vibDelay = 0.5f, vibIntensity = 0.4f, vibSpeed = 5.5f;
    bool ensembleOn = true;
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

    DividerVco vco1, vco2;
    Ladder vcf;
    Adsr fenv, aenv;
    Ar brGateEnv, stEnv, bowEnv;
    OnePoleLP balLp1, balLp2;   // strings KBD BALANCE filter (12 dB, key tracked)

    bool isActive() const { return aenv.isActive() || brGateEnv.isActive() || stEnv.isActive(); }
};

//==============================================================================
class Engine
{
public:
    static constexpr int kVoices = 8;
    Params p;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        Rng r (0xC0FFEEu);
        for (auto& v : voices)
        {
            v = Voice {};
            v.vcf.prepare (sr); v.vcf.compensation = 1.0f;   // manual's response graph: flat passband as resonance rises
            v.fenv.prepare (sr); v.aenv.prepare (sr);
            v.driftCents = r.bipolar() * 3.0f;
            v.v2Offset = r.bipolar() * 2.0f;
            v.vco1.phase = 0.5 * (r.bipolar() + 1.0); v.vco2.phase = 0.5 * (r.bipolar() + 1.0);
            v.vco1.mis8 = r.bipolar() * 0.02f; v.vco1.mis16 = r.bipolar() * 0.02f;
            v.vco2.mis8 = r.bipolar() * 0.02f; v.vco2.mis16 = r.bipolar() * 0.02f;
        }
        brassVcf.prepare (sr); brassVcf.compensation = 1.0f; brassEnv.prepare (sr);
        stVib.prepare (0.03f, sr);
        ensemble.prepare (sr);
        flanger.prepare (sr);
        for (auto* d : { &dcL, &dcR }) *d = {};
        brassHeld = stringsHeld = 0; nextVoice = 0;
        held.fill (false); sustained.fill (false);
    }

    //--------------------------------------------------------------------------
    void noteOn (int note, float vel)
    {
        const bool sy = p.synOn && inZone (p.synZone, note);
        const bool br = p.brOn  && inZone (p.brZone, note);
        const bool st = p.stOn  && inZone (p.stZone, note);
        if (! (sy || br || st)) return;

        held[(size_t) note] = true; sustained[(size_t) note] = false;
        if (heldCount++ == 0) delayVibEnv = 0.0f;

        // AUTO DAMP: notes no longer held are muted whenever a new note is played.
        if (p.autoDamp)
            for (auto& o : voices) if (! o.keyDown && o.aenv.isActive()) o.damped = true;

        Voice& v = allocate (note);
        if (v.keyDown) releaseVoice (v, true);           // stolen while held: keep section counters right
        v.damped = false;

        v.note = note; v.keyDown = true;
        v.velocity = 1.0f - p.velSens * (1.0f - vel);
        v.synGate = sy; v.brGate = br; v.stGate = st;

        if (sy) { v.fenv.gateOn(); v.aenv.gateOn(); }
        if (br)
        {
            // TRIGGER: neither switch = single trigger (first note only); MULTIPLE TRIGGER =
            // every note; TRIGGER SELECT = brass only sounds once SILENCE NOTE keys are held.
            v.brGateEnv.gate = true;
            ++brassHeld;
            const int threshold = brassThreshold();
            if (brassHeld == threshold || (brassHeld > threshold && p.multiTrig)) brassEnv.gateOn();
        }
        if (st)
        {
            v.stEnv.gate = true;
            v.bowEnv.value = 1.0f;                       // bowing transient fires on each new string note
            if (stringsHeld++ == 0) stVibEnv = 0.0f;
        }
    }

    void noteOff (int note)
    {
        if (! held[(size_t) note]) return;
        held[(size_t) note] = false;
        heldCount = std::max (0, heldCount - 1);
        if (p.pedalMode == 2 && pedal) { sustained[(size_t) note] = true; return; }
        for (auto& v : voices) if (v.note == note && v.keyDown) releaseVoice (v, false);
    }

    void setPedal (bool down)
    {
        pedal = down;
        if (! down && p.pedalMode == 2)
            for (int n = 0; n < 128; ++n)
                if (sustained[(size_t) n])
                {
                    sustained[(size_t) n] = false;
                    for (auto& v : voices) if (v.note == n && v.keyDown) releaseVoice (v, false);
                }
    }

    void pitchBend (float bipolar) { bend = bipolar; }
    void joystickY (float vibrato01, float trill01) { joyVib = vibrato01; joyTrill = trill01; }

    void allNotesOff()
    {
        for (auto& v : voices) if (v.keyDown) releaseVoice (v, false);
        held.fill (false); sustained.fill (false); heldCount = 0;
    }

    //--------------------------------------------------------------------------
    void render (float* outL, float* outR, int numSamples)
    {
        updateCoefficients();
        const double invSr = 1.0 / sr;
        const float kbd = kKbdTrack[std::clamp (p.kbdTrack, 0, 4)];
        const float attenGain = std::pow (10.0f, p.vcaAttenDb / 20.0f);
        const int v1Oct = 2 - std::clamp (p.v1Oct, 0, 2);   // footage index -> divider stage (0 = 4' core)

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- MG / joystick (KLM-401) — modulates all VCOs via the anti-log amp
            mgPhase += p.mgSpeed * invSr; if (mgPhase >= 1.0) mgPhase -= 1.0;
            const float tri = float (mgPhase < 0.5 ? 4.0 * mgPhase - 1.0 : 3.0 - 4.0 * mgPhase);
            const float trillSq = mgPhase < 0.5 ? 1.0f : 0.0f;
            delayVibEnv = std::min (1.0f, delayVibEnv + float (invSr / kDelayVibTime));
            // With DELAY VIBRATO on, joystick up/down (vibrato & trill) has no effect.
            const float jv = p.delayVib ? 0.0f : joyVib, jt = p.delayVib ? 0.0f : joyTrill;
            const float vibSemis = tri * (jv * p.mgIntensity + (p.delayVib ? delayVibEnv * p.delayVibInt * 0.6f : 0.0f));
            const float trillSemis = trillSq * jt * p.mgIntensity * 2.0f;
            const float globalSemis = p.tuneCents * 0.01f + bend * p.bendRange + vibSemis + trillSemis;

            pwmPhase += p.pwmSpeed * invSr; if (pwmPhase >= 1.0) pwmPhase -= 1.0;
            const float pwmTri = float (pwmPhase < 0.5 ? 4.0 * pwmPhase - 1.0 : 3.0 - 4.0 * pwmPhase);
            // PW: 0 = square, narrowing until the pulse collapses (silence) at 10.
            // PWM: 0 = no modulation; from ~8 upward the pulse collapses for part of each cycle.
            float pw = 0.5f;
            if (p.v1Wave == Vco1Wave::PW)  pw = 0.5f * (1.0f - p.pwInt);
            if (p.v1Wave == Vco1Wave::PWM) pw = 0.5f * (1.0f - p.pwInt * 1.25f * (0.5f + 0.5f * pwmTri));
            pw = std::max (0.0f, pw);

            float synSum = 0.0f, brSum = 0.0f, stSum = 0.0f, bowSum = 0.0f;
            bool brassGateOpen = false;

            for (auto& v : voices)
            {
                if (! v.isActive()) { if (v.note >= 0 && ! v.keyDown) v.note = -1; continue; }

                v.wander += rng.bipolar() * 0.02f - v.wander * 0.0005f;
                const float semis = (float) v.note + globalSemis + p.drift * (v.driftCents + v.wander) * 0.01f;
                // 16' counter frequency = note - 12 semitones (8' = concert pitch)
                const auto o1 = v.vco1.process (midiToHz (semis - 12.0f) * invSr);

                // ---- Synth (KLM-378)
                if (v.aenv.isActive())
                {
                    float osc1;
                    const double ph = v1Oct == 0 ? o1.ph4 : (v1Oct == 1 ? o1.ph8 : o1.ph16);
                    const double dt = v1Oct == 0 ? o1.dt4 : (v1Oct == 1 ? o1.dt8 : o1.dt16);
                    if (p.v1Wave == Vco1Wave::Saw)
                        osc1 = v1Oct == 0 ? o1.saw4 : (v1Oct == 1 ? o1.saw8 : o1.saw16);
                    else if (pw < 0.004f)
                        osc1 = 0.0f;                              // pulse has collapsed: VCO shut off
                    else
                    {
                        osc1 = ph < pw ? 1.0f : -1.0f;
                        osc1 += polyBlep (ph, dt);
                        double t2 = ph - pw; if (t2 < 0.0) t2 += 1.0;
                        osc1 -= polyBlep (t2, dt);
                    }

                    float osc2 = 0.0f;
                    if (p.v2Scale > 0)
                    {
                        const float s2 = semis + (p.v2Detune + v.v2Offset * p.drift) * 0.01f - 12.0f;
                        const auto o2 = v.vco2.process (midiToHz (s2) * invSr);
                        osc2 = p.v2Scale == 1 ? o2.saw16 : (p.v2Scale == 2 ? o2.saw8 : o2.saw4);
                    }

                    const float fe = v.fenv.process();
                    const float ae = v.aenv.process();
                    const float cutOct = p.cutoff * 10.0f + p.egInt * fe * 10.0f + kbd * ((float) v.note - 60.0f) / 12.0f;
                    v.vcf.setCutoff (20.0f * std::exp2 (cutOct), p.res);
                    synSum += v.vcf.process ((osc1 + osc2) * 0.35f) * ae * attenGain * v.velocity;
                    if (! v.aenv.isActive()) v.vcf.reset();
                }

                // ---- Brass per-voice gate (KLM-403 MPX EG)
                if (v.brGateEnv.isActive())
                {
                    const float g = v.brGateEnv.process();
                    brassGateOpen = true;
                    brSum += ((p.br16 ? o1.saw16 : 0.0f) + (p.br8 ? o1.saw8 : 0.0f)) * g * v.velocity;
                }

                // ---- Strings per-voice (KLM-384): KBD balance + AR VCA
                if (v.stEnv.isActive())
                {
                    const float e = v.stEnv.process();
                    const float raw = (p.st16 ? o1.saw16 : 0.0f) + (p.st8 ? o1.saw8 : 0.0f) + (p.st4 ? o1.saw4 : 0.0f);
                    const float sig = v.balLp2.process (v.balLp1.process (raw));
                    stSum += sig * e * v.velocity;
                    if (p.bowOn) bowSum += sig * v.bowEnv.value * e;
                    v.bowEnv.value *= bowDecay;
                }
            }

            // ---- Brass: single VCF + VCA driven by one EG (paraphonic)
            float brass = 0.0f;
            if (brassEnv.isActive() || brassGateOpen)
            {
                const float e = brassEnv.process();
                brassVcf.setCutoff (20.0f * std::exp2 (p.brCutoff * 10.0f + p.brEgInt * e * 8.0f), p.brRes);
                brass = brassVcf.process (brSum * 0.3f) * e * p.brVolume;
            }

            // ---- Strings chain (KLM-404)
            stVibEnv = std::min (1.0f, stVibEnv + float (invSr / std::max (0.01f, p.vibDelay)));
            float s = stSum * 0.22f;
            if (p.vibOn)
            {
                stVibPhase += p.vibSpeed * invSr; if (stVibPhase >= 1.0) stVibPhase -= 1.0;
                const float mod = std::sin (kTwoPi * (float) stVibPhase) * p.vibIntensity * stVibEnv;
                stVib.push (s);
                s = stVib.read ((2.0f + 1.6f * mod) * 0.001f * sr);
            }
            float sL = s, sR = s;
            if (p.ensembleOn) ensemble.process (s, 1.0f, sL, sR);
            if (p.bowOn)
            {
                const float b = bowBp.process (bowSum * 0.22f) * p.bowLevel * 2.0f;
                sL += b; sR += b;
            }
            sL = eqHiL.process (eqLoL.process (sL)) * p.stVolume;
            sR = eqHiR.process (eqLoR.process (sR)) * p.stVolume;

            const float synth = synSum * p.synVolume;

            // ---- Flanger on selected sections
            float fL = 0.0f, fR = 0.0f, dL = 0.0f, dR = 0.0f;
            (p.flSynth   ? fL : dL) += synth; (p.flSynth   ? fR : dR) += synth;
            (p.flBrass   ? fL : dL) += brass; (p.flBrass   ? fR : dR) += brass;
            (p.flStrings ? fL : dL) += sL;    (p.flStrings ? fR : dR) += sR;
            if (p.flSynth || p.flBrass || p.flStrings)
                flanger.process (fL, fR, p.flManual, p.flIntensity, p.flFeedback, p.flSpeed);

            outL[i] = softClip (dcL.process (dL + fL) * p.master);
            outR[i] = softClip (dcR.process (dR + fR) * p.master);
        }
    }

private:
    bool inZone (Zone z, int note) const
    {
        if (z == Zone::Both) return true;
        return z == Zone::Lower ? note < p.splitPoint : note >= p.splitPoint;
    }

    // KLM-377 key assigner (owner's manual p.4/42):
    //  Mode 1: a different unit for each note, so every note keeps its own release.
    //  Mode 2: the same unit plays each new note; the previous note's release is cut.
    Voice& allocate (int note)
    {
        for (auto& v : voices) if (v.note == note && v.isActive()) return stamp (v);

        if (p.assignMode == 0)
        {
            for (int k = 0; k < kVoices; ++k)
            {
                auto& v = voices[(size_t) ((nextVoice + k) % kVoices)];
                if (! v.keyDown && ! v.isActive()) { nextVoice = (nextVoice + k + 1) % kVoices; return stamp (v); }
            }
        }
        else
        {
            for (auto& v : voices) if (! v.keyDown) return stamp (v);   // lowest unit not held, even if releasing
        }
        // Steal: oldest released voice, else oldest held.
        Voice* best = nullptr;
        for (auto& v : voices) if (! v.keyDown && (! best || v.age < best->age)) best = &v;
        if (! best) for (auto& v : voices) if (! best || v.age < best->age) best = &v;
        return stamp (*best);
    }

    int brassThreshold() const { return p.trigSelect ? 2 * (std::clamp (p.silenceNote, 0, 3) + 1) : 1; }

    Voice& stamp (Voice& v) { v.age = ++ageCounter; return v; }

    void releaseVoice (Voice& v, bool stolen)
    {
        v.keyDown = false;
        if (v.synGate) { v.fenv.gateOff(); v.aenv.gateOff(); }
        if (v.brGate)
        {
            v.brGateEnv.gate = false;
            const bool wasOn = brassHeld >= brassThreshold();
            brassHeld = std::max (0, brassHeld - 1);
            if (wasOn && brassHeld < brassThreshold() && ! stolen) brassEnv.gateOff();
        }
        if (v.stGate) { v.stEnv.gate = false; stringsHeld = std::max (0, stringsHeld - 1); }
        v.synGate = v.brGate = v.stGate = false;
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
            v.brGateEnv.set (0.002f, std::max (0.01f, p.brR), sr);   // gate stays open for the brass EG release
            v.stEnv.set (p.stAttack, p.stRelease, sr);
        }
        brassEnv.set (p.brA, p.brD, p.brS, p.brR);
        bowDecay = std::exp (-1.0f / (0.12f * sr));
        bowBp.bandPass (400.0f * std::exp2 (p.bowTone * 3.0f), 1.8f, sr);
        eqLoL.lowShelf (180.0f, p.eqLow * 2.0f, sr);   eqLoR.lowShelf (180.0f, p.eqLow * 2.0f, sr);
        eqHiL.highShelf (3500.0f, p.eqHigh * 2.0f, sr); eqHiR.highShelf (3500.0f, p.eqHigh * 2.0f, sr);
    }

    static constexpr float kDelayVibTime = 0.6f;   // fixed on the hardware
    float sr = 44100.0f;
    std::array<Voice, kVoices> voices;
    uint32_t ageCounter = 0;
    int nextVoice = 0;

    Ladder brassVcf;
    Adsr brassEnv;
    int brassHeld = 0, stringsHeld = 0;

    DelayLine stVib;
    double stVibPhase = 0.0;
    float stVibEnv = 1.0f, bowDecay = 0.999f;
    Biquad bowBp, eqLoL, eqLoR, eqHiL, eqHiR;
    Ensemble ensemble;
    Flanger flanger;

    std::array<bool, 128> held {}, sustained {};
    int heldCount = 0;
    bool pedal = false;

    double mgPhase = 0.0, pwmPhase = 0.0;
    float delayVibEnv = 1.0f, bend = 0.0f, joyVib = 0.0f, joyTrill = 0.0f;
    Rng rng { 0xBADA55u };
    DcBlocker dcL, dcR;
};

} // namespace tdx
