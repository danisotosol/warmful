// WARMFUL — TEXTURE / BROKEN: a failing tape motor — periodic pitch drops, lumpy capstan FM and AM, near-dropouts.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Modulation.h"
#include "TextureCommon.h"

#include <array>

namespace warmful
{

class BrokenEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "BROKEN"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void updateControls() noexcept;
    void startDrop (float amountNow) noexcept;

    double sr = 48000.0;
    texture::AmountControl amount;
    std::array<DelayLine, 2> lines;

    // Motor model: the playback delay integrates (1 - speed). Double precision so tiny speed
    // errors are not lost against delays of thousands of samples.
    double delay = 0.0;
    float baseDelay = 0.0f, minDelay = 0.0f, maxDelay = 0.0f, servoCoef = 0.0f;

    Lfo capstan;
    SmoothRandom capstanJitter;
    Rng events;

    // Current speed drop (stall) event.
    bool dropActive = false;
    float dropPhase = 0.0f, dropInc = 0.0f, dropDepth = 0.0f;
    int samplesToNextDrop = 0;

    // Near-dropouts.
    int dropoutRemaining = 0;
    float dropoutTarget = 1.0f, dropoutGain = 1.0f, dropoutCoef = 0.0f;

    // Control-rate values derived from AMOUNT.
    float amountNow = 0.0f, capstanDev = 0.0f, amDepth = 0.0f, amMakeup = 1.0f, dropoutRate = 0.0f;
    int controlCountdown = 0;
};

} // namespace warmful
