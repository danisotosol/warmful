#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy", "scipy", "soundfile", "matplotlib"]
# ///
# WARMFUL — measure the physical pedal and compare it with the plug-in.
# SPDX-License-Identifier: CC-BY-NC-SA-4.0
"""
Workflow (see tools/measure/README.md):
  1. uv run tools/measure/warmful_measure.py signal test.wav
  2. Play test.wav out of your interface into the pedal, record the pedal's stereo output
     (and once, a loopback with the pedal in TRUE BYPASS: loopback.wav).
  3. uv run tools/measure/warmful_measure.py compare pedal_drive.wav --loopback loopback.wav \
        --render "--char 0 --char-amount 0.5 --tilt 0.5 --cal 1"
     The same test signal is rendered through warmful_render with those settings and both
     results are analysed side by side (frequency response, harmonics, transfer curve,
     echo time/feedback, reverb decay, modulation rate/depth, pitch).
"""

import argparse
import json
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy import signal

SR = 48000          # the pedal's converter rate
SWEEP_SECONDS = 5.0
SWEEP_LEVELS_DB = (-30.0, -18.0, -6.0)
STEP_LEVELS_DB = tuple(np.arange(-42.0, 0.1, 3.0))


# ---- test signal ----------------------------------------------------------------------------

def db_to_gain(db):
    return 10.0 ** (db / 20.0)


def log_sweep(seconds, f0=20.0, f1=20000.0, sr=SR):
    t = np.arange(int(seconds * sr)) / sr
    k = np.log(f1 / f0)
    sweep = np.sin(2 * np.pi * f0 * seconds / k * (np.exp(t * k / seconds) - 1.0))
    fade = int(0.01 * sr)
    sweep[:fade] *= np.linspace(0, 1, fade)
    sweep[-fade:] *= np.linspace(1, 0, fade)
    return sweep


def inverse_sweep(sweep, seconds, f0=20.0, f1=20000.0, sr=SR):
    t = np.arange(len(sweep)) / sr
    k = np.log(f1 / f0)
    return sweep[::-1] * np.exp(-t * k / seconds)


def sync_chirp(sr=SR):
    t = np.arange(int(0.25 * sr)) / sr
    c = signal.chirp(t, 200.0, t[-1], 8000.0, method="logarithmic")
    return c * np.hanning(len(c)) * db_to_gain(-12.0)


def strums(seconds, peak_db, sr=SR, seed=1234):
    """Karplus-Strong chords (same idea as warmful_render's test guitar)."""
    rng = np.random.default_rng(seed)
    n = int(seconds * sr)
    out = np.zeros(n)
    chords = [(82.4, 110.0, 146.8, 196.0, 246.9, 329.6), (110.0, 164.8, 220.0, 277.2, 329.6, 440.0)]
    for s, start_t in enumerate(np.arange(0.0, seconds - 0.5, 1.6)):
        for i, f in enumerate(chords[s % 2]):
            period = max(2, int(sr / f))
            line = rng.uniform(-1, 1, period)
            start = int((start_t + i * 0.012) * sr)
            length = min(n - start, int(2.5 * sr))
            buf = np.empty(length)
            prev = 0.0
            for j in range(length):
                cur = line[j % period]
                line[j % period] = 0.4985 * (cur + prev) + 0.0015 * cur
                prev = cur
                buf[j] = cur
            out[start:start + length] += buf * 0.25
    return out / np.max(np.abs(out)) * db_to_gain(peak_db)


