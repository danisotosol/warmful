// WARMFUL — TEXTURE / INTERFERENCE: radio static, AM-radio fading, codec damage and GSM buzz dissolving the signal.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Interference.h"

namespace warmful
{
namespace
{
namespace voicing
{
// Flavours along AMOUNT (overlapping smoothstep ramps): 1 static & crackle, 2 AM radio,
// 3 telecom codec damage, 4 everything denser while the signal dissolves.
constexpr float kStaticIn0 = 0.0f, kStaticIn1 = 0.25f;
constexpr float kRadioIn0 = 0.2f, kRadioIn1 = 0.45f;
constexpr float kCodecIn0 = 0.45f, kCodecIn1 = 0.75f;
constexpr float kChaosIn0 = 0.8f, kChaosIn1 = 1.0f;
constexpr float kEarlyRecede = 0.4f;          // static/radio step back a little while the next flavour takes over
constexpr float kChaosDensity = 3.0f;         // event rates multiply by up to 1 + this at the top
constexpr float kMinIntensity = 0.4f;         // disturbance level at the start of the travel (x AMOUNT ramp)

// Input envelope the disturbances follow, so they breathe with the playing.
constexpr float kEnvAttackSec = 0.005f;
constexpr float kEnvReleaseSec = 0.25f;

// 1. Static & crackle.
constexpr float kStaticFloor = 0.004f;        // static still present in silence
constexpr float kStaticEnvGain = 0.12f;
constexpr float kStaticBandHz = 3500.0f;
constexpr float kStaticBandQ = 0.7f;
constexpr float kStaticBurstHz = 3.0f;
constexpr float kCrackleRateMin = 4.0f;       // impulses per second
constexpr float kCrackleRateMax = 40.0f;
constexpr float kCrackleDecaySec = 0.0004f;
constexpr float kCrackleFloor = 0.01f;
constexpr float kCrackleEnvGain = 0.4f;

// 2. AM radio.
constexpr float kRadioLowHz = 300.0f;
constexpr float kRadioHighHz = 3000.0f;
constexpr float kRadioQ = 0.707f;
constexpr float kRadioMix = 0.85f;            // a trace of the full-band signal survives
constexpr float kRadioGain = 1.25f;           // band-limiting loses level; give some back
constexpr float kRadioFadeHz = 0.35f;
constexpr float kRadioFadeDepth = 0.6f;
constexpr float kWhistleMinHz = 1200.0f;
constexpr float kWhistleMaxHz = 3800.0f;
constexpr float kWhistleDriftHz = 0.08f;
constexpr float kWhistleFloor = 0.002f;
constexpr float kWhistleEnvGain = 0.03f;

// 3. Codec damage.
constexpr float kCrushBurstRate = 1.0f;       // bursts per second at full codec flavour (x density)
constexpr float kCrushMinSec = 0.08f, kCrushMaxSec = 0.5f;
constexpr float kCrushMinHz = 3000.0f, kCrushMaxHz = 8000.0f;
constexpr float kCrushMinBits = 5.0f, kCrushMaxBits = 9.0f;
constexpr float kPacketRate = 2.0f;           // packet events per second at full codec flavour (x density)
constexpr float kPacketMinSec = 0.01f, kPacketMaxSec = 0.04f;
constexpr float kPacketRepeatChance = 0.5f;   // else the packet is dropped
constexpr int kPacketMaxRepeats = 4;
constexpr float kPacketFadeSec = 0.0015f;     // tiny fades at stutter seams
constexpr float kPacketBufferSec = 0.25f;
constexpr float kEdgeSec = 0.001f;            // glitch on/off smoothing
constexpr float kBuzzHz = 217.0f;             // GSM TDMA frame rate
constexpr float kBuzzDuty = 0.125f;           // one slot in eight
constexpr float kBuzzLowpassHz = 4000.0f;
constexpr float kBuzzLevel = 0.06f;
constexpr float kBuzzFloor = 0.3f;            // buzz partly independent of the playing
constexpr float kBuzzEnvGain = 2.0f;
constexpr float kBuzzBurstRate = 0.5f;
constexpr float kBuzzMinSec = 0.2f, kBuzzMaxSec = 1.2f;
constexpr float kBuzzGateSec = 0.004f;

// 4. Dissolve.
constexpr float kDissolveDepth = 0.75f;
constexpr float kDissolveHz = 1.5f;

constexpr uint32_t kSeed = 0x1A7EF3E5u;
} // namespace voicing
} // namespace

using namespace voicing;
using texture::kControlInterval;

void InterferenceEffect::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    amount.prepare (sampleRate);
    for (auto& c : channels)
    {
        c.radioHighpass.setHighpass (kRadioLowHz, kRadioQ, sampleRate);
        c.radioLowpass.setLowpass (kRadioHighHz, kRadioQ, sampleRate);
        c.staticBand.setBandpass (kStaticBandHz, kStaticBandQ, sampleRate);
        c.packets.prepare (texture::toSamples (kPacketBufferSec, sampleRate));
    }
    envelope.prepare (sampleRate, kEnvAttackSec, kEnvReleaseSec);
    radioFade.setRate (kRadioFadeHz, sampleRate);
    dissolve.setRate (kDissolveHz, sampleRate);
    whistleDrift.setRate (kWhistleDriftHz, sampleRate / (double) kControlInterval);
    buzz.setFrequency (kBuzzHz, sampleRate);
    buzzLowpass.setCutoff (kBuzzLowpassHz, sampleRate);
    buzzDc.prepare (sampleRate);
    edgeCoef = onePoleCoef (kEdgeSec, sampleRate);
    buzzGateCoef = onePoleCoef (kBuzzGateSec, sampleRate);
    crackleDecay = onePoleCoef (kCrackleDecaySec, sampleRate);
    packetFade = kPacketFadeSec * (float) sampleRate;
    reset();
}

