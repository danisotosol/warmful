// WARMFUL — tempo helpers shared by the tap-syncable effects
// (Vibrato, Phaser, Tremolo, Cascade, Reels, Collage, Reverse) and by Gesture.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include <array>

namespace warmful::tempo
{

inline double beatSeconds (double bpm) noexcept { return 60.0 / clamp (bpm, 20.0, 300.0); }

/** Picks a table entry from a 0..1 knob (equal-width zones). */
template <size_t N>
inline float pick (const std::array<float, N>& table, float knob) noexcept
{
    const int i = clamp ((int) (knob * (float) N), 0, (int) N - 1);
    return table[(size_t) i];
}

/** Modulation rate when synced: cycles per beat, slow (left) to fast (right). */
inline float syncedRateHz (float knob, double bpm) noexcept
{
    static constexpr std::array<float, 10> cyclesPerBeat { 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
    return pick (cyclesPerBeat, knob) / (float) beatSeconds (bpm);
}

/** Cascade / Reels echo time when synced: subdivisions up to a quarter note
    (manual: "short slapback to quarter-note taps", quantised when tapped). */
inline float syncedEchoSeconds (float knob, double bpm) noexcept
{
    // 1/16, 1/8T, 1/8, 1/4T, dotted 1/8, 1/4  (in beats)
    static constexpr std::array<float, 6> beats { 0.25f, 1.0f / 3.0f, 0.5f, 2.0f / 3.0f, 0.75f, 1.0f };
    return pick (beats, knob) * (float) beatSeconds (bpm);
}

/** Collage loop length when synced: short granular slices to phrase-length loops. */
inline float syncedCollageSeconds (float knob, double bpm) noexcept
{
    // 1/32, 1/16, 1/8T, 1/8, 1/4, 1/2, 1 bar, 2 bars (in beats)
    static constexpr std::array<float, 8> beats { 0.125f, 0.25f, 1.0f / 3.0f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
    return pick (beats, knob) * (float) beatSeconds (bpm);
}

/** Reverse grain length when synced: one beat, folded into a usable 0.25..2 s window. */
inline float syncedReverseSeconds (double bpm) noexcept
{
    float s = (float) beatSeconds (bpm);
    while (s < 0.25f) s *= 2.0f;
    while (s > 2.0f) s *= 0.5f;
    return s;
}

} // namespace warmful::tempo
