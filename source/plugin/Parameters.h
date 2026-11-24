// WARMFUL — parameter IDs, layout and the mapping onto warmful::EngineParams.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Engine.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace warmful::params
{

// Primary controls, in Primary index order (CC# 64 + index).
inline constexpr std::array<const char*, kNumPrimary> kPrimaryIds {
    "tilt", "charAmount", "rate", "movAmount", "time", "diffAmount", "mix", "texAmount"
};
inline constexpr std::array<const char*, kNumPrimary> kPrimaryNames {
    "Tilt", "Character Amount", "Rate", "Movement Amount", "Time", "Diffusion Amount", "Mix", "Texture Amount"
};

// Secondary controls share the knob of the primary with the same index (CC# 72 + index).
inline constexpr std::array<const char*, kNumPrimary> kSecondaryIds {
    "sensitivity", "charVol", "driftMov", "movVol", "driftDiff", "diffVol", "outputLevel", "texVol"
};
inline constexpr std::array<const char*, kNumPrimary> kSecondaryNames {
    "Sensitivity", "Character Effect Vol", "Movement Drift", "Movement Effect Vol",
    "Diffusion Drift", "Diffusion Effect Vol", "Output Level", "Texture Effect Vol"
};
inline constexpr std::array<float, kNumPrimary> kSecondaryDefaults { 0.5f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f };

inline constexpr std::array<const char*, kNumModules> kEffectIds { "charFx", "movFx", "diffFx", "texFx" };
inline constexpr std::array<const char*, kNumModules> kModuleOnIds { "charOn", "movOn", "diffOn", "texOn" };
inline constexpr std::array<const char*, kNumModules> kDualMaskIds { "dualChar", "dualMov", "dualDiff", "dualTex" };

inline constexpr const char* kBypass = "bypass";
inline constexpr const char* kDualActive = "dualActive";
inline constexpr const char* kOrder = "order";
inline constexpr const char* kBypassMode = "bypassMode";
inline constexpr const char* kCaptureRouting = "captureRouting";
inline constexpr const char* kFilterStyle = "filterStyle";
inline constexpr const char* kCalibration = "calibration";
inline constexpr const char* kSyncMode = "syncMode";
inline constexpr const char* kTempo = "tempo";
inline constexpr const char* kExpTarget = "expTarget";
inline constexpr const char* kExpression = "expression";
inline constexpr const char* kMidiChannel = "midiChannel";

enum class SyncMode : int { Free = 0, Tap = 1, Host = 2 };

/** Module / effect names as printed on the pedal. */
inline constexpr std::array<const char*, kNumModules> kModuleNames { "CHARACTER", "MOVEMENT", "DIFFUSION", "TEXTURE" };
inline constexpr std::array<std::array<const char*, kEffectsPerModule>, kNumModules> kEffectNames { {
    { "DRIVE", "SWEETEN", "FUZZ", "HOWL", "SWELL" },
    { "DOUBLER", "VIBRATO", "PHASER", "TREMOLO", "PITCH" },
    { "CASCADE", "REELS", "SPACE", "COLLAGE", "REVERSE" },
    { "FILTER", "SQUASH", "CASSETTE", "BROKEN", "INTERFERENCE" },
} };

/** All 24 module orders, lexicographic: index 0 is the default CHAR > MOVE > DIFF > TEXT. */
const std::array<std::array<int, kNumModules>, 24>& moduleOrders();
int orderIndexOf (const std::array<int, kNumModules>& order);

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/** Parameters stored by the pedal's user presets (manual ch. 12). Global settings
    (calibration, bypass mode, MIDI channel) and the live bypass state are excluded. */
juce::StringArray presetParameterIds();

/** Cached raw pointers for lock-free reads on the audio thread. */
struct Handles
{
    explicit Handles (juce::AudioProcessorValueTreeState& state);
    void fill (EngineParams& p, double hostBpm, bool hostHasTempo) const noexcept;

    std::array<std::atomic<float>*, kNumPrimary> primary {}, secondary {};
    std::array<std::atomic<float>*, kNumModules> effect {}, moduleOn {}, dualMask {};
    std::atomic<float>* bypass = nullptr;
    std::atomic<float>* dualActive = nullptr;
    std::atomic<float>* order = nullptr;
    std::atomic<float>* bypassMode = nullptr;
    std::atomic<float>* captureRouting = nullptr;
    std::atomic<float>* filterStyle = nullptr;
    std::atomic<float>* calibration = nullptr;
    std::atomic<float>* syncMode = nullptr;
    std::atomic<float>* tempo = nullptr;
    std::atomic<float>* expTarget = nullptr;
    std::atomic<float>* expression = nullptr;
    std::atomic<float>* midiChannel = nullptr;
};

} // namespace warmful::params
