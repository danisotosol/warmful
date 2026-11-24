// WARMFUL — filter building blocks (mono, per-sample, allocation free).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"

namespace warmful
{

/** Topology-preserving one-pole filter giving low-pass and high-pass outputs. */
class OnePole
{
public:
    void setCutoff (float hz, double sampleRate) noexcept
    {
        hz = clamp (hz, 1.0f, (float) sampleRate * 0.49f);
        const float g = std::tan (kPi * hz / (float) sampleRate);
        G = g / (1.0f + g);
    }
    float lowpass (float x) noexcept
    {
        const float v = (x - s) * G;
        const float y = v + s;
        s = flushDenormal (y + v);
        return y;
    }
    float highpass (float x) noexcept { return x - lowpass (x); }
    void reset (float value = 0.0f) noexcept { s = value; }

private:
    float G = 0.5f, s = 0.0f;
};

/** First-order DC blocker (~10 Hz). */
class DcBlocker
{
public:
    void prepare (double sampleRate, float hz = 10.0f) noexcept { R = 1.0f - (kTwoPi * hz / (float) sampleRate); }
    float process (float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y;
    }
    void reset() noexcept { x1 = y1 = 0.0f; }

private:
    float R = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

/** Trapezoidal (Simper/Cytomic) state-variable filter. One call returns all outputs. */
class Svf
{
public:
    struct Out { float lp, bp, hp; };

    void setParams (float hz, float q, double sampleRate) noexcept
    {
        hz = clamp (hz, 5.0f, (float) sampleRate * 0.49f);
        g = std::tan (kPi * hz / (float) sampleRate);
        k = 1.0f / std::max (q, 0.025f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    Out process (float x) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = flushDenormal (2.0f * v1 - ic1);
        ic2 = flushDenormal (2.0f * v2 - ic2);
        return { v2, v1, x - k * v1 - v2 };
    }
    float getK() const noexcept { return k; }
    void reset() noexcept { ic1 = ic2 = 0.0f; }

private:
    float g = 0.1f, k = 1.4f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
};

/** RBJ-cookbook biquad, transposed direct form II. */
class Biquad
{
public:
    void setLowpass (float hz, float q, double sr) noexcept    { design (Type::LP, hz, q, 0.0f, sr); }
    void setHighpass (float hz, float q, double sr) noexcept   { design (Type::HP, hz, q, 0.0f, sr); }
    void setBandpass (float hz, float q, double sr) noexcept   { design (Type::BP, hz, q, 0.0f, sr); }
    void setPeak (float hz, float q, float db, double sr) noexcept      { design (Type::Peak, hz, q, db, sr); }
    void setLowShelf (float hz, float q, float db, double sr) noexcept  { design (Type::LowShelf, hz, q, db, sr); }
    void setHighShelf (float hz, float q, float db, double sr) noexcept { design (Type::HighShelf, hz, q, db, sr); }

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = flushDenormal (b1 * x - a1 * y + z2);
        z2 = flushDenormal (b2 * x - a2 * y);
        return y;
    }
    void reset() noexcept { z1 = z2 = 0.0f; }

private:
    enum class Type { LP, HP, BP, Peak, LowShelf, HighShelf };

    void design (Type t, float hz, float q, float db, double sr) noexcept
    {
        hz = clamp (hz, 5.0f, (float) sr * 0.49f);
        q = std::max (q, 0.05f);
        const float w0 = kTwoPi * hz / (float) sr;
        const float cw = std::cos (w0), sw = std::sin (w0);
        const float alpha = sw / (2.0f * q);
        const float A = std::pow (10.0f, db / 40.0f);
        float nb0 = 1, nb1 = 0, nb2 = 0, na0 = 1, na1 = 0, na2 = 0;

        switch (t)
        {
            case Type::LP:
                nb0 = (1 - cw) * 0.5f; nb1 = 1 - cw; nb2 = nb0;
                na0 = 1 + alpha; na1 = -2 * cw; na2 = 1 - alpha; break;
            case Type::HP:
                nb0 = (1 + cw) * 0.5f; nb1 = -(1 + cw); nb2 = nb0;
                na0 = 1 + alpha; na1 = -2 * cw; na2 = 1 - alpha; break;
            case Type::BP:
                nb0 = alpha; nb1 = 0; nb2 = -alpha;
                na0 = 1 + alpha; na1 = -2 * cw; na2 = 1 - alpha; break;
            case Type::Peak:
                nb0 = 1 + alpha * A; nb1 = -2 * cw; nb2 = 1 - alpha * A;
                na0 = 1 + alpha / A; na1 = -2 * cw; na2 = 1 - alpha / A; break;
            case Type::LowShelf:
            {
                const float sa = 2.0f * std::sqrt (A) * alpha;
                nb0 = A * ((A + 1) - (A - 1) * cw + sa);
                nb1 = 2 * A * ((A - 1) - (A + 1) * cw);
                nb2 = A * ((A + 1) - (A - 1) * cw - sa);
                na0 = (A + 1) + (A - 1) * cw + sa;
                na1 = -2 * ((A - 1) + (A + 1) * cw);
                na2 = (A + 1) + (A - 1) * cw - sa;
                break;
            }
            case Type::HighShelf:
            {
                const float sa = 2.0f * std::sqrt (A) * alpha;
                nb0 = A * ((A + 1) + (A - 1) * cw + sa);
                nb1 = -2 * A * ((A - 1) + (A + 1) * cw);
                nb2 = A * ((A + 1) + (A - 1) * cw - sa);
                na0 = (A + 1) - (A - 1) * cw + sa;
                na1 = 2 * ((A - 1) - (A + 1) * cw);
                na2 = (A + 1) - (A - 1) * cw - sa;
                break;
            }
        }
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0;
        a1 = na1 / na0; a2 = na2 / na0;
    }

    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

/** First-order tilt EQ: +db above the pivot, -db below (gentle, musical). */
class TiltEq
{
public:
    void set (float pivotHz, float tiltDb, double sampleRate) noexcept
    {
        split.setCutoff (pivotHz, sampleRate);
        lowGain = dbToGain (-0.5f * tiltDb);
        highGain = dbToGain (0.5f * tiltDb);
    }
    float process (float x) noexcept
    {
        const float lo = split.lowpass (x);
        return lo * lowGain + (x - lo) * highGain;
    }
    void reset() noexcept { split.reset(); }

private:
    OnePole split;
    float lowGain = 1.0f, highGain = 1.0f;
};

/** First-order allpass (phaser stage). Corner set by `setCorner`. */
class Allpass1
{
public:
    void setCorner (float hz, double sampleRate) noexcept
    {
        const float t = std::tan (kPi * clamp (hz, 5.0f, (float) sampleRate * 0.49f) / (float) sampleRate);
        a = (t - 1.0f) / (t + 1.0f);
    }
    void setCoef (float c) noexcept { a = c; }
    float process (float x) noexcept
    {
        const float y = a * x + s;
        s = flushDenormal (x - a * y);
        return y;
    }
    void reset() noexcept { s = 0.0f; }

private:
    float a = 0.0f, s = 0.0f;
};

} // namespace warmful
