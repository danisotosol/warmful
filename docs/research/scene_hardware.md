# Scene hardware brief: vintage toolkit to WARMFUL

Existing MKG presets already cover the 424, Bowed Pad, Mood, Quadraverb and Rockman ideas (`docs/PRESETS.md`). So the recipes below use the **PRN / DIJ / YNO / MSA** prefixes. The hardware specs are kept for reuse by the other packs.

## 1. Sources
- https://equipboard.com/pros/mk-gee: Mk.gee's VG-8 ("used throughout the new album and frequently live"), 424 under a Chase Bliss MOOD, Rainger Reverb-X, Jaguar. Crowd-sourced.
- https://www.gearnews.com/the-mk-gee-guitar-tone-perfect-match/: GK-2A→VG-8; board has MOOD, Reverb-X, Empress ParaEq, DigiTech Drop, CE-2W, DD-20; 7-string flatwound Chromes set on a Jaguar; CE-2W used in **CE-1 vibrato** mode, "rate pretty slow, low depth".
- https://www.musicradar.com/guitars/guitar-pedals/hes-not-using-a-guitar-amp-hes-using-a-tascam-424-jhs-pedals-puts-a-portastudio-in-a-pedal-to-help-you-recreate-mk-gees-elastic-lo-fi-tones: Mk.gee runs the guitar into a 424 in place of an amp, pushing the preamp.
- https://www.internettattoo.com/blog/mk-gee-guitar-sound-and-tone: third-party guide. Names the VG-8 "Bowed Pad" patch and an Alesis Quadraverb, and suggests decays around 1 s.
- https://www.offsetguitars.com/forums/viewtopic.php?t=128061: concertgoer report: baritone flatwounds, 424 as preamp, "80s throwback chorus", Rainger gated reverb, a VG-8 used by both Mike and Andrew.
- https://www.soundonsound.com/reviews/roland-vg8-0: VG-8 HRM "BOWED" algorithm = "slower attack is used to simulate bowing" with a filter; built-in chorus/delay/reverb; per-patch noise gate.
- https://www.fretboardjournal.com/video/gear-check-the-guitarists-in-dijons-touring-band/: Mk.gee and Ryan Richter in Dijon's touring band (2022). Per Equipboard, Mk.gee names a BF-2 flanger in this video.
- https://reverb.com/news/interview-jonah-yano: Yano demos on a **Tascam 244** and DIs a Yamaha parlor acoustic. *Portrait of a Dog* was cut on a Midas console to Ampex 1″, with a 60s Canadian "Pultec copy" compressor and a hallway plate "the size of a car… in every song". He also says *Absolutely* was recorded with room mics in a "boxy" space.
- https://www.guitarcloud.org/faq/what-effects-pedals-did-prince-use/: Prince's 1999-era board: SD-1, BF-2, DM-2, CE-2, OC-2. The CE-2 was later replaced by a VB-2 vibrato; DD-3 from the 90s; Mu-Tron Bi-Phase in the early years.
- https://www.izotope.com/en/learn/an-interview-with-dr-susan-rogers-part-1-of-3-princes-sound.html: Susan Rogers quoting Jesse Johnson: Prince's "rhythm guitar is always dry, through direct box", panned left. Studio had an API console and an Ampex MM1200; LM-1 percussion ran through Boss pedals (HM-2, distortion, blue delay).
- https://forum.fractalaudio.com/threads/presenting-the-boss-ce-1-moaee-mother-of-all-effect-emulations.59591/: measurements from one CE-1 unit: chorus LFO 1–3 Hz (≈1.75 Hz at noon, triangle); vibrato 3.2–11.6 Hz (sine); delay ≈3–7 ms.
- https://www.electrosmash.com/boss-ce-2-analysis + https://anasounds.com/working-analog-chorus-effect-boss-ce2/: CE-2 triangle LFO from ≈0.3 to 3.5 Hz; MN3007 BBD.
- https://www.diystompboxes.com/smfforum/index.php?topic=120428.0: CE-2 clock ≈100–150 kHz, which works out to ≈3.4–5 ms delay with a "2–3 ms sweep".
- https://www.guitarworld.com/features/how-rolands-jc-120-became-the-king-of-solid-state-guitar-amps: JC-120 chorus ≈10 ms delay; one speaker modulated, one dry.
- https://music-electronics-forum.com/forum/amplification/guitar-amps/maintenance-troubleshooting-repair/969356-roland-jc120-vibrato-problem: a technician puts the JC-120 fixed chorus LFO at ≈0.8 Hz. The JC-120 manual (https://www.manualslib.com/guide/3614560/roland-jazz-chorus-jc-120-manual.html) says the chorus is not adjustable.
- https://help.uaudio.com/hc/en-us/articles/33538271334164-Roland-RE-201-Space-Echo-Manual: RE-201 heads: H1 69–177 ms, H2 131–337 ms, H3 189–489 ms. Wow/flutter rises as the tape wears.
- https://articles.boss.info/space-echo-sounds-and-how-to-use-them/: BOSS calls the original RE-201's wow/flutter "modest".
- https://www.tascam.eu/en/docs/424mkIII_manual.pdf + https://www.manualslib.com/manual/1260447/Tascam-Portastudio-424.html?page=25: 424 W&F 0.05 % WRMS at 9.5 cm/s; 40 Hz–16 kHz ±3 dB; pitch control ±12 %.
- https://cosmod.io/products/tape-jockey-jhs-424-mod: Portastudio pitch-wheel tricks give "slow pitch warped bends, vibrato and chorus".
- https://juniormesa.bandcamp.com/album/cirque-du-freak + https://northerntransmissions.com/junior-mesa-debuts-video-for-risen-up/: Junior Mesa self-engineered the record and played guitar on it. Quote: "pushing their tape recorders to their limits" (about the era he admires). **No gear named.**

