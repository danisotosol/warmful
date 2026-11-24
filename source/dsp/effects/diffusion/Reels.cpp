// WARMFUL — REELS: well-worn tape echo (Diffusion module).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Reels.h"

#include "DiffusionCommon.h"
#include "Dynamics.h"
#include "Tempo.h"

namespace warmful
{

namespace
{
namespace voicing
{
    constexpr float kMinTimeSec = 0.050f;        // TIME fully left: short slapback
    constexpr float kMaxTimeSec = 0.700f;        // TIME fully right: around a quarter note
    constexpr float kMaxSyncedSec = 3.0f;        // longest synced echo (quarter at 20 BPM); longer ones fold by octaves
    constexpr float kInertiaSec = 0.15f;         // each of two cascaded motor-inertia stages (~300 ms total)
    constexpr float kMaxGlide = 0.75f;           // delay change cap per sample: the tape never runs backwards
    constexpr float kEchoFloor = 0.18f;          // echo level at AMOUNT 0 (one quiet repeat)
    constexpr float kEchoRampEnd = 0.1f;         // AMOUNT where the echo reaches full level
    constexpr float kMaxFeedback = 1.04f;        // lively self-oscillation at AMOUNT 1
    constexpr float kFeedbackCurve = 0.8f;
    constexpr float kHeadOffsetSec = 0.004f;     // right playback head sits a few ms further along the tape
    constexpr float kInputGain = 0.9f;
    constexpr float kLossHz = 8000.0f;           // playback HF loss per pass (brighter than Cascade)
    constexpr float kLossQ = 0.65f;
    constexpr float kBumpHz = 100.0f;            // playback head bump
    constexpr float kBumpDb = 2.0f;
    constexpr float kLowCutHz = 80.0f;
    constexpr float kRecDrive = 1.4f;            // record-head saturation drive
    constexpr float kRecDriveDrift = 1.0f;       // extra drive at DRIFT 1 (grit)
    constexpr float kSatBias = 0.15f;            // asymmetry of the tape curve (even harmonics)
    constexpr float kSatCeiling = 0.95f;
    constexpr float kWowHz = 0.7f;
    constexpr float kWowSec = 0.0009f;           // ~7 cents at DRIFT 0: the machine is already worn
    constexpr float kWowDriftSec = 0.004f;       // up to ~40 cents at DRIFT 1
    constexpr float kWowWanderHz = 0.45f;        // irregular wow component
    constexpr float kWowWanderRatio = 0.6f;
    constexpr float kFlutterHz = 7.3f;
    constexpr float kFlutterSec = 0.00003f;      // ~2.5 cents
    constexpr float kFlutterDriftSec = 0.00012f; // up to ~12 cents
    constexpr float kScrapeHz = 28.0f;           // scrape flutter (rough, random)
    constexpr float kScrapeSec = 0.000006f;
    constexpr float kScrapeDriftSec = 0.00003f;
    constexpr float kNoiseBase = 0.00003f;       // tape hiss recorded into the loop
    constexpr float kNoiseDrift = 0.002f;
    constexpr float kDriftLoss = 0.12f;          // per-repeat loss at DRIFT 1: repeats fade away instead of oscillating
    constexpr float kDriftDarken = 0.55f;        // relative HF-loss cutoff reduction at DRIFT 1
    constexpr float kDropRateHz = 1.2f;          // dropout events per second at DRIFT 1
    constexpr float kDropMinSec = 0.02f;
    constexpr float kDropMaxSec = 0.12f;
    constexpr float kDropMinDepth = 0.3f;
    constexpr float kDropMaxDepth = 0.9f;
    constexpr float kWetGain = 1.0f;
    constexpr float kGainSmoothSec = 0.03f;
    constexpr float kDriftSmoothSec = 0.05f;
} // namespace voicing

/** Asymmetric tape curve, unity-ish small-signal gain, bounded by the ceiling. */
inline float tapeSaturate (float x, float drive) noexcept
{
    using namespace voicing;
    return kSatCeiling * (shape::fastTanh (drive * x / kSatCeiling + kSatBias) - shape::fastTanh (kSatBias)) / drive;
}
} // namespace

void ReelsEffect::prepare (double sampleRate, int)
{
    using namespace voicing;
    sr = sampleRate;
    maxDelaySamples = kMaxSyncedSec * (float) sr;
    headOffset = kHeadOffsetSec * (float) sr;
    const float modMargin = (kWowSec + kWowDriftSec) * (1.0f + kWowWanderRatio) + kFlutterSec + kFlutterDriftSec
                            + kScrapeSec + kScrapeDriftSec;
    tape.prepare ((int) std::ceil (maxDelaySamples + headOffset + modMargin * (float) sr) + 8);

    inertiaCoef = 1.0f - onePoleCoef (kInertiaSec, sr);
    level.prepare (sr, kGainSmoothSec);
    feedback.prepare (sr, kGainSmoothSec);
    wet.prepare (sr, kGainSmoothSec);
    drift.prepare (sr / diffusion::kControlBlock, kDriftSmoothSec);
    wow.setFrequency (kWowHz, sr);
    flutter.setFrequency (kFlutterHz, sr);
    wowWander.setRate (kWowWanderHz, sr);
    scrape.setRate (kScrapeHz, sr);
    for (Biquad* b : { &bumpL, &bumpR })
        b->setLowShelf (kBumpHz, 0.707f, kBumpDb, sr);
    for (Biquad* b : { &lowCutL, &lowCutR })
        b->setHighpass (kLowCutHz, 0.707f, sr);
    reset();
}

void ReelsEffect::reset() noexcept
{
    tape.reset();
    lossL.reset();
    lossR.reset();
    bumpL.reset();
    bumpR.reset();
    lowCutL.reset();
    lowCutR.reset();
    wow.setPhase (0.0f);
    flutter.setPhase (0.0f);
    wowWander.seed (0x2EE15001u);
    scrape.seed (0x2EE15002u);
    rng.seed (0x2EE15003u);
    dropPos = dropLen = dropDepth = 0.0f;
    primed = false;
}

void ReelsEffect::updateControl (float d) noexcept
{
    using namespace voicing;
    const float s = (float) sr;
    wowDepth = (kWowSec + d * kWowDriftSec) * s;
    wanderDepth = wowDepth * kWowWanderRatio;
    flutterDepth = (kFlutterSec + d * kFlutterDriftSec) * s;
    scrapeDepth = (kScrapeSec + d * kScrapeDriftSec) * s;
    noiseLevel = kNoiseBase + d * kNoiseDrift;
    loss = 1.0f - d * kDriftLoss;
    recDrive = kRecDrive + d * kRecDriveDrift;
    dropChance = d * kDropRateHz / s;

    const float lossHz = kLossHz * (1.0f - kDriftDarken * d);
    lossL.setParams (lossHz, kLossQ, sr);
    lossR.setParams (lossHz, kLossQ, sr);
}

/** Short amplitude dips where the worn tape loses contact with the head (raised-cosine shaped). */
float ReelsEffect::dropoutGain() noexcept
{
    using namespace voicing;
    if (dropLen <= 0.0f)
    {
        if (rng.uniform() >= dropChance)
            return 1.0f;
        dropLen = lerp (kDropMinSec, kDropMaxSec, rng.uniform()) * (float) sr;
        dropDepth = lerp (kDropMinDepth, kDropMaxDepth, rng.uniform());
        dropPos = 0.0f;
    }
    const float g = 1.0f - dropDepth * (0.5f - 0.5f * std::cos (kTwoPi * dropPos / dropLen));
    dropPos += 1.0f;
    if (dropPos >= dropLen)
        dropLen = 0.0f;
    return g;
}

void ReelsEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    using namespace voicing;
    const float seconds = params.tempo.synced
        ? diffusion::foldSeconds (tempo::syncedEchoSeconds (params.knob, params.tempo.bpm), kMaxSyncedSec)
        : expMap (clamp (params.knob, 0.0f, 1.0f), kMinTimeSec, kMaxTimeSec);
    const float target = clamp (seconds * (float) sr, 1.0f, maxDelaySamples);
    const auto echo = diffusion::echoAmount (params.amount, kEchoFloor, kEchoRampEnd, kMaxFeedback, kFeedbackCurve);

