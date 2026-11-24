// WARMFUL — CHARACTER / HOWL: resonant envelope-filter fuzz, sustaining howls to synth stabs.
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

/** High-gain tanh fuzz core -> 4-pole ladder low-pass with tanh feedback (4x oversampled) whose
    cutoff follows a stereo-linked envelope: TILT left = low, slow, vocal sweep; TILT right =
    high base cutoff with a fast-decaying transient sweep. AMOUNT raises gain and resonance. */
class HowlEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "HOWL"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    /** Four TPT one-poles with saturated, one-sample-delayed global feedback (fine at 4x). */
    struct Ladder
    {
        float process (float x, float G, float k) noexcept;
        void reset() noexcept { s = {}; y4 = 0.0f; }
        std::array<float, 4> s {};
        float y4 = 0.0f;
    };

    struct Channel
    {
        OnePole inputHpf;
        Oversampler os;
        Ladder ladder;
        TiltEq tone;
        DcBlocker dc;
    };

    void updateChunk (int n) noexcept;
    void sweepCutoff (const float* left, const float* right, int n) noexcept;
    void processChannel (Channel& c, float* io, int n) noexcept;

    double sr = 48000.0, osRate = 192000.0;
    character::Controls controls;
    character::Ramp gainRamp, resonanceRamp, makeupRamp;
    std::array<float, character::kChunk> gain {}, resonance {}, makeup {}, ladderG {};
    // Envelope sweep (stereo-linked): smooth follower and a fast-minus-slow transient detector.
    EnvelopeFollower envSmooth, envFast, envSlow;
    float baseCutoffHz = 300.0f, sweepOctaves = 3.0f, stabMix = 0.0f, envScale = 1.0f, coreOffset = 0.0f;
    std::array<Channel, 2> channels;
};

} // namespace warmful
