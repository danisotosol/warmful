# Gated sounds — research brief for WARMFUL (noise gates, gated reverb, gated/distorted reverb, rhythmic chop, starved fuzz)

## 1. Sources
- https://www.soundonsound.com/techniques/classic-tracks-prince-kiss — David Z (primary): the "Kiss" guitar was gated to a hi-hat trigger; "guitar was dry and gated"; kick through RMX16 Reverse 2.
- https://reverb.com/news/how-the-ams-rmx16-defined-a-generation-of-drum-sounds — Susan Rogers: Prince had her put RMX16 "Non Lin 2" on the LM-1; used all over *Purple Rain*.
- https://media.uaudio.com/support/manuals/dd/AMS%20RMX16%20Manual.pdf — RMX16 manual (UA): NONLIN 2 behaviour; decay in 0.1 s steps, "not calibrated". Read via search excerpts only.
- https://tapeop.com/reviews/gear/136/rmx16-500-series-digital-reverb — Tape Op: Nonlin2 = AMS's take on Padgham/Collins; 16-bit, 18 kHz bandwidth; no tail modulation.
- https://musictech.com/features/interviews/hugh-padgham-on-gated-reverb-effect-and-evolving-technology/ — Padgham (primary): heavily compressed listen mic + stone room + gate timing.
- https://www.vintagedigital.com.au/blog/gated-reverb-explained/ — typical gated-reverb settings (RT, pre-delay, HPF, hold/release practice).
- https://www.raingerfx.com/shop/p/reverb-x — Rainger FX Reverb-X maker page and FAQ: topology, gate behaviour, controls.
- https://www.guitarpedalx.com/news/gpx-blog/rainger-fxs-reverb-x-mini-has-picked-up-a-cult-following-since-dijons-touring-band-guitarist-mkgee-revealed-how-he-uses-it — Reverb-X's popularity after the Dijon touring-band gear check. Video: https://www.fretboardjournal.com/video/gear-check-the-guitarists-in-dijons-touring-band/
- https://www.raingerfx.com/artists/mkgee — Rainger lists Mk.Gee as a Reverb-X and Echo-X user.
- https://www.gearnews.com/the-mk-gee-guitar-tone-perfect-match/ — secondary: "gated, distorted reverbs" in the Mk.gee tone; board includes Reverb-X, CE-2W, Mood, DigiTech Drop.
- https://static.roland.com/assets/media/pdf/NS-2_e01_W.pdf — Boss NS-2 manual: threshold and decay controls, no time specs.
- https://rolandcorp.com.au/blog/how-to-use-boss-ns-2-noise-suppressor-in-your-rig — Roland: a fast NS-2 decay after the gain stage gives a "choppy" sound and cuts off decaying notes.
- https://www.musicradar.com/how-to/the-fx-files-zvex-fuzz-factory — the Fuzz Factory "Gate" control is really transistor bias, which causes sputter and "glitchy cut-outs"; the "velcro" tone.
- https://www.guitarworld.com/features/the-secrets-behind-princes-tone-on-lets-go-crazy — Prince's Let's Go Crazy rig with settings.
- https://www.guitarcloud.org/faq/what-effects-pedals-did-prince-use/ — Prince's Boss pedal history.
- https://www.onestowatch.com/en/blog/junior-mesas-anti-pop-prose-is-infectious-qa — Junior Mesa's influences include Hendrix and Sly Stone.

