// WARMFUL — pedal-style editor.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "PluginEditor.h"
#include "FactoryPresets.h"

namespace warmful
{

namespace
{
    struct MenuEntry
    {
        const char* name;
        std::array<bool, 4> buttons;   // which of A..D the pedal combo uses
    };
    constexpr std::array<MenuEntry, 5> kMenus { {
        { "SECONDARY", { true, true, false, false } },
        { "GESTURE", { false, false, true, true } },
        { "FX SETUP", { true, false, false, true } },
        { "COPY/SAVE", { false, true, true, false } },
        { "GLOBAL", { true, true, true, true } },
    } };

    constexpr std::array<const char*, kNumPrimary> kKnobLabels { "TILT", "AMOUNT", "RATE", "AMOUNT", "TIME", "AMOUNT", "MIX", "AMOUNT" };
    constexpr std::array<const char*, kNumPrimary> kSecondaryLabels { "SENSITIVITY", "EFFECT VOL", "DRIFT", "EFFECT VOL", "DRIFT", "EFFECT VOL", "OUTPUT LEVEL", "EFFECT VOL" };
}

// ---- PedalView --------------------------------------------------------------------------------

PedalView::PedalView (WarmfulProcessor& p) : processor (p)
{
    setLookAndFeel (&lnf);
    auto& state = processor.getState();

    for (int k = 0; k < kNumPrimary; ++k)
    {
        const int column = k / 2;
        const auto cap = k == kMix ? theme::mixKnob : theme::moduleColour[(size_t) column];
        knobs.push_back (std::make_unique<ui::Knob> (state, params::kPrimaryIds[(size_t) k], params::kSecondaryIds[(size_t) k],
                                                     kKnobLabels[(size_t) k], kSecondaryLabels[(size_t) k], cap));
        addAndMakeVisible (*knobs.back());
    }

    for (int m = 0; m < kNumModules; ++m)
    {
        std::array<juce::String, 5> names;
        for (size_t i = 0; i < 5; ++i)
            names[i] = params::kEffectNames[(size_t) m][i];
        auto button = std::make_unique<ui::ModuleButton> (m, params::kModuleNames[(size_t) m], names);
        button->onStep = [this, m] { processor.stepEffect (m); };
        button->onToggle = [this, m] { processor.toggleModule (m); };
        button->onPick = [this, m] (int fx)
        {
            processor.setParam (params::kEffectIds[(size_t) m], (float) fx);
            processor.setParam (params::kModuleOnIds[(size_t) m], 1.0f);
        };
        addAndMakeVisible (*button);
        moduleButtons.push_back (std::move (button));
    }

    tapSwitch.longPressMs = (int) (WarmfulProcessor::kHoldSeconds * 1000.0);
    tapSwitch.onPress = [this] { processor.tapFootswitchDown(); };
    tapSwitch.onRelease = [this] (bool) { processor.tapFootswitchUp(); };
    bypassSwitch.onLongPress = [this] { showPanel (std::make_unique<ui::PresetPanel> (processor, ui::PresetPanel::Mode::Browse)); };
    bypassSwitch.onRelease = [this] (bool wasLong) { if (! wasLong) bypassReleased(); };
    addAndMakeVisible (tapSwitch);
    addAndMakeVisible (bypassSwitch);
    addAndMakeVisible (leds);

    setSize (kWidth, kHeight);
    timerCallback();   // first paint already shows the current effects / LEDs
    startTimerHz (30);
}

PedalView::~PedalView()
{
    panel.reset();
    setLookAndFeel (nullptr);
}

void PedalView::resized()
{
    const int colW = 130, x0 = 20;
    for (int k = 0; k < kNumPrimary; ++k)
    {
        const int column = k / 2;
        const int y = (k % 2 == 0) ? 18 : 158;
        knobs[(size_t) k]->setBounds (x0 + column * colW + 15, y, 100, 132);
    }
    for (int m = 0; m < kNumModules; ++m)
        moduleButtons[(size_t) m]->setBounds (x0 + m * colW + 4, 296, colW - 8, 136);

    tapSwitch.setBounds (30, 556, 200, 96);
    bypassSwitch.setBounds (kWidth - 230, 556, 200, 96);
    leds.setBounds (kWidth / 2 - 38, 572, 76, 64);

    if (panel != nullptr)
        panel->setBounds (16, 14, kWidth - 32, 420);
}

juce::Rectangle<int> PedalView::menuRow (int index) const
{
    auto area = stripArea().withTrimmedLeft (stripArea().getWidth() - 200).reduced (10, 8);
    const int h = area.getHeight() / (int) kMenus.size();
    return area.withY (area.getY() + index * h).withHeight (h);
}

void PedalView::paint (juce::Graphics& g)
{
    // Enclosure.
    const auto body = getLocalBounds().toFloat().reduced (3.0f);
    g.fillAll (juce::Colour (0xff141312));
    g.setColour (theme::enclosure);
    g.fillRoundedRectangle (body, 14.0f);
    g.setColour (theme::enclosureEdge);
    g.drawRoundedRectangle (body, 14.0f, 2.0f);

    // Column frames like the printed panel.
    g.setColour (theme::panelLine.withAlpha (0.6f));
    for (int m = 0; m < kNumModules; ++m)
        g.drawRoundedRectangle (juce::Rectangle<float> (20.0f + (float) m * 130.0f + 4.0f, 12.0f, 122.0f, 282.0f), 8.0f, 1.0f);

    // Black strip with the logo and the button-combo legend.
    const auto strip = stripArea().toFloat();
    g.setColour (theme::strip);
    g.fillRoundedRectangle (strip, 6.0f);

    const juce::String logo = "WARMFUL";
    auto logoArea = strip.withWidth (300.0f).reduced (18.0f, 14.0f);
    g.setFont (theme::font (40.0f, true));
    float x = logoArea.getX();
    for (int i = 0; i < logo.length(); ++i)
    {
        const auto ch = logo.substring (i, i + 1);
        const auto colour = i < 6 ? theme::effectColour[(size_t) (i % 5)] : theme::moduleColour[3];
        g.setColour (colour);
        const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), ch) + 1.0f;
        g.drawText (ch, juce::Rectangle<float> (x, logoArea.getY(), w + 2.0f, 44.0f), juce::Justification::centredLeft);
        x += w;
    }
    g.setColour (theme::stripText.withAlpha (0.6f));
    g.setFont (theme::font (11.0f, true));
    g.drawText ("MULTI-EFFECTOR", logoArea.withTrimmedTop (46.0f), juce::Justification::topLeft);

    const bool gestureRec = processor.getEngine().gestureIsRecording();
    for (int i = 0; i < (int) kMenus.size(); ++i)
    {
        auto row = menuRow (i).toFloat();
        const bool active = (i == 0 && secondaryMode) || (i == 1 && gestureRec);
        g.setColour (active ? (i == 0 ? theme::secondaryTeal : theme::gestureWhite) : theme::stripText.withAlpha (0.8f));
        g.setFont (theme::font (10.5f, true));
        g.drawText (kMenus[(size_t) i].name, row.withTrimmedRight (70.0f), juce::Justification::centredRight);
        for (int b = 0; b < 4; ++b)
        {
            auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ row.getRight() - 56.0f + (float) b * 14.0f, row.getCentreY() });
            if (kMenus[(size_t) i].buttons[(size_t) b])
                g.fillEllipse (dot);
            else
                g.drawEllipse (dot, 1.0f);
        }
    }

    // Status line.
    g.setColour (theme::ink);
    g.setFont (theme::font (11.0f, true));
    g.drawText (statusText(), juce::Rectangle<int> (20, 664, kWidth - 40, 20), juce::Justification::centred);
    g.setColour (theme::inkSoft);
    g.setFont (theme::font (9.5f));
    g.drawText ("Module button: click = next effect, hold = OFF, right-click = list.   TAP: tap tempo / hold = CAPTURE.   BYPASS: hold = presets.",
                juce::Rectangle<int> (16, 686, kWidth - 32, 16), juce::Justification::centred);
}

