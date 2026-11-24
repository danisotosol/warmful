// WARMFUL — GESTURE: records and loops movements of the 8 primary knobs.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include <array>
#include <vector>

namespace warmful
{

/** Primary controls, indexed like their MIDI CCs (CC# = 64 + index). Column = index / 2. */
enum Primary : int
{
    kTilt = 0, kCharacterAmount, kRate, kMovementAmount, kTime, kDiffusionAmount, kMix, kTextureAmount,
    kNumPrimary
};

/** Behaviour (manual ch. 9):
    - While recording, every knob that is moved starts its own lane; the lane runs until recording stops.
    - Lanes loop independently; their playback speed follows tempo relative to the tempo at record time.
    - Moving a knob that owns a lane while *not* recording deletes that lane.
    - Moving a recorded knob during a new recording overwrites its lane when the recording ends. */
class Gesture
{
public:
    static constexpr float kTickHz = 100.0f;
    static constexpr int kMaxSeconds = 60;
    static constexpr int kMaxPoints = (int) kTickHz * kMaxSeconds;

    struct Lane
    {
        std::vector<float> points;   // capacity kMaxPoints, `length` used
        int length = 0;
        double bpm = 120.0;
    };
    using Snapshot = std::array<Lane, kNumPrimary>;

    Gesture();
    void prepare (double sampleRate);
    void clear() noexcept;

    void beginRecording (const float* raw) noexcept;
    void endRecording (double bpm) noexcept;
    bool isRecording() const noexcept { return recording; }

    /** Call once per control block with the raw knob values; writes effective values to `out`. */
    void process (const float* raw, int numSamples, double bpm, float* out) noexcept;

    bool hasLane (int knob) const noexcept { return lanes[(size_t) knob].length > 1; }
    bool isRecordingKnob (int knob) const noexcept { return recording && recTouched[(size_t) knob]; }

    /** Persistence (call with the engine's gesture lock held). */
    Snapshot snapshot() const;
    void restore (const Snapshot& s) noexcept;

private:
    double samplesPerTick = 480.0;
    double tickAccumulator = 0.0;
    bool recording = false;

    std::array<Lane, kNumPrimary> lanes;
    std::array<double, kNumPrimary> position {};     // fractional tick index
    std::array<float, kNumPrimary> anchor {};        // knob value when the lane was committed
    std::array<std::vector<float>, kNumPrimary> recPoints;
    std::array<int, kNumPrimary> recLength {};
    std::array<bool, kNumPrimary> recTouched {};
    std::array<float, kNumPrimary> recBaseline {};
    std::array<float, kNumPrimary> recLatest {};     // knob value at the latest block while recording
};

} // namespace warmful
