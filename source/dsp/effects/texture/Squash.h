// WARMFUL — TEXTURE / SQUASH: heavy-handed stereo-linked compressor that tips into overdrive near 100 %.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Effect.h"
#include "TextureCommon.h"

namespace warmful
{

class SquashEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "SQUASH"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void updateControls() noexcept;
    float gainComputerDb (float levelDb) const noexcept;

    double sr = 48000.0;
    texture::AmountControl amount;

    // Gain reduction (dB, <= 0) and its slow average, which lengthens the release on sustained squashing.
    float grDb = 0.0f, grAverageDb = 0.0f;
    float attackCoef = 0.0f, releaseCoef = 0.0f, averageCoef = 0.0f;

    // Control-rate values derived from AMOUNT.
    float thresholdDb = 0.0f, slope = 0.0f, kneeDb = 1.0f, makeupDb = 0.0f;
    float driveMix = 0.0f, driveGain = 1.0f, driveNorm = 1.0f;
    int controlCountdown = 0;
};

} // namespace warmful