void InterferenceEffect::reset() noexcept
{
    amount.reset();
    uint32_t seed = kSeed;
    for (auto& c : channels)
    {
        c.radioHighpass.reset();
        c.radioLowpass.reset();
        c.staticBand.reset();
        c.packets.reset();
        c.noise.seed (seed += 0x9E3779B9u);
        c.staticBursts.seed (seed ^ 0x5A5A5A5Au);
        c.staticBursts.reset();
        c.crackle = 0.0f;
        c.held = 0.0f;
    }
    envelope.reset();
    radioFade.seed (kSeed ^ 0x1111u);
    whistleDrift.seed (kSeed ^ 0x2222u);
    dissolve.seed (kSeed ^ 0x3333u);
    for (auto* s : { &radioFade, &whistleDrift, &dissolve })
        s->reset();
    whistle.setPhase (0.0f);
    buzz.setPhase (0.0f);
    buzzLowpass.reset();
    buzzDc.reset();
    events.seed (kSeed);
    crushRemaining = 0;
    crushWeight = crushPhase = 0.0f;
    crushInc = 1.0f;
    packetRemaining = packetElapsed = 0;
    packetLength = 1;
    packetRepeat = false;
    packetWeight = 0.0f;
    buzzRemaining = 0;
    buzzGate = 0.0f;
    controlCountdown = 0;
}

void InterferenceEffect::updateControls() noexcept
{
    const float a = amount.next();
    const float chaos = smoothstep (kChaosIn0, kChaosIn1, a);
    const float codec = smoothstep (kCodecIn0, kCodecIn1, a);
    const float staticW = smoothstep (kStaticIn0, kStaticIn1, a) * (1.0f - kEarlyRecede * smoothstep (kRadioIn0, kCodecIn0, a))
                        + kEarlyRecede * chaos;
    const float radioW = smoothstep (kRadioIn0, kRadioIn1, a) * (1.0f - kEarlyRecede * codec) + kEarlyRecede * chaos;
    const float density = 1.0f + kChaosDensity * chaos;
    const float intensity = lerp (kMinIntensity, 1.0f, a);

    staticLevel = staticW * intensity;
    crackleLevel = staticW * intensity;
    crackleProbability = lerp (kCrackleRateMin, kCrackleRateMax, a) * density / (float) sr;
    for (auto& c : channels)
        c.staticBursts.setRate (kStaticBurstHz * density, sr);

    radioMix = kRadioMix * radioW;
    whistleLevel = radioW * intensity;
    whistle.setFrequency (lerp (kWhistleMinHz, kWhistleMaxHz, 0.5f + 0.5f * whistleDrift.next()), sr);

    crushRate = kCrushBurstRate * codec * density;
    packetRate = kPacketRate * codec * density;
    buzzRate = kBuzzBurstRate * codec * density;
    buzzLevel = kBuzzLevel * codec * intensity;

    dissolveDepth = kDissolveDepth * chaos;
}

