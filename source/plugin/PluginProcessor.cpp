// WARMFUL — processor implementation: audio, MIDI implementation chart, presets, footswitches.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"

namespace warmful
{

namespace
{
    double nowMs() { return juce::Time::getMillisecondCounterHiRes(); }

    juce::ValueTree gestureToTree (const Gesture::Snapshot& snapshot)
    {
        juce::ValueTree tree ("GESTURE");
        for (size_t k = 0; k < snapshot.size(); ++k)
        {
            const auto& lane = snapshot[k];
            if (lane.points.size() < 2)
                continue;
            juce::ValueTree t ("LANE");
            t.setProperty ("knob", (int) k, nullptr);
            t.setProperty ("bpm", lane.bpm, nullptr);
            t.setProperty ("data", juce::MemoryBlock (lane.points.data(), lane.points.size() * sizeof (float)).toBase64Encoding(), nullptr);
            tree.appendChild (t, nullptr);
        }
        return tree;
    }

    Gesture::Snapshot treeToGesture (const juce::ValueTree& tree)
    {
        Gesture::Snapshot snapshot;
        for (const auto& t : tree)
        {
            const int k = t.getProperty ("knob", -1);
            if (k < 0 || k >= kNumPrimary)
                continue;
            juce::MemoryBlock block;
            if (! block.fromBase64Encoding (t.getProperty ("data").toString()))
                continue;
            const auto count = std::min (block.getSize() / sizeof (float), (size_t) Gesture::kMaxPoints);
            auto& lane = snapshot[(size_t) k];
            lane.points.resize (count);
            std::memcpy (lane.points.data(), block.getData(), count * sizeof (float));
            lane.length = (int) count;
            lane.bpm = t.getProperty ("bpm", 120.0);
        }
        return snapshot;
    }
}

WarmfulProcessor::WarmfulProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "WARMFUL", params::createLayout()),
      handles (state)
{
}

WarmfulProcessor::~WarmfulProcessor()
{
    cancelPendingUpdate();
}

bool WarmfulProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Mono or stereo on each side, like the pedal's automatic mono/stereo routing.
    auto ok = [] (const juce::AudioChannelSet& s)
    { return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo(); };
    return ok (layouts.getMainInputChannelSet()) && ok (layouts.getMainOutputChannelSet());
}

void WarmfulProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    scratch.setSize (1, std::max (samplesPerBlock, 4096));
}

void WarmfulProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    handleMidi (midi);
    midi.clear();

    double hostBpm = 120.0;
    bool hostHasTempo = false;
    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto bpm = position->getBpm())
            {
                hostBpm = *bpm;
                hostHasTempo = true;
            }
    handles.fill (engineParams, hostBpm, hostHasTempo);

    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    const bool monoInput = numIn == 1;

    for (int offset = 0; offset < n;)
    {
        const int len = buffer.getNumChannels() >= 2 ? n - offset : std::min (n - offset, scratch.getNumSamples());
        float* l = buffer.getWritePointer (0, offset);
        float* r;
        if (buffer.getNumChannels() >= 2)
        {
            r = buffer.getWritePointer (1, offset);
            if (monoInput)
                std::copy (l, l + len, r);
        }
        else
        {
            r = scratch.getWritePointer (0);
            std::copy (l, l + len, r);
        }

        engine.process (l, r, len, engineParams, monoInput);

        if (numOut == 1)
            for (int i = 0; i < len; ++i)
                l[i] = 0.5f * (l[i] + r[i]);   // stereo-to-mono output
        offset += len;
    }

    const int calibrated = engine.takeAutoCalibrationResult();
    if (calibrated >= 0)
        setFromAudioThread (params::kCalibration, (float) calibrated);
}

// ---- MIDI implementation chart (manual ch. 16) -----------------------------------------------

void WarmfulProcessor::handleMidi (const juce::MidiBuffer& midi) noexcept
{
    const int channel = (int) std::lround (handles.midiChannel->load());   // 0 = omni
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (channel != 0 && msg.getChannel() != channel)
            continue;
        if (msg.isController())
            handleController (msg.getControllerNumber(), msg.getControllerValue());
        else if (msg.isProgramChange() && msg.getProgramChangeNumber() < PresetBank::kNumSlots)
        {
            pendingProgram.store (msg.getProgramChangeNumber());
            triggerAsyncUpdate();
        }
    }
}

