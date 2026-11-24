#pragma once
// Core analog-modelling building blocks for Tridex 80.
// Header-only and JUCE-free so the engine can be unit/render tested headless.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace tdx
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float midiToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

// Fast-ish tanh approximation (Pade), good enough for saturation stages.
inline float softClip (float x)
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Polynomial band-limited step residual.
inline float polyBlep (double t, double dt)
{
    if (t < dt)       { t /= dt;               return float (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt;    return float (t * t + t + t + 1.0); }
    return 0.0f;
}

struct Rng
{
    uint32_t state = 0x1234567u;
    explicit Rng (uint32_t seed = 0x1234567u) : state (seed) {}
    float bipolar() { state = state * 1664525u + 1013904223u; return float (state >> 8) * (2.0f / 16777216.0f) - 1.0f; }
};

//==============================================================================
// RC-style ADSR: exponential segments like a capacitor charging through a
// resistor. Attack aims past 1.0 (overshoot target) and stops at 1.0, which
// gives the characteristic analog "convex" attack.
struct Adsr
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) { sr = (float) sampleRate; }

    void set (float a, float d, float s, float r)
    {
        a = std::max (a, 0.0015f);   // shortest attack/release stay click-free
        r = std::max (r, 0.004f);
        constexpr float attackTarget = 1.3f;
        aCoef = coef (a / std::log (attackTarget / (attackTarget - 1.0f)));
        dCoef = coef (d / 4.6f);   // ~-40 dB in d seconds
        rCoef = coef (r / 4.6f);
        sus = s;
    }

    void gateOn()  { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    void reset()   { stage = Idle; value = 0.0f; }
    bool isActive() const { return stage != Idle; }
    bool isReleasing() const { return stage == Release; }

    float process()
    {
        switch (stage)
        {
            case Attack:
                value += (1.3f - value) * aCoef;
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value += (sus - value) * dCoef;
                if (std::abs (value - sus) < 1.0e-4f) { value = sus; stage = Sustain; }
                break;
            case Sustain:
                value += (sus - value) * dCoef; // follows live sustain changes
                break;
            case Release:
                value += (0.0f - value) * rCoef;
                if (value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
            case Idle: default: break;
        }
        return value;
    }

    float value = 0.0f;
    Stage stage = Idle;

private:
    float coef (float tau) const { return 1.0f - std::exp (-1.0f / (std::max (tau, 1.0e-4f) * sr)); }
    float sr = 44100.0f, aCoef = 1.0f, dCoef = 1.0f, rCoef = 1.0f, sus = 1.0f;
};

//==============================================================================
// Smooth 4-pole (24 dB/oct) low-pass, zero-delay-feedback ladder.
//  - 2x oversampled internally (no aliasing from the saturation).
//  - Gentle saturation on every stage, like a transistor/OTA ladder rounding off,
//    instead of one hard clip at the input.
//  - Partial passband compensation and a softened resonance curve.
// Resonance 0..1; self-oscillates gently near 1.
struct Ladder
{
    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset() { s.fill (0.0f); xPrev = 0.0f; }

    void setCutoff (float hz, float res)
    {
        // Normal: coefficients at the 2x rate. Eco: at the host rate (cutoff kept below ~0.42 x sr).
        hz = std::clamp (hz, 10.0f, eco ? sr * 0.42f : sr * 0.45f);
        g = std::tan (kPi * hz / (eco ? sr : 2.0f * sr));
        G = g / (1.0f + g);
        res = std::clamp (res, 0.0f, 1.0f);
        k = 3.95f * (res * (1.25f - 0.25f * res));  // eases in, so the top of the knob is less shrill
    }

    float process (float x)
    {
        if (eco) { xPrev = x; return step (x); }   // Eco quality: one step per sample
        // 2x oversampling: linear-interpolated input, averaged (decimated) output.
        const float a = step (0.5f * (x + xPrev));
        const float b = step (x);
        xPrev = x;
        return 0.5f * (a + b);
    }

    float compensation = 0.6f;   // 0 = classic bass loss with resonance, 1 = flat passband
    float drive = 0.8f;          // saturation amount per stage
    bool eco = false;            // Tridex II QUALITY: false = Normal (2x oversampled), true = Eco (1x, lower CPU)

private:
    float sat (float v) const { return softClip (v * drive) / drive; }

    float step (float x)
    {
        // Linear ZDF estimate of the feedback, then saturating stages.
        const float inv = 1.0f / (1.0f + g);
        const float S = (G * G * G * s[0] + G * G * s[1] + G * s[2] + s[3]) * inv;
        const float G4 = G * G * G * G;
        const float y4est = (G4 * x + S) / (1.0f + k * G4);

        float u = sat (x * (1.0f + compensation * k) - k * y4est);
        for (auto& st : s)
        {
            const float v = (sat (u) - st) * G;
            const float y = v + st;
            st = y + v;
            u = y;
        }
        return u;
    }

    std::array<float, 4> s {};
    float sr = 44100.0f, g = 0.5f, G = 0.33f, k = 0.0f, xPrev = 0.0f;
};

//==============================================================================
struct OnePoleLP
{
    float z = 0.0f, a = 1.0f;
    void set (float hz, float sr) { a = 1.0f - std::exp (-kTwoPi * hz / sr); }
    float process (float x) { z += (x - z) * a; return z; }
};

struct DcBlocker
{
    float x1 = 0.0f, y1 = 0.0f;
    float process (float x) { const float y = x - x1 + 0.995f * y1; x1 = x; y1 = y; return y; }
};

//==============================================================================
// Three-phase BBD-style string ensemble: three modulated delay taps 120 degrees
// apart, driven by a slow + fast LFO pair (the classic string-machine chorus).
struct Ensemble
{
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        buf.assign ((size_t) (sr * 0.05f) + 4, 0.0f);
        w = 0;
        for (auto& f : bbdLp) f.set (7500.0f, sr);
    }

    void process (float in, float depth, float& outL, float& outR)
    {
        const int n = (int) buf.size();
        buf[(size_t) w] = in;

        slow += 0.63f / sr; if (slow >= 1.0f) slow -= 1.0f;
        fast += 5.8f / sr;  if (fast >= 1.0f) fast -= 1.0f;

        float tap[3];
        for (int i = 0; i < 3; ++i)
        {
            const float off = (float) i / 3.0f;
            const float mod = 0.5f * std::sin (kTwoPi * (slow + off)) * 3.2f
                            + 0.5f * std::sin (kTwoPi * (fast + off)) * 0.45f;
            const float delayMs = 7.5f + depth * mod;
            float d = delayMs * 0.001f * sr;
            float rp = (float) w - d;
            while (rp < 0.0f) rp += (float) n;
            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % n;
            const float fr = rp - (float) i0;
            tap[i] = bbdLp[i].process (buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]));
        }

        w = (w + 1) % n;
        const float dry = 1.0f - depth;
        outL = dry * in + depth * (tap[0] + 0.6f * tap[1]) * 0.75f;
        outR = dry * in + depth * (tap[2] + 0.6f * tap[1]) * 0.75f;
    }

