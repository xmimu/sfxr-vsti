#pragma once

#include <JuceHeader.h>
#include "SfxrParams.h"

// Loads/saves sfxr .sfs parameter files, byte-compatible with the original.
// Only version 102 is supported -- the only version sfxr 1.2.1 writes; older
// 100/101 archives are rejected on load.
namespace SfxrPresetFile
{
    bool load (const juce::File& file, SfxrParams& p);
    bool save (const juce::File& file, const SfxrParams& p);
}