void WarmfulProcessor::setFromAudioThread (const char* id, float plainValue) noexcept
{
    if (auto* p = state.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void WarmfulProcessor::handleController (int cc, int v) noexcept
{
    using namespace params;
    const float norm = (float) v / 127.0f;

    if (cc >= 64 && cc <= 71) { setFromAudioThread (kPrimaryIds[(size_t) (cc - 64)], norm); return; }
    if (cc >= 72 && cc <= 79) { setFromAudioThread (kSecondaryIds[(size_t) (cc - 72)], norm); return; }

    if (cc >= 16 && cc <= 19)
    {
        const auto m = (size_t) (cc - 16);
        if (v >= 110)
            setFromAudioThread (kModuleOnIds[m], 0.0f);
        else
        {
            setFromAudioThread (kEffectIds[m], (float) std::min (v / 22, kEffectsPerModule - 1));
            setFromAudioThread (kModuleOnIds[m], 1.0f);
        }
        return;
    }
    if (cc >= 103 && cc <= 106) { setFromAudioThread (kModuleOnIds[(size_t) (cc - 103)], v >= 64 ? 1.0f : 0.0f); return; }

    switch (cc)
    {
        case 91: setFromAudioThread (kBypass, v < 64 ? 1.0f : 0.0f); break;
        case 92:
            setFromAudioThread (kBypass, v < 32 ? 1.0f : 0.0f);
            setFromAudioThread (kDualActive, (v >= 32 && v < 64) ? 1.0f : 0.0f);
            break;
        case 80: if (v >= 64) engine.gestureBeginRecording(); else engine.gestureEndRecording(); break;
        case 81: engine.gestureClear(); break;
        case 82:
            if (v < 44) engine.captureStop();
            else if (v < 88) engine.captureCommit();
            else engine.captureArm();
            break;
        case 83: setFromAudioThread (kCaptureRouting, v >= 64 ? 1.0f : 0.0f); break;
        case 84: setFromAudioThread (kFilterStyle, v < 44 ? 0.0f : (v < 88 ? 1.0f : 2.0f)); break;
        case 93: pendingMidiTaps.fetch_add (1); triggerAsyncUpdate(); break;
        case 94: setFromAudioThread (kCalibration, (float) std::min (v / 32, 3)); break;
        case 95: if (v >= 64) engine.startAutoCalibration(); break;
        default: break;
    }
}

void WarmfulProcessor::handleAsyncUpdate()
{
    const int program = pendingProgram.exchange (-1);
    if (program >= 0)
        loadPreset (program);
    const int factory = pendingFactory.exchange (-1);
    if (factory >= 0)
        loadFactoryPreset (factory);
    if (pendingMidiTaps.exchange (0) > 0)
        tapTempo (nowMs());
}

// ---- Host program list (factory bank) ----------------------------------------------------------

int WarmfulProcessor::getNumPrograms()
{
    return 1 + (int) factoryPresets().size();
}

const juce::String WarmfulProcessor::getProgramName (int index)
{
    const auto presets = factoryPresets();
    if (index >= 1 && index <= (int) presets.size())
        return presets[(size_t) index - 1].name;
    return "(current settings)";
}

void WarmfulProcessor::setCurrentProgram (int index)
{
    if (index < 1 || index > (int) factoryPresets().size())
        return;
    // Apply immediately when the host calls from the message thread (the usual case), so a
    // program change never lands later on top of whatever the host does next (e.g. a state
    // restore). Calls from other threads are deferred to the message thread.
    if (juce::MessageManager::existsAndIsCurrentThread())
        loadFactoryPreset (index - 1);
    else
    {
        pendingFactory.store (index - 1);
        triggerAsyncUpdate();
    }
}

// ---- Pedal actions -------------------------------------------------------------------------

void WarmfulProcessor::setParam (const char* id, float plainValue)
{
    if (auto* p = state.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

float WarmfulProcessor::getParam (const char* id) const
{
    auto* v = state.getRawParameterValue (id);
    return v != nullptr ? v->load() : 0.0f;
}

void WarmfulProcessor::stepEffect (int module)
{
    const auto m = (size_t) module;
    if (getParam (params::kModuleOnIds[m]) < 0.5f)
    {
        setParam (params::kModuleOnIds[m], 1.0f);
        return;
    }
    const int next = ((int) std::lround (getParam (params::kEffectIds[m])) + 1) % kEffectsPerModule;
    setParam (params::kEffectIds[m], (float) next);
}

void WarmfulProcessor::toggleModule (int module)
{
    const auto m = (size_t) module;
    setParam (params::kModuleOnIds[m], getParam (params::kModuleOnIds[m]) >= 0.5f ? 0.0f : 1.0f);
}

void WarmfulProcessor::tapFootswitchDown()
{
    tapDownMs = nowMs();
    const auto mode = engine.getCaptureMode();
    captureWasPlaying = mode == Capture::Mode::Sustain || mode == Capture::Mode::Loop;
    engine.captureArm();   // recording starts at the press; a short tap discards it
}

void WarmfulProcessor::tapFootswitchUp()
{
    const double held = (nowMs() - tapDownMs) / 1000.0;
    if (held >= kHoldSeconds)
    {
        engine.captureCommit();
        return;
    }
    engine.captureCancel();
    if (! captureWasPlaying)   // a tap while looping only stops the loop
        tapTempo (tapDownMs);
}

void WarmfulProcessor::tapTempo (double timeMs)
{
    double bpm = 0.0;
    if (! tapper.tap (timeMs / 1000.0, bpm))
        return;
    setParam (params::kTempo, (float) bpm);
    if ((params::SyncMode) std::lround (getParam (params::kSyncMode)) == params::SyncMode::Free)
        setParam (params::kSyncMode, (float) params::SyncMode::Tap);
}

void WarmfulProcessor::bypassFootswitchPressed()
{
    using namespace params;
    const double now = nowMs();
    bool dualConfigured = false;
    for (auto* id : kDualMaskIds)
        dualConfigured |= getParam (id) < 0.5f;

    const bool bypassed = getParam (kBypass) >= 0.5f;

    if (! dualConfigured)
    {
        setParam (kBypass, bypassed ? 0.0f : 1.0f);
    }
    else if (now - lastBypassPressMs < 400.0 && lastPressToggledDual)
    {
        // Double tap with DUAL BYPASS: undo the first press and switch the whole pedal.
        setParam (kDualActive, getParam (kDualActive) >= 0.5f ? 0.0f : 1.0f);
        setParam (kBypass, bypassed ? 0.0f : 1.0f);
        lastPressToggledDual = false;
        lastBypassPressMs = -1.0e9;
        return;
    }
    else if (bypassed)
    {
        setParam (kBypass, 0.0f);
        lastPressToggledDual = false;
    }
    else
    {
        setParam (kDualActive, getParam (kDualActive) >= 0.5f ? 0.0f : 1.0f);
        lastPressToggledDual = true;
    }
    lastBypassPressMs = now;
}

// ---- Presets ---------------------------------------------------------------------------------

juce::ValueTree WarmfulProcessor::capturePreset()
{
    juce::ValueTree preset ("PRESET");
    for (const auto& id : params::presetParameterIds())
        preset.setProperty (id, getParam (id.toRawUTF8()), nullptr);
    preset.appendChild (gestureToTree (engine.getGestureSnapshot()), nullptr);
    return preset;
}

void WarmfulProcessor::applyPreset (const juce::ValueTree& preset)
{
    for (const auto& id : params::presetParameterIds())
        if (preset.hasProperty (id))
            setParam (id.toRawUTF8(), (float) preset.getProperty (id));
    engine.setGestureSnapshot (treeToGesture (preset.getChildWithName ("GESTURE")));
}

void WarmfulProcessor::savePreset (int slot)
{
    presetBank.put (slot, capturePreset());
    currentPreset.store (slot);
    currentFactory.store (-1);
}

bool WarmfulProcessor::loadPreset (int slot)
{
    presetBank.reload();
    if (! presetBank.has (slot))
        return false;
    applyPreset (presetBank.get (slot));
    currentPreset.store (slot);
    currentFactory.store (-1);
    return true;
}

bool WarmfulProcessor::loadFactoryPreset (int index)
{
    const auto presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return false;
    const auto& f = presets[(size_t) index];

    // A factory preset is a complete preset: everything a user preset stores is set.
    for (size_t k = 0; k < (size_t) kNumPrimary; ++k)
    {
        setParam (params::kPrimaryIds[k], f.primary[k]);
        setParam (params::kSecondaryIds[k], f.secondary[k]);
    }
    for (size_t m = 0; m < (size_t) kNumModules; ++m)
    {
        setParam (params::kEffectIds[m], (float) f.effect[m]);
        setParam (params::kModuleOnIds[m], f.moduleOn[m] ? 1.0f : 0.0f);
        setParam (params::kDualMaskIds[m], 1.0f);
    }
    setParam (params::kOrder, (float) params::orderIndexOf (f.order));
    setParam (params::kFilterStyle, (float) f.filterStyle);
    setParam (params::kCaptureRouting, 0.0f);
    setParam (params::kExpTarget, 0.0f);
    engine.setGestureSnapshot ({});

    currentFactory.store (index);
    currentPreset.store (-1);
    return true;
}

void WarmfulProcessor::stepPreset (int direction)
{
    const int step = direction < 0 ? -1 : 1;
    const int user = currentPreset.load();
    if (user >= 0)
    {
        // Next saved user slot in that direction (wrapping); stay put if it is the only one.
        presetBank.reload();
        for (int i = 1; i < PresetBank::kNumSlots; ++i)
        {
            const int slot = (user + step * i + PresetBank::kNumSlots) % PresetBank::kNumSlots;
            if (presetBank.has (slot))
            {
                loadPreset (slot);
                return;
            }
        }
        return;
    }
    const int count = (int) factoryPresets().size();
    const int factory = currentFactory.load();
    const int next = factory < 0 ? (step > 0 ? 0 : count - 1) : (factory + step + count) % count;
    loadFactoryPreset (next);
}

// ---- State ------------------------------------------------------------------------------------

void WarmfulProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.removeChild (tree.getChildWithName ("GESTURE"), nullptr);
    tree.appendChild (gestureToTree (engine.getGestureSnapshot()), nullptr);
    tree.setProperty ("currentPreset", currentPreset.load(), nullptr);
    tree.setProperty ("currentFactory", currentFactory.load(), nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary (*xml, destData);
}

void WarmfulProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (state.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    const auto gesture = tree.getChildWithName ("GESTURE");
    engine.setGestureSnapshot (treeToGesture (gesture));
    tree.removeChild (gesture, nullptr);
    currentPreset.store ((int) tree.getProperty ("currentPreset", -1));
    currentFactory.store ((int) tree.getProperty ("currentFactory", -1));
    tree.removeProperty ("currentPreset", nullptr);
    tree.removeProperty ("currentFactory", nullptr);
    state.replaceState (tree);

    // AudioParameterBool keeps the raw normalised value a host sent (e.g. 0.81 = on), while
    // replaceState() skips parameters whose snapped value already matches. Snap the booleans so
    // after a restore they hold exactly what the state says.
    for (auto* p : getParameters())
        if (p->isBoolean())
            if (const float snapped = p->getValue() >= 0.5f ? 1.0f : 0.0f; ! juce::approximatelyEqual (p->getValue(), snapped))
                p->setValueNotifyingHost (snapped);
}

juce::AudioProcessorParameter* WarmfulProcessor::getBypassParameter() const
{
    return state.getParameter (params::kBypass);
}

juce::AudioProcessorEditor* WarmfulProcessor::createEditor()
{
    return new WarmfulEditor (*this);
}

} // namespace warmful

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new warmful::WarmfulProcessor();
}