private:
    std::vector<float> buf;
    int w = 0;
    float sr = 44100.0f, slow = 0.0f, fast = 0.0f;
    OnePoleLP bbdLp[3];
};


//==============================================================================
// Simple attack/release envelope (models the multiplexed "MPX EG" per-voice
// gates used for the Brass and Strings sections).
struct Ar
{
    float value = 0.0f;
    bool gate = false;

    void set (float attackSec, float releaseSec, float sr)
    {
        a = 1.0f - std::exp (-1.0f / (std::max (attackSec, 0.0005f) / 4.6f * sr));
        r = 1.0f - std::exp (-1.0f / (std::max (releaseSec, 0.0005f) / 4.6f * sr));
    }
    float process()
    {
        value += gate ? (1.0f - value) * a : -value * r;
        if (! gate && value < 1.0e-5f) value = 0.0f;
        return value;
    }
    bool isActive() const { return gate || value > 0.0f; }
    void reset() { value = 0.0f; gate = false; }

private:
    float a = 0.01f, r = 0.001f;
};

//==============================================================================
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    // RBJ cookbook shelves, gain in dB.
    void lowShelf (float f, float dB, float sr)  { shelf (f, dB, sr, false); }
    void highShelf (float f, float dB, float sr) { shelf (f, dB, sr, true); }

    void bandPass (float f, float q, float sr)
    {
        const float w = kTwoPi * f / sr, alpha = std::sin (w) / (2.0f * q), c = std::cos (w), a0 = 1.0f + alpha;
        b0 = alpha / a0; b1 = 0.0f; b2 = -alpha / a0; a1 = -2.0f * c / a0; a2 = (1.0f - alpha) / a0;
    }

