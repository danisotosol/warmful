# Chroma Console community settings → WARMFUL recipes (ChromaCommunity brief)

**Bottom line:** I found no published Chroma Console patch that is tied to Dijon, Jonah Yano, Dean Blunt, Prince or Junior Mesa. None of the five appears in Hologram's artist roster, on Equipboard's Chroma Console page, or in Dijon's Equipboard gear list. I also found almost no community posts with clock-position settings (no Reddit threads with knob values; paid packs such as Sam Wittek's sell their knob PDFs). What exists are **routing and module tricks** that users describe in words. The recipes below are those tricks mapped onto WARMFUL, with each artist link marked.

## 1. Sources
- https://www.hologramelectronics.com/pages/chroma-console : official effect descriptions ("process reverb through a fuzz", "cassette into a stereo double-tracker").
- https://www.hologramelectronics.com/blogs/tutorials/changing-fx-order-on-chroma-console : official blog (Jan 2026). Default order is Char>Move>Diff>Tex; it encourages trying reverb first and fuzz last.
- https://www.hologramelectronics.com/pages/artists : roster (Crumb, King Britt, Ariel Posen, Yvette Young, Tops, Sarah Tudzin, Spencer Zahn, Ed O'Brien…). None of the 5 target artists.
- https://www.youtube.com/watch?v=M6rZzfBAXp4 : Sam Wittek, "preset showcase" (transcript). Best community source for module choices.
- https://www.samwittekmusic.com/shop/p/chroma-console-bundle : his knob positions are only in a paid PDF (not obtained).
- https://wolfewithane.com/why-hologram-chroma-console : owner blog with routing recipes (Swell as a gate, Sensitivity, Filter).
- https://guitar.com/reviews/effects-pedal/hands-on-hologram-electronics-chroma-console-review/ : faux-baritone via Pitch; favourite chain Diffusion→Character→Cassette.
- https://www.talkbass.com/threads/new-hologram-electronics-chroma-console.1628753/page-2 : user chain Sweeten→Vibrato/Tremolo→Space→Squash "turned down". The designer calls Sweeten his proudest effect (quoted via search snippet).
- https://www.youtube.com/watch?v=JUkS0X5j23s : "Mk.gee Tone with JHS Colourbox and Chroma Console". Chain: Colour Box → Walrus M1 → Chroma Console → Strymon Flint. No settings given.
- https://www.youtube.com/watch?v=yJFYjsuo5Xs : Hologram "Studio Sounds" (studio polish on guitar, bass, EP and synths). No settings.
- https://blog.thepedalcollaborative.com/dont-stomp-your-chroma-chroma-your-stomp/ : HX Stomp clones grouped by Chroma colour row (e.g. Yellow = Sweeten/Vibrato/Reels/Squash).
- https://equipboard.com/pros/dijon : Dijon's listed gear includes a Fairfield Shallow Water, Bastl Thyme and Eventide H3000. No Chroma Console.
- https://www.gearnews.com/the-mk-gee-guitar-tone-perfect-match/ and https://www.internettattoo.com/blog/mk-gee-guitar-sound-and-tone : third-party "how to" chains (Drop −5 st, CE-2W vibrato slow/low, 424, reverb *before* the drive).
- https://www.northernvalleyaudio.com/blog/modern-indie-guitar-tone-settings : a vendor blog (low weight). Plugin chain: −3 st 100 % wet → cassette → vibrato 4 Hz / 12 % → gated dirty reverb 2 s.

