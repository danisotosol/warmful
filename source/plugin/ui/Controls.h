// WARMFUL — pedal-style controls: knobs, module buttons, footswitches, LED bars.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

namespace warmful::ui
{

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return theme::font (12.0f, true); }
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return theme::font (12.0f, true); }
};

/** One pot with its bold PRIMARY label and grey SECONDARY label; re-attaches to the
    secondary parameter while the SECONDARY menu is active. */
class Knob final : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const char* primaryId, const char* secondaryId,
          juce::String primaryLabel, juce::String secondaryLabel, juce::Colour cap);

    void setSecondaryMode (bool secondary);
    /** Effective value from GESTURE/expression (drawn as a marker), or < 0 for none. */
    void setMarker (float value, bool recording);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& state;
    const char* primaryId;
    const char* secondaryId;
    juce::String primaryLabel, secondaryLabel;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::Colour primaryRing;
    bool secondaryMode = false;
};

/** Module button (A–D) with its effect list. Click: next effect. Hold: module OFF/ON.
    Right click: pick an effect directly. */
class ModuleButton final : public juce::Component, private juce::Timer
{
public:
    ModuleButton (int moduleIndex, juce::String moduleName, std::array<juce::String, 5> effectNames);

    std::function<void()> onStep, onToggle;
    std::function<void (int)> onPick;

    void setState (int effect, bool on, bool gestureLane, bool recording, bool blink);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> buttonBounds() const;

    int module;
    juce::String name;
    std::array<juce::String, 5> effects;
    int effect = 0;
    bool on = true, lane = false, rec = false, blinkPhase = false, held = false;
};

/** Round metal footswitch with an indicator LED. */
class Footswitch final : public juce::Component, private juce::Timer
{
public:
    Footswitch (juce::String title, juce::String subtitle, bool ledOnLeft);

    void setLed (juce::Colour colour) { if (led != colour) { led = colour; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    std::function<void()> onPress, onLongPress;
    std::function<void (bool wasLongPress)> onRelease;
    int longPressMs = 600;

private:
    void timerCallback() override;
    juce::String title, subtitle;
    bool ledLeft;
    bool down = false, longFired = false;
    juce::Colour led = juce::Colours::transparentBlack;
};

/** The four vertical indicator bars between the footswitches. */
class LedBars final : public juce::Component
{
public:
    void setLevels (std::array<float, 4> levels, juce::Colour colour);
    void paint (juce::Graphics&) override;

private:
    std::array<float, 4> values {};
    juce::Colour colour = theme::recordRed;
};

} // namespace warmful::ui
