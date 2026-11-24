// WARMFUL — overlay menus.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Panels.h"
#include "../PluginProcessor.h"
#include "FactoryPresets.h"

namespace warmful::ui
{

namespace
{
    constexpr const char* kShort[] { "CHAR", "MOVE", "DIFF", "TEXT" };
}

// ---- Panel ------------------------------------------------------------------------------------

Panel::Panel (juce::String t) : title (std::move (t))
{
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);
}

void Panel::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 10.0f);
    g.setColour (theme::strip);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (theme::stripText);
    g.setFont (theme::font (15.0f, true));
    g.drawText (title, getLocalBounds().reduced (18).removeFromTop (26), juce::Justification::centredLeft);
}

void Panel::resized()
{
    close.setBounds (getWidth() - 46, 14, 28, 24);
}

void Panel::styleLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setFont (theme::font (11.0f, true));
    l.setColour (juce::Label::textColourId, theme::stripText.withAlpha (0.7f));
}

// ---- FX SETUP ---------------------------------------------------------------------------------

FxSetupPanel::FxSetupPanel (WarmfulProcessor& p) : Panel ("FX SETUP"), processor (p)
{
    auto& state = processor.getState();

    styleLabel (routingLabel, "MODULE ROUTING - tap the modules in the order you want");
    styleLabel (dualLabel, "DUAL BYPASS - lit modules are switched by the BYPASS footswitch");
    styleLabel (captureLabel, "CAPTURE ROUTING");
    styleLabel (filterLabel, "FILTER STYLE");
    styleLabel (expLabel, "EXPRESSION");
    for (auto* l : { &routingLabel, &dualLabel, &captureLabel, &filterLabel, &expLabel })
        addAndMakeVisible (*l);

    for (int m = 0; m < 4; ++m)
    {
        auto& rb = routeButtons[(size_t) m];
        rb.setButtonText (kShort[m]);
        rb.onClick = [this, m] { pressRoutingButton (m); };
        addAndMakeVisible (rb);

        auto& db = dualButtons[(size_t) m];
        db.setButtonText (kShort[m]);
        db.setClickingTogglesState (true);
        db.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xfff4c430));
        dualAttachments[(size_t) m] = std::make_unique<APVTS::ButtonAttachment> (state, params::kDualMaskIds[(size_t) m], db);
        addAndMakeVisible (db);
    }

    captureRouting.addItemList ({ "Post-FX", "Pre-FX" }, 1);
    filterStyle.addItemList ({ "Low-Pass", "Tilt", "High-Pass" }, 1);
    expTarget.addItemList ({ "None", "Tilt", "Character Amount", "Rate", "Movement Amount", "Time",
                             "Diffusion Amount", "Mix", "Texture Amount" }, 1);
    captureAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, params::kCaptureRouting, captureRouting);
    filterAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, params::kFilterStyle, filterStyle);
    expAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, params::kExpTarget, expTarget);
    for (auto* c : { &captureRouting, &filterStyle, &expTarget })
        addAndMakeVisible (*c);

    expression.setSliderStyle (juce::Slider::LinearHorizontal);
    expression.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    expression.setColour (juce::Slider::trackColourId, theme::secondaryTeal);
    expressionAttachment = std::make_unique<APVTS::SliderAttachment> (state, params::kExpression, expression);
    addAndMakeVisible (expression);

    reset.onClick = [this] { resetSetup(); };
    addAndMakeVisible (reset);

    state.addParameterListener (params::kOrder, this);
}

FxSetupPanel::~FxSetupPanel()
{
    processor.getState().removeParameterListener (params::kOrder, this);
}

void FxSetupPanel::parameterChanged (const juce::String&, float)
{
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<FxSetupPanel> (this)]
    {
        if (safe != nullptr) safe->repaint();
    });
}

