// WARMFUL — tap tempo and automatic input calibration.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include "Common.h"
#include <array>

namespace warmful
{

/** Averages the last few tap intervals; a long pause starts a new measurement. */
class TapTempo
{
public:
    /** Returns true and writes `bpm` once at least two taps form a valid interval. */
    bool tap (double timeSeconds, double& bpm) noexcept
    {
        const double interval = timeSeconds - lastTap;
        lastTap = timeSeconds;
        if (interval <= 0.0 || interval > kMaxInterval || interval < kMinInterval)
        {
            count = 0;
            return false;
        }
        intervals[(size_t) (count % kHistory)] = interval;
        ++count;
        const int n = std::min (count, kHistory);
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += intervals[(size_t) i];
        bpm = 60.0 / (sum / n);
        return true;
    }

private:
    static constexpr int kHistory = 4;
    static constexpr double kMaxInterval = 2.0;   // 30 BPM
    static constexpr double kMinInterval = 0.2;   // 300 BPM
    std::array<double, kHistory> intervals {};
    int count = 0;
    double lastTap = -100.0;
};

/** Calibration headroom levels (manual ch. 2). The gain brings the instrument's typical
    peak to the chain's nominal level (~ -6 dBFS internal). */
namespace calibration
{
    constexpr int kNumLevels = 4;   // Low, Medium, High, Very High
    constexpr std::array<float, kNumLevels> kGainDb { 18.0f, 12.0f, 6.0f, 0.0f };
    constexpr float kNominalPeakDb = -6.0f;
    inline const char* levelName (int i) noexcept
    {
        constexpr const char* names[] { "LOW", "MEDIUM", "HIGH", "VERY HIGH" };
        return names[clamp (i, 0, kNumLevels - 1)];
    }
}

/** "Begin playing your instrument... the pedal will listen and place it into one of four
    input headroom levels." Listens to ~2.5 s of playing and picks the closest level. */
class AutoCalibrator
{
public:
    enum class State { Idle, Waiting, Listening };

    void prepare (double sampleRate) noexcept
    {
        windowSamples = std::max (1, (int) (0.05 * sampleRate));
        timeoutWindows = (int) (10.0 / 0.05);
        state = State::Idle;
    }
    void start() noexcept
    {
        state = State::Waiting;
        windowsSeen = windowPos = numPeaks = 0;
        windowPeak = 0.0f;
    }
    void cancel() noexcept { state = State::Idle; }
    State getState() const noexcept { return state; }

    /** Returns -1 while busy/idle, otherwise the chosen level once (then goes idle). */
    int process (const float* l, const float* r, int n) noexcept
    {
        if (state == State::Idle)
            return -1;
        for (int i = 0; i < n; ++i)
        {
            windowPeak = std::max (windowPeak, std::max (std::abs (l[i]), std::abs (r[i])));
            if (++windowPos < windowSamples)
                continue;

            windowPos = 0;
            ++windowsSeen;
            if (state == State::Waiting)
            {
                if (windowPeak > kSignalThreshold)
                    state = State::Listening;
                else if (windowsSeen > timeoutWindows)
                {
                    state = State::Idle;   // nobody played: keep the current level
                    return -1;
                }
            }
            if (state == State::Listening)
            {
                peaks[(size_t) numPeaks++] = windowPeak;
                if (numPeaks == kListenWindows)
                {
                    state = State::Idle;
                    return chooseLevel();
                }
            }
            windowPeak = 0.0f;
        }
        return -1;
    }

private:
    static constexpr int kListenWindows = 50;          // 2.5 s
    static constexpr float kSignalThreshold = 0.0056f; // -45 dBFS

    int chooseLevel() noexcept
    {
        // Typical peak = mean of the 4 loudest 50 ms windows: the strums themselves, not the decay
        // between them, set the headroom ("play at your normal playing volume").
        std::sort (peaks.begin(), peaks.end());
        float sum = 0.0f;
        const int top = 4;
        for (int i = kListenWindows - top; i < kListenWindows; ++i)
            sum += peaks[(size_t) i];
        const float typicalDb = gainToDb (sum / (float) top);
        const float neededDb = calibration::kNominalPeakDb - typicalDb;
        int best = 0;
        for (int i = 1; i < calibration::kNumLevels; ++i)
            if (std::abs (calibration::kGainDb[(size_t) i] - neededDb) < std::abs (calibration::kGainDb[(size_t) best] - neededDb))
                best = i;
        return best;
    }

    State state = State::Idle;
    std::array<float, kListenWindows> peaks {};
    int windowSamples = 2400, timeoutWindows = 200, windowsSeen = 0, windowPos = 0, numPeaks = 0;
    float windowPeak = 0.0f;
};

} // namespace warmful
