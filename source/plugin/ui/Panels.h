// WARMFUL — overlay menus: FX SETUP, COPY/SAVE + PRESET BROWSER, GLOBAL SETTINGS / CALIBRATION.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Controls.h"
#include "FactoryPresets.h"

namespace warmful
{
class WarmfulProcessor;
}

namespace warmful::ui
{

using APVTS = juce::AudioProcessorValueTreeState;

/** Dark card drawn over the module area with a title and a close button. */
class Panel : public juce::Component
{
public:
    explicit Panel (juce::String title);
    std::function<void()> onClose;
    void paint (juce::Graphics&) override;
    void resized() override;

protected:
    /** Area below the title for the panel's own layout. */
    juce::Rectangle<int> content() const { return getLocalBounds().reduced (18).withTrimmedTop (34); }
    static void styleLabel (juce::Label&, const juce::String& text);

private:
    juce::String title;
    juce::TextButton close { "X" };
};

class FxSetupPanel final : public Panel, private juce::AudioProcessorValueTreeState::Listener
{
public:
    FxSetupPanel (WarmfulProcessor&);
    ~FxSetupPanel() override;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void parameterChanged (const juce::String&, float) override;
    void pressRoutingButton (int module);
    void resetSetup();

    WarmfulProcessor& processor;
    std::array<juce::TextButton, 4> routeButtons, dualButtons;
    std::array<std::unique_ptr<APVTS::ButtonAttachment>, 4> dualAttachments;
    juce::ComboBox captureRouting, filterStyle, expTarget;
    std::unique_ptr<APVTS::ComboBoxAttachment> captureAttachment, filterAttachment, expAttachment;
    juce::Slider expression;
    std::unique_ptr<APVTS::SliderAttachment> expressionAttachment;
    juce::TextButton reset { "RESET FX SETUP" };
    juce::Label routingLabel, dualLabel, captureLabel, filterLabel, expLabel;
    std::vector<int> pendingOrder;
};

class PresetPanel final : public Panel
{
public:
    enum class Mode { Browse, Save };
    PresetPanel (WarmfulProcessor&, Mode);
    void resized() override;
    void refresh();

private:
    void slotClicked (int slotInBank);

    WarmfulProcessor& processor;
    Mode mode;
    static constexpr int kFactoryBank = 4;   // tab after BANK A..D (browser only)
    int bank = 0, selected = -1;
    std::array<juce::TextButton, 5> bankButtons;
    std::array<juce::TextButton, 20> slotButtons;
    std::array<juce::TextButton, kMaxFactoryPacks> packButtons;   // factory packs (artist/theme tabs)
    int pack = 0;
    std::vector<int> packPresets;                  // factory indices shown in the grid
    juce::TextButton save { "SAVE" };
    juce::Label hint;
};

class GlobalPanel final : public Panel, private juce::Timer
{
public:
    GlobalPanel (WarmfulProcessor&);
    void resized() override;

private:
    void timerCallback() override;

    WarmfulProcessor& processor;
    std::array<juce::TextButton, 4> calButtons;
    juce::TextButton autoCal { "AUTO CALIBRATE" };
    juce::ComboBox bypassMode, midiChannel, syncMode;
    std::unique_ptr<APVTS::ComboBoxAttachment> bypassAttachment, midiAttachment, syncAttachment;
    juce::Slider tempo;
    std::unique_ptr<APVTS::SliderAttachment> tempoAttachment;
    juce::TextButton resetSecondary { "RESET SECONDARY" }, clearGestures { "CLEAR GESTURES" };
    juce::Label calLabel, calStatus, bypassLabel, midiLabel, tempoLabel;
};

} // namespace warmful::ui