juce::String PedalView::statusText() const
{
    auto& eng = const_cast<WarmfulProcessor&> (processor).getEngine();
    if (eng.isAutoCalibrating())
        return "CALIBRATING - play your instrument at your normal level";
    if (eng.gestureIsRecording())
        return "GESTURE RECORDING - move knobs, then click GESTURE or BYPASS to loop them";
    if (secondaryMode)
        return "SECONDARY - knobs edit the grey labels. Click SECONDARY or BYPASS to exit";

    const int preset = processor.getCurrentPreset();
    const int factory = processor.getCurrentFactoryPreset();
    const auto sync = (params::SyncMode) (int) processor.getParam (params::kSyncMode);
    const char* syncName = sync == params::SyncMode::Free ? "FREE" : (sync == params::SyncMode::Tap ? "TAP" : "HOST");
    const int cal = (int) processor.getParam (params::kCalibration);
    juce::String s;
    if (factory >= 0)
        s << "FACTORY " << factoryPresets()[(size_t) factory].name;
    else
        s << "PRESET " << (preset >= 0 ? PresetBank::slotName (preset) : juce::String ("--"));
    s << "   |   " << juce::String (processor.getParam (params::kTempo), 1) << " BPM " << syncName
      << "   |   CAL " << calibration::levelName (cal)
      << "   |   CAPTURE " << ((int) processor.getParam (params::kCaptureRouting) == 0 ? "POST" : "PRE");
    return s;
}