## 2. Verified artist ↔ hardware attributions
| Hardware | Artist | Status |
|---|---|---|
| Roland VG-8 + GK pickup (HRM "Bowed") | Mk.gee (and Andrew Aged) | Photos and live footage (Equipboard, Gearnews, OffsetGuitars). Patch name from a third-party guide only. |
| Boss CE-2W (CE-1 vibrato mode) | Mk.gee | Board photo (Gearnews). |
| Boss CE-2 → VB-2 | Prince | 1999 tour, then replaced by VB-2 (Guitarcloud). |
| Boss DM-2 → DD-3 | Prince | Guitarcloud. |
| Boss DD-20 | Mk.gee | Gearnews. |
| Boss BF-2 | Mk.gee (Dijon band, 2022) and Prince | Equipboard citing the Fretboard Journal video; Guitarcloud. |
| Tascam 424 | Mk.gee | Wikipedia, Equipboard, MusicRadar. |
| Tascam 244 | Jonah Yano | His own words (Reverb). |
| Chase Bliss MOOD | Mk.gee | Equipboard, Gearnews. |
| Alesis Quadraverb | Mk.gee | Third-party guide only: low confidence. |
| Fairfield Shallow Water | Dijon | Equipboard, from an Instagram pedalboard photo. Not opened directly: the page was 403. |
| Plate reverb, Ampex 1″, Pultec-type compressor | Jonah Yano (*Portrait of a Dog*) | His own words. |
| Roland JC-120 / JC-40 | Mk.gee: JC-40 on Equipboard (secondary) | — |
| Roland Space Echo, Eventide harmonizers, Lexicon/AMS reverbs | **No sourced link to any of the five artists.** | Prince used a Roland GP-16, Zoom 9010/9030 and Whammy for pitch shifting (Guitarcloud), not an Eventide. |
| Any pedal | Junior Mesa | None found. |

