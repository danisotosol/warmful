// WARMFUL — the contract every one of the 20 effects implements.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include <array>
#include <memory>

namespace warmful
{

enum class ModuleType : int { Character = 0, Movement = 1, Diffusion = 2, Texture = 3 };
constexpr int kNumModules = 4;
constexpr int kEffectsPerModule = 5;

/** Texture FILTER style, configured per preset in FX SETUP. */
enum class FilterStyle : int { LowPass = 0, Tilt = 1, HighPass = 2 };

struct TempoInfo
{
    double bpm = 120.0;
    /** True when tap tempo / host sync is active: RATE and TIME knobs pick quantised
        subdivisions of `bpm` instead of free values (see Tempo.h). */
    bool synced = false;
};

/** Block-constant control targets handed to an effect. All knobs are normalised 0..1,
    exactly like the pedal's pots. Effects smooth whatever they need internally. */
struct EffectParams
{
    /** Top knob of the column: TILT (Character), RATE (Movement), TIME (Diffusion).
        Unused by Texture. */
    float knob = 0.5f;
    /** AMOUNT knob of the column. */
    float amount = 0.0f;
    /** DRIFT secondary (Movement / Diffusion only). 0 = stable. */
    float drift = 0.0f;
    /** SENSITIVITY secondary (Character only). 0.5 = neutral; +-12 dB around the calibrated level. */
    float sensitivity = 0.5f;
    /** Diffusion only: EFFECT VOL as a linear gain applied to the wet component only.
        Other modules receive 1 and get their EFFECT VOL applied after the effect. */
    float wetLevel = 1.0f;
    FilterStyle filterStyle = FilterStyle::Tilt;
    TempoInfo tempo;
};

/** Signal level convention inside the effect chain: after calibration the instrument's
    typical peaks sit around 0.5 (-6 dBFS); 1.0 is the converter-style ceiling.
    Effects should stay roughly loudness-neutral at moderate settings. */
class Effect
{
public:
    virtual ~Effect() = default;
    virtual const char* name() const noexcept = 0;
    /** Allocate everything here. Never allocate in process(). */
    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    /** Clear all state (delay lines, envelopes...). No allocation. */
    virtual void reset() noexcept = 0;
    /** In-place stereo processing; numSamples <= maxBlockSize. */
    virtual void process (float* left, float* right, int numSamples, const EffectParams& params) noexcept = 0;
};

using EffectSet = std::array<std::unique_ptr<Effect>, kEffectsPerModule>;

// One factory per module, each implemented next to its effects.
EffectSet createCharacterEffects();  // Drive, Sweeten, Fuzz, Howl, Swell
EffectSet createMovementEffects();   // Doubler, Vibrato, Phaser, Tremolo, Pitch
EffectSet createDiffusionEffects();  // Cascade, Reels, Space, Collage, Reverse
EffectSet createTextureEffects();    // Filter, Squash, Cassette, Broken, Interference

inline EffectSet createEffects (ModuleType type)
{
    switch (type)
    {
        case ModuleType::Character: return createCharacterEffects();
        case ModuleType::Movement:  return createMovementEffects();
        case ModuleType::Diffusion: return createDiffusionEffects();
        case ModuleType::Texture:   return createTextureEffects();
    }
    return {};
}

} // namespace warmful
