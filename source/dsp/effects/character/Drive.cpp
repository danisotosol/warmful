// WARMFUL — CHARACTER / DRIVE: tube-like drive from warm preamp to full overdrive.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Drive.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr int kOversampleLog2 = 2;               // 4x around the tube stages
constexpr float kInputHpfHz = 25.0f;             // strips subsonics before the gain
constexpr float kTiltPivotHz = 800.0f;
constexpr float kPreTiltDb = 12.0f;              // pre-emphasis span at full TILT: decides which band saturates
constexpr float kPostTiltDb = -4.0f;             // partial de-emphasis: clean tone keeps an 8 dB span
constexpr float kMaxDriveDb = 36.0f;             // AMOUNT 0..1 -> 0..+36 dB into the stages
constexpr float kSensitivityRangeDb = 12.0f;     // SENSITIVITY +-12 dB of drive
constexpr float kStage1Share = 0.6f;             // part of the drive (in dB) ahead of the first stage
constexpr float kStage1Bias = 0.12f;             // operating points off-centre: mostly 2nd harmonic when clean
constexpr float kStage2Bias = 0.06f;
constexpr float kInterStageHpfHz = 80.0f;        // coupling cap between the stages keeps the lows tight
constexpr float kInterStageLpfHz = 11000.0f;     // Miller-style roll-off: tames fizz before stage 2
constexpr float kSagAttackSec = 0.005f;
constexpr float kSagReleaseSec = 0.12f;
constexpr float kSagBias = 0.10f;                // stage-1 bias shift at full sag: loud notes get grittier
constexpr float kSagGain = 0.3f;                 // stage-2 gain loss at full sag: supply droop / dynamic feel
constexpr float kPostLpfCleanHz = 10000.0f;      // speaker-ish roll-off at AMOUNT 0 ...
constexpr float kPostLpfDriveHz = 6500.0f;       // ... lowered at full drive so it never gets fizzy
constexpr float kPostLpfTiltOctaves = 0.6f;      // TILT also moves the roll-off +-0.6 octave
constexpr float kPostLpfQ = 0.707f;
constexpr float kRefLevel = 0.3f;                // calibrated playing peak used by the auto make-up
constexpr float kCrestComp = 0.15f;              // pulls heavy settings down to match perceived loudness
constexpr float kOutputTrim = 1.0f;
} // namespace voicing
} // namespace

using namespace voicing;
using character::kChunk;

void DriveEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    const double osRate = sampleRate * (double) (1 << kOversampleLog2);
    controls.prepare (sampleRate);
    for (auto& c : channels)
    {
        c.inputHpf.setCutoff (kInputHpfHz, sampleRate);
        c.interHpf.setCutoff (kInterStageHpfHz, osRate);
        c.interLpf.setCutoff (kInterStageLpfHz, osRate);
        c.dc.prepare (sampleRate);
        c.sag.prepare (sampleRate, kSagAttackSec, kSagReleaseSec);
        c.os.prepare (kChunk, kOversampleLog2);
    }
    reset();
}

void DriveEffect::reset() noexcept
{
    controls.reset();
    stage1Ramp.reset();
    stage2Ramp.reset();
    makeupRamp.reset();
    for (auto& c : channels)
    {
        c.inputHpf.reset();
        c.interHpf.reset();
        c.interLpf.reset();
        c.preTilt.reset();
        c.postTilt.reset();
        c.postLpf.reset();
        c.dc.reset();
        c.sag.reset();
        c.os.reset();
    }
}

void DriveEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    controls.setTargets (params);
    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = std::min (kChunk, numSamples - start);
        controls.advance (n);
        updateChunk (n);
        processChannel (channels[0], left + start, n);
        processChannel (channels[1], right + start, n);
    }
}

void DriveEffect::updateChunk (int n) noexcept
{
    const float tilt = controls.tilt();
    const float amount = controls.amount();

    const float driveDb = amount * kMaxDriveDb + controls.sensitivity() * kSensitivityRangeDb;
    const float g1 = dbToGain (driveDb * kStage1Share);
    const float g2 = dbToGain (driveDb * (1.0f - kStage1Share));
    const float shaped = shape::asymTube (g2 * shape::asymTube (g1 * kRefLevel, kStage1Bias), kStage2Bias);
    const float mk = character::makeupGain (kRefLevel, g1 * g2, shaped, kCrestComp) * kOutputTrim;

    stage1Ramp.fill (g1, stage1Gain.data(), n);
    stage2Ramp.fill (g2, stage2Gain.data(), n);
    makeupRamp.fill (mk, makeup.data(), n);

    const float lpfHz = lerp (kPostLpfCleanHz, kPostLpfDriveHz, amount) * std::exp2 (kPostLpfTiltOctaves * tilt);
    for (auto& c : channels)
    {
        c.preTilt.set (kTiltPivotHz, kPreTiltDb * tilt, sr);
        c.postTilt.set (kTiltPivotHz, kPostTiltDb * tilt, sr);
        c.postLpf.setLowpass (lpfHz, kPostLpfQ, sr);
    }
}

void DriveEffect::processChannel (Channel& c, float* io, int n) noexcept
{
    std::array<float, kChunk> driven, sag;
    for (int i = 0; i < n; ++i)
    {
        const float x = c.preTilt.process (c.inputHpf.highpass (io[i])) * stage1Gain[(size_t) i];
        const float e = c.sag.process (x);
        sag[(size_t) i] = e / (1.0f + e);
        driven[(size_t) i] = x;
    }

    float* up = c.os.upsample (driven.data(), n);
    const int upN = n << kOversampleLog2;
    for (int j = 0; j < upN; ++j)
    {
        const auto i = (size_t) (j >> kOversampleLog2);
        const float s = sag[i];
        float y = shape::asymTube (up[j], kStage1Bias - kSagBias * s);
        y = c.interLpf.lowpass (c.interHpf.highpass (y));
        up[j] = shape::asymTube (y * stage2Gain[i] * (1.0f - kSagGain * s), kStage2Bias);
    }
    c.os.downsample (io, n);

    for (int i = 0; i < n; ++i)
        io[i] = c.dc.process (c.postTilt.process (c.postLpf.process (io[i]))) * makeup[(size_t) i];
}

} // namespace warmful
