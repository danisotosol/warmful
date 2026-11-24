// WARMFUL — preset bank storage (XML file in the user's application data folder).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "PresetBank.h"

namespace warmful
{

PresetBank::PresetBank (juce::File storage) : file (std::move (storage))
{
    if (file == juce::File())
        file = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("WARMFUL")
                   .getChildFile ("user_presets.xml");
    reload();
}

void PresetBank::reload()
{
    for (auto& s : slots)
        s = {};
    if (auto xml = juce::XmlDocument::parse (file))
    {
        const auto bank = juce::ValueTree::fromXml (*xml);
        for (const auto& child : bank)
        {
            const int slot = child.getProperty ("slot", -1);
            if (slot >= 0 && slot < kNumSlots && child.getNumChildren() > 0)
                slots[(size_t) slot] = child.getChild (0).createCopy();
        }
    }
}

void PresetBank::save() const
{
    juce::ValueTree bank ("WARMFUL_PRESETS");
    for (int i = 0; i < kNumSlots; ++i)
        if (slots[(size_t) i].isValid())
        {
            juce::ValueTree entry ("SLOT");
            entry.setProperty ("slot", i, nullptr);
            entry.appendChild (slots[(size_t) i].createCopy(), nullptr);
            bank.appendChild (entry, nullptr);
        }
    file.getParentDirectory().createDirectory();
    if (auto xml = bank.createXml())
        xml->writeTo (file);
}

bool PresetBank::has (int slot) const
{
    return slot >= 0 && slot < kNumSlots && slots[(size_t) slot].isValid();
}

juce::ValueTree PresetBank::get (int slot) const
{
    return has (slot) ? slots[(size_t) slot] : juce::ValueTree();
}

void PresetBank::put (int slot, const juce::ValueTree& preset)
{
    if (slot < 0 || slot >= kNumSlots)
        return;
    slots[(size_t) slot] = preset.createCopy();
    save();
}

void PresetBank::clear (int slot)
{
    if (slot < 0 || slot >= kNumSlots)
        return;
    slots[(size_t) slot] = {};
    save();
}

juce::String PresetBank::slotName (int slot)
{
    const char bank = (char) ('A' + slot / kSlotsPerBank);
    return juce::String::charToString (bank) + "-" + juce::String (slot % kSlotsPerBank + 1).paddedLeft ('0', 2);
}

} // namespace warmful
