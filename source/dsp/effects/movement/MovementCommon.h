// WARMFUL — helpers shared by the Movement effects.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Effect.h"
#include "Tempo.h"

namespace warmful::movement
{

/** LFO rate of the tap-syncable Movement effects (VIBRATO, PHASER, TREMOLO):
    tempo subdivision when synced, otherwise exponential on RATE between the free limits. */
inline float lfoRateHz (const EffectParams& params, float freeMinHz, float freeMaxHz) noexcept
{
    const float knob = clamp (params.knob, 0.0f, 1.0f);
    return params.tempo.synced ? tempo::syncedRateHz (knob, params.tempo.bpm)
                               : expMap (knob, freeMinHz, freeMaxHz);
}

} // namespace warmful::movement
