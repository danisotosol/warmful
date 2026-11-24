// WARMFUL — PHASER: vintage-voiced 2- to 12-stage phaser.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Twelve first-order allpasses in series with light feedback. AMOUNT moves the output tap
    continuously from 2 to 12 stages and widens the sweep; DRIFT randomises the sweep. */
class PhaserEffect : public Effect
{
public:
    const char* name() const noexcept override { return "PHASER"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    static constexpr int kNumStages = 12;
    /** Samples between sweep-coefficient updates; the coefficient ramps linearly in between. */
    static constexpr int kControlInterval = 16;

    struct Channel
    {
        std::array<Allpass1, kNumStages> stages;
        SmoothRandom random;
        float coef = 0.0f, coefStep = 0.0f, feedback = 0.0f;
    };

    void updateControl() noexcept;

    double sr = 48000.0;
    std::array<Channel, 2> channels;
    SmoothRandom rateJitter;
    Smoothed rate, amount, drift;  // run at the control rate
    float phase = 0.0f;
    float wet = 0.5f, wetStep = 0.0f;
    float tapFrac = 0.0f, feedbackGain = 0.0f;
    int tapLow = 2, controlCountdown = 0;
    bool primed = false;
};

} // namespace warmful
