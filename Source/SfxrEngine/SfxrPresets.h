#pragma once

#include <JuceHeader.h>
#include "SfxrParams.h"

// Preset generators ported from the original sfxr UI (the seven category
// buttons plus Randomize and Mutate in main.cpp).

enum class PresetCategory
{
    PickupCoin = 0,
    LaserShoot,
    Explosion,
    Powerup,
    HitHurt,
    Jump,
    BlipSelect,
    Count
};

const char* presetCategoryName (PresetCategory c);

void generatePreset (SfxrParams& p, PresetCategory c, juce::Random& r);
void randomize (SfxrParams& p, juce::Random& r);
void mutate (SfxrParams& p, juce::Random& r);
