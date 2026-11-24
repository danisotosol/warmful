// WARMFUL — DOUBLER: stereo double tracking, from tight ADT to short slap-back.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Two "re-performed" takes (one per side) next to the centred dry signal. Each take has
    its own delay with human timing wander, its own EQ and, with DRIFT, sporadic
    momentary pitch bends. */
class DoublerEffect : public Effect
{
public:
    const char* name() const noexcept override { return "DOUBLER"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Take
    {
        DelayLine delay;
        SmoothRandom wander;
        Rng rng;
        OnePole lowpass, highpass;
        bool bending = false;
        float bendPos = 0.0f, bendInc = 0.0f, bendDepth = 0.0f;
    };

    /** Delay offset (samples) of the take's current momentary pitch bend; may start a new one. */
    float bendOffset (Take& take, float startChance, float drift) noexcept;

    double sr = 48000.0;
    std::array<Take, 2> takes;
    Smoothed baseDelay, mix, drift;
    bool primed = false;
};

} // namespace warmful
