// WARMFUL — parameter layout.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Parameters.h"

namespace warmful::params
{

namespace
{
    constexpr std::array<float, kNumPrimary> kPrimaryDefaults { 0.5f, 0.35f, 0.4f, 0.4f, 0.45f, 0.35f, 1.0f, 0.5f };
    constexpr const char* kShortModule[] { "CHAR", "MOVE", "DIFF", "TEXT" };

    juce::String percent (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }

    std::unique_ptr<juce::AudioParameterFloat> knob (const char* id, const char* name, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, 1.0f), def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent));
    }

    std::unique_ptr<juce::AudioParameterChoice> choice (const char* id, const char* name, juce::StringArray items, int def)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def);
    }

    std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const char* name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
    }
}

const std::array<std::array<int, kNumModules>, 24>& moduleOrders()
{
    static const auto orders = []
    {
        std::array<std::array<int, kNumModules>, 24> all {};
        std::array<int, kNumModules> o { 0, 1, 2, 3 };
        size_t i = 0;
        do { all[i++] = o; } while (std::next_permutation (o.begin(), o.end()));
        return all;
    }();
    return orders;
}

int orderIndexOf (const std::array<int, kNumModules>& order)
{
    const auto& all = moduleOrders();
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i] == order)
            return (int) i;
    return 0;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
        layout.add (knob (kPrimaryIds[k], kPrimaryNames[k], kPrimaryDefaults[k]));
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
        layout.add (knob (kSecondaryIds[k], kSecondaryNames[k], kSecondaryDefaults[k]));

    for (size_t m = 0; m < (size_t) kNumModules; ++m)
    {
        juce::StringArray names;
        for (auto* n : kEffectNames[m])
            names.add (n);
        const juce::String module = juce::String (kModuleNames[m]).toLowerCase();
        layout.add (choice (kEffectIds[m], (juce::String (kModuleNames[m]) + " Effect").toRawUTF8(), names, 0));
        layout.add (toggle (kModuleOnIds[m], (juce::String (kModuleNames[m]) + " On").toRawUTF8(), true));
        layout.add (toggle (kDualMaskIds[m], (juce::String ("Dual Bypass ") + kModuleNames[m]).toRawUTF8(), true));
    }

    juce::StringArray orderNames;
    for (const auto& o : moduleOrders())
        orderNames.add (juce::String (kShortModule[o[0]]) + " > " + kShortModule[o[1]] + " > "
                        + kShortModule[o[2]] + " > " + kShortModule[o[3]]);

    layout.add (toggle (kBypass, "Bypass", false));
    layout.add (toggle (kDualActive, "Dual Bypass Engaged", false));
    layout.add (choice (kOrder, "Module Routing", orderNames, 0));
    layout.add (choice (kBypassMode, "Bypass Mode", { "Buffered", "Buffered + Trails", "True Bypass" }, 0));
    layout.add (choice (kCaptureRouting, "Capture Routing", { "Post-FX", "Pre-FX" }, 0));
    layout.add (choice (kFilterStyle, "Filter Style", { "Low-Pass", "Tilt", "High-Pass" }, 1));
    layout.add (choice (kCalibration, "Calibration", { "Low", "Medium", "High", "Very High" }, 1));
    layout.add (choice (kSyncMode, "Tempo Sync", { "Free", "Tap", "Host" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kTempo, 1 }, "Tempo", juce::NormalisableRange<float> (30.0f, 300.0f, 0.01f), 120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("BPM")));
    layout.add (choice (kExpTarget, "Expression Target",
                        { "None", "Tilt", "Character Amount", "Rate", "Movement Amount", "Time", "Diffusion Amount", "Mix", "Texture Amount" }, 0));
    layout.add (knob (kExpression, "Expression", 0.0f));

    juce::StringArray channels { "Omni" };
    for (int c = 1; c <= 16; ++c)
        channels.add (juce::String (c));
    layout.add (choice (kMidiChannel, "MIDI Channel", channels, 1));

    return layout;
}