private:
    void shelf (float f, float dB, float sr, bool high)
    {
        const float A = std::pow (10.0f, dB / 40.0f), w = kTwoPi * f / sr, c = std::cos (w);
        const float alpha = std::sin (w) * 0.5f * std::sqrt (2.0f), sA = 2.0f * std::sqrt (A) * alpha;
        float a0;
        if (high)
        {
            a0 = (A + 1) - (A - 1) * c + sA;
            b0 = A * ((A + 1) + (A - 1) * c + sA) / a0;  b1 = -2 * A * ((A - 1) + (A + 1) * c) / a0;
            b2 = A * ((A + 1) + (A - 1) * c - sA) / a0;  a1 = 2 * ((A - 1) - (A + 1) * c) / a0;
            a2 = ((A + 1) - (A - 1) * c - sA) / a0;
        }
        else
        {
            a0 = (A + 1) + (A - 1) * c + sA;
            b0 = A * ((A + 1) - (A - 1) * c + sA) / a0;  b1 = 2 * A * ((A - 1) - (A + 1) * c) / a0;
            b2 = A * ((A + 1) - (A - 1) * c - sA) / a0;  a1 = -2 * ((A - 1) + (A + 1) * c) / a0;
            a2 = ((A + 1) + (A - 1) * c - sA) / a0;
        }
    }
};

//==============================================================================
// Fractional delay line (linear interpolation), used for BBD-style vibrato
// and the flanger.
struct DelayLine
{
    void prepare (float maxSeconds, float sampleRate)
    {
        buf.assign ((size_t) (maxSeconds * sampleRate) + 4, 0.0f);
        w = 0;
    }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) % (int) buf.size(); }
    float read (float delaySamples) const
    {
        const int n = (int) buf.size();
        float rp = (float) (w - 1) - std::clamp (delaySamples, 0.0f, (float) n - 3.0f);
        while (rp < 0.0f) rp += (float) n;
        const int i0 = (int) rp, i1 = (i0 + 1) % n;
        const float fr = rp - (float) i0;
        return buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]);
    }

private:
    std::vector<float> buf { 0.0f, 0.0f, 0.0f, 0.0f };
    int w = 0;
};

//==============================================================================
// Stereo BBD flanger: Manual (base delay), Intensity (sweep), Feedback, Speed.
struct Flanger
{
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        dl[0].prepare (0.03f, sr); dl[1].prepare (0.03f, sr);
        fb[0] = fb[1] = 0.0f;
        for (auto& f : lp) f.set (9000.0f, sr);
    }

    void process (float& L, float& R, float manual, float intensity, float feedback, float speedHz)
    {
        phase += speedHz / sr; if (phase >= 1.0f) phase -= 1.0f;
        const float tri = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
        const float baseMs = 0.3f + manual * 9.0f;
        float io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            // Right channel sweeps in quadrature for width.
            const float t = c == 0 ? tri : -tri;
            const float ms = std::max (0.1f, baseMs * (1.0f + intensity * 0.9f * t));
            dl[c].push (softClip (io[c] + fb[c] * feedback * 0.97f));   // can self-oscillate, as the manual warns
            const float wet = lp[c].process (dl[c].read (ms * 0.001f * sr));
            fb[c] = wet;
            io[c] = 0.5f * (io[c] + wet);
        }
        L = io[0]; R = io[1];
    }

private:
    float sr = 44100.0f, phase = 0.0f, fb[2] {};
    DelayLine dl[2];
    OnePoleLP lp[2];
};

