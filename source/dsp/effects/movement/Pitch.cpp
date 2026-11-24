// WARMFUL — PITCH: -1 to +1 octave gliding pitch shifter with lo-fi DRIFT.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Pitch.h"

namespace warmful
{

namespace voicing
{
    constexpr float kWindowMs = 44.0f;          // grain window of the two crossfaded read heads
    constexpr float kMinDelayMs = 1.5f;         // closest a head reads behind the write head (plus the search span)
    constexpr float kSearchLowestHz = 70.0f;    // alignment search spans one period of this frequency
    constexpr float kCorrelationMs = 10.0f;     // waveform stretch compared when aligning a new grain
    constexpr float kCoarseSearchHz = 12000.0f; // coarse search / correlation decimated to about this rate
    constexpr float kGlideSec = 0.08f;          // pitch glide when RATE moves
    constexpr float kControlSmoothSec = 0.03f;
    constexpr float kDriftSmoothSec = 0.05f;
    constexpr float kRestRangeOct = 0.04f;      // near no-shift the heads settle onto one tap (no comb)
    constexpr float kRestTimeSec = 0.4f;        // settling time constant...
    constexpr float kRestMaxSlope = 0.006f;     // ...limited to a ~10 cent excursion
    constexpr float kLofiCurve = 0.7f;          // DRIFT -> lo-fi amount: clearly audible from mid DRIFT
    constexpr float kHoldMinHz = 7000.0f;       // sample-and-hold rate at DRIFT max
    constexpr float kLofiLpRatio = 0.5f;        // post-hold smoothing filter relative to the hold rate
    constexpr float kLofiLpMaxHz = 20000.0f;
    constexpr float kBitsClean = 20.0f;         // effectively transparent at DRIFT 0
    constexpr float kBitsMin = 8.0f;            // bit depth at DRIFT max
    constexpr float kJitterCents = 15.0f;       // random pitch instability at DRIFT max
    constexpr float kJitterHz = 4.0f;
    constexpr float kWindowJitter = 0.15f;      // grain length instability at DRIFT max
    constexpr float kWindowJitterHz = 0.7f;
    constexpr float kEqualPowerShare = 0.5f;    // mix law halfway between linear and equal power
    constexpr float kBufferMs = 120.0f;         // > min delay + search + jittered window + correlation
    constexpr uint32_t kJitterSeed = 0x9174C401u, kWindowSeed = 0x0B5E55E2u;
} // namespace voicing

using namespace voicing;

void PitchEffect::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (Channel& c : channels)
        c.delay.prepare ((int) std::ceil (kBufferMs * 0.001 * sr));
    windowSamples = kWindowMs * 0.001f * (float) sr;
    searchSpan = (int) std::ceil (0.5 * sr / kSearchLowestHz);
    searchStep = std::max (1, (int) std::lround (sr / kCoarseSearchHz));
    correlationLength = (int) std::ceil (kCorrelationMs * 0.001 * sr);
    minDelaySamples = kMinDelayMs * 0.001f * (float) sr + (float) (searchSpan + 1);

    const double controlRate = sr / kControlInterval;
    octaves.prepare (controlRate, kGlideSec);
    drift.prepare (controlRate, kDriftSmoothSec);
    mix.prepare (sr, kControlSmoothSec);
    pitchJitter.setRate (kJitterHz, controlRate);
    windowJitter.setRate (kWindowJitterHz, sr);
    reset();
}

void PitchEffect::reset() noexcept
{
    for (Channel& c : channels)
    {
        c.delay.reset();
        c.lofiLowpass.reset();
        c.held = 0.0f;
    }
    pitchJitter.seed (kJitterSeed);
    pitchJitter.reset();
    windowJitter.seed (kWindowSeed);
    windowJitter.reset();
    headPhase = 0.5f;
    headOffset = {};
    holdPhase = 0.0f;
    controlCountdown = 0;
    primed = false;
}

void PitchEffect::updateControl() noexcept
{
    const float oct = octaves.next();
    const float d = drift.next();
    const float lofi = std::pow (d, kLofiCurve);

    ratio = std::exp2 (oct + kJitterCents * d * pitchJitter.next() / 1200.0f);
    restWeight = 1.0f - smoothstep (0.0f, kRestRangeOct, std::abs (oct));

    // Sample-and-hold decimation from the full rate down to kHoldMinHz, then a gentle smoothing filter.
    holdInc = std::pow (kHoldMinHz / (float) sr, lofi);
    const float lowpassHz = std::min (kLofiLpRatio * holdInc * (float) sr, kLofiLpMaxHz);
    for (Channel& c : channels)
        c.lofiLowpass.setCutoff (lowpassHz, sr);

    quantStep = std::exp2 (1.0f - lerp (kBitsClean, kBitsMin, lofi));
    quantInv = 1.0f / quantStep;
}

float PitchEffect::similarity (int candidateDelay, int referenceDelay, int step) const noexcept
{
    const DelayLine& l = channels[0].delay;
    const DelayLine& r = channels[1].delay;
    float cross = 0.0f, energy = 1.0e-12f;
    for (int j = 0; j < correlationLength; j += step)
    {
        const float x = l.tap (candidateDelay + j) + r.tap (candidateDelay + j);
        const float y = l.tap (referenceDelay + j) + r.tap (referenceDelay + j);
        cross += x * y;
        energy += x * x;
    }
    return cross / std::sqrt (energy);
}

