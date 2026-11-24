// WARMFUL — CHARACTER / SWELL: envelope-triggered volume swells.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Swell.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr float kDetectorAttackSec = 0.001f, kDetectorReleaseSec = 0.03f;
constexpr float kTriggerLevel = 0.05f;           // trigger peak at neutral SENSITIVITY (~-20 dB below calibrated peaks)
constexpr float kRearmRatio = 0.35f;             // re-arm threshold relative to the trigger (~-9 dB hysteresis)
constexpr float kSensitivityRangeDb = 18.0f;     // SENSITIVITY right = lower thresholds
constexpr float kAttackMinSec = 0.04f, kAttackMaxSec = 2.5f;     // AMOUNT, exponential
constexpr float kReleaseMinSec = 0.1f, kReleaseMaxSec = 3.0f;    // AMOUNT, exponential
constexpr float kReleaseTimeConstants = 6.9f;    // release time = time to fall 60 dB
constexpr float kSilenceLevel = 1.0e-5f;         // release considered finished
constexpr float kDeclickSec = 0.002f;
constexpr float kTiltPivotHz = 800.0f;
constexpr float kToneTiltDb = 12.0f;             // TILT tone span
} // namespace voicing
} // namespace

using namespace voicing;
using character::kChunk;

void SwellEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    controls.prepare (sampleRate);
    detector.prepare (sampleRate, kDetectorAttackSec, kDetectorReleaseSec);
    declick.prepare (sampleRate, kDeclickSec);
    reset();
}

void SwellEffect::reset() noexcept
{
    controls.reset();
    detector.reset();
    declick.snap (0.0f);
    for (auto& t : tone)
        t.reset();
    stage = Stage::Idle;
    armed = true;
    level = 0.0f;
    attackPhase = 0.0f;
}

void SwellEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    controls.setTargets (params);
    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = std::min (kChunk, numSamples - start);
        controls.advance (n);
        updateChunk();

        for (int i = start; i < start + n; ++i)
        {
            const float env = detector.process (std::max (std::abs (left[i]), std::abs (right[i])));
            if (armed && env > highThreshold)
            {
                // New note after enough separation: restart the swell from silence.
                armed = false;
                stage = Stage::Attack;
                attackPhase = 0.0f;
            }
            else if (! armed && env < lowThreshold)
            {
                armed = true;
                if (stage == Stage::Attack || stage == Stage::Hold)
                    stage = Stage::Release;
            }

            switch (stage)
            {
                case Stage::Attack:
                    attackPhase = std::min (1.0f, attackPhase + attackStep);
                    level = 0.5f - 0.5f * std::cos (kPi * attackPhase);
                    if (attackPhase >= 1.0f)
                        stage = Stage::Hold;
                    break;
                case Stage::Hold:
                    level = 1.0f;
                    break;
                case Stage::Release:
                    level *= releaseCoef;
                    if (level < kSilenceLevel)
                    {
                        level = 0.0f;
                        stage = Stage::Idle;
                    }
                    break;
                case Stage::Idle:
                    level = 0.0f;
                    break;
            }

            declick.setTarget (level);
            const float g = declick.next();
            left[i] = tone[0].process (left[i]) * g;
            right[i] = tone[1].process (right[i]) * g;
        }
    }
}

void SwellEffect::updateChunk() noexcept
{
    const float amount = controls.amount();
    highThreshold = kTriggerLevel * dbToGain (-kSensitivityRangeDb * controls.sensitivity());
    lowThreshold = highThreshold * kRearmRatio;
    attackStep = 1.0f / (expMap (amount, kAttackMinSec, kAttackMaxSec) * (float) sr);
    releaseCoef = onePoleCoef (expMap (amount, kReleaseMinSec, kReleaseMaxSec) / kReleaseTimeConstants, sr);
    for (auto& t : tone)
        t.set (kTiltPivotHz, kToneTiltDb * controls.tilt(), sr);
}

} // namespace warmful
