// WARMFUL — TEXTURE / FILTER: tilt, low-pass or high-pass 12 dB/oct sweep with a transparent neutral end.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Effect.h"
#include "Filters.h"

#include <array>

namespace warmful
{

class FilterEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "FILTER"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void setTargets (const EffectParams& params) noexcept;
    void updateCoefficients() noexcept;

    double sr = 48000.0;
    // A low-pass and a high-pass in series: each style only moves one of them away from its open
    // position, so every style and every style change is a continuous glide of the same two filters.
    std::array<Svf, 2> lowpass, highpass;
    Smoothed lowpassLog2Hz, highpassLog2Hz, lowpassQ, highpassQ; // control rate
    Smoothed dryMix;                                             // audio rate
    int controlCountdown = 0;
    bool primed = false;
};

} // namespace warmful
