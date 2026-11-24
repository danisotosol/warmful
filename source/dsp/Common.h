// WARMFUL — shared DSP helpers.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace warmful
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kHalfPi = 0.5f * kPi;

template <typename T>
constexpr T clamp (T v, T lo, T hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

/** Exponential map of t in [0,1] onto [lo,hi] (both > 0). Perceptual knob curves. */
inline float expMap (float t, float lo, float hi) noexcept { return lo * std::pow (hi / lo, t); }

inline float smoothstep (float e0, float e1, float x) noexcept
{
    const float t = clamp ((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/** Bipolar knob (0..1, 0.5 = centre) to -1..+1. */
inline float bipolar (float knob) noexcept { return clamp (knob * 2.0f - 1.0f, -1.0f, 1.0f); }

/** Coefficient for a one-pole smoother/follower with time constant `seconds`. */
inline float onePoleCoef (float seconds, double sampleRate) noexcept
{
    if (seconds <= 0.0f || sampleRate <= 0.0)
        return 0.0f;
    return (float) std::exp (-1.0 / ((double) seconds * sampleRate));
}

inline float flushDenormal (float x) noexcept
{
    return std::abs (x) < 1.0e-20f ? 0.0f : x;
}

/** Wraps a phase into [0,1). */
inline float wrap01 (float x) noexcept { return x - std::floor (x); }

/** Per-sample one-pole parameter smoother. */
class Smoothed
{
public:
    void prepare (double sampleRate, float timeSeconds) noexcept
    {
        coef = onePoleCoef (timeSeconds, sampleRate);
    }
    void setTarget (float t) noexcept { target = t; }
    void snap (float v) noexcept { target = current = v; }
    float next() noexcept
    {
        current = target + (current - target) * coef;
        if (std::abs (current - target) < 1.0e-7f)
            current = target;
        return current;
    }
    float getCurrent() const noexcept { return current; }
    float getTarget() const noexcept { return target; }
    bool isSettled() const noexcept { return std::abs (current - target) <= 0.0f; }

private:
    float coef = 0.0f, current = 0.0f, target = 0.0f;
};

/** Linear ramp of fixed duration: reaches its target exactly (crisp bypass/engage fades). */
class LinearRamp
{
public:
    void prepare (double sampleRate, float rampSeconds) noexcept
    {
        rampSamples = std::max (1, (int) (rampSeconds * sampleRate));
    }
    void setTarget (float t) noexcept
    {
        if (std::abs (t - target) <= 0.0f)
            return;
        target = t;
        step = (target - current) / (float) rampSamples;
        remaining = rampSamples;
    }
    void snap (float v) noexcept { target = current = v; remaining = 0; }
    float next() noexcept
    {
        if (remaining > 0)
            current = --remaining == 0 ? target : current + step;
        return current;
    }
    float getCurrent() const noexcept { return current; }
    bool isSettled() const noexcept { return remaining == 0; }

private:
    int rampSamples = 1, remaining = 0;
    float current = 0.0f, target = 0.0f, step = 0.0f;
};

/** Equal-power crossfade gains for t in [0,1]. */
inline void equalPowerGains (float t, float& a, float& b) noexcept
{
    t = clamp (t, 0.0f, 1.0f);
    a = std::cos (t * kHalfPi);
    b = std::sin (t * kHalfPi);
}

/** Maps a 0..1 "volume" knob with 0.5 = unity onto a linear gain.
    Left half fades quadratically to silence, right half boosts up to +maxBoostDb. */
inline float neutralCentreGain (float knob, float maxBoostDb = 12.0f) noexcept
{
    knob = clamp (knob, 0.0f, 1.0f);
    if (knob >= 0.5f)
        return dbToGain ((knob - 0.5f) * 2.0f * maxBoostDb);
    const float t = knob * 2.0f;
    return t * t;
}

} // namespace warmful
