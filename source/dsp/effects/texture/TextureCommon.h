// WARMFUL — helpers shared by the TEXTURE effects (control-rate AMOUNT smoothing, random events).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include "Modulation.h"

namespace warmful::texture
{

/** The TEXTURE effects recompute their AMOUNT-derived coefficients and roll their random events
    once every this many samples; anything that must glide per sample has its own smoother. */
constexpr int kControlInterval = 16;

/** AMOUNT smoothed at control rate (one step per kControlInterval samples). */
class AmountControl
{
public:
    void prepare (double sampleRate, float smoothingSeconds = 0.03f) noexcept
    {
        smoother.prepare (sampleRate / (double) kControlInterval, smoothingSeconds);
    }

    /** The next setTarget() snaps instead of gliding from a stale value. */
    void reset() noexcept { primed = false; }

    void setTarget (float amount) noexcept
    {
        amount = clamp (amount, 0.0f, 1.0f);
        if (! primed)
        {
            smoother.snap (amount);
            primed = true;
            return;
        }
        smoother.setTarget (amount);
    }

    /** Advances one control step and returns the smoothed AMOUNT. */
    float next() noexcept { return smoother.next(); }

private:
    Smoothed smoother;
    bool primed = false;
};

/** True with the probability that a Poisson process of `ratePerSecond` fires within `seconds`
    (first-order, fine for the small control-step windows used here). */
inline bool eventFires (Rng& rng, float ratePerSecond, float seconds) noexcept
{
    return ratePerSecond > 0.0f && rng.uniform() < ratePerSecond * seconds;
}

/** Seconds to a whole number of samples (at least 1). */
inline int toSamples (float seconds, double sampleRate) noexcept
{
    return std::max (1, (int) ((double) seconds * sampleRate + 0.5));
}

} // namespace warmful::texture
