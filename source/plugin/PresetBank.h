// WARMFUL — the pedal's 80 user preset slots (banks A–D × 20), shared by every instance.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <array>

namespace warmful
{

class PresetBank
{
public:
    static constexpr int kNumBanks = 4;
    static constexpr int kSlotsPerBank = 20;
    static constexpr int kNumSlots = kNumBanks * kSlotsPerBank;   // MIDI Program Change 0..79

    /** Uses the per-user application data folder unless a file is given (tests). */
    explicit PresetBank (juce::File storage = {});

    bool has (int slot) const;
    juce::ValueTree get (int slot) const;
    void put (int slot, const juce::ValueTree& preset);
    void clear (int slot);
    /** Reloads from disk (another instance may have saved). */
    void reload();

    static juce::String slotName (int slot);   // e.g. "A-03"

private:
    void save() const;

    juce::File file;
    std::array<juce::ValueTree, kNumSlots> slots;
};

} // namespace warmful
