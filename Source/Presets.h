#pragma once
// Factory programs for the 32 programmer slots (A1..D8). On the hardware only
// the synthesizer section is programmable, so these contain synth parameters
// only. All patches are original. Each lists overrides from parameter defaults.

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
    static const std::vector<Preset> presets {
        // ---- Bank A
        { "Init", {} },
        { "Warm Pad", {
            { "v2_detune", 9.0f }, { "cutoff", 0.5f }, { "res", 0.2f }, { "eg_int", 0.15f },
            { "sf_a", 0.8f }, { "sf_d", 2.0f }, { "sf_s", 0.6f }, { "sf_r", 1.5f },
            { "sa_a", 0.6f }, { "sa_s", 1.0f }, { "sa_r", 1.8f } } },
        { "PWM Ensemble", {
            { "v1_wave", 2.0f }, { "pw_int", 0.6f }, { "pwm_speed", 0.8f }, { "v2_scale", 0.0f },
            { "cutoff", 0.62f }, { "eg_int", 0.08f }, { "sa_a", 0.3f }, { "sa_s", 1.0f }, { "sa_r", 1.0f } } },
        { "Resonant Sweep", {
            { "v1_wave", 1.0f }, { "pw_int", 0.5f }, { "v2_scale", 1.0f }, { "cutoff", 0.3f }, { "res", 0.7f },
            { "eg_int", 0.5f }, { "sf_a", 0.002f }, { "sf_d", 0.9f }, { "sf_s", 0.1f }, { "sa_s", 0.7f } } },
        { "Octave Lead", {
            { "v2_scale", 3.0f }, { "v2_detune", 4.0f }, { "cutoff", 0.62f }, { "res", 0.35f }, { "kbd_track", 3.0f },
            { "eg_int", 0.2f }, { "auto_damp", 1.0f } } },
        { "Hollow Reed", {
            { "v1_wave", 1.0f }, { "pw_int", 0.0f }, { "v2_scale", 0.0f }, { "cutoff", 0.55f }, { "res", 0.25f },
            { "eg_int", 0.1f }, { "sa_a", 0.04f }, { "sa_s", 0.9f }, { "sa_r", 0.15f } } },
        { "Harpsichord", {
            { "v1_wave", 1.0f }, { "pw_int", 0.75f }, { "v2_scale", 3.0f }, { "cutoff", 0.55f }, { "eg_int", 0.3f },
            { "sf_d", 0.5f }, { "sf_s", 0.0f }, { "sa_a", 0.001f }, { "sa_d", 1.2f }, { "sa_s", 0.0f }, { "sa_r", 0.4f } } },
        { "Bass Pluck", {
            { "v1_oct", 0.0f }, { "v2_scale", 1.0f }, { "cutoff", 0.25f }, { "res", 0.4f }, { "eg_int", 0.45f },
            { "sf_d", 0.35f }, { "sf_s", 0.0f }, { "sa_d", 0.8f }, { "sa_s", 0.5f }, { "sa_r", 0.1f }, { "auto_damp", 1.0f } } },
        // ---- Bank B
        { "Synth Brass", {
            { "v2_detune", 8.0f }, { "cutoff", 0.38f }, { "res", 0.15f }, { "eg_int", 0.35f },
            { "sf_a", 0.12f }, { "sf_d", 0.6f }, { "sf_s", 0.5f }, { "sa_a", 0.05f } } },
        { "Soft Strings", {
            { "v2_scale", 2.0f }, { "v2_detune", 12.0f }, { "cutoff", 0.58f }, { "eg_int", 0.0f }, { "kbd_track", 1.0f },
            { "sa_a", 0.45f }, { "sa_s", 1.0f }, { "sa_r", 1.4f } } },
        { "Clav", {
            { "v1_wave", 1.0f }, { "pw_int", 0.55f }, { "v2_scale", 0.0f }, { "cutoff", 0.4f }, { "res", 0.3f },
            { "eg_int", 0.4f }, { "sf_d", 0.25f }, { "sf_s", 0.0f }, { "sa_d", 0.5f }, { "sa_s", 0.2f }, { "sa_r", 0.05f } } },
        { "Self-Osc Whistle", {
            { "v2_scale", 0.0f }, { "cutoff", 0.5f }, { "res", 1.0f }, { "kbd_track", 3.0f }, { "eg_int", 0.0f },
            { "vca_att", -6.0f }, { "sa_a", 0.08f }, { "sa_s", 1.0f } } },
        { "Inverse Sweep", {
            { "cutoff", 0.8f }, { "res", 0.5f }, { "eg_int", -0.4f }, { "sf_a", 1.5f }, { "sf_d", 2.0f }, { "sf_s", 0.3f },
            { "sa_a", 0.2f }, { "sa_s", 1.0f }, { "sa_r", 1.0f } } },
    };
    return presets;
}
} // namespace tdx
