#include "EditorActions.h"
#include "../PluginProcessor.h"
#include "../SfxrEngine/SfxrAudioExporter.h"
#include "../SfxrEngine/SfxrPresetFile.h"
#include "../SfxrEngine/SfxrPresets.h"

SfxrEditorActions::SfxrEditorActions (SfxrVstiAudioProcessor& p) : processor (p)
{
}

void SfxrEditorActions::randomize()
{
    SfxrParams p = processor.readParams();
    ::randomize (p, rng);
    processor.applyParams (p);
    processor.playPreview();
}

void SfxrEditorActions::mutate()
{
    SfxrParams p = processor.readParams();
    ::mutate (p, rng);
    processor.applyParams (p);
    processor.playPreview();
}

void SfxrEditorActions::generate (PresetCategory c)
{
    SfxrParams p = processor.readParams();
    generatePreset (p, c, rng);
    processor.applyParams (p);
    processor.playPreview();
}

bool SfxrEditorActions::loadFromFile (const juce::File& file)
{
    SfxrParams p;
    if (! SfxrPresetFile::load (file, p))
        return false;

    processor.applyParams (p);
    return true;
}

bool SfxrEditorActions::saveToFile (const juce::File& file)
{
    return SfxrPresetFile::save (file, processor.readParams());
}

bool SfxrEditorActions::exportSound (const juce::File& target, const SfxrAudioExporter::Options& options,
                                     juce::String& errorMessage)
{
    const auto params = processor.readParams();
    const bool mono = processor.readBoolParam (ParamID::mono);
    const bool oneShot = processor.readBoolParam (ParamID::one_shot);

    const auto audio = SfxrAudioExporter::renderPreview (params, mono, oneShot, options.sampleRate);
    return SfxrAudioExporter::writeFile (target, audio, options, errorMessage);
}
