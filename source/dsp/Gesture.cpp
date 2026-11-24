// WARMFUL — GESTURE implementation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Gesture.h"

namespace warmful
{

namespace
{
    constexpr float kTouchThreshold = 0.004f;   // knob travel that starts a lane while recording
    constexpr float kDeleteThreshold = 0.02f;   // knob travel that deletes a lane in live mode
}
Gesture::Gesture()
{
    // Allocated up front so presets can be restored before prepare().
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        lanes[k].points.assign ((size_t) kMaxPoints, 0.0f);
        recPoints[k].assign ((size_t) kMaxPoints, 0.0f);
    }
}

void Gesture::prepare (double sampleRate)
{
    // Lanes are stored in time (ticks), so they survive sample-rate changes.
    samplesPerTick = sampleRate / (double) kTickHz;
    tickAccumulator = 0.0;
}

void Gesture::clear() noexcept
{
    for (auto& lane : lanes)
        lane.length = 0;
    position.fill (0.0);
    recording = false;
    recTouched.fill (false);
}

void Gesture::beginRecording (const float* raw) noexcept
{
    recording = true;
    recTouched.fill (false);
    recLength.fill (0);
    std::copy (raw, raw + kNumPrimary, recBaseline.begin());
}

void Gesture::endRecording (double bpm) noexcept
{
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        if (! recTouched[k] || recLength[k] < 2)
            continue;
        // The loop ends where the knob was left; that position is also the "untouched" anchor,
        // so ticks that fell before the last knob movement cannot trigger a false delete.
        if (recLength[k] < kMaxPoints)
            recPoints[k][(size_t) recLength[k]++] = recLatest[k];
        std::swap (lanes[k].points, recPoints[k]);   // both buffers hold kMaxPoints: no allocation
        lanes[k].length = recLength[k];
        lanes[k].bpm = bpm;
        position[k] = 0.0;
        anchor[k] = recLatest[k];
    }
    recording = false;
    recTouched.fill (false);
}

void Gesture::process (const float* raw, int numSamples, double bpm, float* out) noexcept
{
    tickAccumulator += numSamples;
    int ticks = 0;
    while (tickAccumulator >= samplesPerTick)
    {
        tickAccumulator -= samplesPerTick;
        ++ticks;
    }
    const double elapsedTicks = (double) numSamples / samplesPerTick;

    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        const float value = raw[k];

        if (recording)
        {
            if (! recTouched[k] && std::abs (value - recBaseline[k]) > kTouchThreshold)
                recTouched[k] = true;

            if (recTouched[k])
            {
                for (int t = 0; t < ticks && recLength[k] < kMaxPoints; ++t)
                    recPoints[k][(size_t) recLength[k]++] = value;
                recLatest[k] = value;
                out[k] = value;
                continue;
            }
        }

        Lane& lane = lanes[k];
        if (lane.length < 2)
        {
            out[k] = value;
            continue;
        }

        if (! recording && std::abs (value - anchor[k]) > kDeleteThreshold)
        {
            lane.length = 0;   // "To individually delete a knob's recording, simply move the knob"
            out[k] = value;
            continue;
        }

        const double speed = clamp (bpm, 20.0, 300.0) / clamp (lane.bpm, 20.0, 300.0);
        double pos = position[k] + elapsedTicks * speed;
        pos = std::fmod (pos, (double) lane.length);
        position[k] = pos;

        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % lane.length;
        const float frac = (float) (pos - (double) i0);
        out[k] = lerp (lane.points[(size_t) i0], lane.points[(size_t) i1], frac);
    }
}

Gesture::Snapshot Gesture::snapshot() const
{
    Snapshot s;
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        s[k].length = lanes[k].length;
        s[k].bpm = lanes[k].bpm;
        s[k].points.assign (lanes[k].points.begin(), lanes[k].points.begin() + lanes[k].length);
    }
    return s;
}

void Gesture::restore (const Snapshot& s) noexcept
{
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        const int n = clamp ((int) s[k].points.size(), 0, std::min ((int) lanes[k].points.size(), kMaxPoints));
        std::copy (s[k].points.begin(), s[k].points.begin() + n, lanes[k].points.begin());
        lanes[k].length = n > 1 ? n : 0;
        lanes[k].bpm = s[k].bpm > 0.0 ? s[k].bpm : 120.0;
        anchor[k] = n > 0 ? lanes[k].points[(size_t) n - 1] : 0.0f;
        position[k] = 0.0;
    }
    recording = false;
    recTouched.fill (false);
}

} // namespace warmful
