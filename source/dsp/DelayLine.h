// WARMFUL — interpolating circular delay line (mono).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include <vector>

namespace warmful
{

/** Power-of-two circular buffer. Allocate in prepare(); read/write are allocation free.
    Delay arguments are in samples measured from the most recently written sample
    (delay 0 = last written sample). */
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 4)
            size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    void push (float x) noexcept
    {
        writePos = (writePos + 1) & mask;
        buffer[(size_t) writePos] = x;
    }

    /** Integer tap. */
    float tap (int delay) const noexcept { return buffer[(size_t) ((writePos - delay) & mask)]; }

    float readLinear (float delay) const noexcept
    {
        delay = clamp (delay, 0.0f, (float) maxDelay());
        const int i = (int) delay;
        const float f = delay - (float) i;
        return lerp (tap (i), tap (i + 1), f);
    }

    /** 4-point, 3rd-order Hermite interpolation. Best for modulated delays. */
    float readHermite (float delay) const noexcept
    {
        delay = clamp (delay, 1.0f, (float) maxDelay() - 2.0f);
        const int i = (int) delay;
        const float f = delay - (float) i;
        const float xm1 = tap (i - 1), x0 = tap (i), x1 = tap (i + 1), x2 = tap (i + 2);
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }

    /** Largest delay that can safely be read. */
    int maxDelay() const noexcept { return mask - 3; }
    int size() const noexcept { return mask + 1; }

private:
    std::vector<float> buffer { 0.0f, 0.0f, 0.0f, 0.0f };
    int mask = 3;
    int writePos = 0;
};

} // namespace warmful
