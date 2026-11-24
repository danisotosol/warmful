// WARMFUL — TEXTURE / FILTER: tilt, low-pass or high-pass 12 dB/oct sweep with a transparent neutral end.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Filter.h"
#include "TextureCommon.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr float kOpenLowpassHz = 20000.0f;   // where the low-pass rests when it is not in use
constexpr float kOpenHighpassHz = 20.0f;     // where the high-pass rests when it is not in use
constexpr float kTiltLowpassMinHz = 250.0f;  // TILT fully left
constexpr float kTiltHighpassMaxHz = 2500.0f;// TILT fully right
constexpr float kLowpassMinHz = 150.0f;      // LPF style fully counter-clockwise
constexpr float kHighpassMaxHz = 3000.0f;    // HPF style fully clockwise
constexpr float kQNeutral = 0.9f;            // mild vintage bump near the open end ...
constexpr float kQFar = 1.2f;                // ... a little more resonance far from neutral
constexpr float kNeutralFade = 0.05f;        // last 5 % of travel crossfades to the untouched dry signal
constexpr float kCutoffSmoothSec = 0.04f;    // cutoff glide (log domain), also covers style changes
constexpr float kMixSmoothSec = 0.025f;
} // namespace voicing
} // namespace

using namespace voicing;
using texture::kControlInterval;

void FilterEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    const double controlRate = sampleRate / (double) kControlInterval;
    lowpassLog2Hz.prepare (controlRate, kCutoffSmoothSec);
    highpassLog2Hz.prepare (controlRate, kCutoffSmoothSec);
    lowpassQ.prepare (controlRate, kCutoffSmoothSec);
    highpassQ.prepare (controlRate, kCutoffSmoothSec);
    dryMix.prepare (sampleRate, kMixSmoothSec);
    reset();
}

void FilterEffect::reset() noexcept
{
    for (auto& f : lowpass)
        f.reset();
    for (auto& f : highpass)
        f.reset();
    controlCountdown = 0;
    primed = false;
}

void FilterEffect::setTargets (const EffectParams& params) noexcept
{
    const float amount = clamp (params.amount, 0.0f, 1.0f);

    // Distance of each filter from its open position (0 = open, 1 = fully swept) and its sweep end.
    float lowpassDistance = 0.0f, highpassDistance = 0.0f;
    float lowpassEndHz = kTiltLowpassMinHz, highpassEndHz = kTiltHighpassMaxHz;
    switch (params.filterStyle)
    {
        case FilterStyle::LowPass:
            lowpassDistance = 1.0f - amount;
            lowpassEndHz = kLowpassMinHz;
            break;
        case FilterStyle::HighPass:
            highpassDistance = amount;
            highpassEndHz = kHighpassMaxHz;
            break;
        case FilterStyle::Tilt:
        default:
        {
            const float b = bipolar (amount);
            lowpassDistance = std::max (0.0f, -b);
            highpassDistance = std::max (0.0f, b);
            break;
        }
    }

    // Exponential cutoff mapping = linear in log2(Hz).
    const float lpLog2 = lerp (std::log2 (kOpenLowpassHz), std::log2 (lowpassEndHz), lowpassDistance);
    const float hpLog2 = lerp (std::log2 (kOpenHighpassHz), std::log2 (highpassEndHz), highpassDistance);
    const float lpQ = lerp (kQNeutral, kQFar, lowpassDistance);
    const float hpQ = lerp (kQNeutral, kQFar, highpassDistance);
    const float dry = 1.0f - smoothstep (0.0f, kNeutralFade, std::max (lowpassDistance, highpassDistance));

    if (! primed)
    {
        lowpassLog2Hz.snap (lpLog2);
        highpassLog2Hz.snap (hpLog2);
        lowpassQ.snap (lpQ);
        highpassQ.snap (hpQ);
        dryMix.snap (dry);
        primed = true;
        return;
    }
    lowpassLog2Hz.setTarget (lpLog2);
    highpassLog2Hz.setTarget (hpLog2);
    lowpassQ.setTarget (lpQ);
    highpassQ.setTarget (hpQ);
    dryMix.setTarget (dry);
}

void FilterEffect::updateCoefficients() noexcept
{
    const float lpHz = std::exp2 (lowpassLog2Hz.next());
    const float hpHz = std::exp2 (highpassLog2Hz.next());
    const float lpQ = lowpassQ.next();
    const float hpQ = highpassQ.next();
    for (auto& f : lowpass)
        f.setParams (lpHz, lpQ, sr);
    for (auto& f : highpass)
        f.setParams (hpHz, hpQ, sr);
}

void FilterEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    setTargets (params);
    float* const io[2] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        if (--controlCountdown < 0)
        {
            controlCountdown = kControlInterval - 1;
            updateCoefficients();
        }

        // The filters always run so leaving the neutral zone never starts from cold state;
        // at neutral the output is exactly the input.
        const float dry = dryMix.next();
        const float wet = 1.0f - dry;
        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = io[ch][i];
            const float y = highpass[(size_t) ch].process (lowpass[(size_t) ch].process (x).lp).hp;
            io[ch][i] = x * dry + y * wet;
        }
    }
}

} // namespace warmful
