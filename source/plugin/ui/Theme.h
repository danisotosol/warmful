// WARMFUL — colours and fonts of the pedal-style interface.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace warmful::theme
{

inline const juce::Colour enclosure { 0xffebe6da };
inline const juce::Colour enclosureEdge { 0xffcfc8b8 };
inline const juce::Colour panelLine { 0xffbdb5a3 };
inline const juce::Colour ink { 0xff2a2724 };
inline const juce::Colour inkSoft { 0xff8d877c };
inline const juce::Colour strip { 0xff1d1b1a };
inline const juce::Colour stripText { 0xffe8e2d4 };
inline const juce::Colour secondaryTeal { 0xff35c6b5 };
inline const juce::Colour gestureWhite { 0xfff4f1ea };
inline const juce::Colour recordRed { 0xffe5402f };

/** Knob cap colour per column (Character, Movement, Diffusion, Texture) and the MIX knob. */
inline const std::array<juce::Colour, 4> moduleColour { juce::Colour (0xffe8553f), juce::Colour (0xfff4b638),
                                                        juce::Colour (0xff43a85d), juce::Colour (0xff57bde8) };
inline const juce::Colour mixKnob { 0xfff2efe8 };

/** Effect LED colours, identical in every module: red, orange, green, blue, purple. */
inline const std::array<juce::Colour, 5> effectColour { juce::Colour (0xffe8463a), juce::Colour (0xfff29a2e),
                                                        juce::Colour (0xff4cb648), juce::Colour (0xff3fa2f0),
                                                        juce::Colour (0xff9a62d6) };

inline juce::Font font (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions {}.withHeight (height).withStyleFlags (bold ? juce::Font::bold : juce::Font::plain));
}

} // namespace warmful::theme
