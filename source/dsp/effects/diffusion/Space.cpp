// WARMFUL — SPACE: reverb morphing from echo chamber to sustaining cloud (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Space.h"

#include "DiffusionCommon.h"
#include "Dynamics.h"

namespace warmful
{

namespace
{
namespace voicing
{
    /** One of the five reverb sounds TIME blends between. */
    struct Voice
    {
        float rt60;        // decay time, seconds
        float size;        // FDN delay-length scale
        float dampHz;      // per-line HF damping cutoff
        float preDelayMs;
        float erLevel;     // early reflections (chamber end)
        float diffusion;   // input allpass coefficient
        float smear;       // input allpass length scale (pre-smear)
    };

    constexpr std::array<Voice, 5> kVoices { {
        { 0.4f, 0.35f, 4500.0f, 3.0f, 1.0f, 0.55f, 0.5f },    // recording echo chamber: tight, dense ERs, darker
        { 1.2f, 0.60f, 9500.0f, 8.0f, 0.45f, 0.65f, 0.7f },   // room / plate: bright
        { 2.5f, 0.85f, 7000.0f, 16.0f, 0.2f, 0.68f, 1.0f },   // hall
        { 6.0f, 1.15f, 5500.0f, 24.0f, 0.08f, 0.70f, 1.4f },  // large hall
        { 22.0f, 1.50f, 3200.0f, 45.0f, 0.0f, 0.75f, 2.4f },  // cloud: long pre-smear, dark damping
    } };
    constexpr float kMaxSize = 1.5f;
    constexpr float kMaxSmear = 2.4f;
    constexpr float kMaxPreDelayMs = 45.0f;

    constexpr std::array<float, SpaceEffect::kLines> kLineMs { 31.3f, 37.9f, 41.7f, 47.3f, 53.1f, 61.7f, 67.3f, 73.9f };
    constexpr std::array<std::array<float, SpaceEffect::kDiffusers>, 2> kDiffuserMs { {
        { 4.7f, 3.6f, 12.7f, 9.3f },
        { 4.9f, 3.4f, 13.1f, 8.9f },
    } };
    constexpr std::array<std::array<float, 6>, 2> kErMs { {
        { 4.3f, 9.7f, 15.1f, 21.7f, 29.3f, 37.1f },
        { 5.1f, 11.3f, 16.9f, 24.1f, 31.7f, 35.3f },
    } };
    constexpr std::array<std::array<float, 6>, 2> kErTapGain { {
        { 0.8f, -0.6f, 0.5f, -0.45f, 0.35f, -0.3f },
        { -0.75f, 0.6f, -0.5f, 0.4f, -0.35f, 0.3f },
    } };
    constexpr float kErSpacing = 2.0f;           // ER tap spread relative to the FDN size
    constexpr float kErGain = 0.5f;

    // Orthogonal Hadamard rows: decorrelated stereo injection and outputs.
    constexpr std::array<float, SpaceEffect::kLines> kInL { 1, -1, -1, 1, 1, -1, -1, 1 };
    constexpr std::array<float, SpaceEffect::kLines> kInR { 1, 1, 1, 1, -1, -1, -1, -1 };
    constexpr std::array<float, SpaceEffect::kLines> kOutL { 1, -1, 1, -1, 1, -1, 1, -1 };
    constexpr std::array<float, SpaceEffect::kLines> kOutR { 1, 1, -1, -1, 1, 1, -1, -1 };
    constexpr float kInGain = 0.5f;

