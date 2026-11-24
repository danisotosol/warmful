// WARMFUL — CHARACTER / DRIVE: tube-like drive from warm preamp to full overdrive.
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

/** Two cascaded asymmetric tube stages (4x oversampled) between TILT pre-emphasis and
    de-emphasis, with an envelope-driven bias/gain "sag" and auto make-up. */
class DriveEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "DRIVE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        OnePole inputHpf, interHpf, interLpf;
        TiltEq preTilt, postTilt;
        Biquad postLpf;
        DcBlocker dc;
        EnvelopeFollower sag;
        Oversampler os;
    };

    void updateChunk (int n) noexcept;
    void processChannel (Channel& c, float* io, int n) noexcept;

    double sr = 48000.0;
    character::Controls controls;
    character::Ramp stage1Ramp, stage2Ramp, makeupRamp;
    std::array<float, character::kChunk> stage1Gain {}, stage2Gain {}, makeup {};
    std::array<Channel, 2> channels;
};

} // namespace warmful