void InterferenceEffect::rollEvents() noexcept
{
    const float dt = (float) kControlInterval / (float) sr;
    if (crushRemaining == 0 && texture::eventFires (events, crushRate, dt))
    {
        crushRemaining = texture::toSamples (lerp (kCrushMinSec, kCrushMaxSec, events.uniform()), sr);
        crushInc = lerp (kCrushMinHz, kCrushMaxHz, events.uniform()) / (float) sr;
        crushSteps = std::exp2 (lerp (kCrushMinBits, kCrushMaxBits, events.uniform()) - 1.0f);
    }
    if (packetRemaining == 0 && texture::eventFires (events, packetRate, dt))
    {
        packetLength = texture::toSamples (lerp (kPacketMinSec, kPacketMaxSec, events.uniform()), sr);
        packetRepeat = events.uniform() < kPacketRepeatChance;
        const int repeats = packetRepeat ? 1 + (int) (events.uniform() * (float) kPacketMaxRepeats) : 1;
        packetRemaining = packetLength * repeats;
        packetElapsed = 0;
    }
    if (buzzRemaining == 0 && texture::eventFires (events, buzzRate, dt))
        buzzRemaining = texture::toSamples (lerp (kBuzzMinSec, kBuzzMaxSec, events.uniform()), sr);
}

void InterferenceEffect::process (float* left, float* right, int numSamples, const EffectParams& params) noexcept
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

        const float env = envelope.process (std::max (std::abs (left[i]), std::abs (right[i])));

        // Shared (mono) interference sources.
        const float fade = 1.0f - kRadioFadeDepth * (0.5f + 0.5f * radioFade.next());
        whistle.advance();
        const float whistleOut = whistle.sine() * whistleLevel * (kWhistleFloor + kWhistleEnvGain * env);

        buzz.advance();
        const float buzzTarget = buzzRemaining > 0 ? 1.0f : 0.0f;
        buzzGate = buzzTarget + (buzzGate - buzzTarget) * buzzGateCoef;
        if (buzzRemaining > 0)
            --buzzRemaining;
        const float pulse = (buzz.getPhase() < kBuzzDuty ? 1.0f : 0.0f) - kBuzzDuty;
        const float buzzOut = buzzDc.process (buzzLowpass.lowpass (pulse)) * buzzGate * buzzLevel
                            * (kBuzzFloor + (1.0f - kBuzzFloor) * std::min (1.0f, kBuzzEnvGain * env));

        // Codec damage state.
        const float crushTarget = crushRemaining > 0 ? 1.0f : 0.0f;
        crushWeight = crushTarget + (crushWeight - crushTarget) * edgeCoef;
        if (crushRemaining > 0)
            --crushRemaining;
        crushPhase += crushInc;
        const bool takeSample = crushPhase >= 1.0f;
        if (takeSample)
            crushPhase -= std::floor (crushPhase);

        const bool packetActive = packetRemaining > 0;
        const float packetTarget = packetActive ? 1.0f : 0.0f;
        packetWeight = packetTarget + (packetWeight - packetTarget) * edgeCoef;
        int packetTap = 0;
        float packetWindow = 0.0f;
        if (packetActive)
        {
            // A repeat replays the packet just before the event: the tap steps back one packet per loop.
            const int k = packetElapsed % packetLength;
            packetTap = packetLength + (packetElapsed - k);
            packetWindow = packetRepeat ? std::min (1.0f, (float) std::min (k, packetLength - k) / packetFade) : 0.0f;
            ++packetElapsed;
            --packetRemaining;
        }

        const float dissolveGain = 1.0f - dissolveDepth * (0.6f + 0.4f * dissolve.next());

        for (size_t ch = 0; ch < 2; ++ch)
        {
            auto& c = channels[ch];
            const float x = io[ch][i];

            // 2. AM radio band-limiting with slow fading.
            const float radio = c.radioLowpass.process (c.radioHighpass.process (x)) * kRadioGain * fade;
            float sig = x + radioMix * (radio - x);

            // 3. Sample-rate / bit reduction bursts.
            if (takeSample)
                c.held = sig;
            const float crushed = std::round (c.held * crushSteps) / crushSteps;
            sig += crushWeight * (crushed - sig);

            // 3. Packet loss: dropped (silence) or repeated (stutter).
            c.packets.push (sig);
            const float packet = packetActive ? c.packets.tap (packetTap) * packetWindow : 0.0f;
            sig += packetWeight * (packet - sig);

            // 4. The signal itself dissolves.
            sig *= dissolveGain;

            // 1. Static bursts and crackle, following the playing.
            const float burst = clamp (0.35f + 0.65f * c.staticBursts.next(), 0.0f, 1.0f);
            const float hiss = c.staticBand.process (c.noise.bipolar()) * burst * burst
                             * staticLevel * (kStaticFloor + kStaticEnvGain * env);
            if (c.noise.uniform() < crackleProbability)
                c.crackle = c.noise.bipolar() * crackleLevel * (kCrackleFloor + kCrackleEnvGain * env);
            const float crackle = c.crackle;
            c.crackle = flushDenormal (c.crackle * crackleDecay);

            io[ch][i] = sig + hiss + crackle + whistleOut + buzzOut;
        }
    }
}

} // namespace warmful
