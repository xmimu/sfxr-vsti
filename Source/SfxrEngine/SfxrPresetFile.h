#pragma once

#include <JuceHeader.h>
#include "SfxrParams.h"

// Loads/saves sfxr .sfs parameter files, byte-compatible with the original
// (versions 100/101/102 written by sfxr 1.2.1).
namespace SfxrPresetFile
{
    bool load (const juce::File& file, SfxrParams& p);
    bool save (const juce::File& file, const SfxrParams& p);
}
