// WARMFUL — PHASER: vintage-voiced 2- to 12-stage phaser.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Phaser.h"
#include "Dynamics.h"
#include "MovementCommon.h"

namespace warmful
{

namespace voicing
{
    constexpr float kMinRateHz = 0.1f, kMaxRateHz = 12.0f;  // free RATE range (exponential)
    constexpr float kSweepLoHz = 150.0f;                    // JFET/OTA-style exponential sweep range
    constexpr float kSweepHiHz = 2500.0f;
    constexpr float kMinSweepDepth = 0.35f;                 // share of the sweep range used at AMOUNT 0
    constexpr float kLfoSkew = 0.62f;                       // rising half takes 62 % of the cycle
    constexpr float kFeedbackMin = 0.2f;                    // light colour feedback at AMOUNT 0...
    constexpr float kFeedbackMax = 0.42f;                   // ...and at AMOUNT max
    constexpr float kWetBase = 0.5f;                        // classic 50/50 notch mix
    constexpr float kWetPulseBoost = 0.2f;                  // high AMOUNT + fast RATE: wet dominates
    constexpr float kPulseAmountStart = 0.55f;
    constexpr float kPulseRateLoHz = 2.0f, kPulseRateHiHz = 8.0f;
    constexpr float kDriftRandomBlend = 0.6f;               // share of the random waveform at DRIFT max
    constexpr float kRandomRateRatio = 1.3f;                // random waveform rate relative to RATE
    constexpr float kRateJitter = 0.25f;                    // +-25 % rate jitter at DRIFT max
    constexpr float kRateJitterHz = 0.4f;
    constexpr float kStereoPhase = 30.0f / 360.0f;          // L/R LFO offset
    constexpr float kRateSmoothSec = 0.05f;
    constexpr float kControlSmoothSec = 0.03f;
    constexpr std::array<uint32_t, 2> kSeeds { 0x9FA5E101u, 0x3B17C0D2u };
    constexpr uint32_t kJitterSeed = 0x6A11E7E3u;
} // namespace voicing

using namespace voicing;

namespace
{
    /** Skewed sine LFO in [0,1]: the rise takes kLfoSkew of the cycle, like a vintage
        phaser's asymmetric oscillator. */
    float skewedLfo (float p) noexcept
    {
        p = wrap01 (p);
        const float q = p < kLfoSkew ? 0.5f * p / kLfoSkew
                                     : 0.5f + 0.5f * (p - kLfoSkew) / (1.0f - kLfoSkew);
        return 0.5f - 0.5f * std::cos (kTwoPi * q);
    }

    /** First-order allpass coefficient for a corner frequency (same formula as Allpass1::setCorner). */
    float allpassCoef (float hz, double sampleRate) noexcept
    {
        const float t = std::tan (kPi * clamp (hz, 5.0f, (float) sampleRate * 0.49f) / (float) sampleRate);
        return (t - 1.0f) / (t + 1.0f);
    }
} // namespace

void PhaserEffect::prepare (double sampleRate, int)
{
    sr = sampleRate;
    const double controlRate = sr / kControlInterval;
    rate.prepare (controlRate, kRateSmoothSec);
    amount.prepare (controlRate, kControlSmoothSec);
    drift.prepare (controlRate, kControlSmoothSec);
    rateJitter.setRate (kRateJitterHz, controlRate);
    reset();
}

void PhaserEffect::reset() noexcept
{
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        Channel& c = channels[ch];
        for (Allpass1& s : c.stages)
            s.reset();
        c.random.seed (kSeeds[ch]);
        c.random.reset();
        c.coefStep = 0.0f;
        c.feedback = 0.0f;
    }
    rateJitter.seed (kJitterSeed);
    rateJitter.reset();
    phase = 0.0f;
    wetStep = 0.0f;
    controlCountdown = 0;
    primed = false;
}

void PhaserEffect::updateControl() noexcept
{
    const float f = rate.next();
    const float amt = amount.next();
    const float d = drift.next();
    const double controlRate = sr / kControlInterval;

    // LFO position at the end of this control interval.
    const float rateMul = 1.0f + kRateJitter * d * rateJitter.next();
    phase = wrap01 (phase + f * rateMul * (float) kControlInterval / (float) sr);

    const float rangeOct = std::log2 (kSweepHiHz / kSweepLoHz);
    const float depthOct = rangeOct * lerp (kMinSweepDepth, 1.0f, amt);
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        Channel& c = channels[ch];
        c.random.setRate (f * kRandomRateRatio, controlRate);
        const float randomU = 0.5f + 0.5f * c.random.next();
        const float u = lerp (skewedLfo (phase + (float) ch * kStereoPhase), randomU, kDriftRandomBlend * d);
        const float hz = kSweepLoHz * std::exp2 (0.5f * rangeOct + (u - 0.5f) * depthOct);
        const float target = allpassCoef (hz, sr);
        if (! primed)
            c.coef = target;
        c.coefStep = (target - c.coef) / (float) kControlInterval;
    }

    // AMOUNT: 2 -> 12 stages, crossfading between the even taps around the target count.
    const float tapPos = 0.5f * amt * (float) (kNumStages - 2);
    const int k = std::min ((int) tapPos, kNumStages / 2 - 2);
    tapLow = 2 + 2 * k;
    tapFrac = tapPos - (float) k;
    feedbackGain = lerp (kFeedbackMin, kFeedbackMax, amt);

    const float pulse = smoothstep (kPulseAmountStart, 1.0f, amt) * smoothstep (kPulseRateLoHz, kPulseRateHiHz, f);
    const float wetTarget = kWetBase + kWetPulseBoost * pulse;
    if (! primed)
        wet = wetTarget;
    wetStep = (wetTarget - wet) / (float) kControlInterval;
    primed = true;
}

void PhaserEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    rate.setTarget (movement::lfoRateHz (params, kMinRateHz, kMaxRateHz));
    amount.setTarget (clamp (params.amount, 0.0f, 1.0f));
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    if (! primed)
    {
        rate.snap (rate.getTarget());
        amount.snap (amount.getTarget());
        drift.snap (drift.getTarget());
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (controlCountdown == 0)
        {
            updateControl();
            controlCountdown = kControlInterval;
        }
        --controlCountdown;

        wet += wetStep;
        const int tapHigh = tapLow + 2;
        float* io[2] { left + i, right + i };
        for (size_t ch = 0; ch < channels.size(); ++ch)
        {
            Channel& c = channels[ch];
            const float x = *io[ch];
            c.coef += c.coefStep;
            float s = x + feedbackGain * c.feedback;
            float low = 0.0f, high = 0.0f;
            for (int st = 0; st < kNumStages; ++st)
            {
                c.stages[(size_t) st].setCoef (c.coef);
                s = c.stages[(size_t) st].process (s);
                if (st + 1 == tapLow)
                    low = s;
                else if (st + 1 == tapHigh)
                    high = s;
            }
            const float phased = lerp (low, high, tapFrac);
            c.feedback = shape::fastTanh (phased);  // keeps the colour loop bounded
            *io[ch] = (1.0f - wet) * x + wet * phased;
        }
    }
}

} // namespace warmful