def build_signal():
    parts, manifest, cursor = [], {}, 0

    def add(name, x, gap=1.0):
        nonlocal cursor
        manifest[name] = {"start": cursor, "length": len(x)}
        parts.append(x)
        parts.append(np.zeros(int(gap * SR)))
        cursor += len(x) + int(gap * SR)

    add("lead_in", np.zeros(int(0.5 * SR)), gap=0.0)
    add("sync", sync_chirp(), gap=1.5)
    impulse = np.zeros(int(4.0 * SR))
    impulse[0] = db_to_gain(-6.0)
    add("impulse", impulse, gap=0.5)
    for level in SWEEP_LEVELS_DB:
        add(f"sweep_{int(level)}", log_sweep(SWEEP_SECONDS) * db_to_gain(level), gap=2.0)
    t = np.arange(int(8.0 * SR)) / SR
    add("tone220", np.sin(2 * np.pi * 220.0 * t) * db_to_gain(-18.0), gap=2.0)
    burst_t = np.arange(int(0.3 * SR)) / SR
    burst = np.sin(2 * np.pi * 1000.0 * burst_t) * np.hanning(len(burst_t)) ** 0.1
    steps = np.concatenate([np.concatenate([burst * db_to_gain(l), np.zeros(int(0.1 * SR))]) for l in STEP_LEVELS_DB])
    add("steps1k", steps, gap=1.5)
    add("strums", strums(8.0, -18.0), gap=3.0)
    x = np.concatenate(parts).astype(np.float32)
    return x, manifest


# ---- analysis -------------------------------------------------------------------------------

def load(path):
    x, sr = sf.read(path, always_2d=True, dtype="float64")
    if sr != SR:
        x = signal.resample_poly(x, SR, sr, axis=0)
    if x.shape[1] == 1:
        x = np.repeat(x, 2, axis=1)
    return x[:, :2]


def align(recording, reference):
    """Finds the sync chirp; returns the recording shifted so it lines up with the test signal."""
    ref = reference[: int(3.0 * SR)]
    rec = recording[: int(6.0 * SR)].mean(axis=1)
    corr = signal.fftconvolve(rec, ref[::-1], mode="full")
    lag = int(np.argmax(np.abs(corr))) - (len(ref) - 1)
    out = np.zeros((len(reference), 2))
    src = recording[max(lag, 0):]
    if lag < 0:
        src = np.vstack([np.zeros((-lag, 2)), recording])
    n = min(len(src), len(out))
    out[:n] = src[:n]
    return out, lag


def segment(x, manifest, name, extra=0.0):
    seg = manifest[name]
    start, length = seg["start"], seg["length"] + int(extra * SR)
    return x[start:start + length]


def third_octave_response(ir, n_fft=1 << 16):
    spec = np.fft.rfft(ir, n_fft)
    freqs = np.fft.rfftfreq(n_fft, 1 / SR)
    centres = 1000.0 * 2.0 ** (np.arange(-17, 14) / 3.0)
    out = []
    for fc in centres:
        band = (freqs >= fc / 2 ** (1 / 6)) & (freqs < fc * 2 ** (1 / 6))
        out.append(10 * np.log10(np.mean(np.abs(spec[band]) ** 2) + 1e-20))
    return centres, np.array(out)


def analyse_sweep(y, level_db):
    sweep = log_sweep(SWEEP_SECONDS)
    inv = inverse_sweep(sweep, SWEEP_SECONDS)
    ir = signal.fftconvolve(y, inv, mode="full")
    peak = int(np.argmax(np.abs(ir)))
    norm = np.sum(sweep * sweep[::-1] * np.exp(-np.arange(len(sweep)) / SR * np.log(1000.0) / SWEEP_SECONDS)) / db_to_gain(level_db)
    linear = ir[peak - int(0.002 * SR): peak + int(0.3 * SR)] / norm
    # Harmonic IRs sit before the linear one at dt = T*ln(n)/ln(f1/f0).
    harmonics = {}
    lin_energy = np.sum(linear ** 2) + 1e-20
    for n in (2, 3, 4, 5):
        dt = int(SWEEP_SECONDS * np.log(n) / np.log(1000.0) * SR)
        h = ir[peak - dt - int(0.002 * SR): peak - dt + int(0.02 * SR)] / norm
        harmonics[f"H{n}_db"] = round(10 * np.log10(np.sum(h ** 2) / lin_energy + 1e-20), 1)
    centres, resp = third_octave_response(linear)
    return {"response_db": resp.round(2).tolist(), "centres_hz": centres.round(1).tolist(), **harmonics}


