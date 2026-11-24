// WARMFUL — small helpers shared by the Diffusion effects (delay AMOUNT law, synced-time folding).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"

namespace warmful::diffusion
{

/** Samples between control-rate updates (filter coefficients, modulation targets). */
constexpr int kControlBlock = 16;

struct EchoAmount
{
    float level;
    float feedback;
};

/** Delay AMOUNT law: the echo fades in from `levelFloor` over [0, rampEnd] (a single quiet repeat at 0),
    then the rest of the travel drives the feedback 0 -> maxFeedback. curve < 1 front-loads the repeats. */
inline EchoAmount echoAmount (float amount, float levelFloor, float rampEnd, float maxFeedback, float curve) noexcept
{
    amount = clamp (amount, 0.0f, 1.0f);
    const float level = lerp (levelFloor, 1.0f, smoothstep (0.0f, rampEnd, amount));
    const float t = clamp ((amount - rampEnd) / (1.0f - rampEnd), 0.0f, 1.0f);
    return { level, maxFeedback * std::pow (t, curve) };
}

/** Folds a tempo-synced time down by octaves until it fits the buffer, so it stays a musical subdivision. */
inline float foldSeconds (float seconds, float maxSeconds) noexcept
{
    while (seconds > maxSeconds)
        seconds *= 0.5f;
    return seconds;
}

} // namespace warmful::diffusion
