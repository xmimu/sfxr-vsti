#pragma once

#include <JuceHeader.h>
#include "SfxrVoice.h"

// Manages a fixed pool of voices and dispatches MIDI notes to them.
class SfxrEngine
{
public:
    SfxrEngine();

    void prepare (double sampleRate);
    void setParams (const SfxrParams& p);
    void setMono (bool mono);
    void setOneShot (bool oneShot);

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    // Renders all active voices into stereo output.
    void process (juce::AudioBuffer<float>& audio);

    static constexpr int kNumVoices = 8;

private:
    SfxrVoice* findFreeVoice();

    std::array<SfxrVoice, kNumVoices> voices;
    std::array<int, 128> noteToVoice {};   // -1 = none

    SfxrParams params;
    double sampleRate  = 44100.0;
    bool   monoMode   = false;
    bool   oneShotMode = true;

    // Guards voice state against concurrent noteOn/noteOff (message thread,
    // from the on-screen keyboard) and process() (audio thread).
    juce::CriticalSection voiceLock;
};
