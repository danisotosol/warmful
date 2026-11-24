// WARMFUL — CASCADE: heightened bucket-brigade analog delay (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Mono BBD core (anti-alias filter, clock sample-and-hold, compander-style saturation,
    reconstruction filter) with two slightly different output taps for subtle width.
    TIME glides the BBD clock, so twisting it gives the classic pitch sweep. */
class CascadeEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "CASCADE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void updateControl (float driftAmount) noexcept;

    double sr = 48000.0;
    DelayLine line;
    Svf antiAlias;
    OnePole reconL, reconR, loopHp;
    Lfo wobble;
    SmoothRandom wander;
    Rng noise;
    Smoothed level, feedback, wet, drift;
    float delay = 0.0f, maxDelaySamples = 1.0f, glideCoef = 0.0f;
    float holdPhase = 0.0f, holdInc = 1.0f, held = 0.0f;
    float wobbleDepth = 0.0f, wanderDepth = 0.0f, noiseLevel = 0.0f, loss = 1.0f;
    bool primed = false;
};

} // namespace warmful
