// WARMFUL — CAPTURE implementation.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Capture.h"

namespace warmful
{

namespace
{
    constexpr float kSustainMaxSeconds = 1.2f;  // shorter captures play as an overlapping pad
    constexpr float kMinSeconds = 0.03f;        // anything shorter is discarded
    constexpr float kSeamSeconds = 0.02f;       // loop seam crossfade
    constexpr float kFadeSeconds = 0.03f;       // start/stop fades
}

void Capture::prepare (double sr)
{
    sampleRate = sr;
    capacity = (int) (kMaxSeconds * sr);
    bufL.assign ((size_t) capacity, 0.0f);
    bufR.assign ((size_t) capacity, 0.0f);
    const int tailLen = (int) (kFadeSeconds * sr) + 1;
    tailL.assign ((size_t) tailLen, 0.0f);
    tailR.assign ((size_t) tailLen, 0.0f);
    gainStep = 1.0f / std::max (1.0f, kFadeSeconds * (float) sr);
    reset();
}

void Capture::reset() noexcept
{
    mode = Mode::Off;
    length = playPos = seam = 0;
    outGain = outTarget = 0.0f;
    fadingToStop = false;
    tailPos = tailLength = 0;
}

void Capture::renderStopTail() noexcept
{
    // The new recording overwrites the buffer, so pre-render the fade-out of the old loop.
    tailLength = (int) tailL.size();
    for (int i = 0; i < tailLength; ++i)
    {
        const float fade = outGain * (1.0f - (float) i / (float) tailLength);
        const bool sustain = mode == Mode::Sustain;
        tailL[(size_t) i] = (sustain ? readSustain (bufL) : readLoop (bufL)) * fade;
        tailR[(size_t) i] = (sustain ? readSustain (bufR) : readLoop (bufR)) * fade;
        advance();
    }
    tailPos = 0;
}

void Capture::arm() noexcept
{
    if (isPlaying())
        renderStopTail();
    mode = Mode::Recording;
    length = 0;
    outGain = outTarget = 0.0f;
    fadingToStop = false;
}

void Capture::cancel() noexcept
{
    if (mode == Mode::Recording)
    {
        mode = Mode::Off;
        length = 0;
    }
}

void Capture::commit() noexcept
{
    if (mode != Mode::Recording)
        return;
    if ((float) length < kMinSeconds * (float) sampleRate)
    {
        mode = Mode::Off;
        length = 0;
        return;
    }
    const bool sustain = (float) length < kSustainMaxSeconds * (float) sampleRate;
    mode = sustain ? Mode::Sustain : Mode::Loop;
    seam = sustain ? 0 : std::min ((int) (kSeamSeconds * sampleRate), length / 4);
    playPos = 0;
    outGain = 0.0f;
    outTarget = 1.0f;
    fadingToStop = false;
}

void Capture::stop() noexcept
{
    if (mode == Mode::Recording)
    {
        mode = Mode::Off;
        length = 0;
    }
    else if (isPlaying())
    {
        outTarget = 0.0f;
        fadingToStop = true;
    }
}

float Capture::readSustain (const std::vector<float>& buf) const noexcept
{
    // Two voices half a capture apart, each Hann-windowed over the whole capture: sums to unity.
    const int p1 = playPos;
    const int p2 = (playPos + length / 2) % length;
    const float w1 = std::sin (kPi * (float) p1 / (float) length);
    const float w2 = std::sin (kPi * (float) p2 / (float) length);
    return buf[(size_t) p1] * w1 * w1 + buf[(size_t) p2] * w2 * w2;
}

float Capture::readLoop (const std::vector<float>& buf) const noexcept
{
    const float head = buf[(size_t) playPos];
    if (playPos >= seam)
        return head;
    const float t = (float) playPos / (float) seam;
    const float in = 0.5f - 0.5f * std::cos (kPi * t);
    return head * in + buf[(size_t) (length - seam + playPos)] * (1.0f - in);
}

void Capture::advance() noexcept
{
    const int loopLength = length - seam;
    if (++playPos >= loopLength)
        playPos = 0;
}

void Capture::process (float* l, float* r, int n) noexcept
{
    for (int i = 0; i < n; ++i)
    {
        const float inL = l[i], inR = r[i];
        float addL = 0.0f, addR = 0.0f;

        if (tailPos < tailLength)
        {
            addL += tailL[(size_t) tailPos];
            addR += tailR[(size_t) tailPos];
            ++tailPos;
        }

        if (mode == Mode::Recording)
        {
            bufL[(size_t) length] = inL;
            bufR[(size_t) length] = inR;
            if (++length >= capacity)
                commit();   // 30 s limit reached: start looping
        }
        else if (isPlaying())
        {
            if (outGain < outTarget)
                outGain = std::min (outTarget, outGain + gainStep);
            else if (outGain > outTarget)
                outGain = std::max (outTarget, outGain - gainStep);

            const bool sustain = mode == Mode::Sustain;
            addL += (sustain ? readSustain (bufL) : readLoop (bufL)) * outGain;
            addR += (sustain ? readSustain (bufR) : readLoop (bufR)) * outGain;
            advance();

            if (fadingToStop && outGain <= 0.0f)
            {
                mode = Mode::Off;
                fadingToStop = false;
            }
        }

        l[i] = inL + addL;
        r[i] = inR + addR;
    }
}

} // namespace warmful
