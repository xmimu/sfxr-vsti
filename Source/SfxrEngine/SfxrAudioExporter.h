#pragma once

#include <JuceHeader.h>
#include "SfxrEngine.h"

namespace SfxrAudioExporter
{
    enum class Format { wav, ogg };

    struct Options
    {
        Format format = Format::wav;
        int sampleRate = 44100;
        int wavBitDepth = 24;
        int oggQualityIndex = 4; // 128 kbps
    };

    // Renders the root-note preview. One-shot sounds end naturally; sustained
    // sounds are intentionally bounded to keep the export operation finite.
    juce::AudioBuffer<float> renderPreview (const SfxrParams&, bool mono, bool oneShot,
                                            int sampleRate, double sustainDurationSeconds = 10.0);

    bool writeFile (const juce::File&, const juce::AudioBuffer<float>&, const Options&,
                    juce::String& errorMessage);
}
