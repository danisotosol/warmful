// WARMFUL — pedal-style controls.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Controls.h"

namespace warmful::ui
{

namespace
{
    constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
    constexpr float kEndAngle = juce::MathConstants<float>::pi * 2.8f;

    juce::Point<float> onCircle (juce::Point<float> c, float r, float angle)
    {
        return { c.x + r * std::sin (angle), c.y - r * std::cos (angle) };
    }

    juce::Colour colourProperty (const juce::Component& c, const char* key, juce::Colour fallback)
    {
        const auto v = c.getProperties()[key];
        return v.isVoid() ? fallback : juce::Colour ((juce::uint32) (juce::int64) v);
    }
}

// ---- LookAndFeel ------------------------------------------------------------------------------

LookAndFeel::LookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, theme::strip);
    setColour (juce::PopupMenu::textColourId, theme::stripText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::secondaryTeal.withAlpha (0.35f));
    setColour (juce::TextButton::buttonColourId, theme::strip);
    setColour (juce::TextButton::buttonOnColourId, theme::secondaryTeal);
    setColour (juce::TextButton::textColourOffId, theme::stripText);
    setColour (juce::TextButton::textColourOnId, theme::strip);
    setColour (juce::ComboBox::textColourId, theme::stripText);
    setColour (juce::ComboBox::arrowColourId, theme::stripText);
    setColour (juce::BubbleComponent::backgroundColourId, theme::strip);
    setColour (juce::BubbleComponent::outlineColourId, theme::strip);
    setColour (juce::TooltipWindow::textColourId, theme::stripText);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float startAngle, float endAngle, juce::Slider& slider)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float outer = std::min (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
    const auto c = area.getCentre();
    const float capR = outer * 0.66f;
    const auto cap = colourProperty (slider, "cap", juce::Colours::grey);
    const auto ring = colourProperty (slider, "ring", cap);
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Printed scale ticks.
    g.setColour (theme::inkSoft);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        g.drawLine ({ onCircle (c, outer * 0.9f, a), onCircle (c, outer, a) }, i == 5 ? 1.6f : 1.0f);
    }

    // Value arc.
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, outer * 0.8f, outer * 0.8f, 0.0f, startAngle, angle, true);
    g.setColour (ring.withAlpha (0.9f));
    g.strokePath (arc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // GESTURE / expression playback marker.
    const float marker = slider.getProperties().getWithDefault ("marker", -1.0f);
    if (marker >= 0.0f)
    {
        const bool rec = slider.getProperties().getWithDefault ("markerRec", false);
        const auto p = onCircle (c, outer * 0.8f, startAngle + marker * (endAngle - startAngle));
        g.setColour (rec ? theme::recordRed : theme::gestureWhite);
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (p));
        g.setColour (theme::ink);
        g.drawEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (p), 1.0f);
    }

    // Knob body.
    const auto body = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (body.translated (0.0f, 2.5f).expanded (1.0f));
    juce::ColourGradient grad (cap.brighter (0.45f), c.x - capR * 0.45f, c.y - capR * 0.55f,
                               cap.darker (0.35f), c.x + capR * 0.6f, c.y + capR * 0.8f, true);
    g.setGradientFill (grad);
    g.fillEllipse (body);
    g.setColour (cap.darker (0.55f));
    g.drawEllipse (body, 1.2f);

    // Pointer.
    g.setColour (theme::ink.withAlpha (0.85f));
    g.drawLine ({ onCircle (c, capR * 0.25f, angle), onCircle (c, capR * 0.9f, angle) }, 3.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto fill = b.findColour (b.getToggleState() ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);
    if (down) fill = fill.brighter (0.25f);
    else if (over) fill = fill.brighter (0.1f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (theme::stripText.withAlpha (0.25f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    g.setColour (theme::strip);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (theme::stripText.withAlpha (0.25f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (theme::stripText);
    g.fillPath (arrow);
}

// ---- Knob -------------------------------------------------------------------------------------

Knob::Knob (juce::AudioProcessorValueTreeState& s, const char* pid, const char* sid,
            juce::String pl, juce::String sl, juce::Colour cap)
    : state (s), primaryId (pid), secondaryId (sid), primaryLabel (std::move (pl)), secondaryLabel (std::move (sl))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    slider.setPopupDisplayEnabled (true, true, nullptr, 900);
    slider.getProperties().set ("cap", (juce::int64) cap.getARGB());
    // A white cap (MIX) would leave its value ring invisible on the cream enclosure.
    primaryRing = cap.getPerceivedBrightness() > 0.85f ? theme::inkSoft : cap;
    slider.getProperties().set ("ring", (juce::int64) primaryRing.getARGB());
    addAndMakeVisible (slider);
    secondaryMode = true;
    setSecondaryMode (false);
}

void Knob::setSecondaryMode (bool secondary)
{
    if (secondary == secondaryMode && attachment != nullptr)
        return;
    secondaryMode = secondary;
    const char* id = secondary ? secondaryId : primaryId;
    attachment.reset();
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
    if (auto* p = state.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.getProperties().set ("ring", (juce::int64) (secondary ? theme::secondaryTeal : primaryRing).getARGB());
    repaint();
}

void Knob::setMarker (float value, bool recording)
{
    auto& props = slider.getProperties();
    const float old = props.getWithDefault ("marker", -1.0f);
    const bool oldRec = props.getWithDefault ("markerRec", false);
    if (std::abs (old - value) < 0.002f && oldRec == recording)
        return;
    props.set ("marker", value);
    props.set ("markerRec", recording);
    slider.repaint();
}

void Knob::resized()
{
    auto r = getLocalBounds();
    slider.setBounds (r.removeFromTop (r.getWidth()).reduced (4));
}

void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    r.removeFromTop (r.getWidth() - 2);
    g.setFont (theme::font (12.5f, true));
    g.setColour (secondaryMode ? theme::inkSoft : theme::ink);
    g.drawText (primaryLabel, r.removeFromTop (15), juce::Justification::centred);
    g.setFont (theme::font (10.0f, secondaryMode));
    g.setColour (secondaryMode ? theme::secondaryTeal.darker (0.3f) : theme::inkSoft);
    g.drawText (secondaryLabel, r.removeFromTop (13), juce::Justification::centred);
}

// ---- ModuleButton -----------------------------------------------------------------------------

ModuleButton::ModuleButton (int moduleIndex, juce::String moduleName, std::array<juce::String, 5> effectNames)
    : module (moduleIndex), name (std::move (moduleName)), effects (std::move (effectNames))
{
}

void ModuleButton::setState (int fx, bool isOn, bool gestureLane, bool recording, bool blink)
{
    if (fx == effect && isOn == on && gestureLane == lane && recording == rec && blink == blinkPhase)
        return;
    effect = fx;
    on = isOn;
    lane = gestureLane;
    rec = recording;
    blinkPhase = blink;
    repaint();
}

juce::Rectangle<float> ModuleButton::buttonBounds() const
{
    return juce::Rectangle<float> (30.0f, 30.0f).withCentre ({ (float) getWidth() * 0.5f, 17.0f });
}

void ModuleButton::paint (juce::Graphics& g)
{
    // Button with its effect LED colour.
    const auto b = buttonBounds();
    auto led = on ? theme::effectColour[(size_t) effect] : juce::Colour (0xff6d6860);
    if (rec)
        led = theme::recordRed;
    else if (lane && blinkPhase)
        led = theme::gestureWhite;   // "LEDs oscillate between the effect colour and white"

    if (on || rec)
    {
        g.setColour (led.withAlpha (0.35f));
        g.fillEllipse (b.expanded (4.0f));
    }
    g.setColour (theme::ink);
    g.fillEllipse (b);
    g.setColour (led);
    g.fillEllipse (b.reduced (3.5f));
    g.setColour (theme::ink);
    g.setFont (theme::font (14.0f, true));
    g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + module)), b, juce::Justification::centred);

    // Module panel with the effect list.
    auto panel = getLocalBounds().toFloat().withTrimmedTop (38.0f).reduced (2.0f, 0.0f);
    g.setColour (theme::enclosure.brighter (0.04f));
    g.fillRoundedRectangle (panel, 6.0f);
    g.setColour (theme::panelLine);
    g.drawRoundedRectangle (panel, 6.0f, 1.0f);

    auto rows = panel.reduced (8.0f, 6.0f);
    g.setColour (theme::ink);
    g.setFont (theme::font (11.5f, true));
    g.drawText (name, rows.removeFromTop (17.0f), juce::Justification::centred);
    g.setColour (theme::panelLine);
    g.drawHorizontalLine ((int) rows.getY(), rows.getX(), rows.getRight());
    rows.removeFromTop (3.0f);

    for (int i = 0; i < 5; ++i)
    {
        auto row = rows.removeFromTop (14.0f);
        const bool selected = i == effect;
        g.setColour (theme::effectColour[(size_t) i].withAlpha (selected && on ? 1.0f : 0.55f));
        g.fillRect (row.removeFromLeft (9.0f).withSizeKeepingCentre (7.0f, 7.0f));
        row.removeFromLeft (5.0f);
        g.setColour (selected ? (on ? theme::ink : theme::inkSoft) : theme::inkSoft.withAlpha (0.8f));
        g.setFont (theme::font (9.5f, selected));
        g.drawText (effects[(size_t) i] + (selected && ! on ? "  (OFF)" : ""), row, juce::Justification::centredLeft);
    }
}

void ModuleButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        for (int i = 0; i < 5; ++i)
            menu.addItem (i + 1, effects[(size_t) i], true, i == effect);
        menu.addSeparator();
        menu.addItem (100, on ? "Bypass module (hold button)" : "Engage module");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
        {
            if (r >= 1 && r <= 5 && onPick) onPick (r - 1);
            if (r == 100 && onToggle) onToggle();
        });
        return;
    }

    if (buttonBounds().expanded (6.0f).contains (e.position))
    {
        held = false;
        startTimer (600);   // "Press and hold a button to bypass the selected effect."
        return;
    }

    // Clicking an effect name selects it directly.
    const float listTop = 38.0f + 6.0f + 17.0f + 3.0f;
    const int row = (int) ((e.position.y - listTop) / 14.0f);
    if (e.position.y >= listTop && row >= 0 && row < 5 && onPick)
        onPick (row);
}

