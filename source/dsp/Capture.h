// WARMFUL — CAPTURE: ephemeral sustainer / looper (up to 30 s), manual ch. 10.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include <vector>

namespace warmful
{

/** Hold TAP to record (recording starts at the press), release to loop it.
    Very short captures become a seamless pad (two overlapping Hann-windowed voices, "blue");
    longer ones play as a traditional loop with a crossfaded seam ("green").
    Any new press stops the current loop ("press TAP once to stop/delete"). */
class Capture
{
public:
    enum class Mode : int { Off = 0, Recording = 1, Sustain = 2, Loop = 3 };

    static constexpr float kMaxSeconds = 30.0f;

    void prepare (double sampleRate);
    void reset() noexcept;

    /** Footswitch pressed: stops any playing loop and starts a tentative recording. */
    void arm() noexcept;
    /** Released before the hold threshold: discard the tentative recording. */
    void cancel() noexcept;
    /** Released after the hold threshold (or MIDI "PLAY"): loop what was recorded. */
    void commit() noexcept;
    /** Stop / clear (fades out). */
    void stop() noexcept;

    /** Records the incoming signal and adds the loop playback, in place. */
    void process (float* left, float* right, int numSamples) noexcept;

    Mode getMode() const noexcept { return mode; }
    bool isPlaying() const noexcept { return mode == Mode::Sustain || mode == Mode::Loop; }

private:
    float readSustain (const std::vector<float>& buf) const noexcept;
    float readLoop (const std::vector<float>& buf) const noexcept;
    void advance() noexcept;
    void renderStopTail() noexcept;

    double sampleRate = 48000.0;
    Mode mode = Mode::Off;
    std::vector<float> bufL, bufR, tailL, tailR;
    int capacity = 0, length = 0, playPos = 0, seam = 0;
    int tailPos = 0, tailLength = 0;
    float outGain = 0.0f, outTarget = 0.0f, gainStep = 0.0f;
    bool fadingToStop = false;
};

} // namespace warmful
