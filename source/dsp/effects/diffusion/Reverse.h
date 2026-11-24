// WARMFUL — REVERSE: reverse delay with variable playback speed/pitch (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Modulation.h"

namespace warmful
{

/** Two Hann-windowed reverse grains per channel, overlapping by half a grain (constant overlap-add).
    Each grain starts at "now" and plays the history backwards at speed 0.5..2 (TIME), with a little
    feedback of the reversed signal for haunting ambience. DRIFT wanders the speed and offsets the
    right channel's grains for width. */
class ReverseEffect final : public Effect
{
public:
    const char* name() const noexcept override { return "REVERSE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void updateControl (int chunkLength, float grainSamples) noexcept;

    double sr = 48000.0;
    std::array<DelayLine, 2> lines;
    std::array<SmoothRandom, 2> wander;
    std::array<float, 2> phase {}, delayA {}, delayB {}, speed {}, speedStep {}, phaseInc {};
    Smoothed logSpeed, drift, dryGain, wetGain;
    bool primed = false;
};

} // namespace warmful