void ModuleButton::mouseUp (const juce::MouseEvent& e)
{
    if (! isTimerRunning() && ! held)
        return;
    stopTimer();
    if (! held && buttonBounds().expanded (6.0f).contains (e.position) && onStep)
        onStep();
    held = false;
}

void ModuleButton::timerCallback()
{
    stopTimer();
    held = true;
    if (onToggle)
        onToggle();
}

// ---- Footswitch -------------------------------------------------------------------------------

Footswitch::Footswitch (juce::String t, juce::String s, bool ledOnLeft)
    : title (std::move (t)), subtitle (std::move (s)), ledLeft (ledOnLeft)
{
}

void Footswitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float d = std::min (r.getHeight(), r.getWidth() * 0.5f) - 6.0f;
    const auto sw = juce::Rectangle<float> (d, d).withCentre ({ ledLeft ? r.getRight() - d * 0.5f - 6.0f : r.getX() + d * 0.5f + 6.0f,
                                                                r.getCentreY() });

    // Hex nut and plunger.
    g.setColour (juce::Colour (0xff9c9a95));
    g.fillEllipse (sw);
    g.setColour (juce::Colour (0xff5e5c58));
    g.drawEllipse (sw, 1.5f);
    const auto cap = sw.reduced (d * 0.2f).translated (0.0f, down ? 1.5f : 0.0f);
    juce::ColourGradient grad (juce::Colour (0xfff4f3f0), cap.getX(), cap.getY(), juce::Colour (0xff8f8d88),
                               cap.getRight(), cap.getBottom(), false);
    g.setGradientFill (grad);
    g.fillEllipse (cap);
    g.setColour (juce::Colour (0xff4a4844));
    g.drawEllipse (cap, 1.0f);

    // LED and labels.
    auto text = ledLeft ? r.withRight (sw.getX() - 8.0f) : r.withLeft (sw.getRight() + 8.0f);
    const auto just = ledLeft ? juce::Justification::centredRight : juce::Justification::centredLeft;
    auto ledArea = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ ledLeft ? text.getRight() - 4.5f : text.getX() + 4.5f,
                                                                     text.getCentreY() - 8.0f });
    g.setColour (theme::ink);
    g.fillEllipse (ledArea.expanded (1.5f));
    g.setColour (led.isTransparent() ? juce::Colour (0xff3b3835) : led);
    g.fillEllipse (ledArea);
    if (! led.isTransparent())
    {
        g.setColour (led.withAlpha (0.3f));
        g.fillEllipse (ledArea.expanded (4.0f));
    }

    auto labels = ledLeft ? text.withTrimmedRight (16.0f) : text.withTrimmedLeft (16.0f);
    g.setColour (theme::ink);
    g.setFont (theme::font (12.0f, true));
    g.drawText (title, labels.withHeight (16.0f).withCentre ({ labels.getCentreX(), labels.getCentreY() - 8.0f }), just);
    g.setColour (ledLeft ? theme::recordRed : theme::ink);
    g.drawText (subtitle, labels.withHeight (16.0f).withCentre ({ labels.getCentreX(), labels.getCentreY() + 8.0f }), just);
}

