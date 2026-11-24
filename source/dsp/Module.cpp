// WARMFUL — module implementation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Module.h"
#include "Dynamics.h"

namespace warmful
{

namespace
{
    constexpr float kEngageFadeSec = 0.012f;
    constexpr float kVolSmoothSec = 0.03f;
    constexpr float kSwitchFadeSec = 0.035f;
    constexpr float kDiffusionSwitchFadeSec = 0.08f;
    /** Module output headroom: linear up to ~-2.5 dBFS internal, then gentle soft clipping
        ("the entire signal chain can even be pushed into gentle soft clipping"). */
    constexpr float kHeadroomKnee = 0.75f;
}

Module::Module (ModuleType t) : type (t), effects (createEffects (t)) {}

void Module::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    for (auto& e : effects)
        e->prepare (sr, maxBlockSize);

    for (auto* v : { &dryL, &dryR, &fadeL, &fadeR, &gainBuf })
        v->assign ((size_t) maxBlockSize, 0.0f);

    engageGain.prepare (sr, kEngageFadeSec);
    volGain.prepare (sr, kVolSmoothSec);
    const float fadeSec = type == ModuleType::Diffusion ? kDiffusionSwitchFadeSec : kSwitchFadeSec;
    fadeLength = std::max (1, (int) (fadeSec * sr));
    reset();
}

void Module::reset() noexcept
{
    resetEffects();
    snapOnNextBlock = true;   // the next block starts at its target state, without fades
}

void Module::resetEffects() noexcept
{
    for (auto& e : effects)
        e->reset();
    previous = -1;
    pending = -1;
    fadeRemaining = 0;
    idle = false;
}

void Module::selectEffect (int index) noexcept
{
    index = clamp (index, 0, kEffectsPerModule - 1);
    if (fadeRemaining > 0)
    {
        // Finish the running crossfade first; rapid button presses resolve to the last request.
        pending = index == current ? -1 : index;
        return;
    }
    if (index == current)
        return;

    previous = current;
    current = index;
    effects[(size_t) current]->reset();
    fadeRemaining = idle ? 0 : fadeLength;
    if (fadeRemaining == 0)
        previous = -1;
}

void Module::runEffect (float* l, float* r, int n, const EffectParams& p) noexcept
{
    if (fadeRemaining <= 0 || previous < 0)
    {
        effects[(size_t) current]->process (l, r, n, p);
        return;
    }

    std::copy (l, l + n, fadeL.data());
    std::copy (r, r + n, fadeR.data());
    effects[(size_t) previous]->process (fadeL.data(), fadeR.data(), n, p);
    effects[(size_t) current]->process (l, r, n, p);

    for (int i = 0; i < n; ++i)
    {
        const float progress = fadeRemaining > 0 ? 1.0f - (float) fadeRemaining / (float) fadeLength : 1.0f;
        const float in = 0.5f - 0.5f * std::cos (kPi * progress);
        l[i] = l[i] * in + fadeL[(size_t) i] * (1.0f - in);
        r[i] = r[i] * in + fadeR[(size_t) i] * (1.0f - in);
        if (fadeRemaining > 0)
            --fadeRemaining;
    }

    if (fadeRemaining == 0)
    {
        previous = -1;
        if (pending >= 0)
        {
            const int next = pending;
            pending = -1;
            selectEffect (next);
        }
    }
}

void Module::process (float* l, float* r, int n, const EffectParams& params, const State& state) noexcept
{
    const bool isDiffusion = type == ModuleType::Diffusion;
    const bool keepTails = isDiffusion && state.trails;
    const float volume = neutralCentreGain (state.effectVol);

    engageGain.setTarget (state.engaged ? 1.0f : 0.0f);
    volGain.setTarget (isDiffusion ? 1.0f : volume);
    if (snapOnNextBlock)
    {
        engageGain.snap (state.engaged ? 1.0f : 0.0f);
        volGain.snap (isDiffusion ? 1.0f : volume);
        snapOnNextBlock = false;
    }

    EffectParams p = params;
    p.wetLevel = isDiffusion ? volume : 1.0f;

    // Fully disengaged: either pass through untouched, or let a Diffusion tail ring out.
    if (! state.engaged && engageGain.isSettled())
    {
        if (keepTails)
        {
            std::fill (dryL.begin(), dryL.begin() + n, 0.0f);
            std::fill (dryR.begin(), dryR.begin() + n, 0.0f);
            runEffect (dryL.data(), dryR.data(), n, p);
            for (int i = 0; i < n; ++i)
            {
                l[i] += dryL[(size_t) i];
                r[i] += dryR[(size_t) i];
            }
            idle = false;
        }
        else if (! idle)
        {
            resetEffects();
            idle = true;
        }
        return;
    }

    if (idle)
    {
        // Re-engaging from silence: start from clean state (no stale buffers).
        resetEffects();
    }

    std::copy (l, l + n, dryL.data());
    std::copy (r, r + n, dryR.data());
    for (int i = 0; i < n; ++i)
        gainBuf[(size_t) i] = engageGain.next();

    if (keepTails)
        for (int i = 0; i < n; ++i)
        {
            // With trails the effect *input* fades, so repeats/reverb keep ringing after bypass.
            l[i] *= gainBuf[(size_t) i];
            r[i] *= gainBuf[(size_t) i];
        }

    runEffect (l, r, n, p);

    for (int i = 0; i < n; ++i)
    {
        const float g = gainBuf[(size_t) i];
        const float v = volGain.next();
        const float wetGain = keepTails ? v : v * g;
        l[i] = shape::kneeClip (l[i] * wetGain + dryL[(size_t) i] * (1.0f - g), kHeadroomKnee);
        r[i] = shape::kneeClip (r[i] * wetGain + dryR[(size_t) i] * (1.0f - g), kHeadroomKnee);
    }
}

} // namespace warmful
