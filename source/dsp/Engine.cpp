// WARMFUL — engine implementation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Engine.h"
#include "Dynamics.h"
#include <thread>

namespace warmful
{

namespace
{
    /** Final converter-style ceiling of the effected path (internal level). */
    constexpr float kOutputKnee = 0.8f;

    bool isPermutation (const std::array<int, kNumModules>& order) noexcept
    {
        std::array<bool, kNumModules> seen {};
        for (int m : order)
        {
            if (m < 0 || m >= kNumModules || seen[(size_t) m])
                return false;
            seen[(size_t) m] = true;
        }
        return true;
    }

    /** Which primary knob is the "top" control of each module column. */
    constexpr std::array<int, kNumModules> kTopKnob { kTilt, kRate, kTime, -1 };
    constexpr std::array<int, kNumModules> kAmountKnob { kCharacterAmount, kMovementAmount, kDiffusionAmount, kTextureAmount };
}

Engine::Engine()
{
    for (int m = 0; m < kNumModules; ++m)
        modules[(size_t) m] = std::make_unique<Module> ((ModuleType) m);
    for (auto& e : effective)
        e.store (0.5f);
}

void Engine::prepare (double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    for (auto& m : modules)
        m->prepare (sr, kControlBlock);
    gesture.prepare (sr);
    capture.prepare (sr);
    autoCal.prepare (sr);

    for (auto* v : { &dryL, &dryR, &calInL, &calInR, &calBuf })
        v->assign ((size_t) kControlBlock, 0.0f);

    calGain.prepare (sr, 0.05f);
    mixDry.prepare (sr, 0.02f);
    mixWet.prepare (sr, 0.02f);
    outGain.prepare (sr, 0.02f);
    bypassGain.prepare (sr, 0.01f);
    orderGain.prepare (sr, 0.004f);
    reset();
}

void Engine::reset() noexcept
{
    for (auto& m : modules)
        m->reset();
    capture.reset();
    orderChangePending = false;
    firstBlock = true;
}

Gesture::Snapshot Engine::getGestureSnapshot()
{
    while (gestureLock.exchange (true, std::memory_order_acquire))
        std::this_thread::yield();
    auto s = gesture.snapshot();
    gestureLock.store (false, std::memory_order_release);
    return s;
}

void Engine::setGestureSnapshot (const Gesture::Snapshot& snapshot)
{
    while (gestureLock.exchange (true, std::memory_order_acquire))
        std::this_thread::yield();
    gesture.restore (snapshot);
    gestureLock.store (false, std::memory_order_release);
}

void Engine::applyCommands (const float* raw, double bpm) noexcept
{
    if (pendingGestureClear.exchange (false))
        gesture.clear();
    if (pendingGestureBegin.exchange (false))
        gesture.beginRecording (raw);
    if (pendingGestureEnd.exchange (false))
        gesture.endRecording (bpm);

    switch ((CaptureCmd) pendingCapture.exchange (0))
    {
        case CaptureCmd::Arm:    capture.arm(); break;
        case CaptureCmd::Cancel: capture.cancel(); break;
        case CaptureCmd::Commit: capture.commit(); break;
        case CaptureCmd::Stop:   capture.stop(); break;
        case CaptureCmd::None:   break;
    }

    if (pendingAutoCal.exchange (false))
        autoCal.start();
}

void Engine::process (float* l, float* r, int numSamples, const EngineParams& p, bool monoInput) noexcept
{
    // Knob values as the pedal sees them: expression pedal overrides its assigned knob.
    rawKnobs = p.primary;
    if (p.expressionTarget >= 0 && p.expressionTarget < kNumPrimary)
        rawKnobs[(size_t) p.expressionTarget] = clamp (p.expression, 0.0f, 1.0f);

    const bool gestureLocked = ! gestureLock.exchange (true, std::memory_order_acquire);
    if (gestureLocked)
        applyCommands (rawKnobs.data(), p.tempo.bpm);

    if (firstBlock)
    {
        float d, w;
        equalPowerGains (p.primary[kMix], d, w);
        calGain.snap (dbToGain (calibration::kGainDb[(size_t) clamp (p.calibrationLevel, 0, 3)]));
        mixDry.snap (d);
        mixWet.snap (w);
        outGain.snap (neutralCentreGain (p.outputLevel));
        bypassGain.snap (p.bypassed ? 0.0f : 1.0f);
        orderGain.snap (1.0f);
        if (isPermutation (p.order))
            activeOrder = p.order;
        for (int m = 0; m < kNumModules; ++m)
            modules[(size_t) m]->selectEffect (p.effect[(size_t) m]);
        firstBlock = false;
    }

    for (int offset = 0; offset < numSamples; offset += kControlBlock)
    {
        const int n = std::min (kControlBlock, numSamples - offset);
        if (gestureLocked)
            gesture.process (rawKnobs.data(), n, p.tempo.bpm, effKnobs.data());
        else
            effKnobs = rawKnobs;
        processChunk (l + offset, r + offset, n, p, monoInput);
    }

    if (gestureLocked)
    {
        uint32_t lanes = 0, rec = 0;
        for (int k = 0; k < kNumPrimary; ++k)
        {
            lanes |= (gesture.hasLane (k) ? 1u : 0u) << k;
            rec |= (gesture.isRecordingKnob (k) ? 1u : 0u) << k;
        }
        gestureLaneMask.store (lanes);
        gestureRecMask.store (rec);
        gestureRecording.store (gesture.isRecording());
        gestureLock.store (false, std::memory_order_release);
    }
    chainPeak.store (blockPeak, std::memory_order_relaxed);
    blockPeak = 0.0f;
    for (int k = 0; k < kNumPrimary; ++k)
        effective[(size_t) k].store (effKnobs[(size_t) k], std::memory_order_relaxed);
    captureMode.store ((int) capture.getMode());
    autoCalBusy.store (autoCal.getState() != AutoCalibrator::State::Idle);
}

void Engine::processChunk (float* l, float* r, int n, const EngineParams& p, bool monoInput) noexcept
{
    // ---- Controls -------------------------------------------------------------------------
    std::array<EffectParams, kNumModules> ep;
    for (int m = 0; m < kNumModules; ++m)
    {
        auto& e = ep[(size_t) m];
        e.knob = kTopKnob[(size_t) m] >= 0 ? effKnobs[(size_t) kTopKnob[(size_t) m]] : 0.5f;
        e.amount = effKnobs[(size_t) kAmountKnob[(size_t) m]];
        e.drift = m == (int) ModuleType::Movement ? p.driftMovement
                : m == (int) ModuleType::Diffusion ? p.driftDiffusion : 0.0f;
        e.sensitivity = p.sensitivity;
        e.filterStyle = p.filterStyle;
        e.tempo = p.tempo;
        modules[(size_t) m]->selectEffect (p.effect[(size_t) m]);
    }

    if (! orderChangePending && p.order != activeOrder && isPermutation (p.order))
    {
        orderChangePending = true;
        requestedOrder = p.order;
        orderGain.setTarget (0.0f);
    }
    if (orderChangePending && orderGain.isSettled())
    {
        activeOrder = requestedOrder;
        orderChangePending = false;
        orderGain.setTarget (1.0f);
    }

    float dTarget, wTarget;
    equalPowerGains (effKnobs[kMix], dTarget, wTarget);
    mixDry.setTarget (dTarget);
    mixWet.setTarget (wTarget);
    outGain.setTarget (neutralCentreGain (p.outputLevel));
    calGain.setTarget (dbToGain (calibration::kGainDb[(size_t) clamp (p.calibrationLevel, 0, 3)]));
    bypassGain.setTarget (p.bypassed ? 0.0f : 1.0f);

    // ---- Input ----------------------------------------------------------------------------
    if (p.captureRouting == CaptureRouting::PreFx)
        capture.process (l, r, n);

    const int calResult = autoCal.process (l, r, n);
    if (calResult >= 0)
        autoCalResult.store (calResult);

    for (int i = 0; i < n; ++i)
    {
        const float c = calGain.next();
        calBuf[(size_t) i] = c;
        dryL[(size_t) i] = l[i];
        dryR[(size_t) i] = r[i];
        l[i] *= c;
        r[i] *= c;
        calInL[(size_t) i] = l[i];
        calInR[(size_t) i] = r[i];
    }

    // ---- Modules --------------------------------------------------------------------------
    const bool trails = p.bypassMode == BypassMode::BufferedTrails;
    for (int slot = 0; slot < kNumModules; ++slot)
    {
        const int m = activeOrder[(size_t) slot];
        Module::State st;
        st.engaged = ! p.bypassed && p.moduleOn[(size_t) m] && ! (p.dualBypassActive && p.dualMask[(size_t) m]);
        st.trails = trails;
        st.effectVol = p.effectVol[(size_t) m];
        modules[(size_t) m]->process (l, r, n, ep[(size_t) m], st);
    }

    // Safety net: a non-finite sample would poison feedback loops forever.
    bool finite = true;
    for (int i = 0; i < n && finite; ++i)
        finite = std::isfinite (l[i]) && std::isfinite (r[i]);
    if (! finite)
    {
        for (auto& m : modules)
            m->reset();
        std::copy (calInL.begin(), calInL.begin() + n, l);
        std::copy (calInR.begin(), calInR.begin() + n, r);
    }

    // ---- Output: MIX, OUTPUT LEVEL, bypass ---------------------------------------------------
    const bool trueBypassMono = monoInput && p.bypassMode == BypassMode::TrueBypass;
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float c = calBuf[(size_t) i];
        const float og = outGain.next() / c;
        const float dg = mixDry.next(), wg = mixWet.next();
        const float bg = bypassGain.next();
        const float rg = orderGain.next();

        // Re-ordering dips the effected path to dry for a few ms instead of clicking.
        const float wl = calInL[(size_t) i] + (l[i] - calInL[(size_t) i]) * rg;
        const float wr = calInR[(size_t) i] + (r[i] - calInR[(size_t) i]) * rg;
        peak = std::max (peak, std::max (std::abs (wl), std::abs (wr)));

        const float engagedL = dryL[(size_t) i] * dg + shape::kneeClip (wl, kOutputKnee) * og * wg;
        const float engagedR = dryR[(size_t) i] * dg + shape::kneeClip (wr, kOutputKnee) * og * wg;

        // Bypassed: dry plus whatever the modules still add (Diffusion tails with trails).
        const float bypL = dryL[(size_t) i] + (wl - calInL[(size_t) i]) * og * wg;
        const float bypR = (trueBypassMono ? 0.0f : dryR[(size_t) i]) + (wr - calInR[(size_t) i]) * og * wg;

        l[i] = engagedL * bg + bypL * (1.0f - bg);
        r[i] = engagedR * bg + bypR * (1.0f - bg);
    }
    blockPeak = std::max (blockPeak, peak);

    if (p.captureRouting == CaptureRouting::PostFx)
        capture.process (l, r, n);
}

} // namespace warmful
