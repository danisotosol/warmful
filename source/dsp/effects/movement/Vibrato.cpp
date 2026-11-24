// WARMFUL — VIBRATO: sine to tape-warble pitch modulation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Vibrato.h"
#include "MovementCommon.h"

namespace warmful
{

namespace voicing
{
    constexpr float kMinRateHz = 0.1f, kMaxRateHz = 12.0f;  // free RATE range (exponential)
    constexpr float kMaxCents = 75.0f;                      // peak pitch deviation at full AMOUNT
    constexpr float kDepthCurve = 1.5f;                     // AMOUNT -> cents: finer control of subtle vibrato
    constexpr float kCentreMs = 6.0f;                       // nominal centre delay
    constexpr float kMaxDepthMs = 10.0f;                    // sweep cap: very slow rates lose some depth
    constexpr float kMarginMs = 0.5f;                       // closest the tap gets to the write head
    constexpr float kRateSmoothSec = 0.05f;
    constexpr float kControlSmoothSec = 0.03f;
    constexpr float kCentreGlideSec = 0.3f;                 // centre follows the depth slowly (no pitch blips)
    constexpr float kRandomRateLo = 0.6f;                   // random warble rate relative to RATE at DRIFT 0...
    constexpr float kRandomRateHi = 1.9f;                   // ...and at DRIFT max
    constexpr float kRandomRateSpreadR = 1.13f;             // right random source runs slightly faster
    constexpr float kSineFadeStart = 0.4f;                  // DRIFT where the sine starts giving way
    constexpr float kRandomFullAt = 0.9f;                   // DRIFT where the random shape is fully in
    constexpr float kMaxStereoPhase = 0.25f;                // L/R LFO offset at DRIFT max (90 degrees)
    constexpr float kBufferMs = 40.0f;
    constexpr std::array<uint32_t, 2> kSeeds { 0x71B4A701u, 0x2C0FFEE2u };
} // namespace voicing

using namespace voicing;

void VibratoEffect::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (Channel& c : channels)
        c.delay.prepare ((int) std::ceil (kBufferMs * 0.001 * sr));
    rate.prepare (sr, kRateSmoothSec);
    slope.prepare (sr, kControlSmoothSec);
    drift.prepare (sr, kControlSmoothSec);
    centre.prepare (sr, kCentreGlideSec);
    reset();
}

void VibratoEffect::reset() noexcept
{
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        channels[ch].delay.reset();
        channels[ch].random.seed (kSeeds[ch]);
        channels[ch].random.reset();
    }
    phase = 0.0f;
    primed = false;
}

void VibratoEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    const float msToSamples = 0.001f * (float) sr;
    const float invSr = 1.0f / (float) sr;

    // Delay modulation d(t) = D sin(2 pi f t) has peak slope 2 pi f D: the pitch deviation is set
    // by the slope, so the depth in time scales with 1/rate.
    const float rateTarget = movement::lfoRateHz (params, kMinRateHz, kMaxRateHz);
    const float cents = kMaxCents * std::pow (clamp (params.amount, 0.0f, 1.0f), kDepthCurve);
    const float slopeTarget = std::exp2 (cents / 1200.0f) - 1.0f;
    const float depthTargetMs = std::min (slopeTarget / (kTwoPi * rateTarget) * 1000.0f, kMaxDepthMs);
    rate.setTarget (rateTarget);
    slope.setTarget (slopeTarget);
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    centre.setTarget (std::max (kCentreMs, depthTargetMs + kMarginMs) * msToSamples);
    if (! primed)
    {
        rate.snap (rate.getTarget());
        slope.snap (slope.getTarget());
        drift.snap (drift.getTarget());
        centre.snap (centre.getTarget());
        primed = true;
    }

    const float randomRate = rate.getCurrent() * lerp (kRandomRateLo, kRandomRateHi, drift.getCurrent());
    channels[0].random.setRate (randomRate, sr);
    channels[1].random.setRate (randomRate * kRandomRateSpreadR, sr);

    const float maxDepth = kMaxDepthMs * msToSamples;
    const float margin = kMarginMs * msToSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        const float f = rate.next();
        const float s = slope.next();
        const float d = drift.next();
        const float c = centre.next();

        phase += f * invSr;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float depth = std::min ({ s / (kTwoPi * f) * (float) sr, maxDepth, c - margin });

        // DRIFT: sine -> sine with random wobble -> random warble (normalised to stay within +-1).
        const float sineW = 1.0f - smoothstep (kSineFadeStart, 1.0f, d);
        const float randW = smoothstep (0.0f, kRandomFullAt, d);
        const float norm = 1.0f / (sineW + randW);
        const float randL = channels[0].random.next();
        const float randR = lerp (randL, channels[1].random.next(), d);
        const float mod[2] {
            (sineW * Lfo::sine (phase) + randW * randL) * norm,
            (sineW * Lfo::sine (phase + d * kMaxStereoPhase) + randW * randR) * norm
        };

        float* io[2] { left + i, right + i };
        for (size_t ch = 0; ch < channels.size(); ++ch)
        {
            channels[ch].delay.push (*io[ch]);
            *io[ch] = channels[ch].delay.readHermite (c + depth * mod[ch]);
        }
    }
}

} // namespace warmful