void FxSetupPanel::pressRoutingButton (int module)
{
    if (std::find (pendingOrder.begin(), pendingOrder.end(), module) != pendingOrder.end())
        return;
    pendingOrder.push_back (module);
    routeButtons[(size_t) module].setToggleState (true, juce::dontSendNotification);
    if (pendingOrder.size() == 4)
    {
        std::array<int, kNumModules> order {};
        std::copy (pendingOrder.begin(), pendingOrder.end(), order.begin());
        processor.setParam (params::kOrder, (float) params::orderIndexOf (order));
        pendingOrder.clear();
        for (auto& b : routeButtons)
            b.setToggleState (false, juce::dontSendNotification);
    }
    repaint();
}

void FxSetupPanel::resetSetup()
{
    // "press and hold A and D ... all FX SETUP parameters have been reset"
    for (const char* id : { params::kOrder, params::kCaptureRouting, params::kFilterStyle, params::kExpTarget,
                            params::kDualMaskIds[0], params::kDualMaskIds[1], params::kDualMaskIds[2], params::kDualMaskIds[3] })
        if (auto* prm = processor.getState().getParameter (id))
            processor.setParam (id, prm->convertFrom0to1 (prm->getDefaultValue()));
    pendingOrder.clear();
    for (auto& b : routeButtons)
        b.setToggleState (false, juce::dontSendNotification);
    repaint();
}

void FxSetupPanel::paint (juce::Graphics& g)
{
    Panel::paint (g);

    // Current routing, drawn as a chain of chips under the routing buttons.
    const auto& order = params::moduleOrders()[(size_t) juce::jlimit (0, 23, (int) processor.getParam (params::kOrder))];
    auto row = routeButtons[0].getBounds().withRight (routeButtons[3].getRight()).translated (0, 30).toFloat();
    const float w = row.getWidth() / 4.0f;
    g.setFont (theme::font (11.0f, true));
    for (int i = 0; i < 4; ++i)
    {
        auto chip = row.withX (row.getX() + w * (float) i).withWidth (w).reduced (10.0f, 2.0f);
        g.setColour (theme::moduleColour[(size_t) order[(size_t) i]]);
        g.fillRoundedRectangle (chip, 4.0f);
        g.setColour (theme::ink);
        g.drawText (juce::String (i + 1) + ". " + kShort[order[(size_t) i]], chip, juce::Justification::centred);
        if (i < 3)
        {
            g.setColour (theme::stripText);
            g.drawText (">", juce::Rectangle<float> (chip.getRight(), chip.getY(), 20.0f, chip.getHeight()), juce::Justification::centred);
        }
    }
    if (! pendingOrder.empty())
    {
        g.setColour (theme::secondaryTeal);
        juce::String seq;
        for (int m : pendingOrder)
            seq << kShort[m] << " > ";
        g.drawText (seq + "...", row.translated (0.0f, 22.0f), juce::Justification::centredLeft);
    }
}

void FxSetupPanel::resized()
{
    Panel::resized();
    auto r = content();
    routingLabel.setBounds (r.removeFromTop (18));
    auto buttons = r.removeFromTop (28);
    const int w = buttons.getWidth() / 4;
    for (auto& b : routeButtons)
        b.setBounds (buttons.removeFromLeft (w).reduced (4, 0));
    r.removeFromTop (58);

    dualLabel.setBounds (r.removeFromTop (18));
    auto duals = r.removeFromTop (28);
    for (auto& b : dualButtons)
        b.setBounds (duals.removeFromLeft (w).reduced (4, 0));
    r.removeFromTop (14);

    auto row = r.removeFromTop (46);
    auto left = row.removeFromLeft (row.getWidth() / 2).reduced (4, 0);
    auto right = row.reduced (4, 0);
    captureLabel.setBounds (left.removeFromTop (18));
    captureRouting.setBounds (left.removeFromTop (26));
    filterLabel.setBounds (right.removeFromTop (18));
    filterStyle.setBounds (right.removeFromTop (26));
    r.removeFromTop (10);

    auto exp = r.removeFromTop (46).reduced (4, 0);
    expLabel.setBounds (exp.removeFromTop (18));
    expTarget.setBounds (exp.removeFromLeft (exp.getWidth() / 2 - 4).withHeight (26));
    expression.setBounds (exp.withTrimmedLeft (8).withHeight (26));
    r.removeFromTop (10);
    reset.setBounds (r.removeFromTop (28).withSizeKeepingCentre (160, 28));
}