juce::StringArray presetParameterIds()
{
    juce::StringArray ids;
    for (auto* id : kPrimaryIds) ids.add (id);
    for (auto* id : kSecondaryIds) ids.add (id);
    for (auto* id : kEffectIds) ids.add (id);
    for (auto* id : kModuleOnIds) ids.add (id);
    for (auto* id : kDualMaskIds) ids.add (id);
    ids.addArray ({ kOrder, kCaptureRouting, kFilterStyle, kSyncMode, kTempo, kExpTarget });
    return ids;
}

Handles::Handles (juce::AudioProcessorValueTreeState& s)
{
    auto get = [&s] (const char* id)
    {
        auto* v = s.getRawParameterValue (id);
        jassert (v != nullptr);
        return v;
    };
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        primary[k] = get (kPrimaryIds[k]);
        secondary[k] = get (kSecondaryIds[k]);
    }
    for (size_t m = 0; m < (size_t) kNumModules; ++m)
    {
        effect[m] = get (kEffectIds[m]);
        moduleOn[m] = get (kModuleOnIds[m]);
        dualMask[m] = get (kDualMaskIds[m]);
    }
    bypass = get (kBypass);
    dualActive = get (kDualActive);
    order = get (kOrder);
    bypassMode = get (kBypassMode);
    captureRouting = get (kCaptureRouting);
    filterStyle = get (kFilterStyle);
    calibration = get (kCalibration);
    syncMode = get (kSyncMode);
    tempo = get (kTempo);
    expTarget = get (kExpTarget);
    expression = get (kExpression);
    midiChannel = get (kMidiChannel);
}

void Handles::fill (EngineParams& p, double hostBpm, bool hostHasTempo) const noexcept
{
    auto i = [] (const std::atomic<float>* v) { return (int) std::lround (v->load (std::memory_order_relaxed)); };
    auto b = [] (const std::atomic<float>* v) { return v->load (std::memory_order_relaxed) >= 0.5f; };
    auto f = [] (const std::atomic<float>* v) { return v->load (std::memory_order_relaxed); };

    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
        p.primary[k] = f (primary[k]);

    p.sensitivity = f (secondary[0]);
    p.effectVol = { f (secondary[1]), f (secondary[3]), f (secondary[5]), f (secondary[7]) };
    p.driftMovement = f (secondary[2]);
    p.driftDiffusion = f (secondary[4]);
    p.outputLevel = f (secondary[6]);

    for (size_t m = 0; m < (size_t) kNumModules; ++m)
    {
        p.effect[m] = juce::jlimit (0, kEffectsPerModule - 1, i (effect[m]));
        p.moduleOn[m] = b (moduleOn[m]);
        p.dualMask[m] = b (dualMask[m]);
    }
    p.order = moduleOrders()[(size_t) juce::jlimit (0, 23, i (order))];

    p.bypassed = b (bypass);
    p.dualBypassActive = b (dualActive);
    p.bypassMode = (BypassMode) juce::jlimit (0, 2, i (bypassMode));
    p.captureRouting = (CaptureRouting) juce::jlimit (0, 1, i (captureRouting));
    p.filterStyle = (FilterStyle) juce::jlimit (0, 2, i (filterStyle));
    p.calibrationLevel = juce::jlimit (0, 3, i (calibration));
    p.expressionTarget = i (expTarget) - 1;
    p.expression = f (expression);

    const auto mode = (SyncMode) juce::jlimit (0, 2, i (syncMode));
    const double tapped = (double) f (tempo);
    p.tempo.synced = mode == SyncMode::Tap || (mode == SyncMode::Host && hostHasTempo);
    p.tempo.bpm = (mode == SyncMode::Host && hostHasTempo) ? hostBpm : tapped;
}

} // namespace warmful::params
