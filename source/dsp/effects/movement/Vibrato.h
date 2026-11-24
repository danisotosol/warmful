// WARMFUL — VIBRATO: sine to tape-warble pitch modulation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Modulation.h"

namespace warmful
{

/** 100 % wet modulated delay. AMOUNT sets the peak pitch deviation independently of RATE;
    DRIFT morphs the sine LFO into a random warble and decorrelates L/R. */
class VibratoEffect : public Effect
{
public:
    const char* name() const noexcept override { return "VIBRATO"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        DelayLine delay;
        SmoothRandom random;
    };

    double sr = 48000.0;
    std::array<Channel, 2> channels;
    Smoothed rate, slope, drift, centre;
    float phase = 0.0f;
    bool primed = false;
};

} // namespace warmful