// ---- PRESETS ----------------------------------------------------------------------------------

PresetPanel::PresetPanel (WarmfulProcessor& p, Mode m)
    : Panel (m == Mode::Browse ? "PRESET BROWSER" : "COPY / SAVE"), processor (p), mode (m)
{
    const int current = processor.getCurrentPreset();
    bank = current >= 0 ? current / PresetBank::kSlotsPerBank : 0;
    // The browser opens on FACTORY unless a user preset is loaded (fresh installs have empty banks).
    if (mode == Mode::Browse && current < 0)
        bank = kFactoryBank;
    selected = mode == Mode::Save ? current : -1;

    for (int b = 0; b < (int) bankButtons.size(); ++b)
    {
        auto& bb = bankButtons[(size_t) b];
        bb.setButtonText (b == kFactoryBank ? juce::String ("FACTORY") : juce::String ("BANK ") + (char) ('A' + b));
        bb.setRadioGroupId (1);
        bb.setClickingTogglesState (true);
        bb.onClick = [this, b] { bank = b; refresh(); resized(); };
        // Factory sounds are read-only: they are browsed here and saved into a user slot via COPY/SAVE.
        bb.setVisible (b != kFactoryBank || mode == Mode::Browse);
        addChildComponent (bb);
    }
    for (int s = 0; s < 20; ++s)
    {
        slotButtons[(size_t) s].onClick = [this, s] { slotClicked (s); };
        addAndMakeVisible (slotButtons[(size_t) s]);
    }

    hint.setFont (theme::font (11.0f));
    hint.setColour (juce::Label::textColourId, theme::stripText.withAlpha (0.7f));
    hint.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (hint);

    save.setEnabled (mode == Mode::Save);
    save.onClick = [this]
    {
        if (selected >= 0)
        {
            processor.savePreset (selected);
            refresh();
            if (onClose) onClose();
        }
    };
    addChildComponent (save);
    save.setVisible (mode == Mode::Save);
    refresh();
}

void PresetPanel::slotClicked (int slotInBank)
{
    if (bank == kFactoryBank)
    {
        processor.loadFactoryPreset (slotInBank);
        refresh();
        return;
    }
    const int slot = bank * PresetBank::kSlotsPerBank + slotInBank;
    if (mode == Mode::Browse)
        processor.loadPreset (slot);   // presets are auditioned while browsing
    else
        selected = slot;
    refresh();
}