//==============================================================================
// Stereo chorus / ensemble (Tridex II). Modes:
//   0 CHORUS I  - two BBD-style taps, triangle LFO in anti-phase (gentle, wide)
//   1 CHORUS II - same circuit, faster and deeper
//   2 ENSEMBLE  - three taps per side, 120 degrees apart, slow + fast LFO (lush)
//   3 SPACE     - dimension-style: one slow LFO sweeps two delays in opposite directions on a
//                 mono sum, cross-mixed into both sides; wide, smooth, nearly no pitch wobble.
//                 SPACE 1..4 select increasing depth / rate.
// WIDTH sets the stereo spread, TONE the darkness of the delayed signal (BBD-style low-pass).
struct StereoChorus
{
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        dl[0].prepare (0.05f, sr); dl[1].prepare (0.05f, sr);
        lastTone = -1.0f;
        phase = slow = fast = 0.0f;
    }

    void process (float& L, float& R, int mode, float rateHz, float depth, float mix,
                  int space, float width, float tone)
    {
        if (tone != lastTone)
        {
            lastTone = tone;
            const float hz = 2500.0f * std::exp2 (tone * 2.4f);   // ~2.5 kHz (dark) .. ~13 kHz (clear)
            for (auto& f : lp) f.set (hz, sr);
        }
        float wl, wr;
        if (mode == 3)
        {
            // Dimension-style
            static constexpr float kSpaceRate[]  = { 0.25f, 0.25f, 0.30f, 0.50f };
            static constexpr float kSpaceDepth[] = { 0.55f, 0.95f, 1.40f, 2.00f };   // ms
            const int sp = std::clamp (space, 0, 3);
            phase += kSpaceRate[sp] * (rateHz / 0.6f) / sr; if (phase >= 1.0f) phase -= 1.0f;
            const float tri = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
            const float m = 0.5f * (L + R);
            dl[0].push (m); dl[1].push (m);
            const float d = kSpaceDepth[sp] * (0.4f + 1.2f * depth);
            const float a = lp[0].process (dl[0].read ((3.5f + d * tri) * 0.001f * sr));
            const float b = lp[1].process (dl[1].read ((3.5f - d * tri) * 0.001f * sr));
            const float x = 0.8f * width;                          // cross-feed (inverted) = spatial spread
            wl = a - x * b;
            wr = b - x * a;
        }
        else
        {
            dl[0].push (L); dl[1].push (R);
            if (mode == 2)
            {
                slow += rateHz * 0.6f / sr; if (slow >= 1.0f) slow -= 1.0f;
                fast += (5.5f + rateHz) / sr; if (fast >= 1.0f) fast -= 1.0f;
                const auto tap = [&] (int ch, float off)
                {
                    const float mm = 3.0f * std::sin (kTwoPi * (slow + off)) + 0.45f * std::sin (kTwoPi * (fast + off));
                    return dl[ch].read ((7.0f + depth * mm) * 0.001f * sr);
                };
                wl = (tap (0, 0.0f) + tap (0, 1.0f / 3.0f) + tap (0, 2.0f / 3.0f)) * 0.45f;
                wr = (tap (1, 1.0f / 6.0f) + tap (1, 0.5f) + tap (1, 5.0f / 6.0f)) * 0.45f;
            }
            else
            {
                const float r = mode == 1 ? rateHz * 1.6f : rateHz;
                const float d = (mode == 1 ? 2.8f : 1.8f) * depth;
                phase += r / sr; if (phase >= 1.0f) phase -= 1.0f;
                const float tri = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
                wl = dl[0].read ((5.0f + d * tri) * 0.001f * sr);
                wr = dl[1].read ((5.0f - d * tri) * 0.001f * sr);
            }
            wl = lp[0].process (wl); wr = lp[1].process (wr);
            // WIDTH: blend the two wet sides towards mono (0) or keep them apart (1)
            const float w = 0.5f + 0.5f * width, o = 1.0f - w;
            const float nl = wl * w + wr * o, nr = wr * w + wl * o;
            wl = nl; wr = nr;
        }
        const float dry = 1.0f - 0.5f * mix, wet = 0.75f * mix;
        L = L * dry + wl * wet;
        R = R * dry + wr * wet;
    }

private:
    float sr = 44100.0f, phase = 0.0f, slow = 0.0f, fast = 0.0f, lastTone = -1.0f;
    DelayLine dl[2];
    OnePoleLP lp[2];
};

