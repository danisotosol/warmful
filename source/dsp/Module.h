// WARMFUL — one effect module (column of the pedal): 5 effects, selection, bypass, EFFECT VOL.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include "Effect.h"
#include <vector>

namespace warmful
{

class Module
{
public:
    explicit Module (ModuleType type);

    void prepare (double sampleRate, int maxBlockSize);
    void reset() noexcept;

    /** Selects one of the 5 effects; switches with a short crossfade. */
    void selectEffect (int index) noexcept;
    int getSelectedEffect() const noexcept { return current; }
    ModuleType getType() const noexcept { return type; }

    struct State
    {
        bool engaged = true;     // module active (not OFF, not bypassed by a footswitch)
        bool trails = false;     // when disengaged, keep tails ringing (Diffusion in "buffered bypass with trails")
        float effectVol = 0.5f;  // EFFECT VOL knob (0.5 = unity)
    };

    /** In-place stereo processing at the chain's internal level. */
    void process (float* left, float* right, int numSamples, const EffectParams& params, const State& state) noexcept;

    /** True when the module currently produces no output beyond its input (fully bypassed, no tail running). */
    bool isIdle() const noexcept { return idle; }

private:
    void runEffect (float* l, float* r, int n, const EffectParams& p) noexcept;
    void resetEffects() noexcept;

    ModuleType type;
    EffectSet effects;
    double sampleRate = 48000.0;
    int current = 0, previous = -1, pending = -1;
    int fadeLength = 1, fadeRemaining = 0;
    LinearRamp engageGain;
    Smoothed volGain;
    bool idle = false, snapOnNextBlock = true;
    std::vector<float> dryL, dryR, fadeL, fadeR, gainBuf;
};

} // namespace warmful