void PedalView::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) kMenus.size(); ++i)
    {
        if (! menuRow (i).contains (e.getPosition()))
            continue;
        if (e.mods.isPopupMenu())
        {
            // Long-press equivalents on the pedal: reset secondaries / delete all gestures.
            if (i == 0)
                for (size_t k = 0; k < params::kSecondaryIds.size(); ++k)
                    processor.setParam (params::kSecondaryIds[k], params::kSecondaryDefaults[k]);
            if (i == 1)
                processor.getEngine().gestureClear();
            return;
        }
        openMenu ((Menu) i);
        return;
    }
}

void PedalView::openMenu (Menu menu)
{
    switch (menu)
    {
        case Menu::Secondary:
            closePanel();
            setSecondaryMode (! secondaryMode);
            break;
        case Menu::Gesture:
            closePanel();
            if (processor.getEngine().gestureIsRecording())
                processor.getEngine().gestureEndRecording();
            else
            {
                setSecondaryMode (false);   // GESTURE records primary controls
                processor.getEngine().gestureBeginRecording();
            }
            break;
        case Menu::FxSetup:  showPanel (std::make_unique<ui::FxSetupPanel> (processor)); break;
        case Menu::CopySave: showPanel (std::make_unique<ui::PresetPanel> (processor, ui::PresetPanel::Mode::Save)); break;
        case Menu::Global:   showPanel (std::make_unique<ui::GlobalPanel> (processor)); break;
    }
    repaint();
}

void PedalView::showPanel (std::unique_ptr<ui::Panel> p)
{
    panel = std::move (p);
    panel->onClose = [this] { juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<PedalView> (this)]
                                                               { if (safe != nullptr) safe->closePanel(); }); };
    addAndMakeVisible (*panel);
    resized();
}

void PedalView::closePanel()
{
    panel.reset();
    repaint();
}

void PedalView::setSecondaryMode (bool secondary)
{
    secondaryMode = secondary;
    for (auto& k : knobs)
        k->setSecondaryMode (secondary);
    repaint();
}

void PedalView::bypassReleased()
{
    // In menus the BYPASS footswitch exits instead of bypassing (as on the pedal).
    if (panel != nullptr) { closePanel(); return; }
    if (processor.getEngine().gestureIsRecording()) { processor.getEngine().gestureEndRecording(); return; }
    if (secondaryMode) { setSecondaryMode (false); return; }
    processor.bypassFootswitchPressed();
}

