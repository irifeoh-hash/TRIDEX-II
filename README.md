<p align="center"><img src="docs/tridex-ii-logo.png" width="640" alt="TRIDEX II"></p>

# TRIDEX II

**TRIDEX II** is a performing-keyboard instrument plugin by **Irife**, inspired by the Korg Trident Mk II (1980–82).
It is available as a VST3 for Windows and as a VST3 and AU for macOS (Apple Silicon and Intel).
It is built with JUCE 8 and C++17.

![TRIDEX II](docs/screenshot.png)

📖 **[Read the manual](MANUAL.md)**. It covers every section, control and feature.

## Download

Get the latest build from the **[tridex-ii-latest release](../../releases/tag/tridex-ii-latest)**:

- `TridexII-Windows-VST3.zip`: Windows VST3
- `TridexII-macOS.zip`: macOS VST3 and AU, universal

Install steps are in the [manual](MANUAL.md#1-installing) and in the README inside each zip.

## Features

- **Three sections that share one voice pool**: 8 voices (classic) or 16, selectable under KEY ASSIGN.
  - **SYNTHESIZER**: two divider VCOs, a 4-pole resonant VCF, two ADSRs, and a 32-slot programmer.
  - **BRASS**: two oscillators, with three voicings: PARA (the original), PARA+VCF and POLY.
  - **STRINGS**: ensemble, bowing, vibrato, EQ and a level for each footage, plus a phase-locked **DIVIDE DOWN** string-machine mode.
- **Per-part keyboard split**: L, L+H or H, each part with its own split point, shown in a colour strip above the keys.
- **Mono modes** for synth and brass: true monophonic with note priority (Last, First, Low, High) and RETRIG or LEGATO, plus PORTA or GLISS glide.
- **Per-part arpeggiators and L/R step gates**, which can sync to the host.
- **Effects**:
  - flanger,
  - dimension-style chorus and ensemble,
  - tape and VHS processor (Studio, Cassette, VHS, Worn VHS) with wow, flutter, drift, age, hiss and drive for each part.

  Each effect has a selector for each part, and the **FX ROUTING** page sets the order of gate, chorus, flanger and tape for each part.
- **Assignable LFO**: 7 waves, free or host-synced, fade-in and key retrigger, two destinations from a list of 32. The list includes filter and volume targets, VCO 2 pitch, waveform sweeps and a **patch morph** through the programmer bank. The mod wheel can bring the LFO in.
- **SETTINGS page**: pedal mode, note priority, bend range, velocity, analog drift, and NORMAL / ECO filter quality.
- **Separate outputs**: Main, plus Synthe, Brass and Strings stereo buses.
- **Performance controls**: joystick (bend, vibrato, trill), delay vibrato, transpose, and octave shift for each part.

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # uses ../JUCE if present, else fetches JUCE 8.0.4
cmake --build build
```

Add `-DTRIDEX_STANDALONE=ON` to also build a standalone app.
GitHub Actions (`.github/workflows/build.yml`) builds Windows (MSVC) and macOS (Xcode, universal) on every push,
and publishes the zips to the `tridex-ii-latest` release.

To run the headless DSP test, which renders WAVs and checks levels, NaN and silence:
`g++ -std=c++17 -O2 -ISource tests/render_test.cpp -o rt && ./rt /tmp`

## How the circuit maps to code

| Hardware (service manual) | Code |
|---|---|
| KLM-377 key assigner, 8 voices shared by all sections, Mode 1/2, split L / L+H / H per section | `Engine::allocate`, `inZone` |
| KLM-379 VCO1/VCO2: saw core + 4013 flip-flop dividers. 16'/8' saws are built by summing the core saw with divided squares, so all footages are phase-locked | `DividerVco` (exact identity, plus per-voice summing-resistor tolerance) |
| VCO1 saw / PW / PWM with a shared PWM LFO; VCO2 Off/16'/8'/4' saw only, detune | `Engine::render` synth block |
| KLM-378 per-voice SSM2044 4-pole VCF, SSM2056 ADSRs, KBD track Off/¼/½/Full/Over, VCA attenuator ±10 dB, Auto Damp | `Ladder` (ZDF, saturating), `Adsr` (RC curves) |
| KLM-403 brass: per-voice MPX-EG gate, then **one** VCF/VCA and one EG (paraphonic), 16'+8', multiple trigger | `brassVcf`, `brassEnv` |
| KLM-384/404 strings: KBD balance, per-voice AR VCA, vibrato, ensemble, bowing, EQ | strings chain in `render` |
| KLM-401 MG: joystick X = bend, Y+ = vibrato, Y− = trill; delay vibrato; flanger with per-section select | MG block plus `Flanger` |

MIDI: pitch bend = joystick X, CC1 = joystick Y+ (vibrato), CC2 = joystick Y− (trill),
CC64 = foot switch (Off, Damper/Auto Damp release switch, or Hold).

## Modelling notes (best interpretations, not verified against hardware)

- **Auto Damp:** a new note cuts released tails unless the Damper (S-1) pedal is down.
- **Assign Mode 1/2:** modelled as rotating assignment vs. same-voice/lowest-free assignment.
- **Brass Trigger Select / Silence Note:** the brass EG fires only once 2, 4, 6 or 8 keys are held.
- **Bowing:** a band-passed attack transient for each note.
- **Compander:** omitted. It is BBD noise reduction and is ideally transparent.
- Component values (divider weights, EG curves, filter scaling) are approximations.

## Credits and legal

TRIDEX II is designed by Irife ([instagram.com/irife](https://www.instagram.com/irife)).
The Irife logo is © Irife. All rights reserved.
Korg and Trident are trademarks of Korg Inc. This independent project is not affiliated with or endorsed by Korg.
