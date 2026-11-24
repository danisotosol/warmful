// WARMFUL — COLLAGE: wild, destructively edited looping delay (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Collage.h"

#include "DiffusionCommon.h"
#include "Dynamics.h"
#include "Tempo.h"

namespace warmful
{

namespace
{
namespace voicing
{
    constexpr float kMinLoopSec = 0.025f;        // TIME fully left: short granular slices
    constexpr float kMaxLoopSec = 2.4f;          // TIME fully right: phrase-length loops
    constexpr float kMaxSyncedSec = 4.8f;        // longest synced loop kept; longer ones fold by octaves
    constexpr float kScrubSmoothSec = 0.02f;     // only de-zippers TIME: knob speed becomes read-head speed
    constexpr float kMaxScrub = 4.0f;            // delay change cap per sample: read speed -3..+5
    constexpr float kEchoFloor = 0.2f;           // loop level at AMOUNT 0 (one quiet repeat)
    constexpr float kEchoRampEnd = 0.1f;
    constexpr float kMaxFeedback = 1.02f;        // evolving oscillation at AMOUNT 1
    constexpr float kFeedbackCurve = 0.85f;
    constexpr float kLimiterKnee = 0.7f;         // soft limiter inside the loop: clean below the knee
    constexpr float kHfLossHz = 11000.0f;        // gentle per-pass HF loss of the "digital" loop
    constexpr float kWanderHz = 0.5f;            // slow random pitch modulation of the read head (DRIFT)
    constexpr float kWanderSec = 0.006f;         // depth at DRIFT 1 (about +-15 cents)
    constexpr float kWanderMaxRel = 0.25f;       // never wander more than this fraction of the loop
    constexpr float kDoubleChance = 0.35f;       // probability per loop cycle of a double-speed cycle at DRIFT 1
    constexpr float kDoubleFadeSec = 0.01f;      // raised-cosine fade in/out of the double-speed head
    constexpr float kWetGain = 0.8f;             // dry at unity, loop slightly below
    constexpr float kGainSmoothSec = 0.03f;
} // namespace voicing
} // namespace

void CollageEffect::prepare (double sampleRate, int)
{
    using namespace voicing;
    sr = sampleRate;
    maxLoopSamples = kMaxSyncedSec * (float) sr;
    // The double-speed head reads two loop lengths of history.
    for (size_t c = 0; c < 2; ++c)
    {
        lines[c].prepare ((int) std::ceil (2.0f * maxLoopSamples + 2.0f * kWanderSec * (float) sr) + 8);
        hfLoss[c].setCutoff (kHfLossHz, sr);
        dcBlock[c].prepare (sr);
    }
    scrubCoef = 1.0f - onePoleCoef (kScrubSmoothSec, sr);
    level.prepare (sr, kGainSmoothSec);
    feedback.prepare (sr, kGainSmoothSec);
    wet.prepare (sr, kGainSmoothSec);
    drift.prepare (sr, kGainSmoothSec);
    wander.setRate (kWanderHz, sr);
    reset();
}

void CollageEffect::reset() noexcept
{
    for (size_t c = 0; c < 2; ++c)
    {
        lines[c].reset();
        hfLoss[c].reset();
        dcBlock[c].reset();
    }
    wander.seed (0xC011A6E1u);
    rng.seed (0xC011A6E2u);
    cyclePos = doubleTime = doubleLength = 0.0f;
    doubleActive = false;
    primed = false;
}

void CollageEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    using namespace voicing;
    const float seconds = params.tempo.synced
        ? diffusion::foldSeconds (tempo::syncedCollageSeconds (params.knob, params.tempo.bpm), kMaxSyncedSec)
        : expMap (clamp (params.knob, 0.0f, 1.0f), kMinLoopSec, kMaxLoopSec);
    const float target = clamp (seconds * (float) sr, 1.0f, maxLoopSamples);
    const auto echo = diffusion::echoAmount (params.amount, kEchoFloor, kEchoRampEnd, kMaxFeedback, kFeedbackCurve);

    level.setTarget (echo.level);
    feedback.setTarget (echo.feedback);
    wet.setTarget (params.wetLevel * kWetGain);
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    if (! primed)
    {
        loopDelay = target;
        level.snap (echo.level);
        feedback.snap (echo.feedback);
        wet.snap (params.wetLevel * kWetGain);
        drift.snap (drift.getTarget());
        primed = true;
    }

    const float wanderMax = kWanderSec * (float) sr;
    const float fadeMax = kDoubleFadeSec * (float) sr;
    for (int i = 0; i < numSamples; ++i)
    {
        // The read head moves with the knob: its speed relative to the write head is the pitch of the edit.
        loopDelay += clamp ((target - loopDelay) * scrubCoef, -kMaxScrub, kMaxScrub);
        const float d = drift.next();
        const float mod = wander.next() * d * std::min (wanderMax, kWanderMaxRel * loopDelay);
        const float mainTap = loopDelay + mod;

        // Once per loop cycle DRIFT may replay the last two cycles at double speed for one cycle.
        cyclePos += 1.0f;
        if (cyclePos >= loopDelay)
        {
            cyclePos = 0.0f;
            if (! doubleActive && rng.uniform() < d * kDoubleChance)
            {
                doubleActive = true;
                doubleTime = 0.0f;
                doubleLength = loopDelay;
            }
        }
        float blend = 0.0f, doubleTap = 0.0f;
        if (doubleActive)
        {
            const float fade = std::max (1.0f, std::min (fadeMax, 0.25f * doubleLength));
            const float edge = std::min (doubleTime, doubleLength - doubleTime) / fade;
            blend = edge >= 1.0f ? 1.0f : 0.5f - 0.5f * std::cos (kPi * edge);
            doubleTap = 2.0f * doubleLength - doubleTime + mod; // delay shrinks 1/sample: reads at 2x speed
            doubleTime += 1.0f;
            if (doubleTime >= doubleLength)
                doubleActive = false;
        }

        const float fb = feedback.next();
        const float g = level.next() * wet.next();
        float* io[2] = { left + i, right + i };
        for (size_t c = 0; c < 2; ++c)
        {
            DelayLine& line = lines[c];
            float y = line.readHermite (mainTap);
            if (blend > 0.0f)
                y = lerp (y, line.readHermite (doubleTap), blend);

            // Write the (possibly edited) loop back: the manipulations fold into the following repeats.
            const float x = *io[c];
            const float recirculated = dcBlock[c].process (hfLoss[c].lowpass (y)) * fb;
            line.push (shape::kneeClip (x + recirculated, kLimiterKnee));
            *io[c] = x + y * g;
        }
    }
}

} // namespace warmful
