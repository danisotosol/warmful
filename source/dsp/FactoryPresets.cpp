// WARMFUL — factory presets.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
//
// Every pack is "inspired by" publicly documented sources (interviews, credits, rig rundowns) and
// is not endorsed by the artists. The research behind each preset, with links, is in
// docs/research/. Short version:
//  MKG  Mk.gee: 24" Jaguar with baritone flatwounds into a Tascam 424 preamp pushed until it
//       "collapses", DigiTech Drop, CE-2W as slow vibrato, Rainger Reverb-X before the preamp,
//       Chase Bliss MOOD, VG-8 "Bowed Pad", Alesis Quadraverb, DD-20.
//  DIJ  Dijon: Absolutely = one omni C414 in a boxy room through a hot 73-style preamp with the
//       mids cut; Baby = fuzz guitar, reversed/gated parts, defective-deck skips, huge reverbs;
//       Fairfield Shallow Water; Mk.gee's Reverb-X and BF-2 in his touring band.
//  PRN  Prince: Hohner MadCat into a clean Mesa Mark II, Boss board (DS-1/SD-1, BF-2 at
//       Rate 3 / Res 10, CE-2, DM-2, OC-2); "Kiss" guitar dry and gated to the hi-hat with a
//       touch of tape delay; RMX16 Non Lin 2 era.
//  BLT  Dean Blunt: tape-recorded, overdubbed and looped; reverb-washed sampled jangle,
//       untuned strums (Joanne Robertson), granular loops and whole-mix phasing on Black Metal 2,
//       pirate-radio grit (Babyfather). No gear documented: built from the recordings.
//  YNO  Jonah Yano: Yamaha parlor DI'd into a Tascam 244; portrait of a dog cut live through a
//       Midas console to 1" Ampex with a car-sized hallway plate on every song; Heavy Loop jams.
//  MSA  Junior Mesa: self-recorded glam-funk/soul, "old cassette in a car's rattling speakers",
//       Beatlesque fuzz outros. No gear documented: built from press descriptions.
//  BAR  Baritone Jaguar: Bb/C standard on flatwounds, strangle switch (~140 Hz high-pass),
//       rhythm circuit (~2.2 kHz low-pass), Bass VI octave-down doubling.
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

    constexpr const char* MKG = "MKG";
    constexpr const char* DIJ = "DIJ";
    constexpr const char* PRN = "PRN";
    constexpr const char* BLT = "BLT";
    constexpr const char* YNO = "YNO";
    constexpr const char* MSA = "MSA";
    constexpr const char* BAR = "BAR";

    constexpr FactoryPack kPacks[] {
        { MKG, "MK.GEE" },
        { DIJ, "DIJON" },
        { PRN, "PRINCE" },
        { BLT, "BLUNT" },
        { YNO, "YANO" },
        { MSA, "MESA" },
        { BAR, "BARI JAG" },
    };

    /** PITCH RATE position for a shift in semitones (centre = 0, ends = +-12). */
    constexpr float semitones (float st) { return 0.5f + st / 24.0f; }

    // Knob values: see the mappings in each effect's `voicing` block. Worked examples:
    //   VIBRATO RATE 0.34 ~ 0.5 Hz, 0.44 ~ 0.8 Hz (0.1 * 120^x); depth 75 * AMOUNT^1.5 cents.
    //   COLLAGE TIME 0.48 ~ 230 ms (25 ms * 96^x); CASCADE TIME 0.54 ~ 180 ms; REELS TIME 0.68 ~ 300 ms.
    //   DOUBLER RATE 0.23 ~ 20 ms. SPACE TIME 0 chamber, 0.25 room/plate, 0.5 hall, 0.75 large, 1 cloud.
    constexpr FactoryPreset kPresets[] {
        { MKG, "MKG 424 Edge",
          "Jaguar into a pushed Portastudio: elastic clean on the edge of collapse, slow CE-2 wobble, small room, warm deck.",
          { SWEETEN, VIBRATO, SPACE, CASSETTE },
          { 0.40f, 0.62f, 0.34f, 0.22f, 0.10f, 0.20f, 1.0f, 0.28f },
          { 0.62f, 0.50f, 0.30f, 0.50f, 0.10f, 0.50f, 0.50f, 0.50f } },

        { MKG, "MKG Portastudio Crush",
          "The 424 preamp past the point of control: dark, squashed, half-broken. Dig in for the blow-up.",
          { DRIVE, VIBRATO, SPACE, SQUASH },
          { 0.36f, 0.52f, 0.36f, 0.18f, 0.08f, 0.16f, 1.0f, 0.80f },
          { 0.70f, 0.45f, 0.25f, 0.50f, 0.10f, 0.50f, 0.40f, 0.42f } },

        { MKG, "MKG Baritone Drop",
          "Drop-style -5 semitones before the preamp: baritone weight on a standard-tuned guitar.",
          { SWEETEN, PITCH, SPACE, CASSETTE },
          { 0.42f, 0.55f, semitones (-5.0f), 1.0f, 0.10f, 0.16f, 1.0f, 0.22f },
          { 0.60f, 0.50f, 0.08f, 0.50f, 0.05f, 0.50f, 0.50f, 0.50f },
          { M, C, D, T } },

        { MKG, "MKG Distorted Room",
          "Reverb placed before the preamp (Reverb-X idea): the room itself breaks up when you play harder.",
          { DRIVE, VIBRATO, SPACE, SQUASH },
          { 0.40f, 0.46f, 0.38f, 0.20f, 0.30f, 0.42f, 1.0f, 0.45f },
          { 0.65f, 0.50f, 0.30f, 0.50f, 0.15f, 0.55f, 0.31f, 0.50f },
          { M, D, C, T } },

        { MKG, "MKG Dream Wobble",
          "Seasick CE-2 vibrato into a worn cassette: chords that sound like they are falling apart.",
          { SWEETEN, VIBRATO, SPACE, CASSETTE },
          { 0.44f, 0.50f, 0.44f, 0.42f, 0.14f, 0.22f, 1.0f, 0.52f },
          { 0.55f, 0.50f, 0.55f, 0.50f, 0.20f, 0.50f, 0.50f, 0.50f } },

        { MKG, "MKG Bowed Pad",
          "VG-8 'Bowed Pad' idea: slow volume swells drifting into a wide, darkened hall. Let notes ring.",
          { SWELL, VIBRATO, SPACE, FILTER },
          { 0.42f, 0.55f, 0.30f, 0.30f, 0.78f, 0.55f, 1.0f, 0.40f },
          { 0.55f, 0.50f, 0.70f, 0.50f, 0.35f, 0.50f, 0.45f, 0.50f } },

        { MKG, "MKG Mood Smear",
          "MOOD-like micro-loops that fold back into themselves. Twist TIME (or record a GESTURE) for edits.",
          { SWEETEN, VIBRATO, COLLAGE, CASSETTE },
          { 0.45f, 0.45f, 0.34f, 0.18f, 0.48f, 0.55f, 1.0f, 0.35f },
          { 0.55f, 0.50f, 0.30f, 0.50f, 0.40f, 0.45f, 0.45f, 0.50f } },

        { MKG, "MKG Rockman Clean",
          "80s headphone-amp clean: compressed first, then preamp, doubled width and a short echo.",
          { SWEETEN, DOUBLER, CASCADE, SQUASH },
          { 0.60f, 0.38f, 0.23f, 0.55f, 0.54f, 0.18f, 1.0f, 0.62f },
          { 0.50f, 0.50f, 0.15f, 0.50f, 0.10f, 0.45f, 0.34f, 0.50f },
          { T, C, M, D } },

        { MKG, "MKG Two Star Lead",
          "Crunchy, blown-out fuzz lead with a wonky DD-style tape echo. Roll the guitar volume back to clean up.",
          { FUZZ, VIBRATO, REELS, SQUASH },
          { 0.40f, 0.50f, 0.36f, 0.16f, 0.68f, 0.30f, 1.0f, 0.45f },
          { 0.50f, 0.50f, 0.25f, 0.50f, 0.35f, 0.50f, 0.325f, 0.50f } },

        { MKG, "MKG Quadraverb Grain",
          "Short, grainy early-digital room crushed by the preamp after it. Movement is off.",
          { DRIVE, VIBRATO, SPACE, CASSETTE },
          { 0.38f, 0.42f, 0.35f, 0.20f, 0.03f, 0.38f, 1.0f, 0.30f },
          { 0.62f, 0.50f, 0.20f, 0.50f, 0.25f, 0.60f, 0.37f, 0.50f },
          { D, C, M, T },
          { true, false, true, true } },

        // ---- DIJ: Dijon ------------------------------------------------------------------------
        { DIJ, "DIJ Omni Room",
          "Absolutely-era spare room: one omni mic, hot 73-style preamp, mids and lows thinned. Pick sparse single notes.",
          { SWEETEN, DOUBLER, SPACE, FILTER },
          { 0.58f, 0.55f, 0.15f, 0.22f, 0.06f, 0.38f, 1.0f, 0.68f },
          { 0.65f, 0.50f, 0.20f, 0.50f, 0.05f, 0.50f, 0.482f, 0.50f },
          { D, C, M, T } },

        { DIJ, "DIJ Big Mike's Swell",
          "Feedback-swell intro: notes bloom in, tape repeats pile up and warp. Hold notes and ride the volume knob.",
          { SWELL, VIBRATO, REELS, CASSETTE },
          { 0.45f, 0.62f, 0.40f, 0.22f, 0.60f, 0.80f, 1.0f, 0.38f },
          { 0.62f, 0.55f, 0.60f, 0.50f, 0.55f, 0.55f, 0.419f, 0.50f } },

        { DIJ, "DIJ Shallow Water",
          "Shallow Water-style random, non-repeating wobble with a dulled top. Slow-jam chords and Kindalove lines.",
          { SWEETEN, VIBRATO, SPACE, FILTER },
          { 0.45f, 0.30f, 0.50f, 0.34f, 0.22f, 0.22f, 0.72f, 0.58f },
          { 0.50f, 0.50f, 0.85f, 0.50f, 0.30f, 0.50f, 0.498f, 0.50f },
          { C, M, D, T }, { true, true, true, true }, FilterStyle::LowPass },

        { DIJ, "DIJ Lightning Riff",
          "Baby-tour crunch: small room into compressed overdrive with a slow BF-2-style jet. Dig in for lightning bursts.",
          { DRIVE, PHASER, SPACE, SQUASH },
          { 0.60f, 0.52f, 0.30f, 0.72f, 0.12f, 0.30f, 1.0f, 0.70f },
          { 0.58f, 0.45f, 0.20f, 0.50f, 0.15f, 0.50f, 0.423f, 0.50f },
          { D, C, T, M } },

        { DIJ, "DIJ Yamaha Fuzz",
          "Baby-era fuzz guitar torn by radio rips and sampler stutters. Short phrases; let the loop chew them.",
          { FUZZ, TREMOLO, COLLAGE, INTERFERENCE },
          { 0.35f, 0.45f, 0.50f, 0.00f, 0.48f, 0.40f, 1.0f, 0.26f },
          { 0.55f, 0.42f, 0.00f, 0.50f, 0.40f, 0.45f, 0.412f, 0.50f },
          { C, M, D, T }, { true, false, true, true } },

        { DIJ, "DIJ Fever Storm",
          "Blown-out fuzz into a huge slow-motion reverb, then slammed. Play hard; chords bloom and pump.",
          { FUZZ, DOUBLER, SPACE, SQUASH },
          { 0.42f, 0.45f, 0.28f, 0.35f, 0.68f, 0.45f, 1.0f, 0.76f },
          { 0.55f, 0.40f, 0.35f, 0.50f, 0.30f, 0.50f, 0.37f, 0.50f } },

        { DIJ, "DIJ Gated Rewind",
          "Reversed grains chopped by a square gate through a cheap radio. Hold notes; RATE sets the stutter.",
          { DRIVE, TREMOLO, REVERSE, INTERFERENCE },
          { 0.55f, 0.40f, 0.80f, 0.85f, 0.50f, 0.50f, 1.0f, 0.28f },
          { 0.55f, 0.50f, 0.35f, 0.55f, 0.20f, 0.50f, 0.458f, 0.50f },
          { C, D, M, T } },

        { DIJ, "DIJ Gated Bootleg",
          "Reverb-X idea: clean dry note plus a distorted hall that sputters shut about a second after you stop. Leave gaps.",
          { FUZZ, VIBRATO, SPACE, FILTER },
          { 0.90f, 0.25f, 0.44f, 0.18f, 0.50f, 1.00f, 0.55f, 0.40f },
          { 0.42f, 0.40f, 0.30f, 0.50f, 0.10f, 0.60f, 0.426f, 0.50f },
          { D, C, M, T } },

        { DIJ, "DIJ Busted Deck",
          "90s R&B chords through a defective deck: no top end, warble and motor skips. Slow, sparse chords.",
          { DRIVE, VIBRATO, SPACE, BROKEN },
          { 0.28f, 0.42f, 0.40f, 0.25f, 0.22f, 0.22f, 1.0f, 0.40f },
          { 0.60f, 0.45f, 0.60f, 0.50f, 0.10f, 0.50f, 0.434f, 0.50f } },

        { DIJ, "DIJ Freak It Echo",
          "Every hit throws a long, dirty tape echo that clips the preamp. Short stabs; ride TIME for edits.",
          { DRIVE, VIBRATO, REELS, SQUASH },
          { 0.45f, 0.50f, 0.40f, 0.20f, 0.55f, 0.80f, 1.0f, 0.65f },
          { 0.55f, 0.42f, 0.30f, 0.50f, 0.35f, 0.55f, 0.308f, 0.50f },
          { D, C, T, M }, { true, false, true, true } },

        // ---- PRN: Prince -----------------------------------------------------------------------
        { PRN, "PRN Crazy Flange Solo",
          "DS-1-style lead into a clean Boogie with a slow, resonant jet sweep. Bridge pickup; bend and hold.",
          { DRIVE, PHASER, SPACE, SQUASH },
          { 0.64f, 0.80f, 0.23f, 0.85f, 0.22f, 0.15f, 1.0f, 0.30f },
          { 0.55f, 0.42f, 0.00f, 0.50f, 0.05f, 0.50f, 0.366f, 0.50f } },

        { PRN, "PRN Doves Octave Fuzz",
          "OC-2-style octave down blended into brash fuzz with a dark slap. Play the intro lick high on the neck.",
          { FUZZ, PITCH, CASCADE, SQUASH },
          { 0.60f, 0.65f, semitones (-12.0f), 0.50f, 0.54f, 0.20f, 1.0f, 0.25f },
          { 0.55f, 0.40f, 0.00f, 0.55f, 0.15f, 0.35f, 0.407f, 0.50f },
          { M, C, D, T } },

        { PRN, "PRN Kiss Gate",
          "Dry DI funk chords chopped by a hard 16th gate (~111 BPM) with a ghost of tape echo. Tap tempo to lock it.",
          { SWEETEN, TREMOLO, REELS, FILTER },
          { 0.60f, 0.30f, 0.90f, 1.00f, 0.416f, 0.12f, 1.0f, 0.40f },
          { 0.50f, 0.50f, 0.00f, 0.50f, 0.10f, 0.30f, 0.50f, 0.50f },
          { C, M, D, T }, { true, true, true, true }, FilterStyle::HighPass },

        { PRN, "PRN Purple Rain Chords",
          "Barely-edged Boogie clean, tight stereo shimmer and a big plate/hall. Let the Bb chords ring.",
          { DRIVE, DOUBLER, SPACE, SQUASH },
          { 0.55f, 0.22f, 0.00f, 0.45f, 0.50f, 0.30f, 1.0f, 0.30f },
          { 0.50f, 0.50f, 0.25f, 0.50f, 0.20f, 0.50f, 0.374f, 0.50f },
          { C, M, T, D } },

        { PRN, "PRN Rain Solo",
          "Singing SD-1-into-Boogie lead with a 300 ms analog echo. Hold one high note and let it bloom.",
          { DRIVE, DOUBLER, CASCADE, SQUASH },
          { 0.50f, 0.82f, 0.00f, 0.25f, 0.72f, 0.35f, 1.0f, 0.45f },
          { 0.60f, 0.40f, 0.15f, 0.50f, 0.20f, 0.45f, 0.257f, 0.50f },
          { C, T, M, D } },

        { PRN, "PRN Nonlin Stab",
          "Funk stabs into a short, flat, compressed room that stops dead (Non Lin 2 era). Play staccato.",
          { DRIVE, DOUBLER, SPACE, SQUASH },
          { 0.62f, 0.28f, 0.23f, 0.00f, 0.05f, 0.45f, 1.0f, 0.70f },
          { 0.50f, 0.50f, 0.00f, 0.50f, 0.00f, 0.60f, 0.374f, 0.50f },
          { C, M, D, T }, { true, false, true, true } },

        { PRN, "PRN DI Rhythm CE-2",
          "Always-dry direct-box funk rhythm with a light CE-2 shimmer. 16ths on the bridge pickup, mute hard.",
          { SWEETEN, VIBRATO, SPACE, SQUASH },
          { 0.60f, 0.35f, 0.52f, 0.20f, 0.10f, 0.00f, 0.55f, 0.35f },
          { 0.50f, 0.50f, 0.05f, 0.50f, 0.00f, 0.30f, 0.392f, 0.50f },
          { C, M, D, T }, { true, true, false, true } },

        // ---- BLT: Dean Blunt -------------------------------------------------------------------
        { BLT, "BLT Black Metal Jangle",
          "Reverb-washed, doubled clean jangle (LUSH, 50 CENT). Strum soft open chords or loop a two-chord figure.",
          { SWEETEN, DOUBLER, SPACE, CASSETTE },
          { 0.55f, 0.30f, 0.23f, 0.45f, 0.42f, 0.40f, 1.0f, 0.20f },
          { 0.50f, 0.50f, 0.20f, 0.50f, 0.25f, 0.50f, 0.465f, 0.50f } },

        { BLT, "BLT Tape Ghosts",
          "Worn, overdubbed tape with wow and dark echo: Hype Williams-era murky guitar. Play sparse.",
          { DRIVE, VIBRATO, REELS, CASSETTE },
          { 0.40f, 0.20f, 0.34f, 0.25f, 0.60f, 0.30f, 1.0f, 0.55f },
          { 0.50f, 0.50f, 0.60f, 0.50f, 0.50f, 0.45f, 0.45f, 0.50f } },

        { BLT, "BLT Semtex Loop",
          "Granular, crinkly guitar micro-loops (SEMTEX). Play a phrase, let it stack and warp; move TIME for edits.",
          { SWEETEN, PITCH, COLLAGE, CASSETTE },
          { 0.50f, 0.25f, 0.50f, 0.40f, 0.75f, 0.75f, 1.0f, 0.35f },
          { 0.50f, 0.50f, 0.40f, 0.50f, 0.35f, 0.55f, 0.425f, 0.50f } },

        { BLT, "BLT ZaZa Phase",
          "Twangy dry riff with a slow phaser over the whole wet signal; switch MOVEMENT on/off for ZaZa-style warps.",
          { DRIVE, PHASER, SPACE, CASSETTE },
          { 0.62f, 0.15f, 0.44f, 0.60f, 0.25f, 0.25f, 1.0f, 0.15f },
          { 0.50f, 0.50f, 0.30f, 0.50f, 0.20f, 0.50f, 0.592f, 0.50f },
          { C, D, M, T } },

        { BLT, "BLT Pirate Chop",
          "Babyfather pirate-radio guitar: hard chop, dub echo, radio-band crunch. Stab chords and let it cut.",
          { DRIVE, TREMOLO, CASCADE, INTERFERENCE },
          { 0.45f, 0.35f, 0.70f, 0.95f, 0.66f, 0.55f, 1.0f, 0.30f },
          { 0.55f, 0.50f, 0.25f, 0.50f, 0.40f, 0.50f, 0.441f, 0.50f } },

        { BLT, "BLT Packet Stutter",
          "Trance gate meets a dropped call: 10 Hz chop, 16th micro-loop, packet-loss glitches. Sustain chords.",
          { SWEETEN, TREMOLO, COLLAGE, INTERFERENCE },
          { 0.45f, 0.30f, 0.96f, 0.90f, 0.37f, 0.35f, 1.0f, 0.52f },
          { 0.50f, 0.50f, 0.25f, 0.55f, 0.20f, 0.45f, 0.543f, 0.50f },
          { C, D, M, T } },

        { BLT, "BLT Gated Room",
          "Room reverb chopped by an envelope gate: mute the strings to kill the tail. Worn tape after.",
          { SWELL, DOUBLER, SPACE, CASSETTE },
          { 0.45f, 0.05f, 0.50f, 0.00f, 0.20f, 0.50f, 1.0f, 0.40f },
          { 0.35f, 0.55f, 0.00f, 0.50f, 0.20f, 0.50f, 0.46f, 0.50f },
          { D, C, M, T }, { true, false, true, true } },

        { BLT, "BLT Untuned Strum",
          "A second guitar 30 cents flat smears every strum, Joanne Robertson-style. Lo-fi; strum open chords.",
          { DRIVE, PITCH, SPACE, CASSETTE },
          { 0.42f, 0.18f, semitones (-0.3f), 0.50f, 0.30f, 0.25f, 1.0f, 0.40f },
          { 0.50f, 0.50f, 0.25f, 0.55f, 0.10f, 0.45f, 0.50f, 0.50f },
          { M, C, D, T } },

        // ---- YNO: Jonah Yano -------------------------------------------------------------------
        { YNO, "YNO 4-Track Parlor",
          "Parlor acoustic DI'd into a worn 244 four-track: soft squash, faint wow, boxy room. Fingerpick quietly.",
          { SWEETEN, VIBRATO, SPACE, CASSETTE },
          { 0.46f, 0.40f, 0.34f, 0.18f, 0.08f, 0.15f, 1.0f, 0.38f },
          { 0.50f, 0.50f, 0.70f, 0.50f, 0.00f, 0.50f, 0.482f, 0.50f } },

        { YNO, "YNO Marquee Plate",
          "Clean jazz electric through a Midas console and 1-inch tape into a big hallway plate. Comp extended chords.",
          { DRIVE, VIBRATO, SPACE, CASSETTE },
          { 0.44f, 0.18f, 0.50f, 0.00f, 0.28f, 0.32f, 1.0f, 0.12f },
          { 0.55f, 0.48f, 0.00f, 0.50f, 0.05f, 0.50f, 0.482f, 0.50f },
          { C, M, D, T }, { true, false, true, true } },

        { YNO, "YNO Strawberry Fuzz",
          "Gentle round fuzz, doubled and slapped onto worn tape: souvenir-era bedroom grit. Strum softly.",
          { FUZZ, DOUBLER, REELS, CASSETTE },
          { 0.32f, 0.30f, 0.23f, 0.35f, 0.50f, 0.30f, 1.0f, 0.42f },
          { 0.45f, 0.42f, 0.15f, 0.45f, 0.25f, 0.40f, 0.422f, 0.50f } },

        { YNO, "YNO Heavy Loop",
          "Driven guitar into a building micro-loop and AM-radio haze. Improvise; ride TIME for loop edits.",
          { DRIVE, VIBRATO, COLLAGE, INTERFERENCE },
          { 0.55f, 0.50f, 0.44f, 0.30f, 0.60f, 0.78f, 1.0f, 0.22f },
          { 0.55f, 0.45f, 0.60f, 0.50f, 0.30f, 0.55f, 0.323f, 0.50f } },

        { YNO, "YNO Steel Door",
          "Auto-swelled notes with a slow shimmer into a hall: pedal-steel-like pads. Let notes ring and separate.",
          { SWELL, VIBRATO, SPACE, FILTER },
          { 0.42f, 0.45f, 0.70f, 0.20f, 0.50f, 0.30f, 1.0f, 0.45f },
          { 0.60f, 0.55f, 0.20f, 0.50f, 0.15f, 0.50f, 0.465f, 0.50f } },

        // ---- MSA: Junior Mesa ------------------------------------------------------------------
        { MSA, "MSA Cops Cassette",
          "Saturated arpeggios off a worn tape in a car stereo. Pick clean broken chords; dig in for grit.",
          { DRIVE, VIBRATO, SPACE, CASSETTE },
          { 0.42f, 0.50f, 0.40f, 0.24f, 0.18f, 0.20f, 1.0f, 0.55f },
          { 0.55f, 0.42f, 0.40f, 0.50f, 0.10f, 0.50f, 0.436f, 0.50f } },

        { MSA, "MSA Glam Funk Phase",
          "Cirque Du Freak-style funk rhythm: squeezed, slowly phased chanks. Mute-strum 16ths on the neck pickup.",
          { SWEETEN, PHASER, SPACE, SQUASH },
          { 0.58f, 0.55f, 0.45f, 0.35f, 0.05f, 0.12f, 1.0f, 0.40f },
          { 0.50f, 0.50f, 0.15f, 0.50f, 0.00f, 0.50f, 0.442f, 0.50f },
          { C, T, M, D } },

        { MSA, "MSA Grip Outro",
          "Hendrix-lineage fuzz lead for outros: round fuzz, Uni-Vibe-style throb, one tape slap. Bend and sing.",
          { FUZZ, PHASER, REELS, CASSETTE },
          { 0.40f, 0.45f, 0.52f, 0.35f, 0.45f, 0.25f, 1.0f, 0.20f },
          { 0.50f, 0.35f, 0.25f, 0.50f, 0.20f, 0.40f, 0.683f, 0.50f } },

        { MSA, "MSA Empty & Warm",
          "Hushed clean in the style of Caroline: soft comp, faint wobble, dark ~2 s space. Fingerpick quietly.",
          { SWEETEN, VIBRATO, SPACE, FILTER },
          { 0.45f, 0.30f, 0.34f, 0.18f, 0.40f, 0.28f, 1.0f, 0.72f },
          { 0.50f, 0.50f, 0.55f, 0.50f, 0.25f, 0.50f, 0.50f, 0.50f },
          { C, M, D, T }, { true, true, true, true }, FilterStyle::LowPass },

        { MSA, "MSA Velcro Sputter",
          "Starved, splatty fuzz whose note tails choke into crackle. Dig in for sustain; ease off and it falls apart.",
          { FUZZ, DOUBLER, SPACE, FILTER },
          { 0.78f, 0.70f, 0.23f, 0.00f, 0.00f, 0.18f, 1.0f, 0.44f },
          { 0.40f, 0.38f, 0.00f, 0.50f, 0.00f, 0.45f, 0.581f, 0.50f },
          { C, M, D, T }, { true, false, true, true } },

        { MSA, "MSA Verb Into Fuzz",
          "Slow random vibrato into a room that a round fuzz then distorts. Reverb tails bloom; play dynamically.",
          { FUZZ, VIBRATO, SPACE, CASSETTE },
          { 0.35f, 0.30f, 0.44f, 0.25f, 0.30f, 0.45f, 1.0f, 0.35f },
          { 0.45f, 0.40f, 0.40f, 0.50f, 0.15f, 0.50f, 0.421f, 0.50f },
          { M, D, C, T } },

        // ---- BAR: baritone Jaguar --------------------------------------------------------------
        { BAR, "BAR Bb Flat Jag",
          "24-inch Jaguar tuned to Bb on flatwounds (-6 st) into a pushed 4-track. Thumb or soft pick; let chords ring.",
          { DRIVE, PITCH, SPACE, FILTER },
          { 0.58f, 0.35f, semitones (-6.0f), 1.00f, 0.10f, 0.18f, 1.0f, 0.60f },
          { 0.60f, 0.45f, 0.05f, 0.50f, 0.05f, 0.50f, 0.482f, 0.50f },
          { M, C, D, T }, { true, true, true, true }, FilterStyle::LowPass },

        { BAR, "BAR Strangle Lead",
          "C-standard Jaguar (-4 st) on the lead circuit with the strangle switch on: thin, twangy, short slap.",
          { SWEETEN, PITCH, CASCADE, FILTER },
          { 0.62f, 0.45f, semitones (-4.0f), 1.00f, 0.42f, 0.22f, 1.0f, 0.39f },
          { 0.55f, 0.50f, 0.05f, 0.50f, 0.15f, 0.40f, 0.435f, 0.55f },
          { M, C, D, T }, { true, true, true, true }, FilterStyle::HighPass },

        { BAR, "BAR Flatwound Thump",
          "For a real baritone (no shift): flatwound thump, rhythm-circuit darkness (~2.2 kHz), slow tape sway.",
          { SWEETEN, VIBRATO, SPACE, FILTER },
          { 0.35f, 0.50f, 0.44f, 0.20f, 0.12f, 0.20f, 1.0f, 0.25f },
          { 0.55f, 0.50f, 0.25f, 0.50f, 0.08f, 0.50f, 0.50f, 0.50f } },

        { BAR, "BAR Bass VI Tic-Tac",
          "Guitar plus an octave-down Bass VI double in a small boxy room (tic-tac). Single lines and low riffs.",
          { SWEETEN, PITCH, SPACE, CASSETTE },
          { 0.45f, 0.40f, semitones (-12.0f), 0.55f, 0.05f, 0.30f, 1.0f, 0.22f },
          { 0.55f, 0.50f, 0.00f, 0.55f, 0.05f, 0.55f, 0.466f, 0.50f },
          { M, C, D, T } },
    };
}

std::span<const FactoryPreset> factoryPresets()
{
    return kPresets;
}

std::span<const FactoryPack> factoryPacks()
{
    return kPacks;
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
