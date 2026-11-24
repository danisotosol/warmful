// WARMFUL — TEXTURE / CASSETTE: tape saturation, EQ, wow/flutter, hiss and dropouts, from clean deck to eaten tape.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Dynamics.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"
#include "TextureCommon.h"

#include <array>

namespace warmful
{

class CassetteEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "CASSETTE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        Biquad preEmphasis, deEmphasis, headBump, hfLoss;
        OnePole magnetisation;  // recent output: shifts the saturator's operating point (hysteresis flavour)
        DcBlocker dcBlocker;
        float memory = 0.0f;
        DelayLine transport;    // modulated read = wow, flutter, warble and pitch dips
        Rng hissRng;
        OnePole hissHighpass, hissLowpass;
    };

    void updateControls() noexcept;
    void rollEvents() noexcept;

    double sr = 48000.0;
    texture::AmountControl amount;
    float designedAmount = -1.0f;   // AMOUNT the tone filters were last designed for
    std::array<Channel, 2> channels;

    // Transport modulation; depths in samples of delay, smoothed per sample so the read never jumps.
    Lfo wow, flutter;
    SmoothRandom warble, crinkle, levelWobble, crinkleLevel;
    Smoothed wowDepth, flutterDepth, warbleDepth, crinkleDepth, azimuth;
    float baseDelay = 0.0f;

    // Random events (seeded, deterministic).
    Rng events;
    bool dipActive = false;
    float dipPhase = 0.0f, dipInc = 0.0f, dipDepth = 0.0f;
    int dropoutRemaining = 0;
    float dropoutTarget = 1.0f, dropoutGain = 1.0f, dropoutAttack = 0.0f, dropoutRelease = 0.0f;

    // Gentle program compression after playback.
    EnvelopeFollower compEnvelope;
    float compGain = 1.0f, compTarget = 1.0f, compCoef = 0.0f;

    // Control-rate values derived from AMOUNT.
    float driveGain = 1.0f, satNorm = 1.0f;
    float wobbleDepth = 0.0f, crinkleAmDepth = 0.0f, hissLevel = 0.0f;
    float dipRate = 0.0f, dipDepthMax = 0.0f, dropoutRate = 0.0f, dropoutDepthDb = 0.0f;
    float compThresholdDb = 0.0f, compSlope = 0.0f, compMakeupDb = 0.0f;
    int controlCountdown = 0;
    bool primed = false;
};

} // namespace warmful
