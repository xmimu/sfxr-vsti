#pragma once

#include <JuceHeader.h>
#include "SfxrEngine/SfxrParams.h"
#include "SfxrEngine/SfxrEngine.h"
#include "SfxrEngine/SfxrPresets.h"
#include "SfxrEngine/SfxrPresetFile.h"

// Notes that will actually trigger a sound. Pitch is transposed relative to the
// Start Frequency parameter, so this is a usable-range convention rather than a
// hard technical limit -- it matches the keys the on-screen keyboard enables, and
// notes outside it are ignored so that the UI and host MIDI behave identically.
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

    // Factory programs: an init sound plus one per sfxr generator category.
    // These are deterministic on purpose -- see setCurrentProgram().
    int getNumPrograms() override { return 1 + (int) PresetCategory::Count; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
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
    // withGesture wraps the writes in begin/endChangeGesture, which is what a
    // user-initiated change should do; a host program change should not.
    void applyParams (const SfxrParams& p, bool withGesture = true);

    // Used by the editor's PLAY button.
    void playPreview();

    // ---- oscilloscope (lock-free, audio -> GUI) ----
    void pushScope (const float* data, int numSamples);
    int readScope (float* dest, int maxSamples);

private:
    void timerCallback() override;

    // Applies one MIDI message to the engine. Audio thread only.
    void handleMidiEvent (const juce::MidiMessage& msg);

    // The note the PLAY SOUND button auditions: the root of the transposition.
    static constexpr int kPreviewNote = 69;

    juce::AudioProcessorValueTreeState apvts;
    int currentProgram = 0;
    SfxrEngine engine;
    juce::MidiKeyboardState keyboardState;

    static constexpr int scopeSize = 16384;
    juce::AbstractFifo scopeFifo { scopeSize };
    std::vector<float> scopeData = std::vector<float> (scopeSize, 0.0f);

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVstiAudioProcessor)
};
