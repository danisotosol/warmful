// WARMFUL — CASCADE: heightened bucket-brigade analog delay (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Cascade.h"

#include "DiffusionCommon.h"
#include "Dynamics.h"
#include "Tempo.h"

namespace warmful
{

namespace
{
namespace voicing
{
    constexpr float kMinTimeSec = 0.040f;        // TIME fully left: short slapback
    constexpr float kMaxTimeSec = 0.650f;        // TIME fully right: around a quarter note
    constexpr float kMaxSyncedSec = 3.0f;        // longest synced echo (quarter at 20 BPM); longer ones fold by octaves
    constexpr float kTimeSlewSec = 0.2f;         // RC on the delay pot: twisting TIME glides the BBD clock
    constexpr float kMaxGlide = 0.8f;            // delay change cap per sample: read speed 0.2..1.8, never reverses
    constexpr float kEchoFloor = 0.18f;          // echo level at AMOUNT 0 (one quiet repeat)
    constexpr float kEchoRampEnd = 0.1f;         // AMOUNT where the echo reaches full level
    constexpr float kMaxFeedback = 1.05f;        // loop gain at AMOUNT 1: self-oscillates into the saturator
    constexpr float kFeedbackCurve = 0.8f;       // < 1 front-loads the repeats along the knob
    constexpr float kAaShortHz = 6000.0f;        // anti-alias cutoff at the shortest free delay
    constexpr float kAaLongHz = 2500.0f;         // ... and at the longest (slower BBD clock = darker)
    constexpr float kAaQ = 0.6f;
    constexpr float kReconRatio = 1.4f;          // reconstruction filter sits a little above the AA filter
    constexpr float kLoopHpHz = 120.0f;          // coupling HPF in the repeat path
    constexpr float kSatCeiling = 0.9f;          // compander/BBD headroom inside the loop
    constexpr float kInputGain = 0.9f;
    constexpr float kRightTapRatio = 0.035f;     // right output tap slightly later: subtle width
    constexpr float kWobbleHz = 0.6f;            // clock wobble LFO
    constexpr float kWanderHz = 1.7f;            // random clock instability (DRIFT)
    constexpr float kWobbleBaseRel = 0.0005f;    // modulation depth at DRIFT 0, fraction of the delay
    constexpr float kWobbleDriftRel = 0.006f;    // added at DRIFT 1, fraction of the delay
    constexpr float kWobbleDriftSec = 0.0008f;   // absolute depth added at DRIFT 1 so short delays wobble too
    constexpr float kBbdStages = 8192.0f;        // two 4096-stage chips: clock = stages / (2 * delay)
    constexpr float kHoldHeadroom = 8.0f;        // at DRIFT 0 the S&H clock runs this far above the BBD clock
    constexpr float kNoiseMax = 0.0015f;         // loop noise at DRIFT 1 (about -56 dBFS)
    constexpr float kDriftLoss = 0.04f;          // extra per-repeat loss at DRIFT 1
    constexpr float kDriftDarken = 0.35f;        // relative AA cutoff reduction at DRIFT 1
    constexpr float kWetGain = 1.0f;
    constexpr float kGainSmoothSec = 0.03f;
    constexpr float kDriftSmoothSec = 0.05f;
} // namespace voicing
} // namespace

void CascadeEffect::prepare (double sampleRate, int)
{
    using namespace voicing;
    sr = sampleRate;
    maxDelaySamples = kMaxSyncedSec * (float) sr;
    const float modMargin = maxDelaySamples * (kRightTapRatio + 2.0f * (kWobbleBaseRel + kWobbleDriftRel))
                            + 2.0f * kWobbleDriftSec * (float) sr;
    line.prepare ((int) std::ceil (maxDelaySamples + modMargin) + 8);

    glideCoef = 1.0f - onePoleCoef (kTimeSlewSec, sr);
    level.prepare (sr, kGainSmoothSec);
    feedback.prepare (sr, kGainSmoothSec);
    wet.prepare (sr, kGainSmoothSec);
    drift.prepare (sr / diffusion::kControlBlock, kDriftSmoothSec);
    wobble.setFrequency (kWobbleHz, sr);
    wander.setRate (kWanderHz, sr);
    loopHp.setCutoff (kLoopHpHz, sr);
    reset();
}

void CascadeEffect::reset() noexcept
{
    line.reset();
    antiAlias.reset();
    reconL.reset();
    reconR.reset();
    loopHp.reset();
    wobble.setPhase (0.0f);
    wander.seed (0xCA5CAD01u);
    noise.seed (0xCA5CAD02u);
    holdPhase = 0.0f;
    held = 0.0f;
    primed = false;
}

void CascadeEffect::updateControl (float d) noexcept
{
    using namespace voicing;
    const float seconds = delay / (float) sr;

    // Longer delay = slower BBD clock = lower anti-alias/reconstruction cutoffs.
    const float t = clamp (std::log (seconds / kMinTimeSec) / std::log (kMaxTimeSec / kMinTimeSec), 0.0f, 1.0f);
    const float aaHz = expMap (t, kAaShortHz, kAaLongHz) * (1.0f - kDriftDarken * d);
    antiAlias.setParams (aaHz, kAaQ, sr);
    reconL.setCutoff (aaHz * kReconRatio, sr);
    reconR.setCutoff (aaHz * kReconRatio, sr);

    wobbleDepth = delay * (kWobbleBaseRel + d * kWobbleDriftRel) + d * kWobbleDriftSec * (float) sr;
    wanderDepth = wobbleDepth * d;

    // Clock sample-and-hold: inaudible at DRIFT 0, approaches the real BBD clock (aliasing grit) at DRIFT 1.
    const float clockHz = kBbdStages / (2.0f * seconds);
    holdInc = std::min (1.0f, clockHz * (1.0f + (1.0f - d) * kHoldHeadroom) / (float) sr);
    noiseLevel = d * kNoiseMax;
    loss = 1.0f - d * kDriftLoss;
}

void CascadeEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    using namespace voicing;
    const float seconds = params.tempo.synced
        ? diffusion::foldSeconds (tempo::syncedEchoSeconds (params.knob, params.tempo.bpm), kMaxSyncedSec)
        : expMap (clamp (params.knob, 0.0f, 1.0f), kMinTimeSec, kMaxTimeSec);
    const float target = clamp (seconds * (float) sr, 1.0f, maxDelaySamples);
    const auto echo = diffusion::echoAmount (params.amount, kEchoFloor, kEchoRampEnd, kMaxFeedback, kFeedbackCurve);