void Footswitch::mouseDown (const juce::MouseEvent&)
{
    down = true;
    longFired = false;
    repaint();
    startTimer (longPressMs);
    if (onPress)
        onPress();
}

void Footswitch::mouseUp (const juce::MouseEvent&)
{
    stopTimer();
    down = false;
    repaint();
    if (onRelease)
        onRelease (longFired);
}

void Footswitch::timerCallback()
{
    stopTimer();
    longFired = true;
    if (onLongPress)
        onLongPress();
}

// ---- LedBars ----------------------------------------------------------------------------------

void LedBars::setLevels (std::array<float, 4> levels, juce::Colour c)
{
    bool changed = c != colour;
    for (size_t i = 0; i < 4; ++i)
        changed |= std::abs (levels[i] - values[i]) > 0.01f;
    if (! changed)
        return;
    values = levels;
    colour = c;
    repaint();
}

void LedBars::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const float w = r.getWidth() / 4.0f;
    for (size_t i = 0; i < 4; ++i)
    {
        auto bar = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (3.0f, 0.0f);
        g.setColour (theme::ink);
        g.fillRoundedRectangle (bar, 2.0f);
        auto inner = bar.reduced (2.0f);
        g.setColour (juce::Colour (0xff3b3835));
        g.fillRoundedRectangle (inner, 1.5f);
        const float v = juce::jlimit (0.0f, 1.0f, values[i]);
        if (v > 0.0f)
        {
            g.setColour (colour);
            g.fillRoundedRectangle (inner.withTop (inner.getBottom() - inner.getHeight() * v), 1.5f);
        }
    }
}

} // namespace warmful::ui
