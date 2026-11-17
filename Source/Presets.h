#pragma once
// 32 program slots (banks A-D x 1-8, as on the Mk II programmer). These are
// original patches; each lists only overrides from parameter defaults.
// Empty slots are initialised to "Init".

#include <string>
#include <utility>
#include <vector>

namespace tdx
{
struct Preset
{
    std::string name;
    std::vector<std::pair<std::string, float>> values;
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> p {
            { "Init", {} },
            { "Warm Poly Pad", {
                { "v2_detune", 9.0f }, { "cutoff", 0.5f }, { "res", 0.2f }, { "eg_int", 0.2f },
                { "sf_a", 0.8f }, { "sf_d", 2.0f }, { "sf_s", 0.6f }, { "sf_r", 1.5f },
                { "sa_a", 0.6f }, { "sa_s", 1.0f }, { "sa_r", 1.8f }, { "dvib_on", 1.0f }, { "dvib_int", 0.15f } } },
            { "PWM Strings Synth", {
                { "v1_wave", 2.0f }, { "pw_int", 0.7f }, { "pwm_speed", 0.8f }, { "v2_scale", 0.0f },
                { "cutoff", 0.62f }, { "eg_int", 0.1f }, { "sa_a", 0.3f }, { "sa_s", 1.0f }, { "sa_r", 1.0f } } },
            { "Resonant Sweep", {
                { "v1_wave", 1.0f }, { "pw_int", 0.6f }, { "v2_scale", 1.0f }, { "cutoff", 0.3f }, { "res", 0.75f },
                { "eg_int", 0.6f }, { "sf_a", 0.002f }, { "sf_d", 0.9f }, { "sf_s", 0.1f }, { "sa_s", 0.7f } } },
            { "Octave Lead", {
                { "v2_scale", 3.0f }, { "v2_detune", 4.0f }, { "cutoff", 0.62f }, { "res", 0.35f }, { "kbd_track", 3.0f },
                { "eg_int", 0.25f }, { "dvib_on", 1.0f }, { "dvib_int", 0.35f }, { "dvib_time", 0.5f } } },
            { "Square Organ", {
                { "v1_wave", 1.0f }, { "pw_int", 0.0f }, { "v2_scale", 3.0f }, { "v2_detune", 0.0f },
                { "cutoff", 0.7f }, { "eg_int", 0.0f }, { "sa_a", 0.003f }, { "sa_s", 1.0f }, { "sa_r", 0.08f } } },
            { "Flanged Poly", {
                { "fl_syn", 1.0f }, { "fl_fb", 0.7f }, { "fl_speed", 0.15f }, { "fl_int", 0.8f },
                { "cutoff", 0.55f }, { "sa_r", 1.2f } } },
            { "Bass Pluck", {
                { "v1_oct", 0.0f }, { "v2_scale", 1.0f }, { "cutoff", 0.25f }, { "res", 0.4f }, { "eg_int", 0.55f },
                { "sf_d", 0.35f }, { "sf_s", 0.0f }, { "sa_d", 0.8f }, { "sa_s", 0.5f }, { "sa_r", 0.1f } } },

            // Bank B: Brass
            { "Brass Section", {
                { "syn_on", 0.0f }, { "br_on", 1.0f }, { "br_16", 1.0f }, { "br_cutoff", 0.36f }, { "br_egint", 0.55f },
                { "br_a", 0.06f }, { "br_d", 0.4f }, { "br_s", 0.65f }, { "br_r", 0.25f } } },
            { "Brass Stab", {
                { "syn_on", 0.0f }, { "br_on", 1.0f }, { "br_cutoff", 0.32f }, { "br_egint", 0.65f },
                { "br_a", 0.02f }, { "br_d", 0.25f }, { "br_s", 0.3f }, { "br_r", 0.12f } } },
            { "Swelling Horns", {
                { "syn_on", 0.0f }, { "br_on", 1.0f }, { "br_cutoff", 0.3f }, { "br_egint", 0.6f }, { "br_multi", 0.0f },
                { "br_a", 0.6f }, { "br_d", 1.2f }, { "br_s", 0.8f }, { "br_r", 0.6f }, { "dvib_on", 1.0f }, { "dvib_int", 0.2f } } },
            { "Brass + Synth", {
                { "br_on", 1.0f }, { "syn_vol", 0.45f }, { "br_vol", 0.5f }, { "cutoff", 0.5f }, { "sa_a", 0.08f } } },

            // Bank C: Strings
            { "Ensemble Strings", {
                { "syn_on", 0.0f }, { "st_on", 1.0f }, { "st_16", 0.0f }, { "st_attack", 0.35f }, { "st_release", 1.5f } } },
            { "Bowed Strings", {
                { "syn_on", 0.0f }, { "st_on", 1.0f }, { "bow_on", 1.0f }, { "bow_level", 0.6f }, { "svib_on", 1.0f },
                { "svib_int", 0.3f }, { "st_attack", 0.25f }, { "st_release", 1.2f }, { "st_eq_hi", 3.0f } } },
            { "Cello Section", {
                { "syn_on", 0.0f }, { "st_on", 1.0f }, { "st_16", 1.0f }, { "st_4", 0.0f }, { "st_balance", -0.5f },
                { "st_eq_lo", 4.0f }, { "st_eq_hi", -3.0f }, { "st_attack", 0.3f }, { "svib_on", 1.0f } } },
            { "Flanged Strings", {
                { "syn_on", 0.0f }, { "st_on", 1.0f }, { "fl_st", 1.0f }, { "fl_fb", 0.6f }, { "fl_speed", 0.1f } } },

            // Bank D: Combinations
            { "Strings + Brass Split", {
                { "syn_on", 0.0f }, { "br_on", 1.0f }, { "br_zone", 2.0f }, { "st_on", 1.0f }, { "st_zone", 0.0f },
                { "br_a", 0.06f }, { "st_attack", 0.3f }, { "st_release", 1.2f } } },
            { "Trinity Stack", {
                { "br_on", 1.0f }, { "st_on", 1.0f }, { "syn_vol", 0.45f }, { "br_vol", 0.4f }, { "st_vol", 0.5f },
                { "cutoff", 0.55f }, { "sa_a", 0.2f }, { "sa_r", 1.2f }, { "br_a", 0.25f }, { "st_attack", 0.3f } } },
            { "Bass / Strings Split", {
                { "syn_zone", 0.0f }, { "v1_oct", 0.0f }, { "cutoff", 0.3f }, { "eg_int", 0.5f }, { "sf_s", 0.1f },
                { "st_on", 1.0f }, { "st_zone", 2.0f } } },
        };

        const char banks[] = { 'A', 'B', 'C', 'D' };
        std::vector<Preset> out;
        // Lay out: A = synth (0-7), B = brass (8-11), C = strings (12-15), D = combos (16-18)
        const int bankStart[] = { 0, 8, 12, 16 }, bankEnd[] = { 8, 12, 16, (int) p.size() };
        for (int b = 0; b < 4; ++b)
            for (int s = 0; s < 8; ++s)
            {
                const int src = bankStart[b] + s;
                Preset pr = src < bankEnd[b] ? p[(size_t) src] : Preset { "Init", {} };
                pr.name = std::string (1, banks[b]) + std::to_string (s + 1) + " " + pr.name;
                out.push_back (std::move (pr));
            }
        return out;
    }();
    return presets;
}
} // namespace tdx
