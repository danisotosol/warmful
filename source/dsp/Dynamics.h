// WARMFUL — envelope detection and waveshaping primitives.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"

namespace warmful
{

/** Peak (or RMS when `rms`) envelope follower with separate attack/release. */
class EnvelopeFollower
{
public:
    void prepare (double sampleRate, float attackSec, float releaseSec, bool useRms = false) noexcept
    {
        sr = sampleRate;
        rms = useRms;
        setTimes (attackSec, releaseSec);
    }
    void setTimes (float attackSec, float releaseSec) noexcept
    {
        att = onePoleCoef (attackSec, sr);
        rel = onePoleCoef (releaseSec, sr);
    }
    float process (float x) noexcept
    {
        const float in = rms ? x * x : std::abs (x);
        const float c = in > env ? att : rel;
        env = flushDenormal (in + (env - in) * c);
        return rms ? std::sqrt (env) : env;
    }
    float get() const noexcept { return rms ? std::sqrt (env) : env; }
    void reset() noexcept { env = 0.0f; }

private:
    double sr = 48000.0;
    bool rms = false;
    float att = 0.0f, rel = 0.0f, env = 0.0f;
};

namespace shape
{
    /** Pade tanh approximation, accurate to ~1e-3, clamped. */
    inline float fastTanh (float x) noexcept
    {
        x = clamp (x, -4.97f, 4.97f);
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    /** Smooth cubic soft clip, unity slope at 0, saturates at +-1 for |x| >= 1.5. */
    inline float cubic (float x) noexcept
    {
        x = clamp (x, -1.5f, 1.5f);
        return x - (4.0f / 27.0f) * x * x * x;
    }

    /** Linear below `knee`, then smoothly saturates to +-1. Used for headroom limits. */
    inline float kneeClip (float x, float knee = 0.6f) noexcept
    {
        const float a = std::abs (x);
        if (a <= knee)
            return x;
        const float r = 1.0f - knee;
        const float y = knee + r * std::tanh ((a - knee) / r);
        return x < 0.0f ? -y : y;
    }

    /** Asymmetric "triode-like" curve: unity slope at the operating point, negative swing
        saturates earlier (at -1/1.35) than the positive one, producing even harmonics.
        `bias` in [-0.5, 0.5] moves the operating point. Block DC afterwards. */
    inline float asymTube (float x, float bias) noexcept
    {
        auto f = [] (float v) noexcept { return v >= 0.0f ? std::tanh (v) : std::tanh (v * 1.35f) / 1.35f; };
        return f (x + bias) - f (bias);
    }

    /** Hard clip with a small rounded knee. */
    inline float softHard (float x, float ceiling = 1.0f) noexcept
    {
        const float k = 0.1f * ceiling;
        const float a = std::abs (x);
        if (a < ceiling - k)
            return x;
        float y;
        if (a > ceiling + k)
            y = ceiling;
        else
        {
            const float d = a - (ceiling - k);
            y = (ceiling - k) + d - d * d / (4.0f * k);
        }
        return x < 0.0f ? -y : y;
    }
} // namespace shape

} // namespace warmful