    level.setTarget (echo.level);
    feedback.setTarget (echo.feedback);
    wet.setTarget (params.wetLevel * kWetGain);
    drift.setTarget (clamp (params.drift, 0.0f, 1.0f));
    if (! primed)
    {
        motor = delay = target;
        level.snap (echo.level);
        feedback.snap (echo.feedback);
        wet.snap (params.wetLevel * kWetGain);
        drift.snap (drift.getTarget());
        primed = true;
    }

    for (int start = 0; start < numSamples; start += diffusion::kControlBlock)
    {
        const int end = std::min (numSamples, start + diffusion::kControlBlock);
        updateControl (drift.next());

        for (int i = start; i < end; ++i)
        {
            // Two inertia stages: the capstan motor eases into the new speed, so the pitch glides smoothly.
            motor += (target - motor) * inertiaCoef;
            delay += clamp ((motor - delay) * inertiaCoef, -kMaxGlide, kMaxGlide);

            wow.advance();
            flutter.advance();
            const float tap = delay + wow.sine() * wowDepth + wowWander.next() * wanderDepth
                              + flutter.sine() * flutterDepth + scrape.next() * scrapeDepth;
            const float dropout = dropoutGain();
            const float yL = lowCutL.process (bumpL.process (lossL.process (tape.readHermite (tap) * dropout).lp));
            const float yR = lowCutR.process (bumpR.process (lossR.process (tape.readHermite (tap + headOffset) * dropout).lp));

            const float in = 0.5f * (left[i] + right[i]) * kInputGain;
            const float rec = in + yL * feedback.next() * loss + rng.gaussian() * noiseLevel;
            tape.push (tapeSaturate (rec, recDrive));

            const float g = level.next() * wet.next();
            left[i] += yL * g;
            right[i] += yR * g;
        }
    }
}

} // namespace warmful