## 2. Verified facts (community usage)
- **Vibrato and chorus made with the PITCH module:** Wittek says his favourite vibrato and his chorus "aren't actually on the vibrato… it's on the pitch" effect. He describes an irregular, "intermittent" wobble (M6rZzfBAXp4).
- **"Lo-fi" vibrato = vibrato + Filter low-pass.** Wittek calls it "like turning the tone knob down… chewy vibrato… blends into the background" (same video).
- **Wide vibrato:** Wittek's "Wide Vibes" uses the VIBRATO effect itself in stereo, with "little bits of chorusing" (same video).
- **Huge fuzz = HOWL + DOUBLER + a little SPACE.** Wittek says it is still controlled enough for a lead (same video).
- **Reverse delay 100 % wet, +1 octave** gives a pulsing, "side-chain" texture (Wittek).
- **Gated reverb:** reorder so the reverb runs *before* SWELL, then "mute a note or chord… when you want the reverb to stop" (wolfe).
- **SENSITIVITY:** 7 o'clock gives the highest fidelity; more sensitivity means breakup at lower input (wolfe).
- **Filter on the hardware's single knob:** low-pass ≈ 7–10 o'clock, band-pass in the middle, high-pass ≈ 2–5 o'clock (wolfe).
- **Faux baritone via PITCH is "genuinely impressive".** Guitar.com's favourite routing was Character after Diffusion with Cassette after that (guitar.com). Sound On Sound notes Pitch latency (cited in the reference doc).
- **Subtle-polish chain** Sweeten → Vibrato/Tremolo → Space → Squash at low level (TalkBass user).
- **Mk.gee chains by third parties** consistently put pitch down (−3…−5 st) first, slow/low vibrato, and reverb *before* the drive (gearnews, internettattoo, northernvalley). The MKG pack already covers this.

## 3. Sonic signature (community consensus, in measurable terms)
- **Gain:** preamp-level (Sweeten/low Drive). Dirt goes *after* the reverb when it is used.
- **Modulation:** slow and shallow (≈0.5–1 Hz, ±5–15 c), with random rather than sine waveforms (DRIFT).
- **Pitch:** 0 st with a ~50 % blend for "pitch chorus", or −3/−5 st at 100 % for baritone.
- **Reverb:** room/plate around 0.5–1.2 s, often gated by SWELL or clipped by the dirt that follows it.
- **Tone:** low-pass around 2 kHz for "lo-fi".
- **Tape:** worn cassette ≤0.4. Users warn that chords go sour as the wow increases **[inference from the search synthesis; unverified]**.

## 4. Reference material
- Wittek "Lo-fi" (vibrato + LPF), "Wide Vibes", the pitch-chorus, and "Huge fuzz" (Howl + Doubler + Space): M6rZzfBAXp4.
- Householder's Mk.gee demo (Colour Box preamp before the CC): JUkS0X5j23s.
- Hologram "Studio Sounds" (polish on a full arrangement): yJFYjsuo5Xs.