## 3. Measurable signature mapped to WARMFUL
- **CE-1 chorus:** 1–3 Hz triangle over a ≈3–7 ms delay. A ±1 ms triangle sweep at 1 Hz gives ≈±7 cents **[inference: derived from the slope]**. → VIBRATO RATE 0.48 (1 Hz) to 0.60 (1.75 Hz), AMOUNT 0.2 (±7 c), MIX 0.5–0.6 so the dry signal makes it a chorus.
- **CE-1 vibrato:** 3.2–11.6 Hz, sine. "Slow and low" for Mk.gee means the bottom of that range **[inference]** → RATE 0.73–0.77 (3.2–4 Hz), AMOUNT 0.2–0.3, MIX 1.
- **CE-2:** 0.3–3.5 Hz over ≈4–5 ms with a 2–3 ms sweep → RATE 0.23–0.76, AMOUNT 0.2–0.3.
- **JC-120 chorus:** fixed ≈0.8 Hz, ≈10 ms, dry speaker plus wet speaker → RATE 0.44, AMOUNT 0.22, MIX 0.5.
- **RE-201 Space Echo:** H1 69–177 ms → REELS TIME 0.12–0.48; H2 to 337 ms → 0.72; H3 to 489 ms → 0.86. Wow is "modest" when the tape is fresh → DRIFT 0.2–0.35.
- **Boss DM-2:** ≈20–300 ms analog **[inference: spec not opened]** → CASCADE TIME up to 0.72. **DD-3:** 12.5–800 ms digital, clean repeats **[inference]** → REELS with DRIFT 0.
- **424 / 244 cassette:** W&F 0.05 % WRMS, which is ≈0.9 cents, so nearly inaudible. The "wobble" comes from the pitch knob (±12 %) and from worn decks. Bandwidth is 40 Hz–16 kHz. → CASSETTE 0.1–0.3 (clean deck, slight head bump). The 424's character is preamp clipping (MusicRadar), so it belongs in CHAR (DRIVE/SWEETEN), not in tape wow.
- **VG-8 Bowed:** a slow attack plus a filter → SWELL AMOUNT 0.3–0.5 (≈150–400 ms attack), TILT < 0.5.
- **Plates and halls:** no decay times are sourced. Use SPACE TIME ≈0.3 for a large plate of ≈1.5–2 s **[inference]**, and ≈1 s for the Quadraverb-style room (third-party).
- **Harmonizer detune:** the classic ±7–10 cent micro-shift doubling has no sourced value **[inference]** → DOUBLER RATE 0.15–0.23 (12–20 ms) with DRIFT, or VIBRATO AMOUNT 0.25 and DRIFT 0.6.

