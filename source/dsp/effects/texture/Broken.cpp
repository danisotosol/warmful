// WARMFUL — TEXTURE / BROKEN: a failing tape motor — periodic pitch drops, lumpy capstan FM and AM, near-dropouts.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Broken.h"

namespace warmful
{
namespace
{
namespace voicing
{
// Transport: mechanical artefacts only, no saturation or tone colouring.
constexpr float kBaseDelayMs = 12.0f;
constexpr float kMinDelayMs = 2.0f;
constexpr float kMaxDelaySec = 1.2f;         // buffer: room for several stacked drops
constexpr float kMaxBacklogSec = 0.25f;      // drops get shallower as the motor falls this far behind

// Periodic stalls: the motor sags then recovers.
constexpr float kDropIntervalSlowSec = 3.0f; // mean period at AMOUNT 0 ...
constexpr float kDropIntervalFastSec = 0.8f; // ... and at AMOUNT 1
constexpr float kDropIntervalJitter = 0.4f;  // +-40 % period jitter
constexpr float kDropMinSemis = 3.0f;        // drop depth at AMOUNT 1, random per event
constexpr float kDropMaxSemis = 7.0f;
constexpr float kDropMinSec = 0.15f;
constexpr float kDropMaxSec = 0.35f;
constexpr float kDropSagPortion = 0.3f;      // fast sag, slower recovery: a stalling motor
constexpr float kDropLevelPerSpeed = 1.5f;   // level sags with the motor (1.5 x the speed loss)

// Servo: after a stall the motor runs slightly fast until it has caught up (bounded overspeed).
constexpr float kServoSec = 0.3f;
constexpr float kServoMaxSpeed = 0.035f;     // ~ +0.6 semitone at most

// Lumpy capstan: non-sinusoidal FM and motor-synchronous AM.
constexpr float kCapstanSlowHz = 2.0f;
constexpr float kCapstanFastHz = 8.0f;
constexpr float kCapstanJitterHz = 1.5f;
constexpr float kCapstanRateJitter = 0.25f;  // +-25 % rate wander at AMOUNT 1
constexpr float kCapstanMinDev = 0.001f;     // peak speed error
constexpr float kCapstanMaxDev = 0.012f;
constexpr float kAmMinDepth = 0.02f;
constexpr float kAmMaxDepth = 0.4f;
constexpr float kAmMeanShape = 0.2f;         // mean of the AM dip shape, for level compensation

// Near-dropouts (head losing contact).
constexpr float kDropoutRateMax = 0.8f;      // events per second at AMOUNT 1 (scaled by AMOUNT^2)
constexpr float kDropoutMinSec = 0.02f;
constexpr float kDropoutMaxSec = 0.12f;
constexpr float kDropoutMinDb = 15.0f;
constexpr float kDropoutMaxDb = 30.0f;
constexpr float kDropoutSmoothSec = 0.003f;

constexpr uint32_t kSeed = 0xB80CE411u;
} // namespace voicing
using namespace voicing;

/** Capstan "lump": a sine with 2nd/3rd harmonics, bounded to [-1, 1]. */
float lumpy (float phase) noexcept
{
    const float w = kTwoPi * phase;
    return (std::sin (w) + 0.5f * std::sin (2.0f * w + 0.9f) + 0.25f * std::sin (3.0f * w + 2.1f)) * (1.0f / 1.75f);
}

/** Speed-loss envelope of one stall, u in [0, 1]: quick sag, slower recovery. */
float dropEnvelope (float u) noexcept
{
    if (u < kDropSagPortion)
    {
        const float s = std::sin (kHalfPi * u / kDropSagPortion);
        return s * s;
    }
    return 0.5f + 0.5f * std::cos (kPi * (u - kDropSagPortion) / (1.0f - kDropSagPortion));
}

float meanDropInterval (float a) noexcept { return expMap (a, kDropIntervalSlowSec, kDropIntervalFastSec); }

} // namespace

using texture::kControlInterval;

void BrokenEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    amount.prepare (sampleRate);
    for (auto& l : lines)
        l.prepare (texture::toSamples (kMaxDelaySec, sampleRate));
    const float fs = (float) sampleRate;
    baseDelay = kBaseDelayMs * 0.001f * fs;
    minDelay = kMinDelayMs * 0.001f * fs;
    maxDelay = (float) (lines[0].maxDelay() - 4);
    servoCoef = 1.0f / (kServoSec * fs);
    capstanJitter.setRate (kCapstanJitterHz, sampleRate / (double) kControlInterval);
    dropoutCoef = onePoleCoef (kDropoutSmoothSec, sampleRate);
    reset();
}

