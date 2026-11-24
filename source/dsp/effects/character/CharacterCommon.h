// WARMFUL — helpers shared by the CHARACTER effects (control smoothing, gain ramps, auto make-up).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include "Effect.h"

namespace warmful::character
{

/** The CHARACTER effects run in chunks of this many samples: coefficients are recomputed once per
    chunk and the oversamplers are sized for it, so any host block size works without allocation. */
constexpr int kChunk = 32;

/** TILT, AMOUNT and SENSITIVITY, smoothed per sample and read once per chunk. */
class Controls
{
public:
    void prepare (double sampleRate, float smoothingSeconds = 0.03f) noexcept
    {
        tiltS.prepare (sampleRate, smoothingSeconds);
        amountS.prepare (sampleRate, smoothingSeconds);
        sensitivityS.prepare (sampleRate, smoothingSeconds);
    }

    /** The next setTargets() snaps instead of gliding from stale values. */
    void reset() noexcept { primed = false; }

    void setTargets (const EffectParams& p) noexcept
    {
        const float t = bipolar (p.knob);
        const float a = clamp (p.amount, 0.0f, 1.0f);
        const float s = bipolar (p.sensitivity);
        if (! primed)
        {
            tiltS.snap (t);
            amountS.snap (a);
            sensitivityS.snap (s);
            primed = true;
            return;
        }
        tiltS.setTarget (t);
        amountS.setTarget (a);
        sensitivityS.setTarget (s);
    }

    /** Advances the smoothers over one chunk of `n` samples. */
    void advance (int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            tiltS.next();
            amountS.next();
            sensitivityS.next();
        }
    }

    /** TILT as -1 (darker) .. +1 (brighter). */
    float tilt() const noexcept { return tiltS.getCurrent(); }
    /** TILT as the 0..1 knob position. */
    float tilt01() const noexcept { return 0.5f + 0.5f * tiltS.getCurrent(); }
    float amount() const noexcept { return amountS.getCurrent(); }
    /** SENSITIVITY as -1 (less) .. +1 (more); 0 = neutral. */
    float sensitivity() const noexcept { return sensitivityS.getCurrent(); }

private:
    Smoothed tiltS, amountS, sensitivityS;
    bool primed = false;
};

/** Linear per-sample interpolation of a chunk-rate gain, so gains never step (zipper noise). */
class Ramp
{
public:
    void reset() noexcept { primed = false; }

    /** Writes `n` samples gliding from the previous target to `target` (snaps on the first call). */
    void fill (float target, float* dst, int n) noexcept
    {
        if (! primed)
        {
            value = target;
            primed = true;
        }
        const float step = (target - value) / (float) n;
        for (int i = 0; i < n; ++i)
            dst[i] = value + step * (float) (i + 1);
        value = target;
    }

private:
    float value = 0.0f;
    bool primed = false;
};

/** Auto make-up gain for a static drive stage. `shaped` is the stage's output for a peak of `ref`
    after a linear `drive`: the gain brings that peak back to `ref`, then `crestExp` pulls saturated
    settings down a little because a clipped signal has a lower crest factor and sounds louder at
    the same peak level. The net effect is that AMOUNT changes character more than loudness. */
inline float makeupGain (float ref, float drive, float shaped, float crestExp) noexcept
{
    shaped = std::max (std::abs (shaped), 1.0e-4f);
    const float linearity = clamp (shaped / std::max (drive * ref, 1.0e-6f), 1.0e-4f, 1.0f);
    return ref / shaped * std::pow (linearity, crestExp);
}

} // namespace warmful::character
