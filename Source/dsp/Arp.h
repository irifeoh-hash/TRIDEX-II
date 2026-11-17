#pragma once
// Per-section arpeggiator (Tridex II addition, not on the original hardware).
// Runs sample-accurately; either locks to the host's beat grid or free-runs
// at a tempo (the host's when sync is on but the transport is stopped, or the
// arp section's own tempo when sync is off).

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tdx
{
struct ArpParams
{
    bool on = false;
    int mode = 0;       // 0 Up, 1 Down, 2 Up/Down, 3 Random, 4 As played
    int rate = 3;       // index into kArpRates
    int octaves = 1;    // 1..4
    float gate = 0.5f;  // fraction of a step
    bool sync = true;
};

// Step lengths in quarter notes: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32
inline constexpr double kArpRates[] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };

class Arp
{
public:
    void keyOn (int note, float vel)
    {
        if (count == 0) { localQn = 0.0; lastStep = -1; index = 0; started = false; }
        for (int i = 0; i < count; ++i) if (order[(size_t) i] == note) return;
        if (count < (int) order.size()) order[(size_t) count++] = note;
        velocity = vel;
    }

    void keyOff (int note)
    {
        for (int i = 0; i < count; ++i)
            if (order[(size_t) i] == note)
            {
                for (int j = i; j < count - 1; ++j) order[(size_t) j] = order[(size_t) j + 1];
                --count;
                return;
            }
    }

    bool isIdle() const { return count == 0 && playing < 0; }

    template <typename OffFn>
    void clear (OffFn&& off)
    {
        count = 0;
        if (playing >= 0) { off (playing); playing = -1; }
    }

    // One sample. hostPos is the host's position in quarter notes (used when
    // useHostGrid is true); qnPerSample is the tempo to free-run at otherwise.
    template <typename OnFn, typename OffFn>
    void tick (const ArpParams& p, bool useHostGrid, double hostPos, double qnPerSample, OnFn&& on, OffFn&& off)
    {
        if (count == 0)
        {
            if (playing >= 0) { off (playing); playing = -1; }
            return;
        }

        const double len = kArpRates[std::clamp (p.rate, 0, 5)];
        const double pos = useHostGrid ? hostPos : localQn;
        localQn += qnPerSample;

        const long long step = (long long) std::floor (pos / len);
        if (! started)
        {
            started = true;
            // Locked to the host: wait for the next grid line. Free-running: start now.
            lastStep = useHostGrid ? step : step - 1;
        }

        if (step != lastStep)
        {
            lastStep = step;
            if (playing >= 0) { off (playing); playing = -1; }
            playing = nextNote (p);
            on (playing, velocity);
            samplesLeft = std::max (1.0, std::clamp ((double) p.gate, 0.05, 1.0) * len / std::max (qnPerSample, 1.0e-9));
        }
        else if (playing >= 0 && (samplesLeft -= 1.0) <= 0.0)
        {
            off (playing);
            playing = -1;
        }
    }

private:
    int nextNote (const ArpParams& p)
    {
        // Build the sequence over the selected octave range.
        std::array<int, 128> base {};
        for (int i = 0; i < count; ++i) base[(size_t) i] = order[(size_t) i];
        if (p.mode != 4) std::sort (base.begin(), base.begin() + count);

        const int octs = std::clamp (p.octaves, 1, 4);
        std::array<int, 512> seq {};
        int n = 0;
        for (int o = 0; o < octs; ++o)
            for (int i = 0; i < count; ++i)
                seq[(size_t) n++] = std::min (127, base[(size_t) i] + 12 * o);

        int note;
        switch (p.mode)
        {
            case 1: note = seq[(size_t) (n - 1 - (index % n))]; break;                     // Down
            case 2:                                                                          // Up/Down
            {
                const int period = std::max (1, 2 * n - 2);
                const int k = index % period;
                note = seq[(size_t) (k < n ? k : period - k)];
                break;
            }
            case 3:                                                                          // Random
                rng = rng * 1664525u + 1013904223u;
                note = seq[(size_t) ((rng >> 8) % (uint32_t) n)];
                break;
            default: note = seq[(size_t) (index % n)]; break;                              // Up / As played
        }
        ++index;
        return note;
    }

    std::array<int, 128> order {};
    int count = 0, playing = -1, index = 0;
    long long lastStep = -1;
    double localQn = 0.0, samplesLeft = 0.0;
    bool started = false;
    float velocity = 1.0f;
    uint32_t rng = 0x2545F491u;
};
} // namespace tdx