## 4. Reference songs
- Prince, "1999" / "Little Red Corvette" (1982): dry DI rhythm guitar panned left (Rogers); the 1999-era board was SD-1/CE-2/DM-2.
- Prince, "When Doves Cry" (1984): dry DI rhythm and an API/MM1200 chain **[inference: song-level guitar chain unsourced]**.
- Mk.gee, "Are You Looking Up" (2024): GK pickup visible in the Kimmel performance (Equipboard); VG-8 pad textures.
- Jonah Yano, *Portrait of a Dog* (2022), e.g. "always": room bleed, a big plate on every song, 1″ tape.
- Dijon, *Absolutely* (2021): boxy room-mic recording (Yano's description).
- Junior Mesa, "Risen Up" (2021): self-engineered, 60s–70s tape aesthetic.

## 5. Recipes
```
name: "PRN DI Rhythm CE-2"
description: "Dry, bright direct-box funk rhythm with a light CE-2 shimmer. Play 16ths on the bridge pickup, mute hard."
effects: [SWEETEN, VIBRATO, SPACE, SQUASH]
moduleOn: [true, true, false, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.60, 0.35, 0.52, 0.20, 0.10, 0.00, 0.55, 0.35]
secondary: [0.50, 0.50, 0.05, 0.50, 0.00, 0.30, 0.50, 0.50]
filterStyle: Tilt
why: CHAR SWEETEN as an API-console DI (rhythm "always dry, through direct box", Rogers/iZotope); TILT 0.6 for the bright DI.
why: MOVE VIBRATO 1.2 Hz ±8 c at MIX 0.55 = CE-2 chorus (0.3–3.5 Hz, ≈5 ms BBD; CE-2 on the 1999 board, Guitarcloud).
why: DIFF off, because the DI rhythm stays dry.
why: TEX SQUASH 0.35 for the even, tight funk dynamics [inference].
```
```
name: "PRN SD1 Corvette Echo"
description: "SD-1 crunch through a CE-2 and a dark DM-2 slap. Lead lines and bends on the neck pickup."
effects: [DRIVE, VIBRATO, CASCADE, FILTER]
moduleOn: [true, true, true, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.60, 0.50, 0.60, 0.22, 0.62, 0.30, 0.75, 0.50]
secondary: [0.55, 0.45, 0.05, 0.50, 0.15, 0.40, 0.50, 0.50]
filterStyle: Tilt
why: CHAR DRIVE 0.5, mid-gain and bright = SD-1 (1999 board, Guitarcloud).
why: MOVE VIBRATO 1.75 Hz ±8 c at MIX 0.75 = CE-2 sitting in front of the delay.
why: DIFF CASCADE ≈230 ms with low feedback = DM-2 BBD slap (dark repeats); DM-2 replaced by DD-3 later.
why: TEX FILTER at neutral tilt; trim the top if the drive gets fizzy.
```
```
name: "DIJ Shallow Room"
description: "Wandering random-mod chorus in a small boxy room. Soft fingerpicked chords; let them ring."
effects: [SWEETEN, VIBRATO, SPACE, CASSETTE]
moduleOn: [true, true, true, true]
order: [CHAR, MOVE, DIFF, TEXT]
primary:   [0.42, 0.40, 0.44, 0.35, 0.12, 0.30, 0.80, 0.20]
secondary: [0.50, 0.50, 0.70, 0.50, 0.15, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: CHAR SWEETEN, slightly dark: warm close-miked feel in a boxy home studio (Yano on Absolutely, Reverb).
why: MOVE VIBRATO 0.8 Hz ±16 c with DRIFT 0.7 (random) = Fairfield Shallow Water style unstable modulation (on Dijon's board per Equipboard; random-LFO behaviour [inference]).
why: DIFF SPACE TIME 0.12 = small chamber/room around 0.6 s ("a lot of room mics… boxy").
why: TEX CASSETTE 0.2 = clean-deck head bump and gentle HF loss.
```
```
name: "YNO 244 Car Plate"
description: "DI acoustic or clean electric into a 244, a big hallway plate and 1-inch glue. Slow ballads."
effects: [SWEETEN, VIBRATO, SPACE, CASSETTE]
moduleOn: [true, true, true, true]
order: [CHAR, MOVE, TEXT, DIFF]
primary:   [0.45, 0.45, 0.34, 0.10, 0.32, 0.35, 1.00, 0.25]
secondary: [0.50, 0.50, 0.60, 0.50, 0.10, 0.50, 0.50, 0.50]
filterStyle: Tilt
why: CHAR SWEETEN 0.45 = Pultec-copy compressor plus console warmth; "we didn't really compress anything" else (Yano, Reverb).
why: MOVE VIBRATO 0.5 Hz ±2 c with DRIFT 0.6 = barely audible tape wow (424/244 class W&F 0.05 %, ≈1 c).
why: TEX CASSETTE 0.25 before the reverb = Tascam 244 DI demos and 1-inch Ampex tracking.
why: DIFF SPACE TIME 0.32 (≈1.5–2 s plate) at 35 % = hallway plate "the size of a car… in every song".
```
```
name: "MSA Pushed Tape Echo"
description: "70s glam-funk guitar into a deck pushed to its limits, with wobbly three-head-style echo. Chunky chords."
effects: [DRIVE, PHASER, REELS, CASSETTE]
moduleOn: [true, true, true, true]
order: [CHAR, MOVE, TEXT, DIFF]
primary:   [0.55, 0.35, 0.30, 0.30, 0.61, 0.40, 0.85, 0.50]
secondary: [0.55, 0.50, 0.20, 0.50, 0.30, 0.40, 0.50, 0.50]
filterStyle: Tilt
why: CHAR DRIVE 0.35 = light 70s amp grit; self-engineered record (Bandcamp credits) [gear inference].
why: MOVE PHASER at slow 0.4 Hz with few stages = 70s glam-funk colour [inference: no pedal sourced].
why: TEX CASSETTE 0.5 = "pushing their tape recorders to their limits" (Mesa, Northern Transmissions).
why: DIFF REELS ≈250 ms (inside RE-201 head 2, 131–337 ms) with DRIFT 0.3 = modest Space Echo wow [Space Echo use inference].
```

## 6. Confidence and open questions
- **High:** Prince's board (Guitarcloud) and dry DI rhythm (Rogers); Mk.gee's VG-8/424/CE-2W/MOOD/DD-20 (multiple photo-based sources); Yano's 244 and plate (his own words); the CE-1/CE-2/RE-201/424 numeric specs (measurements and manuals).
- **Medium:** the "Bowed Pad" patch name and the Quadraverb (one third-party guide); Dijon's Shallow Water (Equipboard snippet, page not opened).
- **Low or none:** any Junior Mesa gear; any artist link for Eventide, Lexicon or AMS units; the Space Echo for any of these artists. The DM-2/DD-3 ranges and harmonizer cents are from memory, not opened sources.
- **Open:**
  - Equipboard pages for Dijon and Prince returned 403.
  - The Dijon *Absolutely* Reverb interview was not readable.
  - Goldenvoice Podcast and VMP interviews with Mesa may name gear.
  - The VG-8 Bowed patch's attack time in ms is undocumented.
