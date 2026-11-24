// WARMFUL — TEXTURE / INTERFERENCE: radio static, AM-radio fading, codec damage and GSM buzz dissolving the signal.
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

class InterferenceEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "INTERFERENCE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        Biquad radioHighpass, radioLowpass, staticBand;
        DelayLine packets;          // recent (pre-packet-loss) signal, replayed by stutter events
        Rng noise;
        SmoothRandom staticBursts;
        float crackle = 0.0f;
        float held = 0.0f;          // sample-and-hold value of the rate reducer
    };

    void updateControls() noexcept;
    void rollEvents() noexcept;

    double sr = 48000.0;
    texture::AmountControl amount;
    std::array<Channel, 2> channels;

    EnvelopeFollower envelope;      // stereo-linked input level: disturbances follow the playing
    SmoothRandom radioFade, whistleDrift, dissolve;
    Lfo whistle, buzz;
    OnePole buzzLowpass;
    DcBlocker buzzDc;
    Rng events;

    // Codec damage events (shared by both channels: one damaged stream).
    int crushRemaining = 0;
    float crushWeight = 0.0f, crushPhase = 0.0f, crushInc = 1.0f, crushSteps = 128.0f;
    int packetRemaining = 0, packetElapsed = 0, packetLength = 1;
    bool packetRepeat = false;
    float packetWeight = 0.0f;
    int buzzRemaining = 0;
    float buzzGate = 0.0f;
    float edgeCoef = 0.0f, buzzGateCoef = 0.0f, crackleDecay = 0.0f, packetFade = 1.0f;

    // Control-rate values derived from AMOUNT.
    float staticLevel = 0.0f, crackleLevel = 0.0f, crackleProbability = 0.0f;
    float radioMix = 0.0f, whistleLevel = 0.0f, dissolveDepth = 0.0f, buzzLevel = 0.0f;
    float crushRate = 0.0f, packetRate = 0.0f, buzzRate = 0.0f;
    int controlCountdown = 0;
};

} // namespace warmful
