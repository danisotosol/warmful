// WARMFUL — TEXTURE / SQUASH: heavy-handed stereo-linked compressor that tips into overdrive near 100 %.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Squash.h"
#include "Dynamics.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr float kRatioMin = 2.0f;            // AMOUNT 0: subtle polish ...
constexpr float kRatioMax = 20.0f;           // ... AMOUNT 1: limiter-like squash
constexpr float kThresholdHighDb = -10.0f;   // AMOUNT 0 only touches playing peaks (~ -6 dBFS)
constexpr float kThresholdLowDb = -38.0f;
constexpr float kKneeSoftDb = 6.0f;
constexpr float kKneeWideDb = 10.0f;
constexpr float kAttackSlowSec = 0.002f;     // lets a little pick attack through at low AMOUNT ...
constexpr float kAttackFastSec = 0.0003f;    // ... and clamps it at high AMOUNT
constexpr float kReleaseFastSec = 0.08f;     // program dependent release: short on transients ...
constexpr float kReleaseSlowSec = 0.3f;      // ... longer when the squash is sustained (~120 ms typical)
constexpr float kReleaseAverageSec = 0.5f;   // how long gain reduction must persist to slow the release
constexpr float kReleaseSlowGrDb = 12.0f;    // average gain reduction that gives the slowest release
constexpr float kMakeupRefDb = -10.0f;       // level whose gain reduction the auto make-up restores
constexpr float kMakeupFraction = 0.9f;
constexpr float kMakeupMaxDb = 24.0f;
constexpr float kDriveStart = 0.72f;         // overdrive fades in over the last quarter of AMOUNT
constexpr float kDriveMaxDb = 18.0f;
constexpr float kDriveRefLevel = 0.5f;       // drive make-up keeps this peak level unchanged
constexpr float kOutputKnee = 0.9f;          // final soft ceiling: catches attack overshoot after make-up
constexpr float kDetectorFloor = 1.0e-6f;
} // namespace voicing
} // namespace

using namespace voicing;
using texture::kControlInterval;

void SquashEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    amount.prepare (sampleRate);
    averageCoef = onePoleCoef (kReleaseAverageSec, sampleRate);
    reset();
}

void SquashEffect::reset() noexcept
{
    amount.reset();
    grDb = grAverageDb = 0.0f;
    controlCountdown = 0;
}

float SquashEffect::gainComputerDb (float levelDb) const noexcept
{
    // Soft-knee static curve; returns the gain change in dB (<= 0).
    const float over = levelDb - thresholdDb;
    if (2.0f * over <= -kneeDb)
        return 0.0f;
    if (2.0f * over < kneeDb)
    {
        const float k = over + 0.5f * kneeDb;
        return slope * k * k / (2.0f * kneeDb);
    }
    return slope * over;
}

void SquashEffect::updateControls() noexcept
{
    const float a = amount.next();

    thresholdDb = lerp (kThresholdHighDb, kThresholdLowDb, a);
    slope = 1.0f / expMap (a, kRatioMin, kRatioMax) - 1.0f;
    kneeDb = lerp (kKneeSoftDb, kKneeWideDb, a);
    makeupDb = std::min (kMakeupMaxDb, -kMakeupFraction * gainComputerDb (kMakeupRefDb));

    attackCoef = onePoleCoef (expMap (a, kAttackSlowSec, kAttackFastSec), sr);
    const float sustained = clamp (-grAverageDb / kReleaseSlowGrDb, 0.0f, 1.0f);
    releaseCoef = onePoleCoef (lerp (kReleaseFastSec, kReleaseSlowSec, sustained), sr);

    // Overdrive after the compressor, normalised so a kDriveRefLevel peak keeps its level.
    driveMix = smoothstep (kDriveStart, 1.0f, a);
    driveGain = dbToGain (driveMix * kDriveMaxDb);
    driveNorm = kDriveRefLevel / shape::fastTanh (driveGain * kDriveRefLevel);
}

void SquashEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    amount.setTarget (params.amount);

    for (int i = 0; i < numSamples; ++i)
    {
        if (--controlCountdown < 0)
        {
            controlCountdown = kControlInterval - 1;
            updateControls();
        }

        // Stereo-linked peak detector, smoothing in the dB domain (decoupled attack/release).
        const float peak = std::max (std::abs (left[i]), std::abs (right[i]));
        const float target = gainComputerDb (gainToDb (std::max (peak, kDetectorFloor)));
        const float coef = target < grDb ? attackCoef : releaseCoef;
        grDb = target + (grDb - target) * coef;
        grAverageDb = grDb + (grAverageDb - grDb) * averageCoef;

        const float gain = dbToGain (grDb + makeupDb);
        auto squash = [this, gain] (float x) noexcept
        {
            const float y = x * gain;
            return shape::kneeClip (y + driveMix * (shape::fastTanh (driveGain * y) * driveNorm - y), kOutputKnee);
        };
        left[i] = squash (left[i]);
        right[i] = squash (right[i]);
    }
}

} // namespace warmful
