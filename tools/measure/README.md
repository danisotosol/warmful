# Matching WARMFUL to a real pedal

The firmware is closed, so the only ground truth is your own unit. This kit re-amps a test signal
through the pedal and compares it, metric by metric, with WARMFUL rendering the same signal with the
same settings. Requires [`uv`](https://docs.astral.sh/uv/) (dependencies install themselves) and
`warmful_render` (`cmake -S . -B build-tools -DWARMFUL_BUILD_PLUGIN=OFF && cmake --build build-tools`).

## 1. Generate the test signal (once)

```sh
uv run tools/measure/warmful_measure.py signal test.wav     # ~75 s, 48 kHz float + test.json
```

It contains a sync chirp, an impulse (echo time, feedback, reverb decay), log sweeps at −30/−18/−6 dBFS
(frequency response + harmonics 2–5), a 220 Hz tone (AM/FM rate & depth, pitch ratio), 1 kHz bursts
from −42 to 0 dBFS (drive/compression transfer curve) and synthetic strums.

## 2. Record

1. Interface out → pedal **INPUT L**, pedal **OUTPUT L + R** → two interface inputs. Disable any
   processing on the interface. Keep the same output/input gain for every recording.
2. Set the pedal to **TRUE BYPASS** (GLOBAL), play `test.wav`, record → `loopback.wav`. This measures
   the interface gain so recordings can be put back on the digital scale.
3. Set the pedal's calibration **manually** (e.g. MEDIUM) and use the same `--cal` in the render.
4. For each sound: set the pedal (one module on, MIX 100 %, secondaries at their defaults unless
   you are measuring them), play `test.wav`, record stereo. Note the knob positions as 0..1
   (7 o'clock = 0, 12 o'clock = 0.5, 5 o'clock = 1). Name the file after the settings.

## 3. Compare

```sh
uv run tools/measure/warmful_measure.py compare pedal_drive_a05.wav --loopback loopback.wav \
    --render "--char 0 --char-amount 0.5 --tilt 0.5 --cal 1" --plot drive_a05.png --json drive_a05.json
```

`--render` takes `warmful_render render` options: `--char/--move/--diff/--tex <0-4|off>`, the 8 knobs
(`--tilt --char-amount --rate --move-amount --time --diff-amount --mix --tex-amount`), `--sens`,
`--drift-move`, `--drift-diff`, `--out`, `--cal 0-3`, `--bpm N --sync`, `--order 0123`,
`--filter lpf|tilt|hpf`. Effects not named are off.

`analyse` prints the same metrics for a single recording.

## 4. Tune

Each effect's `.cpp` starts with `namespace voicing { … }`. Typical mapping from metric to constant:

| Metric off | Look at |
|---|---|
| 1 kHz transfer curve (gain vs level) | drive/compressor gains, `calibration::kGainDb`, SENSITIVITY range |
| Response shape / harmonics | tilt spans and pivots, pre/post filters, waveshaper bias |
| `first_echo_ms` vs TIME | min/max delay times of CASCADE / REELS / COLLAGE |
| `echo_ratio_db` vs AMOUNT | feedback law |
| `rt60_s` vs TIME | SPACE voicing table |
| `am_rate_hz`, `fm_rate_hz`, depths | LFO ranges of VIBRATO / TREMOLO / PHASER / DRIFT |
| `pitch_ratio` | PITCH / REVERSE speed law |

Re-run `warmful_render test` after changing constants to keep the behavioural checks green.
