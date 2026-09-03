#pragma once

#include <JuceHeader.h>

// Palette for the sfxr-styled GUI. Colours are the classic sfxr beige/orange
// scheme, kept bit-for-bit identical to the original UI. Each constant has
// internal linkage, so including this header from several translation units is
// fine (each TU gets its own const-initialised copy).
namespace SfxrTheme
{
    const juce::Colour kBg          (0xFFC0B090);
    const juce::Colour kTextDark    (0xFF504030);
    const juce::Colour kBarBg       (0xFF807060);
    const juce::Colour kBarFill     (0xFFF0C090);
    const juce::Colour kButtonBg    (0xFFE0D0B0);
    const juce::Colour kButtonHover (0xFFFFF0E0);
    const juce::Colour kDivider     (0xFF000000);
}
