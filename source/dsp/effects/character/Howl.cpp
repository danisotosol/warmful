// WARMFUL — CHARACTER / HOWL: resonant envelope-filter fuzz, sustaining howls to synth stabs.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Howl.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr int kOversampleLog2 = 2;               // 4x around fuzz core and ladder
constexpr float kInputHpfHz = 40.0f;
constexpr float kMinGainDb = 18.0f, kMaxGainDb = 40.0f;   // fuzz core gain over AMOUNT
constexpr float kSensitivityRangeDb = 12.0f;     // SENSITIVITY: core drive and envelope sensitivity
constexpr float kCoreBias = 0.1f;                // slight asymmetry in the core: some even harmonics
constexpr float kResonanceMin = 0.8f;            // ladder feedback over AMOUNT; 4 = self-oscillation
constexpr float kResonanceMax = 3.7f;
constexpr float kResonanceComp = 0.7f;           // partial passband compensation as resonance rises
constexpr float kBaseCutoffLowHz = 150.0f;       // TILT left: dark, vocal base
constexpr float kBaseCutoffHighHz = 1000.0f;     // TILT right: bright base for stabs
constexpr float kSweepOctavesLow = 3.0f, kSweepOctavesHigh = 3.5f;   // envelope sweep range
constexpr float kMaxCutoffHz = 14000.0f;
constexpr float kSmoothAttackSec = 0.02f, kSmoothReleaseSec = 0.4f;  // sustaining howl envelope
constexpr float kFastAttackSec = 0.001f, kFastReleaseSec = 0.05f;    // transient detector: fast ...
constexpr float kSlowAttackSec = 0.15f, kSlowReleaseSec = 0.15f;     // ... minus slow = decaying stab sweep
constexpr float kStabGain = 2.0f;                // the transient difference peaks lower than the envelope
constexpr float kEnvRefLevel = 0.3f;             // calibrated playing peak = half the sweep
constexpr float kTiltPivotHz = 800.0f;
constexpr float kToneTiltDb = 6.0f;              // gentle extra TILT tone after the filter
constexpr float kRefLevel = 0.3f;                // calibrated playing peak used by the level compensation
constexpr float kCrestComp = 0.15f;
constexpr float kOutputTrim = 0.8f;
constexpr float kSafetyKnee = 0.7f;              // resonant peaks: soft ceiling at 1.0
} // namespace voicing
} // namespace

using namespace voicing;
using character::kChunk;

float HowlEffect::Ladder::process (float x, float G, float k) noexcept
{
    float u = x - k * shape::fastTanh (y4);
    for (auto& st : s)
    {
        const float v = (u - st) * G;
        const float y = v + st;
        st = flushDenormal (y + v);
        u = y;
    }
    y4 = u;
    return u;
}

void HowlEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    osRate = sampleRate * (double) (1 << kOversampleLog2);
    controls.prepare (sampleRate);
    envSmooth.prepare (sampleRate, kSmoothAttackSec, kSmoothReleaseSec);
    envFast.prepare (sampleRate, kFastAttackSec, kFastReleaseSec);
    envSlow.prepare (sampleRate, kSlowAttackSec, kSlowReleaseSec);
    for (auto& c : channels)
    {
        c.inputHpf.setCutoff (kInputHpfHz, sampleRate);
        c.dc.prepare (sampleRate);
        c.os.prepare (kChunk, kOversampleLog2);
    }
    reset();
}

void HowlEffect::reset() noexcept
{
    controls.reset();
    gainRamp.reset();
    resonanceRamp.reset();
    makeupRamp.reset();
    envSmooth.reset();
    envFast.reset();
    envSlow.reset();
    for (auto& c : channels)
    {
        c.inputHpf.reset();
        c.os.reset();
        c.ladder.reset();
        c.tone.reset();
        c.dc.reset();
    }
}

void HowlEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    controls.setTargets (params);
    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = std::min (kChunk, numSamples - start);
        controls.advance (n);
        updateChunk (n);
        sweepCutoff (left + start, right + start, n);
        processChannel (channels[0], left + start, n);
        processChannel (channels[1], right + start, n);
    }
}

void HowlEffect::updateChunk (int n) noexcept
{
    const float t = controls.tilt01();
    const float amount = controls.amount();
    const float sensDb = controls.sensitivity() * kSensitivityRangeDb;

    baseCutoffHz = expMap (t, kBaseCutoffLowHz, kBaseCutoffHighHz);
    sweepOctaves = lerp (kSweepOctavesLow, kSweepOctavesHigh, t);
    stabMix = t;
    envScale = dbToGain (sensDb) / kEnvRefLevel;
    coreOffset = std::tanh (kCoreBias);

    const float g = dbToGain (lerp (kMinGainDb, kMaxGainDb, amount) + sensDb);
    const float shaped = std::tanh (g * kRefLevel + kCoreBias) - coreOffset;
    gainRamp.fill (g, gain.data(), n);
    resonanceRamp.fill (lerp (kResonanceMin, kResonanceMax, amount), resonance.data(), n);
    makeupRamp.fill (character::makeupGain (kRefLevel, g, shaped, kCrestComp) * kOutputTrim, makeup.data(), n);

    for (auto& c : channels)
        c.tone.set (kTiltPivotHz, kToneTiltDb * controls.tilt(), sr);
}

void HowlEffect::sweepCutoff (const float* left, const float* right, int n) noexcept
{
    const float maxHz = std::min (kMaxCutoffHz, 0.45f * (float) sr);
    for (int i = 0; i < n; ++i)
    {
        const float d = std::max (std::abs (left[i]), std::abs (right[i]));
        const float smooth = envSmooth.process (d);
        const float stab = std::max (0.0f, envFast.process (d) - envSlow.process (d)) * kStabGain;
        const float e = lerp (smooth, stab, stabMix) * envScale;
        const float fc = clamp (baseCutoffHz * std::exp2 (sweepOctaves * e / (1.0f + e)), 20.0f, maxHz);
        const float g = std::tan (kPi * fc / (float) osRate);
        ladderG[(size_t) i] = g / (1.0f + g);
    }
}

void HowlEffect::processChannel (Channel& c, float* io, int n) noexcept
{
    std::array<float, kChunk> driven;
    for (int i = 0; i < n; ++i)
        driven[(size_t) i] = c.inputHpf.highpass (io[i]) * gain[(size_t) i];

    float* up = c.os.upsample (driven.data(), n);
    const int upN = n << kOversampleLog2;
    for (int j = 0; j < upN; ++j)
    {
        const auto i = (size_t) (j >> kOversampleLog2);
        const float k = resonance[i];
        const float core = std::tanh (up[j] + kCoreBias) - coreOffset;
        up[j] = c.ladder.process (core * (1.0f + kResonanceComp * k), ladderG[i], k);
    }
    c.os.downsample (io, n);

    for (int i = 0; i < n; ++i)
    {
        const float y = c.dc.process (c.tone.process (io[i])) * makeup[(size_t) i];
        io[i] = shape::kneeClip (y, kSafetyKnee);
    }
}

} // namespace warmful
