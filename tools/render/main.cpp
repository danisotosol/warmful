// WARMFUL — offline renderer and DSP test-suite (no JUCE needed).
//   warmful_render test                         run the DSP checks (non-zero exit on failure)
//   warmful_render demo <dir> [--sr N]          render every effect at several settings
//   warmful_render presets <dir>                render the factory presets (and print their level)
//   warmful_render presets-md                   print docs/PRESETS.md
//   warmful_render render <in.wav> <out.wav> [options]   process a file like the plugin would
// SPDX-License-Identifier: CC-BY-NC-SA-4.0

#include "Engine.h"
#include "FactoryPresets.h"
#include "Oversampler.h"
#include "Modulation.h"
#include "Wav.h"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <string>

#if defined(__SSE__) || defined(_M_X64)
 #include <xmmintrin.h>
#endif

using namespace warmful;

namespace
{

constexpr const char* kModuleKeys[] { "char", "move", "diff", "tex" };
constexpr const char* kEffectNames[4][5] {
    { "drive", "sweeten", "fuzz", "howl", "swell" },
    { "doubler", "vibrato", "phaser", "tremolo", "pitch" },
    { "cascade", "reels", "space", "collage", "reverse" },
    { "filter", "squash", "cassette", "broken", "interference" },
};

void enableFlushToZero()
{
#if defined(__SSE__) || defined(_M_X64)
    _mm_setcsr (_mm_getcsr() | 0x8040);   // FTZ | DAZ, like juce::ScopedNoDenormals
#endif
}

// ---- Test signals ------------------------------------------------------------------------------

/** Karplus-Strong strummed chords: a guitar-ish DI with peaks near `peak`. */
wav::Audio guitar (double sr, double seconds, float peak)
{
    wav::Audio a;
    a.sampleRate = sr;
    const size_t n = (size_t) (sr * seconds);
    std::vector<float> mono (n, 0.0f);
    Rng rng (1234);
    const float chords[3][6] { { 82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f },
                               { 110.0f, 164.8f, 220.0f, 277.2f, 329.6f, 440.0f },
                               { 98.0f, 123.5f, 146.8f, 196.0f, 293.7f, 392.0f } };
    const double strumEvery = 1.6;
    for (int s = 0; s * strumEvery < seconds - 0.2; ++s)
    {
        const auto& chord = chords[s % 3];
        for (int str = 0; str < 6; ++str)
        {
            const size_t start = (size_t) ((s * strumEvery + str * 0.012) * sr);
            const int period = std::max (2, (int) (sr / chord[str]));
            std::vector<float> line ((size_t) period);
            for (auto& v : line)
                v = rng.bipolar();
            size_t idx = 0;
            float prev = 0.0f;
            for (size_t i = start; i < n && i < start + (size_t) (sr * 2.5); ++i)
            {
                const float cur = line[idx];
                const float next = 0.4985f * (cur + prev) + 0.0015f * cur;
                prev = cur;
                line[idx] = next;
                idx = (idx + 1) % (size_t) period;
                mono[i] += cur * 0.25f;
            }
        }
    }
    float mx = 1e-9f;
    for (float v : mono) mx = std::max (mx, std::abs (v));
    for (float& v : mono) v *= peak / mx;
    a.channels = { mono, mono };
    return a;
}

wav::Audio sine (double sr, double seconds, float hz, float amp)
{
    wav::Audio a;
    a.sampleRate = sr;
    std::vector<float> m ((size_t) (sr * seconds));
    for (size_t i = 0; i < m.size(); ++i)
        m[i] = (float) (amp * std::sin (2.0 * 3.14159265358979 * hz * (double) i / sr));
    a.channels = { m, m };
    return a;
}

wav::Audio impulse (double sr, double seconds, float amp)
{
    wav::Audio a;
    a.sampleRate = sr;
    std::vector<float> m ((size_t) (sr * seconds), 0.0f);
    if (const auto at = (size_t) (0.01 * sr); at < m.size())
        m[at] = amp;   // short buffers are plain silence
    a.channels = { m, m };
    return a;
}

// ---- Metrics -----------------------------------------------------------------------------------

struct Stats { double rms = 0.0, peak = 0.0; bool finite = true; };

Stats stats (const wav::Audio& a, size_t from = 0)
{
    Stats s;
    double sum = 0.0;
    size_t count = 0;
    for (const auto& ch : a.channels)
        for (size_t i = from; i < ch.size(); ++i)
        {
            const float v = ch[i];
            if (! std::isfinite (v)) { s.finite = false; continue; }
            sum += (double) v * v;
            s.peak = std::max (s.peak, (double) std::abs (v));
            ++count;
        }
    s.rms = count ? std::sqrt (sum / (double) count) : 0.0;
    return s;
}

double db (double g) { return 20.0 * std::log10 (std::max (g, 1e-12)); }

/** Strongest frequency between lo and hi (Goertzel scan) over the given channel range. */
double dominantHz (const std::vector<float>& x, double sr, size_t from, double lo, double hi, double step = 0.5)
{
    double bestHz = lo, best = -1.0;
    for (double f = lo; f <= hi; f += step)
    {
        const double w = 2.0 * 3.14159265358979 * f / sr, c = 2.0 * std::cos (w);
        double s1 = 0.0, s2 = 0.0;
        for (size_t i = from; i < x.size(); ++i)
        {
            const double s0 = x[i] + c * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        const double p = s1 * s1 + s2 * s2 - c * s1 * s2;
        if (p > best) { best = p; bestHz = f; }
    }
    return bestHz;
}

/** Frequency from the median period between upward zero crossings. */
double medianZeroCrossingHz (const std::vector<float>& x, double sr, size_t from)
{
    std::vector<double> crossings, periods;
    for (size_t i = std::max<size_t> (from, 1); i < x.size(); ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            crossings.push_back ((double) (i - 1) + (double) (-x[i - 1] / (x[i] - x[i - 1])));
    for (size_t i = 1; i < crossings.size(); ++i)
        periods.push_back (crossings[i] - crossings[i - 1]);
    if (periods.empty())
        return 0.0;
    std::nth_element (periods.begin(), periods.begin() + (long) periods.size() / 2, periods.end());
    return sr / periods[periods.size() / 2];
}

// ---- Processing helpers ------------------------------------------------------------------------

/** Runs one effect directly (internal level), with irregular block sizes. Returns CPU seconds. */
double runEffect (Effect& fx, wav::Audio& a, const EffectParams& p, uint32_t blockSeed = 7)
{
    Rng rng (blockSeed);
    auto& l = a.channels[0];
    auto& r = a.channels[1];
    const auto t0 = std::chrono::steady_clock::now();
    for (size_t pos = 0; pos < l.size();)
    {
        const int n = std::min<int> (1 + (int) (rng.uniform() * 32.0f), (int) (l.size() - pos));
        fx.process (l.data() + pos, r.data() + pos, n, p);
        pos += (size_t) n;
    }
    return std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
}

double runEngine (Engine& engine, wav::Audio& a, const EngineParams& p, int block = 256)
{
    auto& l = a.channels[0];
    auto& r = a.channels[1];
    const auto t0 = std::chrono::steady_clock::now();
    for (size_t pos = 0; pos < l.size(); pos += (size_t) block)
    {
        const int n = std::min<int> (block, (int) (l.size() - pos));
        engine.process (l.data() + pos, r.data() + pos, n, p);
    }
    return std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
}

EngineParams soloModule (int module, int effect)
{
    EngineParams p;
    p.moduleOn = { false, false, false, false };
    p.moduleOn[(size_t) module] = true;
    p.effect[(size_t) module] = effect;
    return p;
}

void setColumn (EngineParams& p, int module, float knob, float amount, float drift)
{
    if (module < 3)
        p.primary[(size_t) (2 * module)] = knob;
    p.primary[(size_t) (2 * module + 1)] = amount;
    if (module == 1) p.driftMovement = drift;
    if (module == 2) p.driftDiffusion = drift;
}

// ---- Test runner -------------------------------------------------------------------------------

struct Runner
{
    int failures = 0, checks = 0;
    void check (bool ok, const std::string& what)
    {
        ++checks;
        if (! ok)
        {
            ++failures;
            std::printf ("  FAIL  %s\n", what.c_str());
        }
    }
};

void testOversampler (Runner& t)
{
    for (int stages : { 1, 2 })
    {
        Oversampler os;
        os.prepare (512, stages);
        // Round trip: a 1 kHz tone keeps its level.
        auto s = sine (48000.0, 0.5, 1000.0f, 0.5f);
        std::vector<float> out (s.frames());
        for (size_t pos = 0; pos + 512 <= s.frames(); pos += 512)
        {
            os.upsample (s.channels[0].data() + pos, 512);
            os.downsample (out.data() + pos, 512);
        }
        double peak = 0.0;
        for (size_t i = 4800; i < out.size() - 512; ++i) peak = std::max (peak, (double) std::abs (out[i]));
        t.check (std::abs (db (peak / 0.5)) < 0.2, "oversampler x" + std::to_string (1 << stages) + " passband gain " + std::to_string (db (peak / 0.5)) + " dB");

        // Decimation filter: a 15 kHz tone at the high rate passes, a 30 kHz tone (would alias) is rejected.
        auto decimatedPeak = [&os, stages] (float hz)
        {
            const int upN = 512 << stages;
            const double upRate = 48000.0 * (1 << stages);
            std::vector<float> zeros (512, 0.0f), dec (512);
            double pk = 0.0;
            for (int block = 0; block < 40; ++block)
            {
                float* up = os.upsample (zeros.data(), 512);
                for (int i = 0; i < upN; ++i)
                    up[i] = (float) (0.5 * std::sin (2.0 * 3.14159265358979 * hz * (double) (block * upN + i) / upRate));
                os.downsample (dec.data(), 512);
                if (block > 4)
                    for (float v : dec) pk = std::max (pk, (double) std::abs (v));
            }
            return db (pk / 0.5);
        };
        const double pass = decimatedPeak (15000.0f), stop = decimatedPeak (30000.0f);
        std::printf ("  x%d: 15 kHz %.2f dB, 30 kHz alias %.1f dB\n", 1 << stages, pass, stop);
        t.check (std::abs (pass) < 0.5 && stop < -60.0, "oversampler x" + std::to_string (1 << stages) + " decimation (pass/stop)");
    }
}

void testEffectsBounded (Runner& t, double sr)
{
    std::printf ("\n[effects @ %.0f Hz]  level change at knob .5 / amount .5 (internal level, guitar peak 0.5)\n", sr);
    for (int m = 0; m < kNumModules; ++m)
    {
        auto set = createEffects ((ModuleType) m);
        for (int e = 0; e < kEffectsPerModule; ++e)
        {
            auto& fx = *set[(size_t) e];
            fx.prepare (sr, 32);
            const std::string name = std::string (kModuleKeys[m]) + "/" + kEffectNames[m][e];

            double worstPeak = 0.0;
            bool allFinite = true;
            for (float knob : { 0.0f, 0.5f, 1.0f })
                for (float amount : { 0.0f, 0.5f, 1.0f })
                    for (float drift : { 0.0f, 1.0f })
                    {
                        if (m != 1 && m != 2 && drift > 0.0f) continue;
                        fx.reset();
                        auto a = guitar (sr, 3.0, 0.5f);
                        EffectParams p;
                        p.knob = knob; p.amount = amount; p.drift = drift;
                        runEffect (fx, a, p, (uint32_t) (knob * 10 + amount * 100 + 3));
                        const auto s = stats (a);
                        allFinite &= s.finite;
                        worstPeak = std::max (worstPeak, s.peak);
                    }
            t.check (allFinite, name + " finite at every corner setting");
            t.check (worstPeak < 4.0, name + " bounded (worst peak " + std::to_string (worstPeak) + ")");

            fx.reset();
            auto in = guitar (sr, 4.0, 0.5f);
            auto out = in;
            EffectParams p;
            p.knob = 0.5f; p.amount = 0.5f;
            const double cpu = runEffect (fx, out, p);
            const double change = db (stats (out).rms / stats (in).rms);
            std::printf ("  %-22s %+6.1f dB   peak %.2f   cpu %5.2f%%\n", name.c_str(), change, stats (out).peak, 100.0 * cpu / 4.0);
            t.check (change > -18.0 && change < 12.0, name + " moderate setting level change within -18..+12 dB (" + std::to_string (change) + ")");
        }
    }
}

void testSpecifics (Runner& t, double sr)
{
    auto character = createCharacterEffects();
    auto movement = createMovementEffects();
    auto diffusion = createDiffusionEffects();
    auto texture = createTextureEffects();
    for (auto* set : { &character, &movement, &diffusion, &texture })
        for (auto& e : *set)
            e->prepare (sr, 32);

    // FILTER neutral positions are transparent.
    auto& filter = *texture[0];
    const std::pair<FilterStyle, float> neutral[] { { FilterStyle::Tilt, 0.5f }, { FilterStyle::LowPass, 1.0f }, { FilterStyle::HighPass, 0.0f } };
    for (auto [style, amount] : neutral)
    {
        filter.reset();
        auto in = guitar (sr, 1.0, 0.5f);
        auto out = in;
        EffectParams p;
        p.amount = amount;
        p.filterStyle = style;
        runEffect (filter, out, p);
        double err = 0.0;
        for (size_t i = (size_t) (0.1 * sr); i < in.frames(); ++i)
            err = std::max (err, (double) std::abs (out.channels[0][i] - in.channels[0][i]));
        t.check (err < 1e-3, "FILTER style " + std::to_string ((int) style) + " transparent at neutral (max err " + std::to_string (err) + ")");
    }

    // PITCH: +-1 octave at the ends, unity at centre.
    auto& pitch = *movement[4];
    for (auto [knob, expected] : { std::pair { 0.0f, 110.0 }, std::pair { 0.5f, 220.0 }, std::pair { 1.0f, 440.0 } })
    {
        pitch.reset();
        auto a = sine (sr, 1.5, 220.0f, 0.4f);
        EffectParams p;
        p.knob = knob; p.amount = 1.0f;
        runEffect (pitch, a, p);
        // Granular shifters jump phase between grains; what is heard is the in-grain frequency,
        // so measure the median period between zero crossings.
        const double hz = medianZeroCrossingHz (a.channels[0], sr, (size_t) (0.5 * sr));
        t.check (std::abs (hz - expected) / expected < 0.02, "PITCH knob " + std::to_string (knob) + " -> " + std::to_string (hz) + " Hz (expected " + std::to_string (expected) + ")");
    }

    // REVERSE at centre keeps the pitch; at the ends it halves/doubles it.
    auto& reverse = *diffusion[4];
    for (auto [knob, expected] : { std::pair { 0.0f, 110.0 }, std::pair { 0.5f, 220.0 }, std::pair { 1.0f, 440.0 } })
    {
        reverse.reset();
        auto a = sine (sr, 3.0, 220.0f, 0.4f);
        EffectParams p;
        p.knob = knob; p.amount = 1.0f;
        runEffect (reverse, a, p);
        const double hz = dominantHz (a.channels[0], sr, (size_t) (1.5 * sr), 80.0, 500.0);
        t.check (std::abs (hz - expected) / expected < 0.03, "REVERSE knob " + std::to_string (knob) + " -> " + std::to_string (hz) + " Hz (expected " + std::to_string (expected) + ")");
    }

    // Echo times: first repeat lands where TIME says (free mode).
    auto firstEcho = [&] (Effect& fx, float knob)
    {
        fx.reset();
        auto a = impulse (sr, 1.5, 0.5f);
        EffectParams p;
        p.knob = knob; p.amount = 0.3f;
        runEffect (fx, a, p);
        const size_t from = (size_t) (0.025 * sr);
        size_t best = from;
        float bestV = 0.0f;
        for (size_t i = from; i < a.frames(); ++i)
            if (std::abs (a.channels[0][i]) > bestV) { bestV = std::abs (a.channels[0][i]); best = i; }
        return ((double) best / sr - 0.01) * 1000.0;
    };
    const double c0 = firstEcho (*diffusion[0], 0.0f), c1 = firstEcho (*diffusion[0], 1.0f);
    std::printf ("  CASCADE first echo: %.1f ms (TIME 0) .. %.1f ms (TIME 1)\n", c0, c1);
    t.check (c0 > 25.0 && c0 < 70.0 && c1 > 500.0 && c1 < 800.0, "CASCADE echo range ~40..650 ms");
    const double r0 = firstEcho (*diffusion[1], 0.0f), r1 = firstEcho (*diffusion[1], 1.0f);
    std::printf ("  REELS first echo:   %.1f ms (TIME 0) .. %.1f ms (TIME 1)\n", r0, r1);
    t.check (r0 > 25.0 && r1 > 500.0 && r1 < 900.0, "REELS echo range");

    // SPACE: decay grows with TIME.
    auto& space = *diffusion[2];
    double previous = 0.0;
    for (float knob : { 0.0f, 0.5f, 1.0f })
    {
        space.reset();
        auto a = impulse (sr, 8.0, 0.5f);
        EffectParams p;
        p.knob = knob; p.amount = 1.0f;
        runEffect (space, a, p);
        const double tail = stats (a, (size_t) (2.0 * sr)).rms;
        std::printf ("  SPACE TIME %.1f: tail RMS after 2 s = %.1f dB\n", knob, db (tail));
        t.check (tail >= previous, "SPACE tail grows with TIME");
        previous = tail;
    }

    // Self-oscillation at max AMOUNT stays bounded for a long time.
    for (int e : { 0, 1, 3 })
    {
        auto& fx = *diffusion[(size_t) e];
        fx.reset();
        auto a = guitar (sr, 12.0, 0.5f);
        EffectParams p;
        p.knob = 0.3f; p.amount = 1.0f; p.drift = 1.0f;
        runEffect (fx, a, p);
        const auto s = stats (a);
        t.check (s.finite && s.peak < 4.0, std::string (kEffectNames[2][e]) + " self-oscillation bounded (peak " + std::to_string (s.peak) + ")");
    }

    // TREMOLO depth at full AMOUNT.
    auto& trem = *movement[3];
    trem.reset();
    auto a = sine (sr, 2.0, 440.0f, 0.4f);
    EffectParams p;
    p.knob = 0.5f; p.amount = 1.0f;
    runEffect (trem, a, p);
    double lo = 1e9, hi = 0.0;
    const size_t win = (size_t) (0.005 * sr);
    for (size_t i = (size_t) (0.5 * sr); i + win < a.frames(); i += win)
    {
        double m = 0.0;
        for (size_t j = i; j < i + win; ++j) m = std::max (m, (double) std::abs (a.channels[0][j]));
        lo = std::min (lo, m); hi = std::max (hi, m);
    }
    t.check (lo < 0.2 * hi, "TREMOLO full depth chops (min/max " + std::to_string (lo / hi) + ")");
}

void testEngine (Runner& t, double sr)
{
    Engine engine;
    engine.prepare (sr, 256);

    // MIX at 0 is exactly the dry input.
    {
        EngineParams p;
        p.primary[kMix] = 0.0f;
        auto in = guitar (sr, 2.0, 0.2f);
        auto out = in;
        engine.reset();
        runEngine (engine, out, p);
        double err = 0.0;
        for (size_t i = 0; i < in.frames(); ++i) err = std::max (err, (double) std::abs (out.channels[0][i] - in.channels[0][i]));
        t.check (err < 1e-6, "engine MIX 0 = dry (err " + std::to_string (err) + ")");
    }
    // Bypass is exactly the input.
    {
        EngineParams p;
        p.bypassed = true;
        auto in = guitar (sr, 2.0, 0.2f);
        auto out = in;
        engine.reset();
        runEngine (engine, out, p);
        double err = 0.0;
        for (size_t i = 0; i < in.frames(); ++i) err = std::max (err, (double) std::abs (out.channels[1][i] - in.channels[1][i]));
        t.check (err < 1e-6, "engine bypass = input (err " + std::to_string (err) + ")");
    }
    // All modules OFF: the wet path is transparent at normal levels.
    {
        EngineParams p;
        p.moduleOn = { false, false, false, false };
        auto in = guitar (sr, 2.0, 0.1f);
        auto out = in;
        engine.reset();
        runEngine (engine, out, p);
        double err = 0.0;
        for (size_t i = (size_t) (0.05 * sr); i < in.frames(); ++i) err = std::max (err, (double) std::abs (out.channels[0][i] - in.channels[0][i]));
        t.check (err < 1e-4, "engine with all modules OFF is transparent (err " + std::to_string (err) + ")");
    }
    // Trails: a delay keeps ringing after bypass in BufferedTrails mode, stops in Buffered.
    for (auto mode : { BypassMode::Buffered, BypassMode::BufferedTrails })
    {
        EngineParams p = soloModule (2, 0);
        p.primary[kTime] = 0.6f;
        p.primary[kDiffusionAmount] = 0.7f;
        p.bypassMode = mode;
        engine.reset();
        auto a = guitar (sr, 2.0, 0.2f);
        runEngine (engine, a, p);
        p.bypassed = true;
        auto silence = impulse (sr, 1.5, 0.0f);
        runEngine (engine, silence, p);
        const double tail = stats (silence, (size_t) (0.05 * sr)).rms;
        if (mode == BypassMode::BufferedTrails)
            t.check (tail > 1e-4, "bypass with trails keeps the echoes ringing");
        else
            t.check (tail < 1e-6, "buffered bypass stops the echoes immediately");
    }
    // Every module order works.
    {
        int bad = 0;
        std::array<int, 4> order { 0, 1, 2, 3 };
        do
        {
            EngineParams p;
            p.order = order;
            engine.reset();
            auto a = guitar (sr, 0.5, 0.2f);
            runEngine (engine, a, p);
            bad += stats (a).finite ? 0 : 1;
        } while (std::next_permutation (order.begin(), order.end()));
        t.check (bad == 0, "all 24 module orders produce finite audio");
    }
    // GESTURE: a recorded MIX sweep loops back.
    {
        EngineParams p;
        engine.reset();
        auto silence = impulse (sr, 0.1, 0.0f);
        runEngine (engine, silence, p);
        engine.gestureBeginRecording();
        const int steps = 100;
        for (int i = 0; i <= steps; ++i)
        {
            // Host-sized blocks that do not line up with the 100 Hz gesture ticks: the lane must
            // still end exactly where the knob was left, or it would delete itself.
            p.primary[kMix] = (float) i / (float) steps;
            auto chunk = impulse (sr, 256.0 / sr, 0.0f);
            runEngine (engine, chunk, p, 256);
        }
        engine.gestureEndRecording();
        // Knob stays put; gesture plays the sweep again.
        float minV = 1.0f, maxV = 0.0f;
        for (int i = 0; i < 2 * steps; ++i)
        {
            auto chunk = impulse (sr, 0.01, 0.0f);
            runEngine (engine, chunk, p, 32);
            const float v = engine.getEffectivePrimary (kMix);
            minV = std::min (minV, v); maxV = std::max (maxV, v);
        }
        t.check (engine.gestureHasLane (kMix) && minV < 0.2f && maxV > 0.8f, "GESTURE replays the recorded MIX sweep");
        // Moving the knob deletes the lane.
        p.primary[kMix] = 0.3f;
        auto chunk = impulse (sr, 0.02, 0.0f);
        runEngine (engine, chunk, p, 32);
        t.check (! engine.gestureHasLane (kMix), "moving a recorded knob deletes its GESTURE lane");
    }
    // CAPTURE: long hold -> looper, short hold -> sustainer.
    for (auto [seconds, mode] : { std::pair { 2.0, Capture::Mode::Loop }, std::pair { 0.5, Capture::Mode::Sustain } })
    {
        EngineParams p;
        p.moduleOn = { false, false, false, false };
        engine.reset();
        engine.captureArm();
        auto rec = guitar (sr, seconds, 0.2f);
        runEngine (engine, rec, p);
        engine.captureCommit();
        auto play = impulse (sr, 3.0, 0.0f);
        runEngine (engine, play, p);
        t.check (engine.getCaptureMode() == mode && stats (play).rms > 1e-3,
                 std::string ("CAPTURE ") + (mode == Capture::Mode::Loop ? "loop" : "sustain") + " plays back");
        engine.captureStop();
        auto after = impulse (sr, 0.5, 0.0f);
        runEngine (engine, after, p);
        t.check (stats (after, (size_t) (0.1 * sr)).rms < 1e-6, "CAPTURE stop silences the loop");
    }
    // Auto calibration picks the level that brings the peaks to ~-6 dBFS.
    for (auto [peakDb, expected] : { std::pair { -18.0f, 1 }, std::pair { -6.0f, 3 }, std::pair { -24.0f, 0 } })
    {
        EngineParams p;
        engine.reset();
        engine.startAutoCalibration();
        auto a = guitar (sr, 4.0, dbToGain (peakDb));
        runEngine (engine, a, p);
        const int level = engine.takeAutoCalibrationResult();
        t.check (level == expected, "auto calibration at " + std::to_string ((int) peakDb) + " dBFS -> " + std::to_string (level) + " (expected " + std::to_string (expected) + ")");
    }
    // CPU with a heavy chain.
    {
        EngineParams p;
        p.effect = { 3, 4, 2, 4 };   // HOWL, PITCH, SPACE, INTERFERENCE
        p.primary = { 0.5f, 0.7f, 0.8f, 0.8f, 0.9f, 0.6f, 1.0f, 0.8f };
        p.driftMovement = p.driftDiffusion = 0.7f;
        engine.reset();
        auto a = guitar (sr, 10.0, 0.2f);
        const double cpu = runEngine (engine, a, p);
        std::printf ("  heavy chain (HOWL > PITCH > SPACE > INTERFERENCE) CPU: %.2f%% of one core\n", 100.0 * cpu / 10.0);
        t.check (stats (a).finite, "heavy chain finite");
    }
}

/** Renders a factory preset over the test guitar (MEDIUM calibration, -18 dBFS peaks). */
wav::Audio renderPreset (const FactoryPreset& preset, double sr, double& levelChangeDb)
{
    Engine engine;
    engine.prepare (sr, 256);
    const auto in = guitar (sr, 8.0, 0.125f);
    auto out = in;
    runEngine (engine, out, toEngineParams (preset));
    const size_t from = (size_t) (0.5 * sr);
    levelChangeDb = db (stats (out, from).rms / stats (in, from).rms);
    return out;
}

void testFactoryPresets (Runner& t, double sr)
{
    std::printf ("\n[factory presets @ %.0f Hz]  level change vs. dry, output peak\n", sr);
    for (const auto& preset : factoryPresets())
    {
        double change = 0.0;
        const auto out = renderPreset (preset, sr, change);
        const auto s = stats (out);
        std::printf ("  %-24s %+5.1f dB   peak %.3f\n", preset.name, change, s.peak);
        t.check (s.finite && s.peak < 0.9, std::string (preset.name) + " finite, no clipping");
        t.check (std::abs (change) < 3.0, std::string (preset.name) + " within 3 dB of the dry level");
    }
}

int runTests()
{
    enableFlushToZero();
    Runner t;
    std::printf ("[oversampler]\n");
    testOversampler (t);
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        testEffectsBounded (t, sr);
    for (double sr : { 48000.0, 96000.0 })
    {
        std::printf ("\n[specific behaviour @ %.0f Hz]\n", sr);
        testSpecifics (t, sr);
        std::printf ("\n[engine @ %.0f Hz]\n", sr);
        testEngine (t, sr);
        testFactoryPresets (t, sr);
    }
    std::printf ("\n%d/%d checks passed\n", t.checks - t.failures, t.checks);
    return t.failures == 0 ? 0 : 1;
}

// ---- demo / render -----------------------------------------------------------------------------

int runDemo (const std::string& dir, double sr)
{
    enableFlushToZero();
    std::filesystem::create_directories (dir);
    Engine engine;
    engine.prepare (sr, 256);
    auto dry = guitar (sr, 8.0, 0.125f);   // -18 dBFS peaks: a typical DI at MEDIUM calibration
    wav::write (dir + "/00_dry.wav", dry);

    for (int m = 0; m < kNumModules; ++m)
        for (int e = 0; e < kEffectsPerModule; ++e)
            for (auto [amount, drift] : { std::pair { 0.25f, 0.0f }, std::pair { 0.6f, 0.0f }, std::pair { 0.9f, 0.6f } })
            {
                if (m != 1 && m != 2 && drift > 0.0f) drift = 0.0f;
                EngineParams p = soloModule (m, e);
                setColumn (p, m, 0.5f, amount, drift);
                if (m == 3 && e == 0) p.primary[kTextureAmount] = amount < 0.5f ? amount : 1.0f - (amount - 0.5f);
                engine.reset();
                auto a = dry;
                runEngine (engine, a, p);
                char name[128];
                std::snprintf (name, sizeof name, "%s/%d%d_%s_%s_a%02d_d%02d.wav", dir.c_str(), m + 1, e + 1, kModuleKeys[m],
                               kEffectNames[m][e], (int) (amount * 100), (int) (drift * 100));
                wav::write (name, a);
            }
    std::printf ("wrote demo renders to %s\n", dir.c_str());
    return 0;
}

int runRender (int argc, char** argv)
{
    if (argc < 4)
    {
        std::fprintf (stderr, "usage: warmful_render render in.wav out.wav [--char N|off] [--move N|off] [--diff N|off] [--tex N|off]\n"
                              "       [--tilt v] [--char-amount v] [--rate v] [--move-amount v] [--time v] [--diff-amount v] [--mix v] [--tex-amount v]\n"
                              "       [--sens v] [--drift-move v] [--drift-diff v] [--out v] [--cal 0-3] [--bpm v --sync] [--order 0123]\n"
                              "       [--filter lpf|tilt|hpf] [--preset N]   (knobs are 0..1 like the pedal; effects default to off;\n"
                              "       --preset loads factory preset N (1-based) first, later options override it)\n");
        return 2;
    }
    enableFlushToZero();
    wav::Audio a;
    std::string error;
    if (! wav::read (argv[2], a, error)) { std::fprintf (stderr, "%s\n", error.c_str()); return 1; }
    if (a.channels.size() == 1) a.channels.push_back (a.channels[0]);
    a.channels.resize (2);

    EngineParams p;
    p.moduleOn = { false, false, false, false };
    const std::map<std::string, int> knobArgs { { "--tilt", kTilt }, { "--char-amount", kCharacterAmount }, { "--rate", kRate },
                                                { "--move-amount", kMovementAmount }, { "--time", kTime }, { "--diff-amount", kDiffusionAmount },
                                                { "--mix", kMix }, { "--tex-amount", kTextureAmount } };
    for (int i = 4; i < argc; ++i)
    {
        const std::string key = argv[i];
        const std::string val = i + 1 < argc ? argv[i + 1] : "";
        auto f = [&] { ++i; return std::stof (val); };
        bool matched = false;
        for (int m = 0; m < 4; ++m)
            if (key == std::string ("--") + kModuleKeys[m])
            {
                ++i;
                matched = true;
                if (val != "off") { p.moduleOn[(size_t) m] = true; p.effect[(size_t) m] = std::clamp (std::stoi (val), 0, 4); }
            }
        if (matched) continue;
        if (auto it = knobArgs.find (key); it != knobArgs.end()) p.primary[(size_t) it->second] = f();
        else if (key == "--sens") p.sensitivity = f();
        else if (key == "--drift-move") p.driftMovement = f();
        else if (key == "--drift-diff") p.driftDiffusion = f();
        else if (key == "--out") p.outputLevel = f();
        else if (key == "--cal") p.calibrationLevel = (int) f();
        else if (key == "--bpm") p.tempo.bpm = f();
        else if (key == "--sync") p.tempo.synced = true;
        else if (key == "--order") { ++i; for (size_t k = 0; k < 4 && k < val.size(); ++k) p.order[k] = val[k] - '0'; }
        else if (key == "--filter") { ++i; p.filterStyle = val == "lpf" ? FilterStyle::LowPass : (val == "hpf" ? FilterStyle::HighPass : FilterStyle::Tilt); }
        else if (key == "--preset")
        {
            ++i;
            const auto presets = factoryPresets();
            const int index = std::stoi (val) - 1;
            if (index < 0 || index >= (int) presets.size()) { std::fprintf (stderr, "--preset must be 1..%zu\n", presets.size()); return 2; }
            p = toEngineParams (presets[(size_t) index], p);
        }
        else { std::fprintf (stderr, "unknown option %s\n", key.c_str()); return 2; }
    }

    Engine engine;
    engine.prepare (a.sampleRate, 256);
    runEngine (engine, a, p);
    if (! wav::write (argv[3], a)) { std::fprintf (stderr, "cannot write %s\n", argv[3]); return 1; }
    return 0;
}

/** Knob position as a clock face: 7:00 = fully left, 12:00 = centre, 5:00 = fully right. */
std::string clock (float v)
{
    const int minutes = (int) std::lround ((7.0 + 10.0 * (double) clamp (v, 0.0f, 1.0f)) * 60.0 / 15.0) * 15;
    const int h = (minutes / 60 - 1) % 12 + 1;
    char text[16];
    std::snprintf (text, sizeof text, "%d:%02d", h, minutes % 60);
    return text;
}

/** Markdown description of the factory bank, for the plug-in and for dialling it on the pedal. */
int printPresetsMarkdown()
{
    constexpr const char* modules[] { "CHARACTER", "MOVEMENT", "DIFFUSION", "TEXTURE" };
    constexpr const char* filterStyles[] { "Low-Pass", "Tilt", "High-Pass" };
    std::printf ("# WARMFUL factory presets\n\n"
                 "Generated by `warmful_render presets-md > docs/PRESETS.md` from `source/dsp/FactoryPresets.cpp`.\n\n"
                 "All presets assume **MEDIUM** calibration and MIX at 100 %%. Values are pot positions in %% and as a\n"
                 "clock face (7:00 = fully left, 12:00 = centre, 5:00 = fully right), so they can also be dialled on a\n"
                 "physical pedal; the plug-in is a behavioural model, so expect to fine-tune by ear there.\n"
                 "Secondary controls are set in the SECONDARY menu (A+B); routing in FX SETUP (A+D).\n");
    const auto presets = factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        const auto& f = presets[i];
        std::printf ("\n## %zu. %s\n\n%s\n\nRouting: ", i + 1, f.name, f.description);
        for (int s = 0; s < kNumModules; ++s)
            std::printf ("%s%s", modules[f.order[(size_t) s]], s < kNumModules - 1 ? " → " : "\n\n");
        std::printf ("| Module | Effect | Top knob | AMOUNT | Secondary |\n|---|---|---|---|---|\n");
        for (int m = 0; m < kNumModules; ++m)
        {
            const auto pm = (size_t) m;
            std::string effect = kEffectNames[m][f.effect[pm]];
            for (auto& ch : effect) ch = (char) std::toupper ((unsigned char) ch);
            if (! f.moduleOn[pm]) effect += " (OFF)";
            if (m == 3 && f.effect[pm] == 0) effect += std::string (" (") + filterStyles[(int) f.filterStyle] + ")";
            const float top = f.primary[(size_t) (2 * m)];
            const float amount = f.primary[(size_t) (2 * m + 1)];
            constexpr const char* topNames[] { "TILT", "RATE", "TIME", "MIX" };
            constexpr const char* secNames[] { "SENSITIVITY", "DRIFT", "DRIFT", "OUTPUT LEVEL" };
            const float sec = f.secondary[(size_t) (2 * m)], vol = f.secondary[(size_t) (2 * m + 1)];
            std::printf ("| %s | %s | %s %d%% (%s) | %d%% (%s) | %s %d%% (%s), EFFECT VOL %d%% (%s) |\n",
                         modules[m], effect.c_str(), topNames[m], (int) std::lround (top * 100), clock (top).c_str(),
                         (int) std::lround (amount * 100), clock (amount).c_str(), secNames[m], (int) std::lround (sec * 100),
                         clock (sec).c_str(), (int) std::lround (vol * 100), clock (vol).c_str());
        }
    }
    return 0;
}

} // namespace

int main (int argc, char** argv)
{
    const std::string cmd = argc > 1 ? argv[1] : "";
    if (cmd == "test") return runTests();
    if (cmd == "demo" && argc > 2)
    {
        double sr = 48000.0;
        for (int i = 3; i + 1 < argc; ++i)
            if (std::string (argv[i]) == "--sr") sr = std::stod (argv[i + 1]);
        return runDemo (argv[2], sr);
    }
    if (cmd == "render") return runRender (argc, argv);
    if (cmd == "presets-md") return printPresetsMarkdown();
    if (cmd == "presets" && argc > 2)
    {
        enableFlushToZero();
        std::filesystem::create_directories (argv[2]);
        const auto presets = factoryPresets();
        for (size_t i = 0; i < presets.size(); ++i)
        {
            double change = 0.0;
            const auto out = renderPreset (presets[i], 48000.0, change);
            std::string name = presets[i].name;
            std::replace (name.begin(), name.end(), ' ', '_');
            wav::write (std::string (argv[2]) + "/" + std::to_string (i + 1) + "_" + name + ".wav", out);
            std::printf ("%2zu  %-24s %+5.1f dB\n", i + 1, presets[i].name, change);
        }
        return 0;
    }
    std::fprintf (stderr, "usage: warmful_render test | demo <dir> [--sr N] | presets <dir> | presets-md | render <in.wav> <out.wav> [options]\n");
    return 2;
}
