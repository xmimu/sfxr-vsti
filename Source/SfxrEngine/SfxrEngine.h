#pragma once

#include <JuceHeader.h>
#include "SfxrVoice.h"

// Manages a fixed pool of voices and dispatches MIDI notes to them.
//
// Threading: everything in here is owned by the audio thread. noteOn/noteOff/
// process are only ever called from processBlock, and the parameters are handed
// in by value, so no locking is needed. UI-generated notes reach us the same way
// as host MIDI, by being merged into the incoming MidiBuffer.
class SfxrEngine
{
public:
    SfxrEngine();

    // Must be called before process(). maxBlockSize sizes the internal mixing
    // buffer up front so that process() never allocates on the audio thread.
    void prepare (double sampleRate, int maxBlockSize);

    void setParams (const SfxrParams& p) { params = p; }
    void setMono (bool mono);
    void setOneShot (bool oneShot)       { oneShotMode = oneShot; }

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    // Silences every voice immediately and drops all note mappings.
    void reset();

    // Renders numSamples of all active voices into audio, starting at
    // startSample. Adds to whatever is already there, so a block can be split
    // around MIDI events.
    void render (juce::AudioBuffer<float>& audio, int startSample, int numSamples);

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
};