def analyse_impulse(y):
    env = np.abs(signal.hilbert(y))
    smooth = np.convolve(env, np.ones(int(0.002 * SR)) / int(0.002 * SR), mode="same")
    direct = smooth[: int(0.01 * SR)].max()
    search = smooth[int(0.015 * SR):]
    peaks, _ = signal.find_peaks(search, height=max(direct * 0.03, 1e-6), distance=int(0.015 * SR))
    peaks = peaks + int(0.015 * SR)
    result = {"direct_db": round(20 * np.log10(direct + 1e-12), 1)}
    if len(peaks):
        order = peaks[np.argsort(smooth[peaks])[::-1]][:4]
        first = int(np.sort(order)[0])
        result["first_echo_ms"] = round(first / SR * 1000.0, 1)
        if len(order) >= 2:
            ordered = np.sort(order)
            ratio = smooth[ordered[1]] / max(smooth[ordered[0]], 1e-12)
            result["echo_ratio_db"] = round(20 * np.log10(ratio + 1e-12), 1)
    # Schroeder decay -> RT60 from the -5..-25 dB slope.
    energy = np.cumsum((y ** 2)[::-1])[::-1]
    edc = 10 * np.log10(energy / (energy[0] + 1e-30) + 1e-30)
    i5, i25 = np.argmax(edc < -5), np.argmax(edc < -25)
    if i25 > i5 > 0:
        result["rt60_s"] = round(3.0 * (i25 - i5) / SR, 2)
    return result


def analyse_tone(y):
    y = y[int(0.5 * SR): -int(0.2 * SR)]
    analytic = signal.hilbert(y)
    env = np.abs(analytic)
    inst = np.diff(np.unwrap(np.angle(analytic))) * SR / (2 * np.pi)
    b, a = signal.butter(2, 40.0 / (SR / 2))
    env_s = signal.filtfilt(b, a, env)
    freq_s = signal.filtfilt(b, a, inst)
    def mod_rate(x):
        x = x - x.mean()
        spec = np.abs(np.fft.rfft(x * np.hanning(len(x))))
        f = np.fft.rfftfreq(len(x), 1 / SR)
        band = (f > 0.05) & (f < 30)
        return float(f[band][np.argmax(spec[band])])
    median_hz = float(np.median(freq_s))
    cents = 1200 * np.log2(np.clip(freq_s, 1, None) / median_hz)
    return {
        "pitch_hz": round(median_hz, 2),
        "pitch_ratio": round(median_hz / 220.0, 4),
        "am_depth": round(float((np.percentile(env_s, 98) - np.percentile(env_s, 2)) / (np.percentile(env_s, 98) + 1e-12)), 3),
        "am_rate_hz": round(mod_rate(env_s), 2),
        "fm_depth_cents": round(float(np.percentile(cents, 98) - np.percentile(cents, 2)) / 2, 1),
        "fm_rate_hz": round(mod_rate(freq_s), 2),
        "level_db": round(20 * np.log10(np.sqrt(np.mean(y ** 2)) * np.sqrt(2) + 1e-12), 1),
    }


def analyse_steps(y):
    step = int(0.4 * SR)
    curve = []
    for i, level in enumerate(STEP_LEVELS_DB):
        chunk = y[i * step + int(0.1 * SR): i * step + int(0.25 * SR)]
        rms = np.sqrt(np.mean(chunk ** 2)) * np.sqrt(2)
        curve.append(round(20 * np.log10(rms + 1e-12) - level, 2))
    return {"input_db": list(STEP_LEVELS_DB), "gain_db": curve}


