// WARMFUL — CHARACTER / SWEETEN: preamp adding EQ, opto compression and gentle saturation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Sweeten.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr int kOversampleLog2 = 1;               // 2x: the saturation is gentle
// "Sweet" EQ at full strength; it fades in over the first part of AMOUNT.
constexpr float kEqFullAt = 0.5f;                // AMOUNT where the EQ reaches full strength
constexpr float kLowShelfHz = 120.0f, kLowShelfDb = 2.0f;
constexpr float kScoopHz = 450.0f, kScoopQ = 0.8f, kScoopDb = -1.5f;           // clears boxiness
constexpr float kPresenceHz = 2800.0f, kPresenceQ = 1.0f, kPresenceDb = 1.5f;
constexpr float kAirHz = 8000.0f, kAirDb = 2.0f;
constexpr float kShelfQ = 0.707f;
constexpr float kTiltPivotHz = 800.0f;
constexpr float kPreTiltDb = 6.0f;               // TILT span ahead of the compressor/saturation
constexpr float kPostTiltDb = 4.0f;              // and after it (gentle preamp: both add up to the tone)
// Opto-style compressor (stereo-linked RMS detector, soft knee, program-dependent release).
constexpr float kDetectorAttackSec = 0.005f, kDetectorReleaseSec = 0.04f;
constexpr float kThresholdMinDb = -12.0f;        // threshold at AMOUNT 0 (dBFS RMS) ...
constexpr float kThresholdMaxDb = -32.0f;        // ... falling to this at AMOUNT 1
constexpr float kRatioMin = 1.5f, kRatioMax = 4.0f;
constexpr float kKneeDb = 8.0f;
constexpr float kSensitivityRangeDb = 12.0f;     // SENSITIVITY moves threshold and saturation drive
constexpr float kAttackSec = 0.01f;
constexpr float kReleaseFastSec = 0.08f;         // after short bursts of reduction ...
constexpr float kReleaseSlowSec = 1.5f;          // ... after sustained reduction (opto cell memory)
constexpr float kMemorySec = 2.0f;
constexpr float kMemoryFullDb = 6.0f;            // sustained reduction that fully slows the release
constexpr float kRefRmsDb = -18.0f;              // calibrated playing level (RMS) for the auto make-up
constexpr float kMakeupFraction = 0.8f;          // leaves a little of the squash audible
// Saturation and output.
constexpr float kSatMinDb = -4.0f, kSatMaxDb = 6.0f;   // drive into the cubic over AMOUNT
constexpr float kRefLevel = 0.3f;                // calibrated playing peak for the saturation make-up
constexpr float kPushDb = 3.0f;                  // output rise at full AMOUNT: pushes the next module
} // namespace voicing

/** Static soft-knee gain computer, returns gain change in dB (<= 0). */
float gainReductionDb (float levelDb, float thresholdDb, float ratio) noexcept
{
    const float over = levelDb - thresholdDb;
    const float slope = 1.0f / ratio - 1.0f;
    if (2.0f * over <= -voicing::kKneeDb)
        return 0.0f;
    if (2.0f * over >= voicing::kKneeDb)
        return slope * over;
    const float t = over + 0.5f * voicing::kKneeDb;
    return slope * t * t / (2.0f * voicing::kKneeDb);
}
} // namespace

using namespace voicing;
using character::kChunk;

void SweetenEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    controls.prepare (sampleRate);
    detector.prepare (sampleRate, kDetectorAttackSec, kDetectorReleaseSec);
    attackCoef = onePoleCoef (kAttackSec, sampleRate);
    releaseCoef = onePoleCoef (kReleaseFastSec, sampleRate);
    memoryCoef = onePoleCoef (kMemorySec, sampleRate);
    for (auto& c : channels)
        c.os.prepare (kChunk, kOversampleLog2);
    reset();
}

void SweetenEffect::reset() noexcept
{
    controls.reset();
    preGainRamp.reset();
    postGainRamp.reset();
    detector.reset();
    grDb = 0.0f;
    grMemory = 0.0f;
    for (auto& c : channels)
    {
        c.lowShelf.reset();
        c.scoop.reset();
        c.presence.reset();
        c.air.reset();
        c.preTilt.reset();
        c.postTilt.reset();
        c.os.reset();
    }
}

void SweetenEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    controls.setTargets (params);
    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = std::min (kChunk, numSamples - start);
        controls.advance (n);
        updateChunk (n);

        float* io[2] = { left + start, right + start };
        for (int ch = 0; ch < 2; ++ch)
        {
            Channel& c = channels[(size_t) ch];
            for (int i = 0; i < n; ++i)
            {
                const float x = c.air.process (c.presence.process (c.scoop.process (c.lowShelf.process (io[ch][i]))));
                c.buf[(size_t) i] = c.preTilt.process (x);
            }
        }
        compress (n);
        for (int ch = 0; ch < 2; ++ch)
            saturate (channels[(size_t) ch], io[ch], n);
    }
}

void SweetenEffect::updateChunk (int n) noexcept
{
    const float tilt = controls.tilt();
    const float amount = controls.amount();
    const float sensDb = controls.sensitivity() * kSensitivityRangeDb;

    const float eq = std::min (1.0f, amount / kEqFullAt);
    for (auto& c : channels)
    {
        c.lowShelf.setLowShelf (kLowShelfHz, kShelfQ, kLowShelfDb * eq, sr);
        c.scoop.setPeak (kScoopHz, kScoopQ, kScoopDb * eq, sr);
        c.presence.setPeak (kPresenceHz, kPresenceQ, kPresenceDb * eq, sr);
        c.air.setHighShelf (kAirHz, kShelfQ, kAirDb * eq, sr);
        c.preTilt.set (kTiltPivotHz, kPreTiltDb * tilt, sr);
        c.postTilt.set (kTiltPivotHz, kPostTiltDb * tilt, sr);
    }

    // More SENSITIVITY = lower threshold and more drive, like calibrating to a hotter input.
    thresholdDb = lerp (kThresholdMinDb, kThresholdMaxDb, amount) - sensDb;
    ratio = lerp (kRatioMin, kRatioMax, amount);
    const float memory = clamp (grMemory / kMemoryFullDb, 0.0f, 1.0f);
    releaseCoef = onePoleCoef (lerp (kReleaseFastSec, kReleaseSlowSec, memory), sr);

    // The compressor make-up goes ahead of the saturation so the squashed signal drives it.
    const float compMakeupDb = -gainReductionDb (kRefRmsDb, thresholdDb, ratio) * kMakeupFraction;
    const float satDrive = dbToGain (lerp (kSatMinDb, kSatMaxDb, amount) + sensDb);
    const float satMakeup = character::makeupGain (kRefLevel, satDrive, shape::cubic (satDrive * kRefLevel), 0.0f);
    preGainRamp.fill (dbToGain (compMakeupDb) * satDrive, preGain.data(), n);
    postGainRamp.fill (satMakeup * dbToGain (kPushDb * amount), postGain.data(), n);
}

void SweetenEffect::compress (int n) noexcept
{
    const auto& l = channels[0].buf;
    const auto& r = channels[1].buf;
    for (int i = 0; i < n; ++i)
    {
        const auto k = (size_t) i;
        const float power = detector.process (0.5f * (l[k] * l[k] + r[k] * r[k]));
        const float levelDb = 10.0f * std::log10 (std::max (power, 1.0e-12f));
        const float target = gainReductionDb (levelDb, thresholdDb, ratio);
        grDb = flushDenormal (target + (grDb - target) * (target < grDb ? attackCoef : releaseCoef));
        grMemory = flushDenormal (-grDb + (grMemory + grDb) * memoryCoef);
        compGain[k] = dbToGain (grDb) * preGain[k];
    }
}

void SweetenEffect::saturate (Channel& c, float* io, int n) noexcept
{
    for (int i = 0; i < n; ++i)
        c.buf[(size_t) i] *= compGain[(size_t) i];

    float* up = c.os.upsample (c.buf.data(), n);
    const int upN = n << kOversampleLog2;
    for (int j = 0; j < upN; ++j)
        up[j] = shape::cubic (up[j]);
    c.os.downsample (io, n);

    for (int i = 0; i < n; ++i)
        io[i] = c.postTilt.process (io[i]) * postGain[(size_t) i];
}

} // namespace warmful