//==============================================================================
// Tape / VHS-style processor (Tridex II): wow + flutter + random drift on a modulated delay,
// asymmetric tape saturation, age (darkening), hiss and (worn tape) dropouts.
// TYPE sets the machine's character; the knobs work on top of it:
//   0 STUDIO    clean reel: little wobble, high headroom, bright, quiet
//   1 CASSETTE  more flutter, softer top, a little hiss
//   2 VHS       strong wow, darker, noisier
//   3 WORN VHS  very wavy and unstable, dark, noisy, with level dropouts
// Instances built alike produce identical modulation, so per-part instances stay in step.
struct TapeFx
{
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        dl[0].prepare (0.08f, sr); dl[1].prepare (0.08f, sr);
        wowPh = flPh = 0.0f; drift = driftTarget = driftSlew = 0.0f; driftCount = 0;
        drop = 1.0f; dropTarget = 1.0f; dropCount = 0;
        rng = Rng (0x7A9E5u); noiseRng = Rng (0x51C3u); dropRng = Rng (0x2D11u);
        lastAge = -1.0f; lastType = -1;
        for (auto& f : hissLp) f.set (7000.0f, sr);
    }

    // drive, wow, flutter, driftAmt, age, hiss, mix: 0..1; speed: 0.25..2 (x the type's base rates).
    void process (float& L, float& R, int type, float drive, float wow, float flutter, float speed,
                  float driftAmt, float age, float hiss, float mix)
    {
        static constexpr float kWowMul[]   = { 0.45f, 1.0f, 1.6f, 2.3f };
        static constexpr float kFlMul[]    = { 0.35f, 1.0f, 1.25f, 1.7f };
        static constexpr float kAgeBase[]  = { 0.0f, 0.22f, 0.42f, 0.58f };
        static constexpr float kSatMul[]   = { 0.7f, 1.0f, 1.15f, 1.4f };
        static constexpr float kHissMul[]  = { 0.3f, 1.0f, 1.6f, 2.2f };
        static constexpr float kWowRate[]  = { 0.35f, 0.7f, 0.5f, 0.42f };   // Hz at SPEED = 1
        static constexpr float kFlRate[]   = { 9.0f, 7.3f, 6.1f, 5.4f };
        static constexpr float kDropout[]  = { 0.0f, 0.0f, 0.15f, 0.55f };
        const int t = std::clamp (type, 0, 3);

        const float ageAll = std::clamp (kAgeBase[t] + age * (1.0f - kAgeBase[t]), 0.0f, 1.0f);
        if (ageAll != lastAge || t != lastType)
        {
            lastAge = ageAll; lastType = t;
            const float hz = 18000.0f * std::exp2 (-ageAll * 2.9f);      // 18 kHz (fresh) .. ~2.4 kHz (worn)
            for (auto& f : ageLp) f.set (hz, sr);
        }

        // Modulation: wow (slow sine) + flutter (fast, two partials) + drift (smoothed random walk)
        wowPh += kWowRate[t] * speed / sr; if (wowPh >= 1.0f) wowPh -= 1.0f;
        flPh  += kFlRate[t] * speed / sr;  if (flPh >= 1.0f) flPh -= 1.0f;
        if (--driftCount <= 0) { driftTarget = rng.bipolar(); driftCount = (int) (sr * 0.12f / std::max (0.25f, speed)); }
        driftSlew += (driftTarget - driftSlew) * (speed / (0.25f * sr));
        drift += (driftSlew - drift) * (speed / (0.35f * sr));

        // Dropouts (worn tape): occasional short level dips
        if (--dropCount <= 0)
        {
            dropCount = (int) (sr * (0.08f + 0.25f * (0.5f + 0.5f * dropRng.bipolar())));
            dropTarget = (dropRng.bipolar() > 0.55f) ? 1.0f - kDropout[t] * (0.5f + 0.5f * dropRng.bipolar()) : 1.0f;
        }
        drop += (dropTarget - drop) * (1.0f / (0.012f * sr));

        const float w = wow * kWowMul[t], f = flutter * kFlMul[t];
        const float wowSig = std::sin (kTwoPi * wowPh);
        const float flSig = 0.7f * std::sin (kTwoPi * flPh) + 0.3f * std::sin (kTwoPi * flPh * 2.37f);
        // Depths (ms): up to ~9 ms x type of wow and drift ("really wavy"), up to 0.6 ms x type of flutter.
        const float mod = w * 9.0f * wowSig * (1.0f - 0.5f * driftAmt)
                        + driftAmt * 9.0f * drift * (0.4f + w)
                        + f * 0.6f * flSig;
        const float delayMs = 24.0f + mod;

        const float g = 1.0f + drive * kSatMul[t] * 7.0f;
        const float bias = 0.12f * drive;
        float io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            dl[c].push (io[c]);
            float y = dl[c].read (std::max (0.5f, delayMs + (c == 1 ? 0.04f * mod : 0.0f)) * 0.001f * sr);
            y = (softClip (y * g + bias) - softClip (bias)) / std::pow (g, 0.65f);   // asymmetric, level-compensated
            y = ageLp[c * 2 + 1].process (ageLp[c * 2].process (y)) * drop;
            y += hissLp[c].process (noiseRng.bipolar()) * hiss * kHissMul[t] * 0.02f;
            io[c] = io[c] + (y - io[c]) * mix;
        }
        L = io[0]; R = io[1];
    }

private:
    float sr = 44100.0f, wowPh = 0.0f, flPh = 0.0f, drift = 0.0f, driftTarget = 0.0f, driftSlew = 0.0f, lastAge = -1.0f;
    float drop = 1.0f, dropTarget = 1.0f;
    int driftCount = 0, dropCount = 0, lastType = -1;
    Rng rng { 0x7A9E5u }, noiseRng { 0x51C3u }, dropRng { 0x2D11u };
    DelayLine dl[2];
    OnePoleLP ageLp[4], hissLp[2];
};

} // namespace tdx
