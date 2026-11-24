// WARMFUL — factory presets.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
//
// The MKG set is inspired by Mk.gee's publicly documented rig (not endorsed by him): a Jaguar with
// baritone flatwounds straight into a Tascam Portastudio 424 preamp pushed until it "collapses",
// a DigiTech Drop (~-5 st), a Boss CE-2W used as slow vibrato, distorted reverb placed *before* the
// preamp (Rainger Reverb-X), Chase Bliss MOOD micro-loops, the Roland VG-8 "Bowed Pad", an Alesis
// Quadraverb and a Boss DD-20. Each piece is mapped onto the module that plays its role, including
// the module order.
#include "FactoryPresets.h"

namespace warmful
{

namespace
{
    // Effect indices per module.
    enum Char { DRIVE, SWEETEN, FUZZ, HOWL, SWELL };
    enum Move { DOUBLER, VIBRATO, PHASER, TREMOLO, PITCH };
    enum Diff { CASCADE, REELS, SPACE, COLLAGE, REVERSE };
    enum Tex { FILTER, SQUASH, CASSETTE, BROKEN, INTERFERENCE };
    enum Mod { C = 0, M = 1, D = 2, T = 3 };

    /** PITCH RATE position for a shift in semitones (centre = 0, ends = +-12). */
    constexpr float semitones (float st) { return 0.5f + st / 24.0f; }

    // Knob values: see the mappings in each effect's `voicing` block. Worked examples:
    //   VIBRATO RATE 0.34 ~ 0.5 Hz, 0.44 ~ 0.8 Hz (0.1 * 120^x); depth 75 * AMOUNT^1.5 cents.
    //   COLLAGE TIME 0.48 ~ 230 ms (25 ms * 96^x); CASCADE TIME 0.54 ~ 180 ms; REELS TIME 0.68 ~ 300 ms.
    //   DOUBLER RATE 0.23 ~ 20 ms. SPACE TIME 0 chamber, 0.25 room/plate, 0.5 hall, 0.75 large, 1 cloud.
    constexpr std::array<FactoryPreset, 10> kPresets { {
        { "MKG 424 Edge",
          "Jaguar into a pushed Portastudio: elastic clean on the edge of collapse, slow CE-2 wobble, small room, warm deck.",
          { SWEETEN, VIBRATO, SPACE, CASSETTE },
          { 0.40f, 0.62f, 0.34f, 0.22f, 0.10f, 0.20f, 1.0f, 0.28f },
          { 0.62f, 0.50f, 0.30f, 0.50f, 0.10f, 0.50f, 0.50f, 0.50f } },

        { "MKG Portastudio Crush",
          "The 424 preamp past the point of control: dark, squashed, half-broken. Dig in for the blow-up.",
          { DRIVE, VIBRATO, SPACE, SQUASH },
          { 0.36f, 0.52f, 0.36f, 0.18f, 0.08f, 0.16f, 1.0f, 0.80f },
          { 0.70f, 0.45f, 0.25f, 0.50f, 0.10f, 0.50f, 0.40f, 0.42f } },

        { "MKG Baritone Drop",
          "Drop-style -5 semitones before the preamp: baritone weight on a standard-tuned guitar.",
          { SWEETEN, PITCH, SPACE, CASSETTE },
          { 0.42f, 0.55f, semitones (-5.0f), 1.0f, 0.10f, 0.16f, 1.0f, 0.22f },
          { 0.60f, 0.50f, 0.08f, 0.50f, 0.05f, 0.50f, 0.50f, 0.50f },
          { M, C, D, T } },

        { "MKG Distorted Room",
          "Reverb placed before the preamp (Reverb-X idea): the room itself breaks up when you play harder.",
          { DRIVE, VIBRATO, SPACE, SQUASH },
          { 0.40f, 0.46f, 0.38f, 0.20f, 0.30f, 0.42f, 1.0f, 0.45f },
          { 0.65f, 0.50f, 0.30f, 0.50f, 0.15f, 0.55f, 0.31f, 0.50f },
          { M, D, C, T } },

        { "MKG Dream Wobble",
          "Seasick CE-2 vibrato into a worn cassette: chords that sound like they are falling apart.",
          { SWEETEN, VIBRATO, SPACE, CASSETTE },
          { 0.44f, 0.50f, 0.44f, 0.42f, 0.14f, 0.22f, 1.0f, 0.52f },
          { 0.55f, 0.50f, 0.55f, 0.50f, 0.20f, 0.50f, 0.50f, 0.50f } },

        { "MKG Bowed Pad",
          "VG-8 'Bowed Pad' idea: slow volume swells drifting into a wide, darkened hall. Let notes ring.",
          { SWELL, VIBRATO, SPACE, FILTER },
          { 0.42f, 0.55f, 0.30f, 0.30f, 0.78f, 0.55f, 1.0f, 0.40f },
          { 0.55f, 0.50f, 0.70f, 0.50f, 0.35f, 0.50f, 0.45f, 0.50f } },

        { "MKG Mood Smear",
          "MOOD-like micro-loops that fold back into themselves. Twist TIME (or record a GESTURE) for edits.",
          { SWEETEN, VIBRATO, COLLAGE, CASSETTE },
          { 0.45f, 0.45f, 0.34f, 0.18f, 0.48f, 0.55f, 1.0f, 0.35f },
          { 0.55f, 0.50f, 0.30f, 0.50f, 0.40f, 0.45f, 0.45f, 0.50f } },

        { "MKG Rockman Clean",
          "80s headphone-amp clean: compressed first, then preamp, doubled width and a short echo.",
          { SWEETEN, DOUBLER, CASCADE, SQUASH },
          { 0.60f, 0.38f, 0.23f, 0.55f, 0.54f, 0.18f, 1.0f, 0.62f },
          { 0.50f, 0.50f, 0.15f, 0.50f, 0.10f, 0.45f, 0.34f, 0.50f },
          { T, C, M, D } },

        { "MKG Two Star Lead",
          "Crunchy, blown-out fuzz lead with a wonky DD-style tape echo. Roll the guitar volume back to clean up.",
          { FUZZ, VIBRATO, REELS, SQUASH },
          { 0.40f, 0.50f, 0.36f, 0.16f, 0.68f, 0.30f, 1.0f, 0.45f },
          { 0.50f, 0.50f, 0.25f, 0.50f, 0.35f, 0.50f, 0.325f, 0.50f } },

        { "MKG Quadraverb Grain",
          "Short, grainy early-digital room crushed by the preamp after it. Movement is off.",
          { DRIVE, VIBRATO, SPACE, CASSETTE },
          { 0.38f, 0.42f, 0.35f, 0.20f, 0.03f, 0.38f, 1.0f, 0.30f },
          { 0.62f, 0.50f, 0.20f, 0.50f, 0.25f, 0.60f, 0.37f, 0.50f },
          { D, C, M, T },
          { true, false, true, true } },
    } };
}

std::span<const FactoryPreset> factoryPresets()
{
    return kPresets;
}

EngineParams toEngineParams (const FactoryPreset& preset, EngineParams p)
{
    const auto& s = preset.secondary;
    p.primary = preset.primary;
    p.sensitivity = s[0];
    p.effectVol = { s[1], s[3], s[5], s[7] };
    p.driftMovement = s[2];
    p.driftDiffusion = s[4];
    p.outputLevel = s[6];
    p.effect = preset.effect;
    p.order = preset.order;
    p.moduleOn = preset.moduleOn;
    p.filterStyle = preset.filterStyle;
    return p;
}

} // namespace warmful
