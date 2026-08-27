#pragma once

#include <JuceHeader.h>
#include "SfxrVoice.h"

// Manages a fixed pool of voices and dispatches MIDI notes to them.
class SfxrEngine
{
public:
    SfxrEngine();

    // Must be called before process(). maxBlockSize sizes the internal mixing
    // buffer up front so that process() never allocates on the audio thread.
    void prepare (double sampleRate, int maxBlockSize);

    void setParams (const SfxrParams& p);
    void setMono (bool mono);
    void setOneShot (bool oneShot);

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    // Silences every voice immediately and drops all note mappings.
    void reset();

    // Renders all active voices into stereo output.
    void process (juce::AudioBuffer<float>& audio);

    static constexpr int kNumVoices = 8;

private:
    int findFreeVoiceIndex();

    // Drops the note -> voice and voice -> note entries for a given voice,
    // so that a later noteOff can never reach a voice that has since been
    // reassigned to a different note.
    void clearVoiceMapping (int voiceIndex);

    std::array<SfxrVoice, kNumVoices> voices;

    // Two-way mapping. noteToVoice[note] is the voice currently owning that
    // note, voiceToNote[voice] is the note that voice was started for. Keeping
    // both lets us invalidate the stale entry when a voice is stolen or ends.
    std::array<int, 128>         noteToVoice {};   // -1 = none
    std::array<int, kNumVoices>  voiceToNote {};   // -1 = none

    // Scratch buffer for one voice, preallocated in prepare().
    juce::AudioBuffer<float> voiceBuffer;

    SfxrParams params;
    double sampleRate  = 44100.0;
    bool   monoMode   = false;
    bool   oneShotMode = true;

    // Guards voice state against concurrent noteOn/noteOff (message thread,
    // from the on-screen keyboard) and process() (audio thread).
    juce::CriticalSection voiceLock;
};
