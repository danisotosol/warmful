// WARMFUL — JUCE audio processor wrapping warmful::Engine.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Parameters.h"
#include "PresetBank.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace warmful
{

class WarmfulProcessor final : public juce::AudioProcessor,
                               private juce::AsyncUpdater
{
public:
    WarmfulProcessor();
    ~WarmfulProcessor() override;

    // ---- juce::AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    // Presets are handled by the pedal-style bank (MIDI Program Change), not host programs.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    // ---- Pedal actions (message thread) -------------------------------------------------
    /** Module button: a press steps to the next effect (wrapping); re-engages an OFF module. */
    void stepEffect (int module);
    /** Module button long press: toggles the module OFF/ON. */
    void toggleModule (int module);
    /** TAP footswitch. Press starts a tentative CAPTURE; release decides tap vs. capture. */
    void tapFootswitchDown();
    void tapFootswitchUp();
    /** BYPASS footswitch single press (handles DUAL BYPASS and its double tap). */
    void bypassFootswitchPressed();
    void tapTempo (double timeMs);

    void savePreset (int slot);
    bool loadPreset (int slot);
    int getCurrentPreset() const noexcept { return currentPreset.load(); }
    PresetBank& getPresetBank() noexcept { return presetBank; }

    /** Sets a parameter by ID in plain units (index for choices), as a host-visible gesture. */
    void setParam (const char* id, float plainValue);
    float getParam (const char* id) const;

    Engine& getEngine() noexcept { return engine; }
    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }

    static constexpr double kHoldSeconds = 0.4;   // TAP hold that starts a CAPTURE

private:
    void handleAsyncUpdate() override;
    void handleMidi (const juce::MidiBuffer& midi) noexcept;
    void handleController (int cc, int value) noexcept;
    void setFromAudioThread (const char* id, float plainValue) noexcept;

    juce::ValueTree capturePreset();
    void applyPreset (const juce::ValueTree& preset);

    juce::AudioProcessorValueTreeState state;
    params::Handles handles;
    Engine engine;
    EngineParams engineParams;
    PresetBank presetBank;
    TapTempo tapper;

    juce::AudioBuffer<float> scratch;
    std::atomic<int> currentPreset { -1 };
    std::atomic<int> pendingProgram { -1 };
    std::atomic<int> pendingMidiTaps { 0 };

    double tapDownMs = 0.0;
    bool captureWasPlaying = false;
    double lastBypassPressMs = -1.0e9;
    bool lastPressToggledDual = false;
};

} // namespace warmful
