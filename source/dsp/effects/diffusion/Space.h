// WARMFUL — SPACE: reverb morphing from echo chamber to sustaining cloud (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Pre-delay with early-reflection taps -> 4 series input allpasses per channel -> 8-line
    Hadamard FDN with per-line damping. TIME continuously interpolates five voicings
    (chamber, room/plate, hall, large hall, cloud); DRIFT modulates line and diffuser taps. */
class SpaceEffect final : public Effect
{
public:
    static constexpr int kLines = 8;
    static constexpr int kDiffusers = 4;

    const char* name() const noexcept override { return "SPACE"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    void updateControl (int chunkLength) noexcept;

    double sr = 48000.0;
    std::array<DelayLine, kLines> lines;
    std::array<OnePole, kLines> damping;
    std::array<SmoothRandom, kLines> lineWander;
    std::array<float, kLines> lineLength {}, lineGain {}, lineMod {}, lineModStep {};
    std::array<DelayLine, 2> preDelay;
    std::array<OnePole, 2> lowCut;
    std::array<std::array<DelayLine, kDiffusers>, 2> diffusers;
    std::array<std::array<SmoothRandom, kDiffusers>, 2> diffuserWander;
    std::array<std::array<float, kDiffusers>, 2> diffuserLength {}, diffuserMod {}, diffuserModStep {};
    std::array<std::array<float, 6>, 2> erDelay {};
    Smoothed morph, drift, dryGain, wetGain;
    float size = 1.0f, sizeStep = 0.0f, smear = 1.0f, smearStep = 0.0f, pre = 0.0f, preStep = 0.0f;
    float diffusion = 0.6f, erLevel = 0.0f, tailGain = 1.0f;
    bool primed = false;
};

} // namespace warmful
