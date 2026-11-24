// WARMFUL — COLLAGE: wild, destructively edited looping delay (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Stereo looping delay whose read head follows TIME almost directly: knob motion becomes
    speed/pitch changes that are written back through the feedback (destructive edits).
    DRIFT triggers double-speed loop cycles and slow pitch wander of the read head. */
class CollageEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "COLLAGE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    double sr = 48000.0;
    std::array<DelayLine, 2> lines;
    std::array<OnePole, 2> hfLoss;
    std::array<DcBlocker, 2> dcBlock;
    SmoothRandom wander;
    Rng rng;
    Smoothed level, feedback, wet, drift;
    float loopDelay = 1.0f, maxLoopSamples = 1.0f, scrubCoef = 0.0f;
    float cyclePos = 0.0f, doubleTime = 0.0f, doubleLength = 0.0f;
    bool doubleActive = false;
    bool primed = false;
};

} // namespace warmful
