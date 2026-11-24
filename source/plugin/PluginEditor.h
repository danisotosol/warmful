// WARMFUL — pedal-style editor.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/Panels.h"

namespace warmful
{

/** The pedal face, laid out at a fixed base size and scaled by the editor. */
class PedalView final : public juce::Component, private juce::Timer
{
public:
    static constexpr int kWidth = 560;
    static constexpr int kHeight = 720;

    explicit PedalView (WarmfulProcessor&);
    ~PedalView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    enum class Menu { Secondary, Gesture, FxSetup, CopySave, Global };

    void timerCallback() override;
    void openMenu (Menu);
    void showPanel (std::unique_ptr<ui::Panel>);
    void closePanel();
    void setSecondaryMode (bool);
    void bypassReleased();
    juce::Rectangle<int> stripArea() const { return { 14, 440, kWidth - 28, 96 }; }
    juce::Rectangle<int> menuRow (int index) const;
    juce::String statusText() const;
    juce::String presetBarText() const;
    juce::Rectangle<int> presetBarArea() const { return { kWidth / 2 - 175, 656, 350, 24 }; }
    static constexpr float kPresetArrowWidth = 30.0f;

    WarmfulProcessor& processor;
    ui::LookAndFeel lnf;
    std::vector<std::unique_ptr<ui::Knob>> knobs;
    std::vector<std::unique_ptr<ui::ModuleButton>> moduleButtons;
    ui::Footswitch tapSwitch { "TAP", "CAPTURE", false };
    ui::Footswitch bypassSwitch { "BYPASS", "PRESETS", true };
    ui::LedBars leds;
    std::unique_ptr<ui::Panel> panel;
    bool secondaryMode = false;
    bool blink = false;
};

class WarmfulEditor final : public juce::AudioProcessorEditor
{
public:
    explicit WarmfulEditor (WarmfulProcessor&);
    void resized() override;

private:
    PedalView view;
};

} // namespace warmful
