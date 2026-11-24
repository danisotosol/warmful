// WARMFUL — CHARACTER / FUZZ: dynamic vintage two-transistor fuzz, smooth to blown-speaker.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Fuzz.h"

namespace warmful
{
namespace
{
namespace voicing
{
constexpr int kOversampleLog2 = 2;               // 4x around the clipping stages
constexpr float kCouplingDarkHz = 60.0f;         // input coupling corner, TILT left: full
constexpr float kCouplingBrashHz = 350.0f;       // TILT right: thin, brash
constexpr float kMinGainDb = 20.0f, kMaxGainDb = 50.0f;   // AMOUNT
constexpr float kSensitivityRangeDb = 12.0f;
constexpr float kNegativeCeiling = 0.3f;         // first transistor clips its negative swing early (asymmetry)
constexpr float kBiasRound = 0.15f;              // TILT left: well biased, smooth and rounded
constexpr float kBiasStarved = -0.3f;            // TILT right: starved, splatty
constexpr float kChargeDepthRound = 0.05f;       // bias shift per unit of envelope (coupling cap charging)
constexpr float kChargeDepthStarved = 0.3f;
constexpr float kMaxChargeShift = 3.0f;          // cap fully charged
constexpr float kChargeAttackRoundSec = 0.01f, kChargeAttackStarvedSec = 0.001f;   // transient character
constexpr float kChargeReleaseRoundSec = 0.25f, kChargeReleaseStarvedSec = 0.07f;
constexpr float kGateRound = 0.005f;             // dead zone after stage 1: crossover gap / sputter on decay
constexpr float kGateStarved = 0.12f;
constexpr float kStage2Gain = 3.0f;              // second, hard-ish stage
constexpr float kToneLpfDarkHz = 2200.0f, kToneLpfBrashHz = 7000.0f;
constexpr float kToneLpfQ = 0.6f;
constexpr float kTiltPivotHz = 800.0f;
constexpr float kToneTiltDb = 12.0f;             // TILT tone span
constexpr float kConeStart = 0.7f;               // TILT position where the blown-speaker flavour begins
constexpr float kConeHz = 1900.0f, kConeQ = 1.8f, kConeDb = 7.0f;   // cone breakup resonance
constexpr float kRectify = 0.25f;                // partial rectification of a torn cone
constexpr float kRefLevel = 0.3f;                // calibrated playing peak used by the level compensation
constexpr float kCrestComp = 0.15f;
constexpr float kOutputTrim = 0.8f;              // fuzz is a square-ish wave: louder at equal peak
} // namespace voicing

/** First transistor: soft on the positive swing, clipping early on the negative one. */
float transistor (float v) noexcept
{
    return v >= 0.0f ? std::tanh (v) : voicing::kNegativeCeiling * std::tanh (v / voicing::kNegativeCeiling);
}
} // namespace

using namespace voicing;
using character::kChunk;

void FuzzEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    controls.prepare (sampleRate);
    for (auto& c : channels)
    {
        c.charge.prepare (sampleRate, kChargeAttackRoundSec, kChargeReleaseRoundSec);
        c.dc.prepare (sampleRate);
        c.os.prepare (kChunk, kOversampleLog2);
    }
    reset();
}

void FuzzEffect::reset() noexcept
{
    controls.reset();
    gainRamp.reset();
    makeupRamp.reset();
    for (auto& c : channels)
    {
        c.coupling.reset();
        c.charge.reset();
        c.os.reset();
        c.toneLpf.reset();
        c.conePeak.reset();
        c.tone.reset();
        c.dc.reset();
    }
}

void FuzzEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
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

void FuzzEffect::updateChunk (int n) noexcept
{
    const float tilt = controls.tilt();
    const float t = controls.tilt01();
    const float amount = controls.amount();

    // TILT sets the bias and transient behaviour as well as the tone.
    bias = lerp (kBiasRound, kBiasStarved, t);
    biasOffset = transistor (bias);
    chargeDepth = lerp (kChargeDepthRound, kChargeDepthStarved, t);
    gate = lerp (kGateRound, kGateStarved, t);
    cone = smoothstep (kConeStart, 1.0f, t);

    const float g = dbToGain (lerp (kMinGainDb, kMaxGainDb, amount) + controls.sensitivity() * kSensitivityRangeDb);
    const float shaped = shape::softHard (kStage2Gain * (transistor (g * kRefLevel + bias) - biasOffset));
    gainRamp.fill (g, gain.data(), n);
    makeupRamp.fill (character::makeupGain (kRefLevel, g * kStage2Gain, shaped, kCrestComp) * kOutputTrim,
                     makeup.data(), n);

    const float couplingHz = expMap (t, kCouplingDarkHz, kCouplingBrashHz);
    const float lpfHz = expMap (t, kToneLpfDarkHz, kToneLpfBrashHz);
    const float attack = lerp (kChargeAttackRoundSec, kChargeAttackStarvedSec, t);
    const float release = lerp (kChargeReleaseRoundSec, kChargeReleaseStarvedSec, t);
    for (auto& c : channels)
    {
        c.coupling.setCutoff (couplingHz, sr);
        c.charge.setTimes (attack, release);
        c.toneLpf.setLowpass (lpfHz, kToneLpfQ, sr);
        c.conePeak.setPeak (kConeHz, kConeQ, kConeDb * cone, sr);
        c.tone.set (kTiltPivotHz, kToneTiltDb * tilt, sr);
    }
}

void FuzzEffect::processChannel (Channel& c, float* io, int n) noexcept
{
    // Loud transients charge the coupling cap, dragging the operating point down: the fuzz
    // gets spittier on the attack and sputters out (gated) as the note decays.
    std::array<float, kChunk> driven, opPoint;
    for (int i = 0; i < n; ++i)
    {
        const float x = c.coupling.highpass (io[i]) * gain[(size_t) i];
        const float e = c.charge.process (x);
        opPoint[(size_t) i] = bias - std::min (chargeDepth * e, kMaxChargeShift);
        driven[(size_t) i] = x;
    }

    float* up = c.os.upsample (driven.data(), n);
    const int upN = n << kOversampleLog2;
    for (int j = 0; j < upN; ++j)
    {
        const auto i = (size_t) (j >> kOversampleLog2);
        float y = transistor (up[j] + opPoint[i]) - biasOffset;
        y -= gate * std::tanh (y / gate);
        y = shape::softHard (kStage2Gain * y);
        up[j] = y + cone * kRectify * std::abs (y);
    }
    c.os.downsample (io, n);

    for (int i = 0; i < n; ++i)
    {
        const float y = c.tone.process (c.conePeak.process (c.toneLpf.process (io[i])));
        io[i] = c.dc.process (y) * makeup[(size_t) i];
    }
}

} // namespace warmful
