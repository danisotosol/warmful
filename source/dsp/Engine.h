// WARMFUL — the whole pedal: calibration, 4 re-orderable modules, MIX, OUTPUT LEVEL,
// bypass modes, GESTURE, CAPTURE, expression mapping. Framework independent.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Capture.h"
#include "Control.h"
#include "Gesture.h"
#include "Module.h"
#include <atomic>
#include <memory>

namespace warmful
{

enum class BypassMode : int { Buffered = 0, BufferedTrails = 1, TrueBypass = 2 };
enum class CaptureRouting : int { PostFx = 0, PreFx = 1 };

/** Everything the host/UI controls. Knobs are normalised 0..1 like the pedal's pots. */
struct EngineParams
{
    std::array<float, kNumPrimary> primary { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 1.0f, 0.5f };
    float sensitivity = 0.5f;
    float driftMovement = 0.0f;
    float driftDiffusion = 0.0f;
    float outputLevel = 0.5f;
    std::array<float, kNumModules> effectVol { 0.5f, 0.5f, 0.5f, 0.5f };

    std::array<int, kNumModules> effect { 0, 0, 0, 0 };
    std::array<bool, kNumModules> moduleOn { true, true, true, true };
    /** Processing order: order[slot] = module index. */
    std::array<int, kNumModules> order { 0, 1, 2, 3 };

    bool bypassed = false;               // whole pedal off
    bool dualBypassActive = false;       // footswitch group off (DUAL BYPASS)
    std::array<bool, kNumModules> dualMask { true, true, true, true };
    BypassMode bypassMode = BypassMode::Buffered;

    CaptureRouting captureRouting = CaptureRouting::PostFx;
    FilterStyle filterStyle = FilterStyle::Tilt;
    int calibrationLevel = 1;

    /** Expression pedal: -1 = unassigned, else the Primary index it drives. */
    int expressionTarget = -1;
    float expression = 0.0f;

    TempoInfo tempo;
};

class Engine
{
public:
    Engine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset() noexcept;

    /** In-place stereo processing. `monoInput` only matters for TRUE BYPASS
        (relays connect each input to its output, so a mono source stays on the left). */
    void process (float* left, float* right, int numSamples, const EngineParams& params, bool monoInput = false) noexcept;

    // ---- Commands: safe to call from any thread; applied at the start of the next block.
    void gestureBeginRecording() noexcept { pendingGestureBegin.store (true); }
    void gestureEndRecording() noexcept   { pendingGestureEnd.store (true); }
    void gestureClear() noexcept          { pendingGestureClear.store (true); }
    void captureArm() noexcept            { pendingCapture.store ((int) CaptureCmd::Arm); }
    void captureCancel() noexcept         { pendingCapture.store ((int) CaptureCmd::Cancel); }
    void captureCommit() noexcept         { pendingCapture.store ((int) CaptureCmd::Commit); }
    void captureStop() noexcept           { pendingCapture.store ((int) CaptureCmd::Stop); }
    void startAutoCalibration() noexcept  { pendingAutoCal.store (true); }

    // ---- State for the UI (lock free).
    float getEffectivePrimary (int knob) const noexcept { return effective[(size_t) knob].load (std::memory_order_relaxed); }
    bool gestureHasLane (int knob) const noexcept { return (gestureLaneMask.load() >> knob) & 1u; }
    bool gestureIsRecordingKnob (int knob) const noexcept { return (gestureRecMask.load() >> knob) & 1u; }
    bool gestureIsRecording() const noexcept { return gestureRecording.load(); }
    Capture::Mode getCaptureMode() const noexcept { return (Capture::Mode) captureMode.load(); }
    bool isAutoCalibrating() const noexcept { return autoCalBusy.load(); }
    /** Returns the level picked by auto calibration once (else -1). */
    int takeAutoCalibrationResult() noexcept { return autoCalResult.exchange (-1); }
    /** Output peak of the last block (internal level of the chain), for the LED meters. */
    float getChainPeak() const noexcept { return chainPeak.load (std::memory_order_relaxed); }

    // ---- GESTURE persistence (message thread).
    Gesture::Snapshot getGestureSnapshot();
    void setGestureSnapshot (const Gesture::Snapshot& snapshot);

    static constexpr int kControlBlock = 32;

private:
    enum class CaptureCmd : int { None = 0, Arm, Cancel, Commit, Stop };

    void applyCommands (const float* raw, double bpm) noexcept;
    void processChunk (float* l, float* r, int n, const EngineParams& p, bool monoInput) noexcept;

    double sampleRate = 48000.0;
    std::array<std::unique_ptr<Module>, kNumModules> modules;
    std::array<int, kNumModules> activeOrder { 0, 1, 2, 3 };
    Gesture gesture;
    Capture capture;
    AutoCalibrator autoCal;

    Smoothed calGain, mixDry, mixWet, outGain;
    LinearRamp bypassGain, orderGain;
    bool orderChangePending = false;
    std::array<int, kNumModules> requestedOrder { 0, 1, 2, 3 };

    std::vector<float> dryL, dryR, calInL, calInR, calBuf;
    std::array<float, kNumPrimary> rawKnobs {}, effKnobs {};
    bool firstBlock = true;
    float blockPeak = 0.0f;

    // Lock-free command mailboxes.
    std::atomic<bool> pendingGestureBegin { false }, pendingGestureEnd { false }, pendingGestureClear { false };
    std::atomic<int> pendingCapture { 0 };
    std::atomic<bool> pendingAutoCal { false };

    // Lightweight try-lock guarding the gesture lanes against message-thread persistence.
    std::atomic<bool> gestureLock { false };

    // UI mirrors.
    std::array<std::atomic<float>, kNumPrimary> effective {};
    std::atomic<uint32_t> gestureLaneMask { 0 }, gestureRecMask { 0 };
    std::atomic<bool> gestureRecording { false }, autoCalBusy { false };
    std::atomic<int> captureMode { 0 }, autoCalResult { -1 };
    std::atomic<float> chainPeak { 0.0f };
};

} // namespace warmful