## 5. WARMFUL recipes
```
name: "DIJ Shallow Pitch"
description: "Pitch-module chorus: 0 st blend + lo-fi jitter, Sweeten polish, small room, light tape. Fingerpicked chords."
effects: [SWEETEN, PITCH, SPACE, CASSETTE]
moduleOn: [true, true, true, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.42, 0.35, 0.50, 0.45, 0.22, 0.25, 1.00, 0.20]
secondary: [0.45, 0.50, 0.40, 0.50, 0.15, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: CHAR Sweeten = the designer's flagship "polish" effect (TalkBass).
     MOVE Pitch at 0 st with ~45 % mix: the 25–40 ms wet latency + DRIFT jitter makes Wittek's "pitch chorus"; it also stands in for the random lo-fi wobble of Dijon's Shallow Water (Equipboard) [inference].
     DIFF short room (~1 s) keeps it intimate. TEX light cassette at 0.2 = a clean, warm deck.
```
```
name: "YNO Chewy Vibe"
description: "Lo-fi vibrato (~0.7 Hz, ±12 c, random) into a dark low-pass at ~2 kHz; small chamber. For soft, close parts."
effects: [SWEETEN, VIBRATO, SPACE, FILTER]
moduleOn: [true, true, true, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.40, 0.30, 0.40, 0.30, 0.15, 0.20, 1.00, 0.55]
secondary: [0.40, 0.50, 0.45, 0.50, 0.10, 0.50, 0.55, 0.55]
filterStyle: LowPass
why: MOVE + TEX = Wittek's "Lo-fi" preset (vibrato + filter "like turning the tone knob down"); LPF 0.55 ≈ 2.2 kHz.
     CHAR mild Sweeten suits a DI'd parlor acoustic/Tascam source (Yano's Reverb interview, via search) [link inference].
     DIFF chamber keeps the sound dry-ish. OUTPUT/TEX VOL raised slightly to make up for the LPF level loss.
```
```
name: "BLT Gated Room"
description: "Room reverb chopped by SWELL used as a gate; mute strings to kill the tail. Worn tape after."
effects: [SWELL, DOUBLER, SPACE, CASSETTE]
moduleOn: [true, false, true, true]
order: [DIFF, CHAR, MOVE, TEXT]
primary:   [0.45, 0.05, 0.50, 0.00, 0.20, 0.50, 1.00, 0.40]
secondary: [0.35, 0.55, 0.00, 0.50, 0.20, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: DIFF before CHAR(SWELL) = wolfe's documented gated-reverb trick. SWELL AMOUNT 0.05 gives ~45 ms attack and ~110 ms release.
     Low SENS 0.35 makes the gate close early on decays.
     TEX worn cassette at 0.4 nods to Dean Blunt's preference for analog tape (Wikipedia, via search) [link inference].
     MOVE off to keep it bare.
```
```
name: "PRN Wide Drive Lead"
description: "Big doubled lead: hot tube drive, 20 ms stereo double, touch of plate. Pick hard, sustain bends."
effects: [DRIVE, DOUBLER, SPACE, FILTER]
moduleOn: [true, true, true, false]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.58, 0.70, 0.23, 0.50, 0.30, 0.20, 1.00, 0.50]
secondary: [0.55, 0.42, 0.15, 0.50, 0.10, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: Structure = Wittek's "Huge fuzz" (dirt + Doubler for width + a little Space), which he says is usable for "a big driving lead".
     DRIVE swapped in for HOWL to give a cleaner overdrive voice [inference re Prince; defer to PrinceTones].
     CHAR VOL 0.42 trims the drive's level jump (wolfe: "easy to overdrive the modules").
```
```
name: "MSA Verb Into Fuzz"
description: "Slow random vibrato → room reverb → round fuzz → cassette. Reverb tails distort and bloom; play dynamically."
effects: [FUZZ, VIBRATO, SPACE, CASSETTE]
moduleOn: [true, true, true, true]
order: [MOVE, DIFF, CHAR, TEXT]
primary:   [0.35, 0.30, 0.44, 0.25, 0.30, 0.45, 1.00, 0.35]
secondary: [0.45, 0.40, 0.40, 0.50, 0.15, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: DIFF→CHAR→TEX(cassette) is guitar.com's favourite routing and matches Hologram's "process reverb through a fuzz".
     FUZZ TILT 0.35 = round tone with little sputter. VIBRATO 0.8 Hz, ±9 c, with DRIFT for random wobble.
     The artist fit is [inference]; defer to JuniorMesa's facts.
```
(A −5 st PITCH baritone patch is the most common community move: guitar.com, gearnews. It already exists as "MKG Baritone Drop", so there is no duplicate here.)

## 6. Confidence and open questions
- **Confidence:** high for the module tricks (they are primary user statements). Low for any artist mapping: there is zero documented use of the Chroma Console by the five artists. All numeric values are my translations, because no free clock-position settings were found. Wittek's PDF and the Pedal Collaborative pack are paywalled.
- **Open:** the Yvette Young reel (Doubler + Cassette on acoustic, Pitch + Cascade on electric) was cited only through Equipboard search snippets and is unverified.
- **To check in the plugin:** whether WARMFUL's SWELL minimum attack of 40 ms softens plucks too much for "BLT Gated Room". If it does, use a TREMOLO square chop or SQUASH after the SPACE module instead.
- **To check in the plugin:** whether PITCH latency at 0 st really reads as chorus rather than slapback.
