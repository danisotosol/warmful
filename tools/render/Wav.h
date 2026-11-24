// WARMFUL — minimal WAV reader/writer for the offline tools (PCM 16/24/32, float 32).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace wav
{

struct Audio
{
    double sampleRate = 48000.0;
    std::vector<std::vector<float>> channels;   // channels[c][i]
    size_t frames() const { return channels.empty() ? 0 : channels[0].size(); }
};

inline bool read (const std::string& path, Audio& out, std::string& error)
{
    std::ifstream f (path, std::ios::binary);
    if (! f) { error = "cannot open " + path; return false; }
    std::vector<char> data ((std::istreambuf_iterator<char> (f)), {});
    auto u16 = [&] (size_t o) { uint16_t v; std::memcpy (&v, &data[o], 2); return v; };
    auto u32 = [&] (size_t o) { uint32_t v; std::memcpy (&v, &data[o], 4); return v; };
    if (data.size() < 12 || std::memcmp (data.data(), "RIFF", 4) != 0 || std::memcmp (&data[8], "WAVE", 4) != 0)
    { error = "not a RIFF/WAVE file"; return false; }

    int format = 0, numChannels = 0, bits = 0;
    size_t pos = 12, dataOffset = 0, dataSize = 0;
    while (pos + 8 <= data.size())
    {
        const uint32_t size = u32 (pos + 4);
        if (std::memcmp (&data[pos], "fmt ", 4) == 0)
        {
            format = u16 (pos + 8);
            numChannels = u16 (pos + 10);
            out.sampleRate = u32 (pos + 12);
            bits = u16 (pos + 22);
            if (format == 0xFFFE && size >= 40)
                format = u16 (pos + 32);   // WAVE_FORMAT_EXTENSIBLE sub-format
        }
        else if (std::memcmp (&data[pos], "data", 4) == 0)
        {
            dataOffset = pos + 8;
            dataSize = std::min<size_t> (size, data.size() - dataOffset);
        }
        pos += 8 + size + (size & 1);
    }
    if (numChannels <= 0 || dataOffset == 0 || ! ((format == 1 && (bits == 16 || bits == 24 || bits == 32)) || (format == 3 && bits == 32)))
    { error = "unsupported WAV format (need PCM 16/24/32 or float 32)"; return false; }

    const size_t bytes = (size_t) bits / 8;
    const size_t frames = dataSize / (bytes * (size_t) numChannels);
    out.channels.assign ((size_t) numChannels, std::vector<float> (frames));
    for (size_t i = 0; i < frames; ++i)
        for (size_t c = 0; c < (size_t) numChannels; ++c)
        {
            const char* p = &data[dataOffset + (i * (size_t) numChannels + c) * bytes];
            float v = 0.0f;
            if (format == 3) std::memcpy (&v, p, 4);
            else if (bits == 16) { int16_t s; std::memcpy (&s, p, 2); v = (float) s / 32768.0f; }
            else if (bits == 24)
            {
                int32_t s = (int32_t) ((uint32_t) (uint8_t) p[0] << 8 | (uint32_t) (uint8_t) p[1] << 16 | (uint32_t) (uint8_t) p[2] << 24) >> 8;
                v = (float) s / 8388608.0f;
            }
            else { int32_t s; std::memcpy (&s, p, 4); v = (float) ((double) s / 2147483648.0); }
            out.channels[c][i] = v;
        }
    return true;
}

/** Writes 32-bit float WAV (no clipping of the analysis data). */
inline bool write (const std::string& path, const Audio& in)
{
    std::ofstream f (path, std::ios::binary);
    if (! f)
        return false;
    const uint16_t channels = (uint16_t) in.channels.size();
    const uint32_t frames = (uint32_t) in.frames();
    const uint32_t sr = (uint32_t) in.sampleRate;
    const uint32_t dataBytes = frames * channels * 4;
    auto w32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto w16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    f.write ("RIFF", 4); w32 (36 + dataBytes); f.write ("WAVE", 4);
    f.write ("fmt ", 4); w32 (16); w16 (3); w16 (channels); w32 (sr); w32 (sr * channels * 4); w16 ((uint16_t) (channels * 4)); w16 (32);
    f.write ("data", 4); w32 (dataBytes);
    for (uint32_t i = 0; i < frames; ++i)
        for (uint16_t c = 0; c < channels; ++c)
            f.write ((const char*) &in.channels[c][i], 4);
    return (bool) f;
}

} // namespace wav
