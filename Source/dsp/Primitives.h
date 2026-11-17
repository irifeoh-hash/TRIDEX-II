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
enum class Wave { Saw = 0, Square, Pulse };

// PolyBLEP VCO. Phase is kept in [0,1).
struct Vco
{
    double phase = 0.0;

    float process (double inc, Wave w, float pw)
    {
        inc = std::min (inc, 0.49);
        float out;
        if (w == Wave::Saw)
        {
            out = float (2.0 * phase - 1.0) - polyBlep (phase, inc);
        }
        else
        {
            const double p = (w == Wave::Square) ? 0.5 : std::clamp ((double) pw, 0.05, 0.95);
            out = phase < p ? 1.0f : -1.0f;
            out += polyBlep (phase, inc);
            double t2 = phase - p;
            if (t2 < 0.0) t2 += 1.0;
            out -= polyBlep (t2, inc);
        }
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        return out;
    }
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
// Zero-delay-feedback 4-pole (24 dB/oct) OTA-style low-pass with saturating
// feedback path. Resonance 0..1, self-oscillates near 1.
struct Ladder
{
    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset() { s.fill (0.0f); }

    void setCutoff (float hz, float res)
    {
        hz = std::clamp (hz, 10.0f, sr * 0.45f);
        g = std::tan (kPi * hz / sr);
        G = g / (1.0f + g);
        k = 4.2f * std::clamp (res, 0.0f, 1.0f);
    }

    float process (float x)
    {
        // Solve the feedback loop linearly, then saturate the input stage.
        const float inv = 1.0f / (1.0f + g);
        const float S = G * G * G * s[0] * inv + G * G * s[1] * inv + G * s[2] * inv + s[3] * inv;
        const float G4 = G * G * G * G;
        const float y4est = (G4 * x + S) / (1.0f + k * G4);

        // Passband gain compensation so resonance doesn't hollow out the bass.
        float u = softClip ((x * (1.0f + compensation * k)) - k * y4est);

        for (auto& st : s)
        {
            const float v = (u - st) * G;
            const float y = v + st;
            st = y + v;
            u = y;
        }
        return u;
    }

    float compensation = 0.5f;

private:
    std::array<float, 4> s {};
    float sr = 44100.0f, g = 0.5f, G = 0.33f, k = 0.0f;
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

} // namespace tdx
