#pragma once

#include <JuceHeader.h>

class SfxrVstiAudioProcessor;
enum class PresetCategory;

namespace SfxrAudioExporter
{
    struct Options;
}

// Pure action layer between the editor view and the plugin: randomise/mutate/
// category generation, .sfs file I/O and audio export all translate into calls
// on the processor or the engine modules. Async UI flow (file choosers, dialogs)
// stays in the editor, which is the natural lifetime anchor for those.
class SfxrEditorActions
{
public:
    explicit SfxrEditorActions (SfxrVstiAudioProcessor&);

    void randomize();
    void mutate();
    void generate (PresetCategory);

    bool loadFromFile (const juce::File&);
    bool saveToFile (const juce::File&);

    bool exportSound (const juce::File&, const SfxrAudioExporter::Options&, juce::String& errorMessage);

private:
    SfxrVstiAudioProcessor& processor;
    juce::Random rng;
};
