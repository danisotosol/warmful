// WARMFUL — TEXTURE / CASSETTE: tape saturation, EQ, wow/flutter, hiss and dropouts, from clean deck to eaten tape.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Cassette.h"

namespace warmful
{
namespace
{
namespace voicing
{
// Flavour zones along AMOUNT (overlapping smoothstep ramps):
// 0-30 % warm clean deck, 30-60 % wobbly worn tape, 60-85 % stretched tape with dropouts, 85-100 % eaten tape.
constexpr float kWornStart = 0.3f;
constexpr float kStretchedStart = 0.6f;
constexpr float kEatenStart = 0.85f;
constexpr float kZoneOverlap = 0.05f;

// Record pre-emphasis / playback de-emphasis around the saturator: highs saturate first, like tape.
constexpr float kEmphasisHz = 3000.0f;
constexpr float kEmphasisMinDb = 3.0f;
constexpr float kEmphasisMaxDb = 8.0f;

// Saturation (drive share per flavour, in fractions of kDriveMaxDb).
constexpr float kDriveMaxDb = 14.0f;
constexpr float kDriveDeck = 0.35f, kDriveWorn = 0.25f, kDriveStretched = 0.2f, kDriveEaten = 0.2f;
constexpr float kSatBias = 0.08f;            // slight asymmetry: a touch of even harmonics
constexpr float kHysteresis = 0.2f;          // operating point follows recent magnetisation
constexpr float kMagnetisationHz = 800.0f;
constexpr float kSatRef = 0.5f;              // peak level the heavy-drive make-up is matched at

// Head bump and HF loss.
constexpr float kBumpHz = 90.0f;
constexpr float kBumpQ = 0.9f;
constexpr float kBumpMinDb = 1.0f;
constexpr float kBumpMaxDb = 3.0f;
constexpr float kHfOpenHz = 16000.0f;
constexpr float kHfClosedHz = 3500.0f;
constexpr float kHfQ = 0.6f;
// HF loss travel (fractions of the log span) contributed by each flavour.
constexpr float kHfDeck = 0.3f, kHfWorn = 0.25f, kHfStretched = 0.15f, kHfEaten = 0.3f;

// Azimuth error: right head slightly late and duller.
constexpr float kAzimuthMinMs = 0.02f;
constexpr float kAzimuthMaxMs = 0.12f;
constexpr float kAzimuthHfRatio = 0.9f;

// Transport. Deviations are peak speed errors (0.01 = 1 % = ~17 cents).
constexpr float kBaseDelayMs = 1.5f;
constexpr float kMaxDelaySec = 0.08f;
constexpr float kWowFastHz = 1.2f, kWowSlowHz = 0.45f;   // worn/stretched tape wows slower
constexpr float kFlutterFastHz = 11.0f, kFlutterSlowHz = 5.5f;
constexpr float kWarbleHz = 2.5f;
constexpr float kCrinkleHz = 22.0f;
constexpr float kWowDeck = 0.0015f, kWowWorn = 0.010f, kWowStretched = 0.004f, kWowEaten = 0.012f;
constexpr float kFlutterDeck = 0.0006f, kFlutterWorn = 0.0015f, kFlutterEaten = 0.004f;
constexpr float kWarbleWorn = 0.002f, kWarbleEaten = 0.012f;
constexpr float kCrinkleDev = 0.004f;

// Pitch dips (tape dragging), stretched/eaten zones.
constexpr float kDipRateStretched = 0.18f, kDipRateEaten = 0.45f;   // events per second
constexpr float kDipMinSec = 0.15f, kDipMaxSec = 0.4f;
constexpr float kDipStretchedMs = 1.5f, kDipEatenMs = 6.0f;          // extra delay at the bottom of a dip

// Dropouts (oxide loss).
constexpr float kDropoutRateStretched = 0.5f, kDropoutRateEaten = 0.5f;
constexpr float kDropoutMinSec = 0.03f, kDropoutMaxSec = 0.2f;
constexpr float kDropoutStretchedDb = 8.0f, kDropoutEatenDb = 16.0f;
constexpr float kDropoutAttackSec = 0.004f, kDropoutReleaseSec = 0.012f;

// Level wobble and crinkle.
constexpr float kWobbleHz = 1.3f;
constexpr float kWobbleWornDb = 1.5f, kWobbleEatenDb = 1.5f;
constexpr float kCrinkleAmDb = 4.0f;

// Gentle compression.
constexpr float kCompStart = 0.1f, kCompFull = 0.9f;
constexpr float kCompThresholdDb = -20.0f;
constexpr float kCompRatioMax = 3.0f;
constexpr float kCompMakeupDb = 2.0f;
constexpr float kCompAttackSec = 0.005f, kCompReleaseSec = 0.12f;
constexpr float kCompGainSmoothSec = 0.01f;

// Hiss (band-shaped white noise).
constexpr float kHissMinDb = -84.0f, kHissMaxDb = -56.0f;
constexpr float kHissHighpassHz = 500.0f, kHissLowpassHz = 9000.0f;

constexpr float kDepthSmoothSec = 0.05f;
constexpr uint32_t kSeed = 0xC455E77Eu;
} // namespace voicing
using namespace voicing;

struct Flavours
{
    float deck, worn, stretched, eaten;
};

Flavours flavoursAt (float a) noexcept
{
    return { smoothstep (0.0f, kWornStart, a),
             smoothstep (kWornStart - kZoneOverlap, kStretchedStart, a),
             smoothstep (kStretchedStart - kZoneOverlap, kEatenStart, a),
             smoothstep (kEatenStart - kZoneOverlap, 1.0f, a) };
}

} // namespace

using texture::kControlInterval;

void CassetteEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    amount.prepare (sampleRate);
    for (auto& c : channels)
    {
        c.magnetisation.setCutoff (kMagnetisationHz, sampleRate);
        c.dcBlocker.prepare (sampleRate);
        c.transport.prepare (texture::toSamples (kMaxDelaySec, sampleRate));
        c.hissHighpass.setCutoff (kHissHighpassHz, sampleRate);
        c.hissLowpass.setCutoff (kHissLowpassHz, sampleRate);
    }
    for (auto* s : { &wowDepth, &flutterDepth, &warbleDepth, &crinkleDepth, &azimuth })
        s->prepare (sampleRate, kDepthSmoothSec);
    baseDelay = kBaseDelayMs * 0.001f * (float) sampleRate;
    dropoutAttack = onePoleCoef (kDropoutAttackSec, sampleRate);
    dropoutRelease = onePoleCoef (kDropoutReleaseSec, sampleRate);
    compEnvelope.prepare (sampleRate, kCompAttackSec, kCompReleaseSec, true);
    compCoef = onePoleCoef (kCompGainSmoothSec, sampleRate);
    reset();
}

