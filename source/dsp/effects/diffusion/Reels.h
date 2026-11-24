// WARMFUL — REELS: well-worn tape echo (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Tape loop with a record head and two playback heads (a few ms apart for stereo spread).
    Tape speed follows TIME with motor inertia (pitch glides), wow/flutter/scrape modulate the
    playback, and DRIFT degrades the tape inside the feedback loop so repeats disintegrate. */
class ReelsEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "REELS"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void updateControl (float driftAmount) noexcept;
    float dropoutGain() noexcept;

    double sr = 48000.0;
    DelayLine tape;
    Svf lossL, lossR;
    Biquad bumpL, bumpR, lowCutL, lowCutR;
    Lfo wow, flutter;
    SmoothRandom wowWander, scrape;
    Rng rng;
    Smoothed level, feedback, wet, drift;
    float motor = 0.0f, delay = 0.0f, maxDelaySamples = 1.0f, inertiaCoef = 0.0f, headOffset = 0.0f;
    float wowDepth = 0.0f, wanderDepth = 0.0f, flutterDepth = 0.0f, scrapeDepth = 0.0f;
    float noiseLevel = 0.0f, loss = 1.0f, recDrive = 1.0f, dropChance = 0.0f;
    float dropPos = 0.0f, dropLen = 0.0f, dropDepth = 0.0f;
    bool primed = false;
};

} // namespace warmful
