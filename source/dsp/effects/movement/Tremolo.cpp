// WARMFUL — TREMOLO: amp-style shimmer to square-wave chop.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Tremolo.h"
#include "MovementCommon.h"

namespace warmful
{

namespace voicing
{
    constexpr float kMinRateHz = 0.1f, kMaxRateHz = 12.0f;  // free RATE range (exponential)
    constexpr float kMaxDepth = 1.0f;                       // full AMOUNT chops down to silence
    constexpr float kChopStart = 0.6f;                      // AMOUNT where the sine starts turning square
    constexpr float kChopSharpness = 14.0f;                 // tanh drive of the square shape (edge steepness)
    constexpr float kGainSmoothSec = 0.0012f;               // rounds the chop edges: hard but click-free
    constexpr float kMakeup = 0.8f;                         // share of the average-level loss compensated
    constexpr float kTintSplitHz = 650.0f;                  // harmonic-tremolo tint crossover
    constexpr float kTintPhase = 0.03f;                     // bands modulated +-11 degrees apart (subtle)
    constexpr float kRateWobble = 0.3f;                     // +-30 % rate wobble at DRIFT max
    constexpr float kRateWobbleHz = 0.6f;
    constexpr float kDepthVariation = 0.55f;                // DRIFT max: a cycle's depth may drop by up to 55 %
    constexpr float kDepthVariationSmoothSec = 0.04f;
    constexpr float kMaxSpread = 0.25f;                     // slow random L/R phase spread at DRIFT max (cycles)
    constexpr float kSpreadHz = 0.15f;
    constexpr float kRateSmoothSec = 0.05f;
    constexpr float kControlSmoothSec = 0.03f;
    constexpr uint32_t kRateSeed = 0x7E3010A1u, kSpreadSeed = 0x51DE5EB2u, kDepthSeed = 0xC401D0C3u;
} // namespace voicing

using namespace voicing;

void TremoloEffect::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (Channel& c : channels)
        c.split.setCutoff (kTintSplitHz, sr);
    rateWobble.setRate (kRateWobbleHz, sr);
    spread.setRate (kSpreadHz, sr);
    rate.prepare (sr, kRateSmoothSec);
    depth.prepare (sr, kControlSmoothSec);
    chop.prepare (sr, kControlSmoothSec);
    drift.prepare (sr, kControlSmoothSec);
    depthVariation.prepare (sr, kDepthVariationSmoothSec);
    gainCoef = onePoleCoef (kGainSmoothSec, sr);
    invTanhSharpness = 1.0f / std::tanh (kChopSharpness);
    reset();
}

void TremoloEffect::reset() noexcept
{
    for (Channel& c : channels)
    {
        c.split.reset();
        c.gainLow = c.gainHigh = 1.0f;
    }
    rateWobble.seed (kRateSeed);
    rateWobble.reset();
    spread.seed (kSpreadSeed);
    spread.reset();
    rng.seed (kDepthSeed);
    depthVariation.snap (1.0f);
    phase = 0.0f;
    primed = false;
}

void TremoloEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    const float amt = clamp (params.amount, 0.0f, 1.0f);
    rate.setTarget (movement::lfoRateHz (params, kMinRateHz, kMaxRateHz));
    depth.setTarget (kMaxDepth * amt);
    chop.setTarget (smoothstep (kChopStart, 1.0f, amt));
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    if (! primed)
    {
        rate.snap (rate.getTarget());
        depth.snap (depth.getTarget());
        chop.snap (chop.getTarget());
        drift.snap (drift.getTarget());
        primed = true;
    }

    const float invSr = 1.0f / (float) sr;
    for (int i = 0; i < numSamples; ++i)
    {
        const float f = rate.next();
        const float dep = depth.next();
        const float ch = chop.next();
        const float d = drift.next();

        phase += f * (1.0f + kRateWobble * d * rateWobble.next()) * invSr;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            depthVariation.setTarget (1.0f - kDepthVariation * d * rng.uniform());
        }

        const float depthNow = dep * depthVariation.next();
        // Average gain is 1 - depth/2 for both shapes; give most of that loss back.
        const float makeup = lerp (1.0f, 1.0f / std::sqrt (1.0f - 0.5f * depthNow), kMakeup);
        const float spreadPhase = d * kMaxSpread * (0.5f + 0.5f * spread.next());

        auto gainAt = [&] (float p) noexcept
        {
            const float s = Lfo::sine (p);
            const float square = std::tanh (kChopSharpness * s) * invTanhSharpness;
            return makeup * (1.0f - depthNow * (0.5f + 0.5f * lerp (s, square, ch)));
        };

        float* io[2] { left + i, right + i };
        for (size_t c = 0; c < channels.size(); ++c)
        {
            Channel& chan = channels[c];
            const float p = phase + (c == 1 ? spreadPhase : 0.0f);
            const float targetLow = gainAt (p - kTintPhase);
            const float targetHigh = gainAt (p + kTintPhase);
            chan.gainLow = flushDenormal (targetLow + (chan.gainLow - targetLow) * gainCoef);
            chan.gainHigh = flushDenormal (targetHigh + (chan.gainHigh - targetHigh) * gainCoef);

            const float x = *io[c];
            const float low = chan.split.lowpass (x);
            *io[c] = low * chan.gainLow + (x - low) * chan.gainHigh;
        }
    }
}

} // namespace warmful