def analyse_strums(y):
    spec = np.abs(np.fft.rfft(y * np.hanning(len(y))))
    f = np.fft.rfftfreq(len(y), 1 / SR)
    return {"rms_db": round(20 * np.log10(np.sqrt(np.mean(y ** 2)) + 1e-12), 1),
            "centroid_hz": round(float(np.sum(f * spec) / (np.sum(spec) + 1e-12)), 0)}


def analyse(recording, manifest, gain=1.0):
    x = recording / gain
    mid = x.mean(axis=1)
    side = (x[:, 0] - x[:, 1]) / 2
    res = {
        "stereo_width_db": round(20 * np.log10((np.sqrt(np.mean(side ** 2)) + 1e-12) / (np.sqrt(np.mean(mid ** 2)) + 1e-12)), 1),
        "impulse": analyse_impulse(segment(mid, manifest, "impulse")),
        "tone220": analyse_tone(segment(mid, manifest, "tone220")),
        "steps1k": analyse_steps(segment(mid, manifest, "steps1k")),
        "strums": analyse_strums(segment(mid, manifest, "strums", extra=2.0)),
    }
    for level in SWEEP_LEVELS_DB:
        res[f"sweep_{int(level)}"] = analyse_sweep(segment(mid, manifest, f"sweep_{int(level)}", extra=1.5), level)
    return res


# ---- commands -------------------------------------------------------------------------------

def manifest_path(wav):
    return Path(wav).with_suffix(".json")


def cmd_signal(args):
    x, manifest = build_signal()
    sf.write(args.out, x, SR, subtype="FLOAT")
    manifest_path(args.out).write_text(json.dumps({"sr": SR, "segments": manifest}, indent=1))
    print(f"wrote {args.out} ({len(x) / SR:.1f} s) and {manifest_path(args.out)}")


def interface_gain(loopback, reference, manifest):
    if loopback is None:
        return 1.0
    aligned, _ = align(load(loopback), reference)
    seg = manifest["sync"]
    rec = aligned[seg["start"]: seg["start"] + seg["length"]].mean(axis=1)
    ref = reference[seg["start"]: seg["start"] + seg["length"]]
    return float(np.sqrt(np.mean(rec ** 2) / np.mean(ref ** 2)))


def find_renderer(explicit):
    if explicit:
        return explicit
    root = Path(__file__).resolve().parents[2]
    for candidate in ("build-tools/warmful_render", "build/warmful_render", "build/Release/warmful_render.exe"):
        if (root / candidate).exists():
            return str(root / candidate)
    sys.exit("warmful_render not found: build it (cmake --build build-tools) or pass --renderer")


def cmd_analyse(args):
    reference_path = Path(args.signal)
    manifest = json.loads(manifest_path(reference_path).read_text())["segments"]
    reference = load(reference_path).mean(axis=1)
    gain = interface_gain(args.loopback, reference, manifest)
    aligned, lag = align(load(args.recording), reference)
    res = analyse(aligned, manifest, gain)
    res["latency_ms"] = round(lag / SR * 1000.0, 2)
    print(json.dumps(res, indent=1))