    constexpr float kMorphSmoothSec = 0.4f;      // TIME glides slowly so size changes never click
    constexpr float kLowCutHz = 90.0f;           // keeps the tail out of the mud
    constexpr float kLineCeiling = 2.0f;         // soft limit inside the FDN loop
    constexpr float kBaseModSec = 0.00004f;      // inaudible tap motion at DRIFT 0 that breaks up metallic modes
    constexpr float kDriftModSec = 0.0025f;      // FDN tap modulation at DRIFT 1: chorus -> obvious wobble
    constexpr float kDiffuserModSec = 0.0006f;   // input-allpass modulation at DRIFT 1
    constexpr float kLineModHz = 0.35f;          // base wander rate (spread per line, faster with DRIFT)
    constexpr float kDiffuserModHz = 0.8f;
    constexpr float kWetGain = 1.1f;
    constexpr float kRefRt60 = 2.5f;             // long tails are trimmed a little so the cloud doesn't swamp
    constexpr float kRtCompensation = 0.2f;
    constexpr float kGainSmoothSec = 0.03f;
    constexpr float kDriftSmoothSec = 0.05f;
} // namespace voicing

/** Schroeder allpass on a DelayLine; `delaySamples` from the current input. */
inline float allpass (DelayLine& d, float x, float delaySamples, float g) noexcept
{
    const float delayed = d.readLinear (delaySamples - 1.0f);
    const float w = x + g * delayed;
    d.push (w);
    return delayed - g * w;
}

/** In-place orthonormal 8-point Hadamard transform (the FDN feedback matrix). */
inline void hadamard (std::array<float, SpaceEffect::kLines>& v) noexcept
{
    for (int h = 1; h < SpaceEffect::kLines; h <<= 1)
        for (int i = 0; i < SpaceEffect::kLines; i += 2 * h)
            for (int j = i; j < i + h; ++j)
            {
                const float a = v[(size_t) j], b = v[(size_t) (j + h)];
                v[(size_t) j] = a + b;
                v[(size_t) (j + h)] = a - b;
            }
    for (float& x : v)
        x *= 0.35355339f;
}

inline float geoLerp (float a, float b, float t) noexcept { return a * std::pow (b / a, t); }
} // namespace

void SpaceEffect::prepare (double sampleRate, int)
{
    using namespace voicing;
    sr = sampleRate;
    const float s = (float) sr;
    const float ms = 0.001f * s;

    for (size_t i = 0; i < kLines; ++i)
    {
        lineLength[i] = kLineMs[i] * ms;
        lines[i].prepare ((int) std::ceil (lineLength[i] * kMaxSize + (kBaseModSec + kDriftModSec) * s) + 8);
    }
    for (size_t c = 0; c < 2; ++c)
    {
        for (size_t k = 0; k < kDiffusers; ++k)
        {
            diffuserLength[c][k] = kDiffuserMs[c][k] * ms;
            diffusers[c][k].prepare ((int) std::ceil (diffuserLength[c][k] * kMaxSmear + kDiffuserModSec * s) + 8);
        }
        float erMax = 0.0f;
        for (size_t k = 0; k < 6; ++k)
        {
            erDelay[c][k] = kErMs[c][k] * ms;
            erMax = std::max (erMax, erDelay[c][k]);
        }
        preDelay[c].prepare ((int) std::ceil (kMaxPreDelayMs * ms + erMax * kMaxSize * kErSpacing) + 8);
        lowCut[c].setCutoff (kLowCutHz, sr);
    }

    morph.prepare (sr / diffusion::kControlBlock, kMorphSmoothSec);
    drift.prepare (sr / diffusion::kControlBlock, kDriftSmoothSec);
    dryGain.prepare (sr, kGainSmoothSec);
    wetGain.prepare (sr, kGainSmoothSec);
    reset();
}

void SpaceEffect::reset() noexcept
{
    for (size_t i = 0; i < kLines; ++i)
    {
        lines[i].reset();
        damping[i].reset();
        lineWander[i].seed (0x5BACE100u + (uint32_t) i);
        lineMod[i] = lineModStep[i] = 0.0f;
    }
    for (size_t c = 0; c < 2; ++c)
    {
        preDelay[c].reset();
        lowCut[c].reset();
        for (size_t k = 0; k < kDiffusers; ++k)
        {
            diffusers[c][k].reset();
            diffuserWander[c][k].seed (0x5BACE200u + (uint32_t) (c * kDiffusers + k));
            diffuserMod[c][k] = diffuserModStep[c][k] = 0.0f;
        }
    }
    primed = false;
}

void SpaceEffect::updateControl (int chunkLength) noexcept
{
    using namespace voicing;
    const float s = (float) sr;
    const float controlRate = s / (float) diffusion::kControlBlock;
    const float invLength = 1.0f / (float) chunkLength;

    // Interpolate the two neighbouring voicings.
    const float m = morph.next() * 4.0f;
    const int i0 = std::min ((int) m, 3);
    const float f = m - (float) i0;
    const Voice& a = kVoices[(size_t) i0];
    const Voice& b = kVoices[(size_t) i0 + 1];
    const float rt60 = geoLerp (a.rt60, b.rt60, f);
    const float sizeTarget = lerp (a.size, b.size, f);
    const float dampHz = geoLerp (a.dampHz, b.dampHz, f);
    sizeStep = (sizeTarget - size) * invLength;
    smearStep = (lerp (a.smear, b.smear, f) - smear) * invLength;
    preStep = (lerp (a.preDelayMs, b.preDelayMs, f) * 0.001f * s - pre) * invLength;
    erLevel = lerp (a.erLevel, b.erLevel, f) * kErGain;
    diffusion = lerp (a.diffusion, b.diffusion, f);
    tailGain = kWetGain * std::pow (rt60 / kRefRt60, -kRtCompensation) * 0.35355339f;

    const float d = drift.next();
    const float curve = d * std::sqrt (d);
    const float lineDepth = (kBaseModSec + curve * kDriftModSec) * s;
    const float diffuserDepth = curve * kDiffuserModSec * s;

    for (size_t i = 0; i < kLines; ++i)
    {
        lineWander[i].setRate (kLineModHz * (1.0f + 0.13f * (float) i) * (1.0f + d), controlRate);
        lineModStep[i] = (lineWander[i].next() * lineDepth - lineMod[i]) * invLength;
        lineGain[i] = std::exp (-6.9077553f * lineLength[i] * sizeTarget / (rt60 * s));
        damping[i].setCutoff (dampHz, sr);
    }
    for (size_t c = 0; c < 2; ++c)
        for (size_t k = 0; k < kDiffusers; ++k)
        {
            diffuserWander[c][k].setRate (kDiffuserModHz * (1.0f + 0.17f * (float) (c * kDiffusers + k)), controlRate);
            diffuserModStep[c][k] = (diffuserWander[c][k].next() * diffuserDepth - diffuserMod[c][k]) * invLength;
        }
}

void SpaceEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    using namespace voicing;
    float dry = 1.0f, wet = 0.0f;
    equalPowerGains (params.amount, dry, wet);
    morph.setTarget (clamp (params.knob, 0.0f, 1.0f));
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    dryGain.setTarget (dry);
    wetGain.setTarget (wet * params.wetLevel);
    if (! primed)
    {
        morph.snap (morph.getTarget());
        drift.snap (drift.getTarget());
        dryGain.snap (dry);
        wetGain.snap (wet * params.wetLevel);
        primed = true;
    }

