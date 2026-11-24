// WARMFUL — LFOs and random sources used by Movement/Diffusion/Texture.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"

namespace warmful
{

/** Small, fast PRNG (xorshift32). Deterministic per seed. */
class Rng
{
public:
    explicit Rng (uint32_t seed = 0x9E3779B9u) noexcept : state (seed ? seed : 1u) {}
    void seed (uint32_t s) noexcept { state = s ? s : 1u; }
    uint32_t nextU32() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    /** [0, 1) */
    float uniform() noexcept { return (float) (nextU32() >> 8) * (1.0f / 16777216.0f); }
    /** [-1, 1) */
    float bipolar() noexcept { return uniform() * 2.0f - 1.0f; }
    /** Approximately Gaussian (Irwin-Hall, 4 terms), unit variance. */
    float gaussian() noexcept
    {
        return (uniform() + uniform() + uniform() + uniform() - 2.0f) * 1.7320508f;
    }

private:
    uint32_t state;
};

/** Phase-accumulator LFO with several shapes; phase in [0,1). */
class Lfo
{
public:
    void setFrequency (float hz, double sampleRate) noexcept { inc = hz / (float) sampleRate; }
    void setPhase (float p) noexcept { phase = wrap01 (p); }
    float getPhase() const noexcept { return phase; }
    /** Advances one sample; returns true when the cycle wrapped. */
    bool advance() noexcept
    {
        phase += inc;
        if (phase >= 1.0f) { phase -= std::floor (phase); return true; }
        return false;
    }

    static float sine (float p) noexcept { return std::sin (kTwoPi * p); }
    static float triangle (float p) noexcept { return 1.0f - 4.0f * std::abs (wrap01 (p + 0.25f) - 0.5f); }
    /** Square with adjustable edge softness (0 = hard, 1 = sine-like). */
    static float softSquare (float p, float softness) noexcept
    {
        const float s = std::sin (kTwoPi * p);
        const float k = 1.0f + (1.0f - clamp (softness, 0.0f, 1.0f)) * 30.0f;
        return std::tanh (k * s) / std::tanh (k);
    }

    float sine() const noexcept { return sine (phase); }
    float triangle() const noexcept { return triangle (phase); }

private:
    float phase = 0.0f, inc = 0.0f;
};

/** Band-limited random source: new random target `rateHz` times per second,
    cosine-interpolated in between. Output in [-1, 1]. */
class SmoothRandom
{
public:
    void seed (uint32_t s) noexcept { rng.seed (s); from = rng.bipolar(); to = rng.bipolar(); }
    void setRate (float hz, double sampleRate) noexcept { inc = hz / (float) sampleRate; }
    float next() noexcept
    {
        pos += inc;
        if (pos >= 1.0f)
        {
            pos -= std::floor (pos);
            from = to;
            to = rng.bipolar();
        }
        const float w = 0.5f - 0.5f * std::cos (kPi * pos);
        return from + (to - from) * w;
    }
    void reset() noexcept { pos = 0.0f; }

private:
    Rng rng;
    float from = 0.0f, to = 0.0f, pos = 0.0f, inc = 0.0f;
};

} // namespace warmful
