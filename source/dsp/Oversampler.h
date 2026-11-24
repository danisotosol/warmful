// WARMFUL — polyphase IIR half-band oversampling (minimum latency, no allocation in process).
// Coefficient design follows the classic elliptic polyphase half-band method
// (Valenzuela & Constantinides; popularised by L. de Soras' HIIR).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include <array>
#include <vector>

namespace warmful
{

namespace halfband
{
    inline void transitionParams (double transition, double& k, double& q)
    {
        k = std::tan ((1.0 - transition * 2.0) * 3.14159265358979323846 / 4.0);
        k *= k;
        const double kksqrt = std::pow (1.0 - k * k, 0.25);
        const double e = 0.5 * (1.0 - kksqrt) / (1.0 + kksqrt);
        const double e2 = e * e, e4 = e2 * e2;
        q = e * (1.0 + e4 * (2.0 + e4 * (15.0 + 150.0 * e4)));
    }

    inline double accNum (double q, int order, int c)
    {
        double acc = 0.0, term;
        int i = 0;
        do
        {
            term = std::pow (q, (double) (i * (i + 1))) * std::sin ((i * 2 + 1) * c * 3.14159265358979323846 / order);
            acc += (i & 1) ? -term : term;
            ++i;
        } while (std::abs (term) > 1e-100 && i < 64);
        return acc;
    }

    inline double accDen (double q, int order, int c)
    {
        double acc = 0.0, term;
        int i = 1;
        do
        {
            term = std::pow (q, (double) (i * i)) * std::cos (i * 2 * c * 3.14159265358979323846 / order);
            acc += (i & 1) ? -term : term;
            ++i;
        } while (std::abs (term) > 1e-100 && i < 64);
        return acc;
    }

    /** Fills `coefs` (size n) for a half-band filter with the given normalised transition band. */
    inline void design (float* coefs, int n, double transition)
    {
        double k, q;
        transitionParams (transition, k, q);
        const int order = n * 2 + 1;
        for (int idx = 0; idx < n; ++idx)
        {
            const int c = idx + 1;
            const double num = accNum (q, order, c) * std::pow (q, 0.25);
            const double den = accDen (q, order, c) + 0.5;
            const double ww = num / den;
            const double wwsq = ww * ww;
            const double x = std::sqrt ((1.0 - wwsq * k) * (1.0 - wwsq / k)) / (1.0 + wwsq);
            coefs[idx] = (float) ((1.0 - x) / (1.0 + x));
        }
    }
} // namespace halfband

/** One 2x half-band stage. Coefficients alternate between two allpass branches. */
template <int NumCoefs>
class HalfbandStage
{
public:
    void design (double transition) { halfband::design (coef.data(), NumCoefs, transition); reset(); }
    void reset() noexcept { x.fill (0.0f); y.fill (0.0f); }

    /** One input sample -> two output samples. */
    void upsample (float in, float& out0, float& out1) noexcept
    {
        float a = in, b = in;
        run (a, b);
        out0 = a;
        out1 = b;
    }

    /** Two input samples -> one output sample. */
    float downsample (float in0, float in1) noexcept
    {
        float a = in1, b = in0;
        run (a, b);
        return 0.5f * (a + b);
    }

private:
    void run (float& a, float& b) noexcept
    {
        for (int i = 0; i < NumCoefs; i += 2)
        {
            const float ta = (a - y[(size_t) i]) * coef[(size_t) i] + x[(size_t) i];
            x[(size_t) i] = a;
            y[(size_t) i] = ta;
            a = ta;
            if (i + 1 < NumCoefs)
            {
                const float tb = (b - y[(size_t) i + 1]) * coef[(size_t) i + 1] + x[(size_t) i + 1];
                x[(size_t) i + 1] = b;
                y[(size_t) i + 1] = tb;
                b = tb;
            }
        }
    }

    std::array<float, NumCoefs> coef {};
    std::array<float, NumCoefs> x {}, y {};
};

/** Mono 2x or 4x oversampler working on whole blocks.
    Usage: float* up = os.upsample (in, n); ...process os.factor()*n samples...; os.downsample (out, n); */
class Oversampler
{
public:
    /** factorLog2: 1 = 2x, 2 = 4x. */
    void prepare (int maxBlockSize, int factorLog2)
    {
        stages = clamp (factorLog2, 1, 2);
        const int maxUp = maxBlockSize << stages;
        bufA.assign ((size_t) maxUp, 0.0f);
        bufB.assign ((size_t) maxUp, 0.0f);
        up1.design (0.0415); down1.design (0.0415);   // ~>90 dB rejection with 12 coefs
        up2.design (0.12);   down2.design (0.12);     // inner stage: passband still covers 20 kHz
        reset();
    }

    void reset() noexcept { up1.reset(); down1.reset(); up2.reset(); down2.reset(); }
    int factor() const noexcept { return 1 << stages; }

    float* upsample (const float* in, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
            up1.upsample (in[i], bufA[(size_t) (2 * i)], bufA[(size_t) (2 * i + 1)]);
        if (stages == 1)
            return bufA.data();
        for (int i = 0; i < 2 * n; ++i)
            up2.upsample (bufA[(size_t) i], bufB[(size_t) (2 * i)], bufB[(size_t) (2 * i + 1)]);
        return bufB.data();
    }

    /** Reads the buffer returned by upsample() (after in-place processing) into `out`. */
    void downsample (float* out, int n) noexcept
    {
        if (stages == 2)
            for (int i = 0; i < 2 * n; ++i)
                bufA[(size_t) i] = down2.downsample (bufB[(size_t) (2 * i)], bufB[(size_t) (2 * i + 1)]);
        for (int i = 0; i < n; ++i)
            out[i] = down1.downsample (bufA[(size_t) (2 * i)], bufA[(size_t) (2 * i + 1)]);
    }

private:
    int stages = 1;
    HalfbandStage<12> up1, down1;
    HalfbandStage<6> up2, down2;
    std::vector<float> bufA, bufB;
};

} // namespace warmful