void PresetPanel::refresh()
{
    auto& presets = processor.getPresetBank();
    presets.reload();
    for (int b = 0; b < (int) bankButtons.size(); ++b)
        bankButtons[(size_t) b].setToggleState (b == bank, juce::dontSendNotification);

    if (bank == kFactoryBank)
    {
        const auto factory = factoryPresets();
        const int active = processor.getCurrentFactoryPreset();
        for (int s = 0; s < (int) slotButtons.size(); ++s)
        {
            auto& btn = slotButtons[(size_t) s];
            const bool used = s < (int) factory.size();
            btn.setVisible (used);
            if (! used)
                continue;
            btn.setButtonText (factory[(size_t) s].name);
            btn.setTooltip (factory[(size_t) s].description);
            btn.setToggleState (s == active, juce::dontSendNotification);
            btn.setColour (juce::TextButton::textColourOffId, theme::stripText);
            btn.setEnabled (true);
        }
        hint.setText (active >= 0 ? juce::String (factory[(size_t) active].description)
                                  : juce::String ("Factory sounds (MEDIUM calibration). Load one, tweak it, then COPY/SAVE it to a user slot."),
                      juce::dontSendNotification);
        return;
    }

    const int active = mode == Mode::Save ? selected : processor.getCurrentPreset();
    for (int s = 0; s < 20; ++s)
    {
        const int slot = bank * PresetBank::kSlotsPerBank + s;
        auto& btn = slotButtons[(size_t) s];
        btn.setVisible (true);
        btn.setTooltip ({});
        btn.setButtonText (PresetBank::slotName (slot) + (presets.has (slot) ? "" : " -"));
        btn.setToggleState (slot == active, juce::dontSendNotification);
        btn.setColour (juce::TextButton::textColourOffId,
                       presets.has (slot) ? theme::stripText : theme::stripText.withAlpha (0.35f));
        btn.setEnabled (mode == Mode::Save || presets.has (slot));
    }
    hint.setText (mode == Mode::Browse ? "Click a preset to load it (MIDI Program Change 0-79 also loads)."
                                       : "Pick a slot, then SAVE. Saves effects, routing, knobs, secondary, gestures, tempo.",
                  juce::dontSendNotification);
    save.setButtonText (selected >= 0 ? "SAVE TO " + PresetBank::slotName (selected) : "SAVE");
    save.setEnabled (selected >= 0);
}

void PresetPanel::resized()
{
    Panel::resized();
    auto r = content();
    auto banks = r.removeFromTop (28);
    const int visibleBanks = mode == Mode::Browse ? (int) bankButtons.size() : 4;
    const int bw = banks.getWidth() / visibleBanks;
    for (int b = 0; b < visibleBanks; ++b)
        bankButtons[(size_t) b].setBounds (banks.removeFromLeft (bw).reduced (3, 0));
    r.removeFromTop (10);

    auto grid = r.removeFromTop (4 * 34);
    if (bank == kFactoryBank)
    {
        // Two wide columns: factory names are longer than slot numbers.
        const int rows = 5, rh = grid.getHeight() / rows, cw = grid.getWidth() / 2;
        for (int s = 0; s < (int) slotButtons.size(); ++s)
            slotButtons[(size_t) s].setBounds (juce::Rectangle<int> (grid.getX() + (s / rows) * cw, grid.getY() + (s % rows) * rh, cw, rh).reduced (3, 2));
    }
    else
    {
        const int cw = grid.getWidth() / 5;
        for (int s = 0; s < 20; ++s)
            slotButtons[(size_t) s].setBounds (juce::Rectangle<int> (grid.getX() + (s % 5) * cw, grid.getY() + (s / 5) * 34, cw, 34).reduced (3));
    }
    r.removeFromTop (8);
    hint.setBounds (r.removeFromTop (48));
    save.setBounds (r.removeFromTop (30).withSizeKeepingCentre (180, 30));
}

// ---- GLOBAL -----------------------------------------------------------------------------------

