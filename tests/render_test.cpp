// Renders a short demo of each section to WAV files and checks for NaN/clipping.
#include "dsp/Engine.h"
#include <cstdio>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), data = n * 4, riff = 36 + data, fmt = 16, rate = (uint32_t) sr, byteRate = rate * 4;
    const uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmt, 4, 1, f); std::fwrite (&pcm, 2, 1, f); std::fwrite (&ch, 2, 1, f); std::fwrite (&rate, 4, 1, f);
    std::fwrite (&byteRate, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&data, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i) { int16_t a = (int16_t) (L[i] * 32000), b = (int16_t) (R[i] * 32000); std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f); }
    std::fclose (f);
}

static int render (const char* name, tdx::Params p, const char* outDir)
{
    const int sr = 48000, block = 256, total = sr * 4;
    tdx::Engine e; e.p = p; e.prepare (sr);
    std::vector<float> L ((size_t) total), R ((size_t) total);
    const int chord[] = { 36, 43, 48, 55, 60, 64, 67, 71, 74, 79 };
    float peak = 0; bool bad = false;
    for (int pos = 0; pos < total; pos += block)
    {
        if (pos == 0) for (int n : chord) e.noteOn (n, 0.8f);
        if (pos == sr * 2) for (int n : chord) e.noteOff (n);
        e.render (&L[(size_t) pos], &R[(size_t) pos], std::min (block, total - pos));
    }
    for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true; peak = std::max ({ peak, std::abs (L[i]), std::abs (R[i]) }); }
    writeWav (std::string (outDir) + "/" + name + ".wav", L, R, sr);
    std::printf ("%-10s peak=%.3f %s\n", name, peak, bad ? "NaN!" : "ok");
    return bad || peak < 0.01f ? 1 : 0;
}

int main (int argc, char** argv)
{
    const char* out = argc > 1 ? argv[1] : ".";
    int fails = 0;
    tdx::Params synth; fails += render ("polysynth", synth, out);
    tdx::Params pwm;   pwm.v1Wave = tdx::Vco1Wave::PWM; pwm.pwInt = 0.8f; pwm.v2Scale = 3; pwm.v2Wave = tdx::Vco1Wave::PW; fails += render ("pwm", pwm, out);
    tdx::Params brass; brass.synOn = false; brass.brOn = true; brass.br16 = true; brass.br1Wave = 2; brass.br2Scale = 2; brass.br2Wave = 1; brass.br2Detune = 12.0f; fails += render ("brass", brass, out);
    tdx::Params str;   str.synOn = false; str.stOn = true; str.kbdBalance = 0.7f; str.bowOn = true; fails += render ("strings", str, out);
    tdx::Params all;   all.brOn = all.stOn = true; all.res = 0.95f; all.master = 1.0f;
    all.flSynth = all.flStrings = true; all.flFeedback = 0.9f; all.autoDamp = true; all.assignMode = 1; all.trigSelect = true; all.silenceNote = 1; fails += render ("all_hot", all, out);
    // Arpeggiators: synth arp free-running, strings arp synced to a (stopped) host at 100 BPM.
    tdx::Params arp; arp.stOn = true; arp.arp[0].on = true; arp.arp[0].mode = 2; arp.arp[0].octaves = 2;
    arp.arp[2].on = true; arp.arp[2].rate = 1; arp.arp[2].mode = 3; fails += render ("arps", arp, out);
    tdx::Params mono; mono.synOn = false; mono.brOn = true; mono.brMono = true; mono.multiTrig = false;
    fails += render ("brass_mono", mono, out);
    std::printf (fails ? "FAILED\n" : "PASSED\n");
    return fails;
}