def cmd_compare(args):
    reference_path = Path(args.signal)
    manifest = json.loads(manifest_path(reference_path).read_text())["segments"]
    reference = load(reference_path).mean(axis=1)
    gain = interface_gain(args.loopback, reference, manifest)
    pedal, lag = align(load(args.recording), reference)
    pedal_res = analyse(pedal, manifest, gain)

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "plugin.wav"
        cmd = [find_renderer(args.renderer), "render", str(reference_path), str(out)] + shlex.split(args.render)
        subprocess.run(cmd, check=True)
        plugin = load(out)
    plugin_res = analyse(plugin, manifest)

    def row(label, a, b, unit=""):
        delta = "" if not all(isinstance(v, (int, float)) for v in (a, b)) else f"{b - a:+.2f}"
        print(f"  {label:<26} pedal {a!s:>10}{unit}   plugin {b!s:>10}{unit}   delta {delta}")

    print(f"latency (pedal + interface): {lag / SR * 1000:.2f} ms, interface gain {20 * np.log10(gain):+.2f} dB")
    for group in ("impulse", "tone220", "strums"):
        print(f"[{group}]")
        for key in sorted(set(pedal_res[group]) | set(plugin_res[group])):
            row(key, pedal_res[group].get(key, "-"), plugin_res[group].get(key, "-"))
    row("stereo_width_db", pedal_res["stereo_width_db"], plugin_res["stereo_width_db"])
    for level in SWEEP_LEVELS_DB:
        key = f"sweep_{int(level)}"
        p, q = pedal_res[key], plugin_res[key]
        diff = np.array(q["response_db"]) - np.array(p["response_db"])
        diff -= np.median(diff)
        print(f"[{key} dBFS] response shape error (median-normalised): max {np.max(np.abs(diff)):.1f} dB")
        for n in (2, 3, 4, 5):
            row(f"H{n} rel. to fundamental", p[f"H{n}_db"], q[f"H{n}_db"], " dB")
    print("[1 kHz transfer curve] gain (dB) per input level")
    for lvl, gp, gq in zip(pedal_res["steps1k"]["input_db"], pedal_res["steps1k"]["gain_db"], plugin_res["steps1k"]["gain_db"]):
        print(f"  in {lvl:+6.1f} dBFS   pedal {gp:+6.2f}   plugin {gq:+6.2f}   delta {gq - gp:+.2f}")

    if args.plot:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, axes = plt.subplots(1, 2, figsize=(13, 4.5))
        for level, style in zip(SWEEP_LEVELS_DB, ("-", "--", ":")):
            key = f"sweep_{int(level)}"
            axes[0].semilogx(pedal_res[key]["centres_hz"], pedal_res[key]["response_db"], "C0" + style, label=f"pedal {int(level)} dBFS")
            axes[0].semilogx(plugin_res[key]["centres_hz"], plugin_res[key]["response_db"], "C1" + style, label=f"plugin {int(level)} dBFS")
        axes[0].set(title="1/3-octave response", xlabel="Hz", ylabel="dB")
        axes[0].legend(fontsize=7)
        axes[1].plot(pedal_res["steps1k"]["input_db"], pedal_res["steps1k"]["gain_db"], "C0o-", label="pedal")
        axes[1].plot(plugin_res["steps1k"]["input_db"], plugin_res["steps1k"]["gain_db"], "C1s-", label="plugin")
        axes[1].set(title="1 kHz gain vs input level", xlabel="input dBFS", ylabel="gain dB")
        axes[1].legend()
        fig.tight_layout()
        fig.savefig(args.plot, dpi=110)
        print(f"plot: {args.plot}")

    if args.json:
        Path(args.json).write_text(json.dumps({"pedal": pedal_res, "plugin": plugin_res, "render": args.render}, indent=1))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("signal", help="write the re-amp test signal (+ .json manifest)")
    s.add_argument("out")
    a = sub.add_parser("analyse", help="analyse one recording of the pedal (or of anything)")
    a.add_argument("recording")
    a.add_argument("--signal", default="test.wav", help="the test signal that was played (default test.wav)")
    a.add_argument("--loopback", help="recording of the test signal through the interface only (pedal in TRUE BYPASS)")
    c = sub.add_parser("compare", help="analyse a pedal recording and the plug-in rendering of the same settings")
    c.add_argument("recording")
    c.add_argument("--render", required=True, help='warmful_render options, e.g. "--char 0 --char-amount 0.5 --cal 1"')
    c.add_argument("--signal", default="test.wav")
    c.add_argument("--loopback")
    c.add_argument("--renderer", help="path to warmful_render")
    c.add_argument("--plot", help="write a PNG with responses / transfer curves")
    c.add_argument("--json", help="write all metrics as JSON")
    args = ap.parse_args()
    {"signal": cmd_signal, "analyse": cmd_analyse, "compare": cmd_compare}[args.cmd](args)


if __name__ == "__main__":
    main()