void CassetteEffect::reset() noexcept
{
    amount.reset();
    designedAmount = -1.0f;
    uint32_t seed = kSeed;
    for (auto& c : channels)
    {
        c.preEmphasis.reset();
        c.deEmphasis.reset();
        c.headBump.reset();
        c.hfLoss.reset();
        c.magnetisation.reset();
        c.memory = 0.0f;
        c.dcBlocker.reset();
        c.transport.reset();
        c.hissRng.seed (seed += 0x9E3779B9u);
        c.hissHighpass.reset();
        c.hissLowpass.reset();
    }
    wow.setPhase (0.0f);
    flutter.setPhase (0.3f);
    warble.seed (kSeed ^ 0x1111u);
    crinkle.seed (kSeed ^ 0x2222u);
    levelWobble.seed (kSeed ^ 0x3333u);
    crinkleLevel.seed (kSeed ^ 0x4444u);
    for (auto* s : { &warble, &crinkle, &levelWobble, &crinkleLevel })
        s->reset();
    events.seed (kSeed ^ 0x5555u);
    dipActive = false;
    dipPhase = dipInc = dipDepth = 0.0f;
    dropoutRemaining = 0;
    dropoutTarget = dropoutGain = 1.0f;
    compEnvelope.reset();
    compGain = compTarget = 1.0f;
    controlCountdown = 0;
    primed = false;
}

