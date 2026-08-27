#pragma once

#include <JuceHeader.h>
#include "SfxrEngine/SfxrParams.h"
#include "SfxrEngine/SfxrEngine.h"
#include "SfxrEngine/SfxrPresets.h"
#include "SfxrEngine/SfxrPresetFile.h"

// Valid MIDI note range for triggering sounds (matches the on-screen keyboard's
// enabled keys; notes outside this range are ignored).
namespace SfxrNoteRange
{
    constexpr int minNote = 36; // C2
    constexpr int maxNote = 84; // C6

    inline bool contains (int midiNoteNumber) noexcept
    {
        return midiNoteNumber >= minNote && midiNoteNumber <= maxNote;
    }
}

class SfxrVstiAudioProcessor : public juce::AudioProcessor,
                               public juce::MidiKeyboardStateListener,
                               private juce::Timer
{
public:
    SfxrVstiAudioProcessor();
    ~SfxrVstiAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    SfxrEngine& getEngine() { return engine; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Reads the current parameter values into an SfxrParams struct.
    SfxrParams readParams() const;

    // Reads a toggle parameter (avoids an implicit float -> bool conversion).
    bool readBoolParam (const juce::String& id) const;

    // Pushes an SfxrParams struct into the parameter tree (for preset buttons).
    void applyParams (const SfxrParams& p);

    // Used by the editor's PLAY button.
    void playPreview();

    // ---- oscilloscope (lock-free, audio -> GUI) ----
    void pushScope (const float* data, int numSamples);
    int readScope (float* dest, int maxSamples);

    // ---- MidiKeyboardStateListener ----
    void handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float velocity) override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState apvts;
    SfxrEngine engine;
    juce::MidiKeyboardState keyboardState;

    static constexpr int scopeSize = 16384;
    juce::AbstractFifo scopeFifo { scopeSize };
    std::vector<float> scopeData = std::vector<float> (scopeSize, 0.0f);

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVstiAudioProcessor)
};
