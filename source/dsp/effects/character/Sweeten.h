// WARMFUL — CHARACTER / SWEETEN: preamp adding EQ, opto compression and gentle saturation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "CharacterCommon.h"
#include "Dynamics.h"
#include "Effect.h"
#include "Filters.h"
#include "Oversampler.h"

#include <array>

namespace warmful
{

/** "Sweet" EQ scaled in with AMOUNT -> stereo-linked opto-style compressor -> 2x oversampled
    cubic saturation, with an output level that rises a little to push the next module. */
class SweetenEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "SWEETEN"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        Biquad lowShelf, scoop, presence, air;
        TiltEq preTilt, postTilt;
        Oversampler os;
        std::array<float, character::kChunk> buf {};
    };

    void updateChunk (int n) noexcept;
    void compress (int n) noexcept;
    void saturate (Channel& c, float* io, int n) noexcept;

    double sr = 48000.0;
    character::Controls controls;
    character::Ramp preGainRamp, postGainRamp;
    std::array<float, character::kChunk> preGain {}, postGain {}, compGain {};
    std::array<Channel, 2> channels;

    EnvelopeFollower detector;   // mean-square (RMS) level of the stereo-linked signal
    float grDb = 0.0f;           // smoothed gain reduction (<= 0)
    float grMemory = 0.0f;       // slow memory of recent reduction: drives the opto release
    float attackCoef = 0.0f, releaseCoef = 0.0f, memoryCoef = 0.0f;
    float thresholdDb = 0.0f, ratio = 1.0f;
};

} // namespace warmful