void CassetteEffect::updateControls() noexcept
{
    const float a = amount.next();
    const Flavours f = flavoursAt (a);

    // Tone: only redesigned while AMOUNT moves.
    if (a != designedAmount)
    {
        designedAmount = a;
        const float emphasisDb = lerp (kEmphasisMinDb, kEmphasisMaxDb, a);
        const float bumpDb = lerp (kBumpMinDb, kBumpMaxDb, f.worn) * f.deck;
        const float hfTravel = kHfDeck * f.deck + kHfWorn * f.worn + kHfStretched * f.stretched + kHfEaten * f.eaten;
        const float hfHz = expMap (hfTravel, kHfOpenHz, kHfClosedHz);
        for (size_t ch = 0; ch < channels.size(); ++ch)
        {
            auto& c = channels[ch];
            c.preEmphasis.setHighShelf (kEmphasisHz, 0.707f, emphasisDb, sr);
            c.deEmphasis.setHighShelf (kEmphasisHz, 0.707f, -emphasisDb, sr);
            c.headBump.setPeak (kBumpHz, kBumpQ, bumpDb, sr);
            c.hfLoss.setLowpass (ch == 1 ? hfHz * kAzimuthHfRatio : hfHz, kHfQ, sr);
        }

        // Saturation: small-signal unity when clean, peak-matched make-up as the drive grows.
        const float driveShare = kDriveDeck * f.deck + kDriveWorn * f.worn + kDriveStretched * f.stretched + kDriveEaten * f.eaten;
        driveGain = dbToGain (driveShare * kDriveMaxDb);
        const float biasSlope = 1.0f - std::tanh (kSatBias) * std::tanh (kSatBias);
        const float smallSignalNorm = 1.0f / (driveGain * biasSlope);
        const float peakNorm = kSatRef / shape::asymTube (driveGain * kSatRef, kSatBias);
        satNorm = lerp (smallSignalNorm, peakNorm, driveShare);

        // Transport depths: peak speed deviation -> delay excursion (sine: dev / (2 pi f); smooth random: dev / (pi f)).
        const float wowHz = lerp (kWowFastHz, kWowSlowHz, 0.5f * (f.worn + f.stretched));
        const float flutterHz = lerp (kFlutterFastHz, kFlutterSlowHz, a);
        wow.setFrequency (wowHz, sr);
        flutter.setFrequency (flutterHz, sr);
        warble.setRate (kWarbleHz, sr);
        crinkle.setRate (kCrinkleHz, sr);
        levelWobble.setRate (kWobbleHz, sr);
        crinkleLevel.setRate (kCrinkleHz, sr);

        const float fs = (float) sr;
        const float wowDev = kWowDeck * f.deck + kWowWorn * f.worn + kWowStretched * f.stretched + kWowEaten * f.eaten;
        const float flutterDev = kFlutterDeck * f.deck + kFlutterWorn * f.worn + kFlutterEaten * f.eaten;
        const float warbleDev = kWarbleWorn * f.worn + kWarbleEaten * f.eaten;
        wowDepth.setTarget (wowDev / (kTwoPi * wowHz) * fs);
        flutterDepth.setTarget (flutterDev / (kTwoPi * flutterHz) * fs);
        warbleDepth.setTarget (warbleDev / (kPi * kWarbleHz) * fs);
        crinkleDepth.setTarget (kCrinkleDev * f.eaten / (kPi * kCrinkleHz) * fs);
        azimuth.setTarget (lerp (kAzimuthMinMs, kAzimuthMaxMs, a) * 0.001f * fs);

        wobbleDepth = 1.0f - dbToGain (-(kWobbleWornDb * f.worn + kWobbleEatenDb * f.eaten));
        crinkleAmDepth = 1.0f - dbToGain (-kCrinkleAmDb * f.eaten);
        hissLevel = dbToGain (lerp (kHissMinDb, kHissMaxDb, a));

        dipRate = kDipRateStretched * f.stretched + kDipRateEaten * f.eaten;
        dipDepthMax = (kDipStretchedMs * f.stretched + kDipEatenMs * f.eaten) * 0.001f * fs;
        dropoutRate = kDropoutRateStretched * f.stretched + kDropoutRateEaten * f.eaten;
        dropoutDepthDb = kDropoutStretchedDb * f.stretched + kDropoutEatenDb * f.eaten;

        const float comp = smoothstep (kCompStart, kCompFull, a);
        compThresholdDb = kCompThresholdDb;
        compSlope = 1.0f / lerp (1.0f, kCompRatioMax, comp) - 1.0f;
        compMakeupDb = kCompMakeupDb * comp;
    }

    if (! primed)
    {
        for (auto* s : { &wowDepth, &flutterDepth, &warbleDepth, &crinkleDepth, &azimuth })
            s->snap (s->getTarget());
        primed = true;
    }

    const float over = gainToDb (compEnvelope.get()) - compThresholdDb;
    compTarget = dbToGain ((over > 0.0f ? over * compSlope : 0.0f) + compMakeupDb);
}

