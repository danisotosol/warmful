// WARMFUL — CHARACTER / FUZZ: dynamic vintage two-transistor fuzz, smooth to blown-speaker.
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

/** Coupling HPF (moves with TILT) -> high gain -> biased asymmetric transistor stage whose
    operating point is shifted by TILT and by a charging-cap envelope (gated sputter) ->
    hard-ish second stage (4x oversampled) -> TILT tone with cone breakup at the far right. */
class FuzzEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "FUZZ"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    struct Channel
    {
        OnePole coupling;
        EnvelopeFollower charge;
        Oversampler os;
        Biquad toneLpf, conePeak;
        TiltEq tone;
        DcBlocker dc;
    };

    void updateChunk (int n) noexcept;
    void processChannel (Channel& c, float* io, int n) noexcept;

    double sr = 48000.0;
    character::Controls controls;
    character::Ramp gainRamp, makeupRamp;
    std::array<float, character::kChunk> gain {}, makeup {};
    // Chunk-rate settings derived from TILT.
    float bias = 0.0f, biasOffset = 0.0f, chargeDepth = 0.0f, gate = 0.0f, cone = 0.0f;
    std::array<Channel, 2> channels;
};

} // namespace warmful