    level.setTarget (echo.level);
    feedback.setTarget (echo.feedback);
    wet.setTarget (params.wetLevel * kWetGain);
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    if (! primed)
    {
        delay = target;
        level.snap (echo.level);
        feedback.snap (echo.feedback);
        wet.snap (params.wetLevel * kWetGain);
        drift.snap (drift.getTarget());
        primed = true;
    }

    for (int start = 0; start < numSamples; start += diffusion::kControlBlock)
    {
        const int end = std::min (numSamples, start + diffusion::kControlBlock);
        updateControl (drift.next());

        for (int i = start; i < end; ++i)
        {
            delay += clamp ((target - delay) * glideCoef, -kMaxGlide, kMaxGlide);
            wobble.advance();
            const float tap = delay + wobble.sine() * wobbleDepth + wander.next() * wanderDepth;
            const float yL = reconL.lowpass (line.readHermite (tap));
            const float yR = reconR.lowpass (line.readHermite (tap + delay * kRightTapRatio));

            const float in = 0.5f * (left[i] + right[i]) * kInputGain;
            const float x = antiAlias.process (in + loopHp.highpass (yL) * feedback.next() * loss).lp;
            holdPhase += holdInc;
            if (holdPhase >= 1.0f)
            {
                holdPhase -= 1.0f;
                held = x;
            }
            const float bucket = held + noise.gaussian() * noiseLevel;
            line.push (kSatCeiling * shape::fastTanh (bucket / kSatCeiling));

            const float g = level.next() * wet.next();
            left[i] += yL * g;
            right[i] += yR * g;
        }
    }
}

} // namespace warmful