void CassetteEffect::rollEvents() noexcept
{
    const float dt = (float) kControlInterval / (float) sr;
    if (! dipActive && dipDepthMax > 0.0f && texture::eventFires (events, dipRate, dt))
    {
        dipActive = true;
        dipPhase = 0.0f;
        dipInc = 1.0f / (lerp (kDipMinSec, kDipMaxSec, events.uniform()) * (float) sr);
        dipDepth = dipDepthMax * lerp (0.5f, 1.0f, events.uniform());
    }
    if (dropoutRemaining == 0 && texture::eventFires (events, dropoutRate, dt))
    {
        dropoutRemaining = texture::toSamples (lerp (kDropoutMinSec, kDropoutMaxSec, events.uniform()), sr);
        dropoutTarget = dbToGain (-dropoutDepthDb * lerp (0.4f, 1.0f, events.uniform()));
    }
}

void CassetteEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
{
    amount.setTarget (params.amount);
    float* const io[2] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        if (--controlCountdown < 0)
        {
            controlCountdown = kControlInterval - 1;
            updateControls();
            rollEvents();
        }

        // Tape speed errors as a modulated playback delay (all terms >= 0, so latency only grows with AMOUNT).
        wow.advance();
        flutter.advance();
        float dip = 0.0f;
        if (dipActive)
        {
            dip = dipDepth * 0.5f * (1.0f - std::cos (kTwoPi * dipPhase));
            dipPhase += dipInc;
            dipActive = dipPhase < 1.0f;
        }
        const float delay = baseDelay
                          + wowDepth.next() * (1.0f + wow.sine())
                          + flutterDepth.next() * (1.0f + flutter.sine())
                          + warbleDepth.next() * (1.0f + warble.next())
                          + crinkleDepth.next() * (1.0f + crinkle.next())
                          + dip;
        const float azimuthDelay = azimuth.next();

        // Level artefacts: slow wobble, crinkle flutter and dropouts (they hit the hiss too: it is on the tape).
        if (dropoutRemaining > 0 && --dropoutRemaining == 0)
            dropoutTarget = 1.0f;
        const float dropoutCoef = dropoutTarget < dropoutGain ? dropoutAttack : dropoutRelease;
        dropoutGain = dropoutTarget + (dropoutGain - dropoutTarget) * dropoutCoef;
        const float level = (1.0f - wobbleDepth * (0.5f + 0.5f * levelWobble.next()))
                          * (1.0f - crinkleAmDepth * (0.5f + 0.5f * crinkleLevel.next()))
                          * dropoutGain;

        float played[2];
        for (size_t ch = 0; ch < 2; ++ch)
        {
            auto& c = channels[ch];
            const float driven = c.preEmphasis.process (io[ch][i]) * driveGain;
            const float saturated = shape::asymTube (driven, kSatBias - kHysteresis * c.memory);
            c.memory = c.magnetisation.lowpass (saturated);
            float y = c.deEmphasis.process (saturated * satNorm);
            y = c.hfLoss.process (c.headBump.process (y));
            c.transport.push (c.dcBlocker.process (y));
            played[ch] = c.transport.readHermite (delay + (ch == 1 ? azimuthDelay : 0.0f));
        }

        compEnvelope.process (std::max (std::abs (played[0]), std::abs (played[1])));
        compGain = compTarget + (compGain - compTarget) * compCoef;

        for (size_t ch = 0; ch < 2; ++ch)
        {
            auto& c = channels[ch];
            const float hiss = c.hissLowpass.lowpass (c.hissHighpass.highpass (c.hissRng.bipolar()));
            io[ch][i] = (played[ch] * compGain + hiss * hissLevel) * level;
        }
    }
}

} // namespace warmful