float PitchEffect::alignOffset (float nominalDelay, float audibleDelay) const noexcept
{
    const int nominal = (int) std::lround (nominalDelay);
    const int reference = (int) std::lround (audibleDelay);

    // Coarse search over one period of the lowest expected pitch (decimated), then refine at full rate.
    int best = 0;
    float bestScore = similarity (nominal, reference, searchStep);
    for (int o = -searchSpan; o <= searchSpan; o += searchStep)
    {
        const float score = similarity (nominal + o, reference, searchStep);
        if (score > bestScore)
        {
            bestScore = score;
            best = o;
        }
    }

    const int lo = std::max (-searchSpan, best - searchStep + 1);
    const int hi = std::min (searchSpan, best + searchStep - 1);
    int refined = best;
    bestScore = similarity (nominal + best, reference, 1);
    for (int o = lo; o <= hi; ++o)
    {
        const float score = similarity (nominal + o, reference, 1);
        if (score > bestScore)
        {
            bestScore = score;
            refined = o;
        }
    }

    // Parabolic peak interpolation for sub-sample alignment.
    const float before = similarity (nominal + refined - 1, reference, 1);
    const float after = similarity (nominal + refined + 1, reference, 1);
    const float curvature = before - 2.0f * bestScore + after;
    const float fraction = curvature < 0.0f ? clamp (0.5f * (before - after) / curvature, -0.5f, 0.5f) : 0.0f;

    // Keep the reference head's fractional position so both heads share the same sub-sample phase.
    return (float) (nominal + refined) + fraction + (audibleDelay - (float) reference) - nominalDelay;
}

void PitchEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    octaves.setTarget (bipolar (params.knob));
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    mix.setTarget (clamp (params.amount, 0.0f, 1.0f));
    if (! primed)
    {
        octaves.snap (octaves.getTarget());
        drift.snap (drift.getTarget());
        mix.snap (mix.getTarget());
        primed = true;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (controlCountdown == 0)
        {
            updateControl();
            controlCountdown = kControlInterval;
        }
        --controlCountdown;

        float* io[2] { left + i, right + i };
        for (size_t ch = 0; ch < channels.size(); ++ch)
            channels[ch].delay.push (*io[ch]);

        // Grain clock: both heads' delays change by (1 - ratio) samples per sample.
        const float window = windowSamples * (1.0f + kWindowJitter * drift.getCurrent() * windowJitter.next());
        float slope = 1.0f - ratio;
        if (restWeight > 0.0f)
        {
            // Without shifting, settle one head at full gain so the two taps don't comb-filter.
            const float target = headPhase < 0.25f ? 0.0f : (headPhase < 0.75f ? 0.5f : 1.0f);
            const float settle = (target - headPhase) * window / (kRestTimeSec * (float) sr);
            slope += restWeight * clamp (settle, -kRestMaxSlope, kRestMaxSlope);
        }
        const float previous = headPhase;
        const float advanced = previous + slope / window;
        headPhase = wrap01 (advanced);
        const float phaseB = wrap01 (headPhase + 0.5f);

        // A head restarts at its silent point; align its new grain with the other (fully audible) head.
        if (advanced >= 1.0f || advanced < 0.0f)
        {
            const float nominal = minDelaySamples + headPhase * window;
            headOffset[0] = alignOffset (nominal, minDelaySamples + phaseB * window + headOffset[1]);
        }
        else if ((previous < 0.5f) != (advanced < 0.5f))
        {
            const float nominal = minDelaySamples + phaseB * window;
            headOffset[1] = alignOffset (nominal, minDelaySamples + headPhase * window + headOffset[0]);
        }

        // Raised-cosine heads half a cycle apart: gains sum to one, each head is silent at its restart.
        const float s = std::sin (kPi * headPhase);
        const float gainA = s * s;
        const float gainB = 1.0f - gainA;
        const float delayA = minDelaySamples + headPhase * window + headOffset[0];
        const float delayB = minDelaySamples + phaseB * window + headOffset[1];

        holdPhase += holdInc;
        const bool sampleNow = holdPhase >= 1.0f;
        if (sampleNow)
            holdPhase -= 1.0f;

        const float a = mix.next();
        const float dryGain = lerp (1.0f - a, std::sqrt (1.0f - a), kEqualPowerShare);
        const float wetGain = lerp (a, std::sqrt (a), kEqualPowerShare);

        for (size_t ch = 0; ch < channels.size(); ++ch)
        {
            Channel& c = channels[ch];
            const float x = *io[ch];
            const float shifted = gainA * c.delay.readHermite (delayA) + gainB * c.delay.readHermite (delayB);
            if (sampleNow)
                c.held = quantStep * std::floor (shifted * quantInv + 0.5f);
            *io[ch] = dryGain * x + wetGain * c.lofiLowpass.lowpass (c.held);
        }
    }
}

} // namespace warmful
