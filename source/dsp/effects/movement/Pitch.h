// WARMFUL — PITCH: -1 to +1 octave gliding pitch shifter with lo-fi DRIFT.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "DelayLine.h"
#include "Effect.h"
#include "Filters.h"
#include "Modulation.h"

namespace warmful
{

/** Dual-head delay-line granular shifter (raised-cosine crossfaded heads). Each restarting
    head is phase-aligned to the audible one by a short cross-correlation search (WSOLA),
    so the crossfades keep the pitch exact. RATE sets the pitch with glide, AMOUNT the wet
    mix, DRIFT degrades the shifted signal (sample-and-hold, bit reduction) and adds
    pitch/window instability. */
class PitchEffect : public Effect
{
public:
    const char* name() const noexcept override { return "PITCH"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() noexcept override;
    void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept override;

private:
    /** Samples between pitch/lo-fi parameter updates. */
    static constexpr int kControlInterval = 16;

    struct Channel
    {
        DelayLine delay;
        OnePole lofiLowpass;
        float held = 0.0f;
    };

    void updateControl() noexcept;
    /** WSOLA-style alignment: offset (samples) to add to a restarting head's nominal delay so
        its grain starts in phase with the head that is currently audible. */
    float alignOffset (float nominalDelay, float audibleDelay) const noexcept;
    /** Correlation of the L+R signal at a candidate delay with the reference delay, normalised
        by the candidate's energy; correlates every `step`-th sample. */
    float similarity (int candidateDelay, int referenceDelay, int step) const noexcept;

    double sr = 48000.0;
    std::array<Channel, 2> channels;
    SmoothRandom pitchJitter, windowJitter;
    Smoothed octaves, drift;  // run at the control rate
    Smoothed mix;
    float windowSamples = 0.0f, minDelaySamples = 0.0f;
    int searchSpan = 0, searchStep = 1, correlationLength = 1;
    /** Grain clock of head A (head B runs half a cycle later) and each head's alignment offset. */
    float headPhase = 0.5f, holdPhase = 0.0f;
    std::array<float, 2> headOffset {};
    float ratio = 1.0f, restWeight = 1.0f, holdInc = 1.0f, quantStep = 0.0f, quantInv = 0.0f;
    int controlCountdown = 0;
    bool primed = false;
};

} // namespace warmful