GlobalPanel::GlobalPanel (WarmfulProcessor& p) : Panel ("GLOBAL SETTINGS"), processor (p)
{
    auto& state = processor.getState();
    styleLabel (calLabel, "CALIBRATION (input headroom)");
    styleLabel (bypassLabel, "BYPASS MODE");
    styleLabel (midiLabel, "MIDI CHANNEL");
    styleLabel (tempoLabel, "TEMPO SYNC / BPM (tap the TAP footswitch)");
    calStatus.setFont (theme::font (11.0f));
    calStatus.setColour (juce::Label::textColourId, theme::secondaryTeal);
    for (auto* l : { &calLabel, &bypassLabel, &midiLabel, &tempoLabel, &calStatus })
        addAndMakeVisible (*l);

    constexpr const char* names[] { "LOW", "MEDIUM", "HIGH", "VERY HIGH" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = calButtons[(size_t) i];
        b.setButtonText (names[i]);
        b.onClick = [this, i] { processor.setParam (params::kCalibration, (float) i); };
        addAndMakeVisible (b);
    }
    autoCal.onClick = [this] { processor.getEngine().startAutoCalibration(); };
    addAndMakeVisible (autoCal);

    bypassMode.addItemList ({ "Buffered", "Buffered + Trails", "True Bypass" }, 1);
    midiChannel.addItem ("Omni", 1);
    for (int c = 1; c <= 16; ++c)
        midiChannel.addItem (juce::String (c), c + 1);
    syncMode.addItemList ({ "Free", "Tap", "Host" }, 1);
    bypassAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, params::kBypassMode, bypassMode);
    midiAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, params::kMidiChannel, midiChannel);
    syncAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, params::kSyncMode, syncMode);
    for (auto* c : { &bypassMode, &midiChannel, &syncMode })
        addAndMakeVisible (*c);

    tempo.setSliderStyle (juce::Slider::LinearHorizontal);
    tempo.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
    tempo.setColour (juce::Slider::trackColourId, theme::secondaryTeal);
    tempo.setColour (juce::Slider::textBoxTextColourId, theme::stripText);
    tempo.setColour (juce::Slider::textBoxOutlineColourId, theme::stripText.withAlpha (0.25f));
    tempoAttachment = std::make_unique<APVTS::SliderAttachment> (state, params::kTempo, tempo);
    addAndMakeVisible (tempo);

    resetSecondary.onClick = [this]
    {
        for (size_t k = 0; k < params::kSecondaryIds.size(); ++k)
            processor.setParam (params::kSecondaryIds[k], params::kSecondaryDefaults[k]);
    };
    clearGestures.onClick = [this] { processor.getEngine().gestureClear(); };
    addAndMakeVisible (resetSecondary);
    addAndMakeVisible (clearGestures);

    timerCallback();
    startTimerHz (10);
}

void GlobalPanel::timerCallback()
{
    const int level = (int) processor.getParam (params::kCalibration);
    for (int i = 0; i < 4; ++i)
        calButtons[(size_t) i].setToggleState (i == level, juce::dontSendNotification);
    calStatus.setText (processor.getEngine().isAutoCalibrating() ? "Listening... play your instrument at normal volume."
                                                                  : juce::String ("Current level: ") + calibration::levelName (level),
                       juce::dontSendNotification);
}

void GlobalPanel::resized()
{
    Panel::resized();
    auto r = content();
    calLabel.setBounds (r.removeFromTop (18));
    auto cal = r.removeFromTop (28);
    const int w = cal.getWidth() / 5;
    for (auto& b : calButtons)
        b.setBounds (cal.removeFromLeft (w).reduced (3, 0));
    autoCal.setBounds (cal.reduced (3, 0));
    calStatus.setBounds (r.removeFromTop (22));
    r.removeFromTop (8);

    auto row = r.removeFromTop (46);
    auto left = row.removeFromLeft (row.getWidth() / 2).reduced (4, 0);
    auto right = row.reduced (4, 0);
    bypassLabel.setBounds (left.removeFromTop (18));
    bypassMode.setBounds (left.removeFromTop (26));
    midiLabel.setBounds (right.removeFromTop (18));
    midiChannel.setBounds (right.removeFromTop (26));
    r.removeFromTop (10);

    auto t = r.removeFromTop (46).reduced (4, 0);
    tempoLabel.setBounds (t.removeFromTop (18));
    syncMode.setBounds (t.removeFromLeft (110).withHeight (26));
    tempo.setBounds (t.withTrimmedLeft (8).withHeight (26));
    r.removeFromTop (14);

    auto buttons = r.removeFromTop (28);
    resetSecondary.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (6, 0));
    clearGestures.setBounds (buttons.reduced (6, 0));
}

} // namespace warmful::ui
