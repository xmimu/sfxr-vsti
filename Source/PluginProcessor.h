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
    int getCurrentProgram() override { return currentProgram.load(); }
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

    // Returns a parameter snapshot that is consistent as a whole: if a preset
    // batch is being applied (or has just been committed) it returns the
    // committed snapshot instead of reading the tree param-by-param, so the
    // audio thread can never see a half-updated mix of old and new values.
    SfxrParams pullParamsForAudio();

    // Reads a toggle parameter (avoids an implicit float -> bool conversion).
    bool readBoolParam (const char* id) const;

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
    // Atomic: hosts may query/switch programs or restore state from a different
    // thread than the one that last wrote it.
    std::atomic<int> currentProgram { 0 };
    SfxrEngine engine;
    juce::MidiKeyboardState keyboardState;

    static constexpr int scopeSize = 16384;
    juce::AbstractFifo scopeFifo { scopeSize };
    std::vector<float> scopeData = std::vector<float> (scopeSize, 0.0f);

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // The number of continuous float parameters whose atomics are cached below.
    // Must equal the size of kParamSlots in PluginProcessor.cpp (asserted there).
    static constexpr int kNumFloatParams = 24;

    // Raw parameter values fetched once at construction. readParams() (called on
    // the audio thread every block) dereferences these directly instead of doing
    // a string-keyed lookup per parameter.
    std::array<std::atomic<float>*, kNumFloatParams> rawParams {};
    std::atomic<float>* rawWaveType   = nullptr;
    std::atomic<float>* rawMono       = nullptr;
    std::atomic<float>* rawOneShot    = nullptr;

    // ---- consistent preset snapshots (see pullParamsForAudio) ----
    // Triple-buffered so the UI thread can publish a whole SfxrParams without
    // ever writing a slot the audio thread may still be copying: the audio
    // thread reads committed[gen % 3] and only ever lags a commit or two, while
    // the UI thread only reuses a slot three generations later. `applying` tells
    // the audio thread not to read the tree while a batch is half-written.
    SfxrParams        committed[3];
    std::atomic<int>  committedGen { 0 };
    std::atomic<bool> applying     { false };
    SfxrParams        lastAudioParams;   // audio thread only
    int               lastSeenGen = 0;   // audio thread only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVstiAudioProcessor)
};