void BrokenEffect::reset() noexcept
{
    amount.reset();
    for (auto& l : lines)
        l.reset();
    delay = baseDelay;
    capstan.setPhase (0.0f);
    capstanJitter.seed (kSeed ^ 0x1111u);
    capstanJitter.reset();
    events.seed (kSeed);
    dropActive = false;
    dropPhase = dropInc = dropDepth = 0.0f;
    // First stall a little after start, so a freshly selected BROKEN announces itself quickly.
    samplesToNextDrop = texture::toSamples (0.5f * kDropIntervalFastSec, sr);
    dropoutRemaining = 0;
    dropoutTarget = dropoutGain = 1.0f;
    controlCountdown = 0;
}

void BrokenEffect::startDrop (float a) noexcept
{
    const float meanSec = meanDropInterval (a);
    samplesToNextDrop = texture::toSamples (meanSec * (1.0f + kDropIntervalJitter * events.bipolar()), sr);
    if (dropActive)
        return;

    const float semis = a * lerp (kDropMinSemis, kDropMaxSemis, events.uniform());
    const float backlogSec = (float) (delay - (double) baseDelay) / (float) sr;
    const float headroom = clamp (1.0f - backlogSec / kMaxBacklogSec, 0.0f, 1.0f);
    dropDepth = (1.0f - std::exp2 (-semis / 12.0f)) * headroom;
    dropInc = 1.0f / (lerp (kDropMinSec, kDropMaxSec, events.uniform()) * (float) sr);
    dropPhase = 0.0f;
    dropActive = true;
}

void BrokenEffect::updateControls() noexcept
{
    const float a = amount.next();
    amountNow = a;

    const float capstanHz = expMap (a, kCapstanSlowHz, kCapstanFastHz)
                          * (1.0f + kCapstanRateJitter * a * capstanJitter.next());
    capstan.setFrequency (capstanHz, sr);
    capstanDev = lerp (kCapstanMinDev, kCapstanMaxDev, a);
    amDepth = lerp (kAmMinDepth, kAmMaxDepth, a);
    amMakeup = 1.0f / (1.0f - amDepth * kAmMeanShape);
    dropoutRate = kDropoutRateMax * a * a;

    // Turning AMOUNT up shortens a long pending wait instead of sitting it out.
    const int longestWait = texture::toSamples (meanDropInterval (a) * (1.0f + kDropIntervalJitter), sr);
    samplesToNextDrop = std::min (samplesToNextDrop, longestWait);

    const float dt = (float) kControlInterval / (float) sr;
    if (dropoutRemaining == 0 && texture::eventFires (events, dropoutRate, dt))
    {
        dropoutRemaining = texture::toSamples (lerp (kDropoutMinSec, kDropoutMaxSec, events.uniform()), sr);
        dropoutTarget = dbToGain (-lerp (kDropoutMinDb, kDropoutMaxDb, events.uniform()));
    }
}

void BrokenEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    amount.setTarget (params.amount);

    for (int i = 0; i < numSamples; ++i)
    {
        if (--controlCountdown < 0)
        {
            controlCountdown = kControlInterval - 1;
            updateControls();
        }
        if (--samplesToNextDrop <= 0)
            startDrop (amountNow);

        capstan.advance();
        const float lump = lumpy (capstan.getPhase());

        float stall = 0.0f;
        if (dropActive)
        {
            stall = dropEnvelope (dropPhase);
            dropPhase += dropInc;
            dropActive = dropPhase < 1.0f;
        }

        // Motor speed -> playback delay: delay grows while the tape runs slow.
        const float servo = clamp ((float) (delay - (double) baseDelay) * servoCoef, -kServoMaxSpeed, kServoMaxSpeed);
        const float speed = 1.0f - dropDepth * stall + capstanDev * lump + servo;
        delay = clamp (delay + (double) (1.0f - speed), (double) minDelay, (double) maxDelay);

        if (dropoutRemaining > 0 && --dropoutRemaining == 0)
            dropoutTarget = 1.0f;
        dropoutGain = dropoutTarget + (dropoutGain - dropoutTarget) * dropoutCoef;

        const float load = 0.5f + 0.5f * lump;
        const float gain = (1.0f - amDepth * load * load * load) * amMakeup
                         * std::max (0.0f, 1.0f - kDropLevelPerSpeed * dropDepth * stall)
                         * dropoutGain;

        const float d = (float) delay;
        lines[0].push (left[i]);
        lines[1].push (right[i]);
        left[i] = lines[0].readHermite (d) * gain;
        right[i] = lines[1].readHermite (d) * gain;
    }
}

} // namespace warmful
