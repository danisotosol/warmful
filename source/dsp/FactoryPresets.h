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
    Presets assume MEDIUM calibration (a DI guitar peaking around -18 dBFS).
    `pack` is the id of the FactoryPack it belongs to; names start with that id ("DIJ ..."). */
struct FactoryPreset
{
    const char* pack;
    const char* name;
    const char* description;
    std::array<int, kNumModules> effect;
    std::array<float, kNumPrimary> primary;
    std::array<float, kNumPrimary> secondary;
    std::array<int, kNumModules> order { 0, 1, 2, 3 };
    std::array<bool, kNumModules> moduleOn { true, true, true, true };
    FilterStyle filterStyle = FilterStyle::Tilt;
};

/** A themed group of factory presets (one tab in the browser). */
struct FactoryPack
{
    const char* id;      // 3-letter prefix used in preset names
    const char* title;   // shown on the tab
};

/** Browser layout limits: one tab per pack, each pack shown as a two-column grid. */
constexpr int kMaxFactoryPacks = 8;
constexpr int kFactoryGridRows = 5;
constexpr int kMaxPresetsPerPack = 2 * kFactoryGridRows;

std::span<const FactoryPreset> factoryPresets();
std::span<const FactoryPack> factoryPacks();

/** The engine state a preset describes (everything else keeps the values in `base`). */
EngineParams toEngineParams (const FactoryPreset& preset, EngineParams base = {});

} // namespace warmful
