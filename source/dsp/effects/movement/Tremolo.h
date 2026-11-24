// WARMFUL — TREMOLO: amp-style shimmer to square-wave chop.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Amplitude modulation whose shape morphs from sine to square with AMOUNT. Low and high
    bands are modulated slightly out of phase for a subtle harmonic-tremolo colour. DRIFT adds
    rate wobble, per-cycle depth variation and a slow random L/R phase spread. */
class TremoloEffect : public Effect
{
public:
    const char* name() const noexcept override { return "TREMOLO"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        OnePole split;
        float gainLow = 1.0f, gainHigh = 1.0f;  // edge-rounded gains of the two bands
    };

    double sr = 48000.0;
    std::array<Channel, 2> channels;
    SmoothRandom rateWobble, spread;
    Rng rng;
    Smoothed rate, depth, chop, drift, depthVariation;
    float phase = 0.0f, gainCoef = 0.0f, invTanhSharpness = 1.0f;
    bool primed = false;
};

} // namespace warmful
