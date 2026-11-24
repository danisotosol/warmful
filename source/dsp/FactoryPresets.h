// WARMFUL — factory presets (read-only bank, separate from the 80 user slots).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Engine.h"
#include <span>

namespace warmful
{

/** One factory sound. Knobs are 0..1 pot positions, indexed like the pedal:
    primary  = TILT, AMOUNT(CHAR), RATE, AMOUNT(MOVE), TIME, AMOUNT(DIFF), MIX, AMOUNT(TEX)
    secondary = SENSITIVITY, VOL(CHAR), DRIFT(MOVE), VOL(MOVE), DRIFT(DIFF), VOL(DIFF), OUTPUT, VOL(TEX)
    Presets assume MEDIUM calibration (a DI guitar peaking around -18 dBFS). */
struct FactoryPreset
{
    const char* name;
    const char* description;
    std::array<int, kNumModules> effect;
    std::array<float, kNumPrimary> primary;
    std::array<float, kNumPrimary> secondary;
    std::array<int, kNumModules> order { 0, 1, 2, 3 };
    std::array<bool, kNumModules> moduleOn { true, true, true, true };
    FilterStyle filterStyle = FilterStyle::Tilt;
};

std::span<const FactoryPreset> factoryPresets();

/** The engine state a preset describes (everything else keeps the values in `base`). */
EngineParams toEngineParams (const FactoryPreset& preset, EngineParams base = {});

} // namespace warmful
