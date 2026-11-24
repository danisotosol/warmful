// WARMFUL — REVERSE: reverse delay with variable playback speed/pitch (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Reverse.h"

#include "DiffusionCommon.h"
#include "Dynamics.h"
#include "Tempo.h"

namespace warmful
{

namespace
{
namespace voicing
{
    constexpr float kFreeGrainSec = 0.7f;        // grain length when not synced
    constexpr float kMaxGrainSec = 2.0f;         // tempo::syncedReverseSeconds never exceeds this
    constexpr float kSpeedSmoothSec = 0.08f;     // TIME glides the playback speed like a tape transport
    constexpr float kFeedback = 0.3f;            // reversed signal recirculated for haunting ambience
    constexpr float kFeedbackKnee = 0.8f;
    constexpr float kDriftCents = 40.0f;         // speed wander at DRIFT 1
    constexpr std::array<float, 2> kWanderHz { 0.35f, 0.41f };
    constexpr float kMaxGrainOffset = 0.25f;     // right grains lag the left ones by this fraction at DRIFT 1
    constexpr float kLockGain = 0.5f;            // how fast the right grain clock settles onto its offset
    constexpr float kLockMax = 0.05f;            // max right grain-rate correction while settling
    constexpr float kGainSmoothSec = 0.03f;
    constexpr float kDriftSmoothSec = 0.05f;
} // namespace voicing
} // namespace

void ReverseEffect::prepare (double sampleRate, int)
{
    using namespace voicing;
    sr = sampleRate;
    // A grain covers (1 + speed) * grain length of history behind the write head.
    const float maxSpeed = 2.0f * std::exp2 (kDriftCents / 1200.0f);
    const float maxHistory = (1.0f + maxSpeed) * kMaxGrainSec * (1.0f + kLockMax) * (float) sr;
    const float controlRate = (float) sr / (float) diffusion::kControlBlock;
    for (size_t c = 0; c < 2; ++c)
    {
        lines[c].prepare ((int) std::ceil (maxHistory) + 8);
        wander[c].setRate (kWanderHz[c], controlRate);
    }
    logSpeed.prepare (controlRate, kSpeedSmoothSec);
    drift.prepare (controlRate, kDriftSmoothSec);
    dryGain.prepare (sr, kGainSmoothSec);
    wetGain.prepare (sr, kGainSmoothSec);
    reset();
}

void ReverseEffect::reset() noexcept
{
    const float halfGrainHistory = voicing::kFreeGrainSec * (float) sr; // grain B is half-way at unity speed
    for (size_t c = 0; c < 2; ++c)
    {
        lines[c].reset();
        wander[c].seed (0x2E7E25E1u + (uint32_t) c);
        phase[c] = 0.0f;
        delayA[c] = 0.0f;
        delayB[c] = halfGrainHistory;
        speedStep[c] = 0.0f;
    }
    primed = false;
}

void ReverseEffect::updateControl (int chunkLength, float grainSamples) noexcept
{
    using namespace voicing;
    const float ls = logSpeed.next();
    const float d = drift.next();
    for (size_t c = 0; c < 2; ++c)
    {
        const float target = std::exp2 (ls + d * kDriftCents * wander[c].next() / 1200.0f);
        speedStep[c] = (target - speed[c]) / (float) chunkLength;
    }

    // The right grain clock is steered (never jumped) onto its DRIFT-dependent offset: clickless width.
    phaseInc[0] = 1.0f / grainSamples;
    float err = phase[0] + d * kMaxGrainOffset - phase[1];
    err -= std::round (err);
    phaseInc[1] = phaseInc[0] * (1.0f + clamp (err * kLockGain, -kLockMax, kLockMax));
}

void ReverseEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    using namespace voicing;
    const float grainSec = params.tempo.synced ? std::min (tempo::syncedReverseSeconds (params.tempo.bpm), kMaxGrainSec)
                                               : kFreeGrainSec;
    const float grainSamples = grainSec * (float) sr;
    float dry = 1.0f, wet = 0.0f;
    equalPowerGains (params.amount, dry, wet);

    logSpeed.setTarget (bipolar (params.knob));
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    dryGain.setTarget (dry);
    wetGain.setTarget (wet * params.wetLevel);
    if (! primed)
    {
        logSpeed.snap (logSpeed.getTarget());
        drift.snap (drift.getTarget());
        dryGain.snap (dry);
        wetGain.snap (wet * params.wetLevel);
        speed[0] = speed[1] = std::exp2 (logSpeed.getTarget());
        primed = true;
    }

    float* io[2] = { left, right };
    for (int start = 0; start < numSamples; start += diffusion::kControlBlock)
    {
        const int end = std::min (numSamples, start + diffusion::kControlBlock);
        updateControl (end - start, grainSamples);

        for (int i = start; i < end; ++i)
        {
            const float dg = dryGain.next();
            const float wg = wetGain.next();
            for (size_t c = 0; c < 2; ++c)
            {
                speed[c] += speedStep[c];
                const float previous = phase[c];
                phase[c] += phaseInc[c];
                if (phase[c] >= 1.0f)
                {
                    phase[c] -= 1.0f;
                    delayA[c] = 0.0f; // grain A restarts at "now"
                }
                else if (previous < 0.5f && phase[c] >= 0.5f)
                {
                    delayB[c] = 0.0f; // grain B restarts at "now"
                }

                // Reading backwards at `speed` while the write head moves forwards: delay grows by 1 + speed.
                delayA[c] += 1.0f + speed[c];
                delayB[c] += 1.0f + speed[c];
                const float s = std::sin (kPi * phase[c]);
                const float wA = s * s; // Hann; grain B's window is the complement (constant overlap-add)
                DelayLine& line = lines[c];
                const float y = wA * line.readHermite (delayA[c]) + (1.0f - wA) * line.readHermite (delayB[c]);

                const float x = io[c][i];
                line.push (x + kFeedback * shape::kneeClip (y, kFeedbackKnee));
                io[c][i] = x * dg + y * wg;
            }
        }
    }
}

} // namespace warmful