    std::array<float, kLines> o {};
    for (int start = 0; start < numSamples; start += diffusion::kControlBlock)
    {
        const int end = std::min (numSamples, start + diffusion::kControlBlock);
        updateControl (end - start);

        for (int n = start; n < end; ++n)
        {
            size += sizeStep;
            smear += smearStep;
            pre += preStep;

            const float inputs[2] = { left[n], right[n] };
            float diffused[2], early[2];
            for (size_t c = 0; c < 2; ++c)
            {
                DelayLine& pd = preDelay[c];
                pd.push (lowCut[c].highpass (inputs[c]));

                const float spacing = size * kErSpacing;
                float er = 0.0f;
                for (size_t k = 0; k < 6; ++k)
                    er += pd.readLinear (pre + erDelay[c][k] * spacing) * kErTapGain[c][k];
                early[c] = er;

                float v = pd.readLinear (pre);
                for (size_t k = 0; k < kDiffusers; ++k)
                {
                    diffuserMod[c][k] += diffuserModStep[c][k];
                    v = allpass (diffusers[c][k], v, diffuserLength[c][k] * smear + diffuserMod[c][k], diffusion);
                }
                diffused[c] = v;
            }

            float tailL = 0.0f, tailR = 0.0f;
            for (size_t i = 0; i < kLines; ++i)
            {
                lineMod[i] += lineModStep[i];
                o[i] = damping[i].lowpass (lines[i].readHermite (lineLength[i] * size + lineMod[i] - 1.0f)) * lineGain[i];
                tailL += kOutL[i] * o[i];
                tailR += kOutR[i] * o[i];
            }
            hadamard (o);
            for (size_t i = 0; i < kLines; ++i)
            {
                const float x = o[i] + kInGain * (kInL[i] * diffused[0] + kInR[i] * diffused[1]);
                lines[i].push (kLineCeiling * shape::kneeClip (x / kLineCeiling, 0.8f));
            }

            const float dg = dryGain.next();
            const float wg = wetGain.next();
            left[n] = inputs[0] * dg + (tailL * tailGain + early[0] * erLevel) * wg;
            right[n] = inputs[1] * dg + (tailR * tailGain + early[1] * erLevel) * wg;
        }
    }
}

} // namespace warmful