void PedalView::timerCallback()
{
    auto& eng = processor.getEngine();
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    blink = std::fmod (nowMs, 700.0) < 350.0;

    // Knob markers show GESTURE / expression playback values.
    const int expTarget = (int) processor.getParam (params::kExpTarget) - 1;
    for (int k = 0; k < kNumPrimary; ++k)
    {
        const bool lane = eng.gestureHasLane (k), rec = eng.gestureIsRecordingKnob (k);
        const bool show = ! secondaryMode && (lane || rec || expTarget == k);
        knobs[(size_t) k]->setMarker (show ? eng.getEffectivePrimary (k) : -1.0f, rec);
    }

    for (int m = 0; m < kNumModules; ++m)
    {
        const int fx = (int) processor.getParam (params::kEffectIds[(size_t) m]);
        const bool on = processor.getParam (params::kModuleOnIds[(size_t) m]) >= 0.5f;
        const bool lane = eng.gestureHasLane (2 * m) || eng.gestureHasLane (2 * m + 1);
        const bool rec = eng.gestureIsRecordingKnob (2 * m) || eng.gestureIsRecordingKnob (2 * m + 1);
        moduleButtons[(size_t) m]->setState (fx, on, lane, rec, blink);
    }

    // TAP LED: CAPTURE state, otherwise the tempo blink.
    switch (eng.getCaptureMode())
    {
        case Capture::Mode::Recording: tapSwitch.setLed (theme::recordRed); break;
        case Capture::Mode::Sustain:   tapSwitch.setLed (theme::effectColour[3]); break;
        case Capture::Mode::Loop:      tapSwitch.setLed (theme::effectColour[2]); break;
        case Capture::Mode::Off:
        {
            const double beatMs = 60000.0 / std::max (30.0, (double) processor.getParam (params::kTempo));
            tapSwitch.setLed (std::fmod (nowMs, beatMs) < 90.0 ? theme::recordRed : juce::Colours::transparentBlack);
            break;
        }
    }

    // BYPASS LED: red = engaged, blinking yellow = DUAL BYPASS group off, dark = bypassed.
    const bool bypassed = processor.getParam (params::kBypass) >= 0.5f;
    const bool dual = processor.getParam (params::kDualActive) >= 0.5f;
    bypassSwitch.setLed (bypassed ? juce::Colours::transparentBlack
                                  : (dual ? (blink ? juce::Colour (0xfff4c430) : juce::Colours::transparentBlack) : theme::recordRed));

    // Indicator bars: level meter, coloured by the active mode.
    juce::Colour colour = juce::Colour (0xfff2efe8);
    if (eng.isAutoCalibrating()) colour = juce::Colour (0xffa86de0);
    else if (eng.gestureIsRecording()) colour = theme::gestureWhite;
    else if (secondaryMode) colour = theme::secondaryTeal;
    else if (eng.getCaptureMode() == Capture::Mode::Recording) colour = theme::recordRed;
    const float level = juce::jlimit (0.0f, 1.0f, (gainToDb (eng.getChainPeak()) + 36.0f) / 36.0f);
    std::array<float, 4> bars {};
    for (int i = 0; i < 4; ++i)
        bars[(size_t) i] = juce::jlimit (0.0f, 1.0f, level * 4.0f - (float) i);
    leds.setLevels (bars, colour);

    repaint (0, 436, kWidth, 270);   // strip legend + status line
}

// ---- WarmfulEditor ----------------------------------------------------------------------------

WarmfulEditor::WarmfulEditor (WarmfulProcessor& p) : AudioProcessorEditor (p), view (p)
{
    addAndMakeVisible (view);
    setResizable (true, true);
    setResizeLimits (PedalView::kWidth * 2 / 3, PedalView::kHeight * 2 / 3, PedalView::kWidth * 2, PedalView::kHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) PedalView::kWidth / (double) PedalView::kHeight);
    setSize (PedalView::kWidth, PedalView::kHeight);
}

void WarmfulEditor::resized()
{
    const float scale = (float) getWidth() / (float) PedalView::kWidth;
    view.setTransform (juce::AffineTransform::scale (scale));
    view.setBounds (0, 0, PedalView::kWidth, PedalView::kHeight);
}

} // namespace warmful