## 2. Verified facts
- **Where gated reverb came from.** Padgham's talkback ("listen") mic had "a very heavy compressor on it" and picked up the stone room. He timed the gate to close "a second and a half after he hit the drum, then get it to close quickly", and asked Collins for a pattern that "would work with the noise gate closing" ([MusicTech](https://musictech.com/features/interviews/hugh-padgham-on-gated-reverb-effect-and-evolving-technology/)). So it is **compressed ambience, then a hard close**.
- **RMX16 NONLIN 2.** "The sound decays only very slightly for a period of time (set by the decay control)." Decay moves in 0.1 s steps but is "not calibrated" in this mode. The related Image program runs from a few ms to about 0.6 s ([UA manual](https://media.uaudio.com/support/manuals/dd/AMS%20RMX16%20Manual.pdf)). Gated reverb "increases in volume upon impact", giving "a hollow effect with a punchy, back-end kick" ([Reverb.com](https://reverb.com/news/how-the-ams-rmx16-defined-a-generation-of-drum-sounds)). It is 16-bit with an 18 kHz bandwidth and has no tail modulation ([Tape Op](https://tapeop.com/reviews/gear/136/rmx16-500-series-digital-reverb)).
- **Typical gated-reverb numbers.** Bright room or plate of **0.6–1.0 s**, pre-delay **0–20 ms**, fast attack, a short hold, a quick release, and HPF of about **120 Hz** on the return ([Vintage Digital](https://www.vintagedigital.com.au/blog/gated-reverb-explained/)).
- **Prince.** He used Non Lin 2 on the LM-1 throughout *Purple Rain*. Rogers: "We really kind of used it to death… by *Sign 'O' the Times* I was pretty sick of it" ([Reverb.com](https://reverb.com/news/how-the-ams-rmx16-defined-a-generation-of-drum-sounds)).
- **"Kiss" (David Z).** He ran the LinnDrum hi-hat through a delay. Then: "I took an acoustic guitar, played these open chords and gated that to the hi-hat trigger… unbelievably funky but also impossible to actually play." Later: "The guitar was dry and gated… no reverb on anything else; just the kick." Prince added a JB-style guitar lick and removed the bass ([SOS](https://www.soundonsound.com/techniques/classic-tracks-prince-kiss)). This is a **keyed rhythmic gate**: 100 % depth, 16th-note grid, dry.
- **Prince's live rig for Let's Go Crazy.** MadCat Tele copy (bridge pickup), Mesa Mark IIB clean, Boss DS-1 (Dist 9, Tone 6), OC-2, BF-2 ([Guitar World](https://www.guitarworld.com/features/the-secrets-behind-princes-tone-on-lets-go-crazy)). His Boss board came in from *1999* on: SD-1, BF-2, DM-2, CE-2, OC-2 ([Guitarcloud](https://www.guitarcloud.org/faq/what-effects-pedals-did-prince-use/)). **No noise gate is documented** on his board; his gating was done in the studio.
- **Rainger Reverb-X.** Digital reverb up to **6 s**, with a full-on overdrive **after** the reverb that is blended in. The **dry signal is always present and stays clean**. The noise gate sits **after the reverb and distortion**: it "closes hard when you stop, cutting off any reverb… like a hard-edited 'sample'"; on a long riff it stays open. The gate's close speed is **not adjustable**; its threshold is set by the **Input** knob, which also sets how much the reverb distorts. There is a **Dark** switch, and the Igor pad punches the reverb send in and out ([Rainger](https://www.raingerfx.com/shop/p/reverb-x), [GPX](https://www.guitarpedalx.com/news/gpx-blog/rainger-fxs-reverb-x-mini-has-picked-up-a-cult-following-since-dijons-touring-band-guitarist-mkgee-revealed-how-he-uses-it)).
- **Who uses the Reverb-X.** Mk.Gee, guitarist in Dijon's touring band, uses it ([Rainger artists](https://www.raingerfx.com/artists/mkgee)). Gearnews calls "gated, distorted reverbs" part of his tone ([gearnews](https://www.gearnews.com/the-mk-gee-guitar-tone-perfect-match/)).
- **Boss NS-2.** It is an expander that acts below a threshold. It has THRESHOLD and DECAY knobs ("normally set DECAY to MIN"); the manual gives **no ms specs** ([manual](https://static.roland.com/assets/media/pdf/NS-2_e01_W.pdf)). Placed after a gain stage with a fast decay it "clamps down", cutting the end of notes, and gives a "choppy" sound ([Roland AU](https://rolandcorp.com.au/blog/how-to-use-boss-ns-2-noise-suppressor-in-your-rig)).
- **Velcro / gated fuzz.** The Fuzz Factory "Gate" control actually rebiases a transistor, which causes "sputtering guitar signal and glitchy cut-outs". The splatty "velcro" tone comes from turning Gate clockwise ([MusicRadar](https://www.musicradar.com/how-to/the-fx-files-zvex-fuzz-factory)).
- **Not verified:** ISP Decimator timings, and any gate use by Dean Blunt, Jonah Yano or Junior Mesa. I found no sources for these.

## 3. Sonic signature in measurable terms
- **Gated room (NonLin/Padgham):** pre-delay 0–20 ms; dense ambience held nearly flat (heavy compression), then a cut within about 50–100 ms **[inference]**. Total length is 0.3–0.6 s on the RMX (Image range) and up to about 1.5 s in Padgham's live version. Bright, HPF about 120 Hz, no modulation, mono-ish.
- **Keyed rhythmic gate (Kiss):** 100 % depth and square shape. The rate is a 16th-note grid, Hz = BPM/15 (110 BPM → 7.3 Hz **[example tempo]**). Dry, no reverb, no low end (no bass on the record).
- **Reverb-X gate mode:** reverb up to 6 s, distorted after the reverb, clean dry in parallel, and a hard un-adjustable close set by input level. Dark mode means a low-passed tail.
- **Starved fuzz:** bias-starved transistors; the note decay turns into sputter and then silence. Thin, brash tone.
- **Stutter:** gate repeats every 10–40 ms (packet-loss range) up to 1/16 note; chop depth 80–100 %.

## 4. Reference songs
- **Prince – "Kiss" (1986):** dry acoustic chords chopped by a hi-hat-keyed gate; reverb (RMX Reverse 2) on the kick only ([SOS](https://www.soundonsound.com/techniques/classic-tracks-prince-kiss)).
- **Prince – "When Doves Cry" (1984):** Non Lin 2 on the LM-1 snare ([Reverb.com](https://reverb.com/news/how-the-ams-rmx16-defined-a-generation-of-drum-sounds)).
- **Prince – "Let's Go Crazy" (1984):** DS-1 + OC-2 rhythm, BF-2 accents, Non Lin 2 drums ([GW](https://www.guitarworld.com/features/the-secrets-behind-princes-tone-on-lets-go-crazy)).
- **Phil Collins – "In the Air Tonight" (1981):** the reference for compressed room plus gate ([MusicTech](https://musictech.com/features/interviews/hugh-padgham-on-gated-reverb-effect-and-evolving-technology/)).
- **Mk.gee – *Two Star & The Dream Police* (2024):** distorted, gated reverb, Reverb-X style ([gearnews](https://www.gearnews.com/the-mk-gee-guitar-tone-perfect-match/)).

## 5. How to build each gate in WARMFUL (from the reference doc)
- **Hard gate after reverb or distortion** = put DIFF before CHAR **FUZZ** with TILT at 0.62–0.70. The starved bias sputters and then cuts as the signal falls. SENSITIVITY plays the role of the threshold: lower SENS closes the gate earlier, like the Reverb-X Input knob.
- **Nonlin envelope** = a very short SPACE (TIME 0–0.15, roughly 0.4–0.8 s **[assumes exponential morph between voicings]**) into **SQUASH** at 0.65–0.71. The tail holds flat above the −38 dB threshold. Below it, the 0.4 s RT60 decay rate (about 150 dB/s) drops it out in about 100–200 ms **[inference]**.
- **Keyed or trance gate** = **TREMOLO**, AMOUNT ≥ 0.85 (square). RATE = ln(Hz/0.1)/ln 120: 4 Hz → 0.77, 6 Hz → 0.855, 7.3 Hz → 0.897, 10 Hz → 0.96. Tempo-sync to lock it to a host.
- **Envelope gate / NS-2 "clamp"** = **SWELL** at AMOUNT 0–0.1 gives about 40–60 ms attack and 100–140 ms release **[assumes both are exponential in AMOUNT]**. Caveat: the 40 ms minimum attack softens picks, so use it for "pad gate" sounds, not funk.
- **Stutter** = **INTERFERENCE** at 0.45–0.6 (10–40 ms packet-loss repeats), optionally with a **COLLAGE** loop at TIME ≈ 0.37 (≈136 ms, a 16th at 110 BPM).
- **The Reverb-X "dry always clean" trait** = DIFF AMOUNT 1 (wet only) with global **MIX < 1**, so the clean input runs in parallel.

## 6. Recipes
```
name: "PRN Kiss Gate"
description: "Dry clean chords chopped by a hard 16th gate (~110 BPM). Strum open chords; tap-sync to lock to a groove."
effects: [SWEETEN, TREMOLO, SPACE, FILTER]
moduleOn: [true, true, false, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.60, 0.35, 0.897, 1.00, 0.00, 0.00, 1.00, 0.58]
secondary: [0.50, 0.50, 0.00, 0.56, 0.00, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: CHAR SWEETEN light comp/presence = dry, upfront console guitar ("guitar was dry and gated", SOS)
why: MOVE TREMOLO 7.3 Hz, full depth = square, zero drift = machine gate keyed to the LinnDrum hi-hat trigger (David Z)
why: DIFF off = "no reverb on anything else; just the kick"
why: TEX FILTER tilt 0.58 = gentle HPF, because Kiss has no bass line; open chords stay thin
```
```
name: "PRN Nonlin Stab"
description: "Funk stabs with a short, flat, chopped room like RMX16 Non Lin 2. Play staccato; the room stops dead."
effects: [DRIVE, DOUBLER, SPACE, SQUASH]
moduleOn: [true, false, true, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.62, 0.28, 0.23, 0.00, 0.05, 0.45, 1.00, 0.70]
secondary: [0.50, 0.50, 0.00, 0.50, 0.00, 0.60, 0.45, 0.50]
filterStyle: Tilt
why: CHAR DRIVE low, bright tilt = Mesa Mark IIB set clean, bridge single-coil bite (GW)
why: DIFF SPACE TIME 0.05 (~0.45 s chamber), no drift = short, unmodulated RMX-style ambience (Tape Op; 0.3–0.6 s range)
why: TEX SQUASH 0.70 (~ratio 16, deep threshold, just below the tanh drive) = "decays only very slightly", then drops fast (NONLIN 2 manual / Padgham's compressed listen mic)
```
```
name: "DIJ Gated Bootleg"
description: "Reverb-X style: a clean dry note plus a distorted 4 s reverb that sputters shut when you stop. Play phrases, then leave gaps."
effects: [FUZZ, VIBRATO, SPACE, FILTER]
moduleOn: [true, true, true, true]
order: [DIFF, CHAR, MOVE, TEXT]
primary:   [0.66, 0.45, 0.44, 0.18, 0.62, 1.00, 0.55, 0.40]
secondary: [0.42, 0.40, 0.30, 0.50, 0.10, 0.60, 0.50, 0.50]
filterStyle: Tilt
why: DIFF SPACE 100 % wet before CHAR = the Reverb-X overdrive placed after the reverb; TIME 0.62 is about 4 s, within the 6 s max (Rainger)
why: CHAR FUZZ TILT 0.66 starved = the post-reverb gate "closes hard… like a hard-edited sample"; SENS 0.42 = Input knob setting both drive and gate threshold
why: MIX 0.55 = "the original guitar signal is always present", undistorted (Rainger FAQ)
why: MOVE VIBRATO 0.8 Hz ±6c with drift = slow CE-2W wobble on Mk.gee's board (gearnews)
why: TEX FILTER tilt 0.40 = the Reverb-X Dark switch's low-passed tail
```
```
name: "MSA Velcro Sputter"
description: "Starved, splatty fuzz whose note tails choke into crackle. Dig in for sustain; ease off and it falls apart."
effects: [FUZZ, DOUBLER, SPACE, FILTER]
moduleOn: [true, false, true, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.78, 0.70, 0.23, 0.00, 0.00, 0.18, 1.00, 0.44]
secondary: [0.40, 0.38, 0.00, 0.50, 0.00, 0.45, 0.50, 0.50]
filterStyle: Tilt
why: CHAR FUZZ TILT 0.78 = bias-starved sputter plus blown-speaker splat, the "velcro" Fuzz Factory Gate setting (MusicRadar); low SENS = more choke on decays
why: DIFF SPACE chamber at low mix = a small-room ambience that the sputter cuts short
why: TEX FILTER 0.44 = slight LPF to tame the fizz; fits the Hendrix/Sly fuzz lineage Junior Mesa cites [inference; also usable as BLT]
```
```
name: "BLT Packet Stutter"
description: "Trance-gate meets dropped-call codec: 10 Hz chop, 136 ms micro-loop, packet-loss glitches. Sustain chords."
effects: [SWEETEN, TREMOLO, COLLAGE, INTERFERENCE]
moduleOn: [true, true, true, true]
order: [CHAR, DIFF, MOVE, TEXT]
primary:   [0.45, 0.30, 0.96, 0.90, 0.37, 0.35, 1.00, 0.52]
secondary: [0.50, 0.50, 0.25, 0.55, 0.20, 0.45, 0.50, 0.50]
filterStyle: Tilt
why: DIFF COLLAGE 136 ms loop before MOVE = 16th repeats (at 110 BPM) that the gate then chops too
why: MOVE TREMOLO 10 Hz square with drift 0.25 = an irregular trance gate
why: TEX INTERFERENCE 0.52 = 10–40 ms packet-loss repeats and codec crush (WARMFUL spec)
why: Dean Blunt fit is [inference]: no sourced gate use; the preset pack would need confirmation from the BLT research
```

## 7. Confidence and open questions
- **High:** the Kiss gate (David Z primary), Prince's use of Non Lin 2 (Rogers), the Reverb-X topology (maker FAQ), Padgham's method, and the velcro bias mechanism.
- **Medium:**
  - NonLin lengths: the RMX decay is uncalibrated.
  - SPACE TIME-to-seconds interpolation.
  - SWELL attack/release curve.
  - Whether WARMFUL's FUZZ sputter closes as abruptly as the Reverb-X gate. Needs a listening test: tune TILT between 0.62 and 0.70 and SENS between 0.35 and 0.5.
- **Low:** artist fit for the MSA and BLT recipes. Also open: the exact Kiss tempo and the Dijon studio gate usage. I found no source for either.
- **Open:**
  - Measure the RMX16 Non Lin 2 gate length per decay unit (impulse test).
  - Check whether TREMOLO AMOUNT 1.0 fully mutes the off-phase. Kiss needs 100 %.
  - Get the Mk.gee settings from the gear-check video at 10:31, which is unwatched.
