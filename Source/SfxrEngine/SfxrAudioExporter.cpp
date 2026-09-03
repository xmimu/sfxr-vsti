#include "SfxrAudioExporter.h"

namespace
{
    constexpr int kRootNote = 69;
    constexpr int kRenderBlockSize = 512;
    constexpr double kMaximumOneShotSeconds = 30.0;
}

juce::AudioBuffer<float> SfxrAudioExporter::renderPreview (const SfxrParams& params, bool mono,
                                                            bool oneShot, int sampleRate,
                                                            double sustainDurationSeconds)
{
    SfxrEngine engine;
    engine.prepare ((double) sampleRate, kRenderBlockSize);
    engine.setParams (params);
    engine.setMono (mono);
    engine.setOneShot (oneShot);
    engine.noteOn (kRootNote, 1.0f);

    const int maxSamples = juce::roundToInt ((oneShot ? kMaximumOneShotSeconds : sustainDurationSeconds)
                                               * (double) sampleRate);
    juce::AudioBuffer<float> result (1, maxSamples);
    juce::AudioBuffer<float> block (1, kRenderBlockSize);
    int rendered = 0;

    while (rendered < maxSamples)
    {
        const int count = juce::jmin (kRenderBlockSize, maxSamples - rendered);
        block.clear();
        engine.render (block, 0, count);
        result.copyFrom (0, rendered, block, 0, 0, count);
        rendered += count;

        if (oneShot && ! engine.hasActiveVoices())
            break;
    }

    result.setSize (1, rendered, true, false, true);
    return result;
}

bool SfxrAudioExporter::writeFile (const juce::File& target,
                                   const juce::AudioBuffer<float>& audio,
                                   const Options& options, juce::String& errorMessage)
{
    if (audio.getNumChannels() != 1 || audio.getNumSamples() == 0)
    {
        errorMessage = "There is no audio to export.";
        return false;
    }

    if (target.existsAsFile() && ! target.deleteFile())
    {
        errorMessage = "Could not replace the existing output file.";
        return false;
    }

    std::unique_ptr<juce::OutputStream> stream = target.createOutputStream();
    if (stream == nullptr)
    {
        errorMessage = "Could not create the output file.";
        return false;
    }

    juce::AudioFormatWriterOptions writerOptions;
    writerOptions = writerOptions.withSampleRate ((double) options.sampleRate)
                                 .withNumChannels (1);
    std::unique_ptr<juce::AudioFormatWriter> writer;

    if (options.format == Format::wav)
    {
        if (options.wavBitDepth != 16 && options.wavBitDepth != 24 && options.wavBitDepth != 32)
        {
            errorMessage = "Unsupported WAV bit depth.";
            return false;
        }

        writerOptions = writerOptions.withBitsPerSample (options.wavBitDepth)
                                     .withSampleFormat (options.wavBitDepth == 32
                                                            ? juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
                                                            : juce::AudioFormatWriterOptions::SampleFormat::integral);
        juce::WavAudioFormat format;
        writer = format.createWriterFor (stream, writerOptions);
    }
    else
    {
        juce::OggVorbisAudioFormat format;
        if (! juce::isPositiveAndBelow (options.oggQualityIndex, format.getQualityOptions().size()))
        {
            errorMessage = "Unsupported OGG quality setting.";
            return false;
        }

        writerOptions = writerOptions.withBitsPerSample (32)
                                     .withQualityOptionIndex (options.oggQualityIndex);
        writer = format.createWriterFor (stream, writerOptions);
    }

    if (writer == nullptr)
    {
        errorMessage = "Could not initialise the audio encoder.";
        return false;
    }

    if (! writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()))
    {
        errorMessage = "Could not write the audio data.";
        return false;
    }

    return true;
}
