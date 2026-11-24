// WARMFUL — CHARACTER / SWELL: envelope-triggered volume swells.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "CharacterCommon.h"
#include "Dynamics.h"
#include "Effect.h"
#include "Filters.h"

#include <array>

namespace warmful
{

/** Stereo-linked fast envelope with a hysteresis trigger (re-arms only after the input falls
    below a low threshold, i.e. after note separation) restarting a raised-cosine fade-in;
    the swell holds while the note sustains and decays once the input drops.
    AMOUNT = attack/decay times, SENSITIVITY = trigger threshold, TILT = tone only. */
class SwellEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "SWELL"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    enum class Stage { Idle, Attack, Hold, Release };

    void updateChunk() noexcept;

    double sr = 48000.0;
    character::Controls controls;
    EnvelopeFollower detector;
    Smoothed declick;            // softens the restart of the swell from silence
    std::array<TiltEq, 2> tone;

    Stage stage = Stage::Idle;
    bool armed = true;
    float level = 0.0f, attackPhase = 0.0f;
    // Chunk-rate settings.
    float highThreshold = 0.0f, lowThreshold = 0.0f, attackStep = 0.0f, releaseCoef = 0.0f;
};

} // namespace warmful
