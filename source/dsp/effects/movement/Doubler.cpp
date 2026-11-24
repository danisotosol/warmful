// WARMFUL — DOUBLER: stereo double tracking, from tight ADT to short slap-back.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Doubler.h"

namespace warmful
{

namespace voicing
{
    constexpr float kMinBaseMs = 12.0f;                         // RATE fully left: tight ADT
    constexpr float kMaxBaseMs = 110.0f;                        // RATE fully right: short slap-back
    constexpr std::array<float, 2> kTakeRatio { 1.0f, 1.37f };  // the two takes sit at different distances
    constexpr float kRateGlideSec = 0.25f;                      // moving RATE slides the takes (tape-like)
    constexpr float kControlSmoothSec = 0.03f;
    constexpr float kWanderFloorMs = 0.45f;                     // timing wander = floor + share of the base delay
    constexpr float kWanderPerMs = 0.025f;                      //   -> ~0.75 ms (tight) ... ~3.2 ms (slap)
    constexpr std::array<float, 2> kWanderHz { 0.37f, 0.53f };  // human timing variance per take
    constexpr float kDriftWanderDepth = 1.0f;                   // DRIFT max doubles the wander depth...
    constexpr float kDriftWanderRate = 1.8f;                    // ...and speeds it up to ~1.5 Hz
    constexpr float kMaxBendsPerSec = 1.6f;                     // DRIFT max: average momentary pitch shifts per take
    constexpr float kBendMinCents = 10.0f;                      // bend size at low DRIFT
    constexpr float kBendMaxCents = 60.0f;                      // bend size at DRIFT max
    constexpr float kBendMinSec = 0.08f, kBendMaxSec = 0.30f;   // bend duration range
    constexpr std::array<float, 2> kTakeLowpassHz { 5200.0f, 10500.0f };  // left take darker
    constexpr std::array<float, 2> kTakeHighpassHz { 140.0f, 90.0f };
    constexpr float kTakeBleed = 0.22f;                         // takes panned wide but not hard
    constexpr float kLevelComp = 0.6f;                          // make-up 1/sqrt(1 + k * mix^2)
    constexpr float kBufferMs = 200.0f;                         // > 110 * 1.37 + wander + bends
    constexpr std::array<uint32_t, 2> kSeeds { 0x0D0B1E01u, 0x5EC0DD02u };
} // namespace voicing

using namespace voicing;

void DoublerEffect::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (size_t t = 0; t < takes.size(); ++t)
    {
        Take& take = takes[t];
        take.delay.prepare ((int) std::ceil (kBufferMs * 0.001 * sr));
        take.lowpass.setCutoff (kTakeLowpassHz[t], sr);
        take.highpass.setCutoff (kTakeHighpassHz[t], sr);
    }
    baseDelay.prepare (sr, kRateGlideSec);
    mix.prepare (sr, kControlSmoothSec);
    drift.prepare (sr, kControlSmoothSec);
    reset();
}

void DoublerEffect::reset() noexcept
{
    for (size_t t = 0; t < takes.size(); ++t)
    {
        Take& take = takes[t];
        take.delay.reset();
        take.wander.seed (kSeeds[t]);
        take.wander.reset();
        take.rng.seed (kSeeds[t] ^ 0xA5A5A5A5u);
        take.lowpass.reset();
        take.highpass.reset();
        take.bending = false;
    }
    primed = false;
}

float DoublerEffect::bendOffset (Take& take, float startChance, float driftAmount) noexcept
{
    if (! take.bending)
    {
        if (startChance <= 0.0f || take.rng.uniform() >= startChance)
            return 0.0f;

        // A raised-cosine bump in the delay: pitch bends one way, then back, peaking at `cents`.
        const float length = lerp (kBendMinSec, kBendMaxSec, take.rng.uniform()) * (float) sr;
        const float cents = lerp (kBendMinCents, kBendMaxCents, driftAmount) * lerp (0.6f, 1.0f, take.rng.uniform());
        const float slope = 1.0f - std::exp2 (-cents / 1200.0f);
        const float sign = take.rng.uniform() < 0.5f ? -1.0f : 1.0f;
        take.bendDepth = sign * slope * length / kPi;
        take.bendInc = 1.0f / length;
        take.bendPos = 0.0f;
        take.bending = true;
    }

    take.bendPos += take.bendInc;
    if (take.bendPos >= 1.0f)
    {
        take.bending = false;
        return 0.0f;
    }
    return take.bendDepth * 0.5f * (1.0f - std::cos (kTwoPi * take.bendPos));
}

void DoublerEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    const float msToSamples = 0.001f * (float) sr;
    baseDelay.setTarget (expMap (clamp (params.knob, 0.0f, 1.0f), kMinBaseMs, kMaxBaseMs) * msToSamples);
    mix.setTarget (clamp (params.amount, 0.0f, 1.0f));
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    if (! primed)
    {
        baseDelay.snap (baseDelay.getTarget());
        mix.snap (mix.getTarget());
        drift.snap (drift.getTarget());
        primed = true;
    }

    // Blocks are short: random rates and the bend probability are updated per block.
    const float blockDrift = drift.getCurrent();
    for (size_t t = 0; t < takes.size(); ++t)
        takes[t].wander.setRate (kWanderHz[t] * (1.0f + kDriftWanderRate * blockDrift), sr);
    const float bendChance = kMaxBendsPerSec * blockDrift / (float) sr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float base = baseDelay.next();
        const float g = mix.next();
        const float d = drift.next();
        const float wanderDepth = (kWanderFloorMs * msToSamples + kWanderPerMs * base) * (1.0f + kDriftWanderDepth * d);

        const float in[2] { left[i], right[i] };
        float take[2];
        for (size_t t = 0; t < takes.size(); ++t)
        {
            Take& tk = takes[t];
            tk.delay.push (in[t]);
            const float delay = base * kTakeRatio[t] + wanderDepth * tk.wander.next() + bendOffset (tk, bendChance, d);
            take[t] = tk.lowpass.lowpass (tk.highpass.highpass (tk.delay.readHermite (delay)));
        }

        const float makeup = 1.0f / std::sqrt (1.0f + kLevelComp * g * g);
        left[i] = makeup * (in[0] + g * (take[0] + kTakeBleed * take[1]));
        right[i] = makeup * (in[1] + g * (take[1] + kTakeBleed * take[0]));
    }
}

} // namespace warmful
