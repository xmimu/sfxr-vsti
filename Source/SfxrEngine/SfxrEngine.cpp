#include "SfxrEngine.h"

SfxrEngine::SfxrEngine()
{
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);

    // Give each voice a distinct noise seed so polyphonic noise waveforms don't
    // end up perfectly correlated.
    for (int i = 0; i < kNumVoices; i++)
        voices[(size_t) i].setSeed (0x5f3759df + i);
}

void SfxrEngine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;

    // Preallocate here so that process() never touches the heap. The extra
    // headroom covers hosts that occasionally hand us a slightly larger block
    // than the one they advertised.
    voiceBuffer.setSize (1, juce::jmax (32, maxBlockSize), false, true, true);

    for (auto& v : voices)
        v.stop();
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
}

void SfxrEngine::setMono (bool mono)
{
    if (monoMode == mono)
        return;

    monoMode = mono;

    for (auto& v : voices)
        v.noteOff();
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
}

void SfxrEngine::reset()
{
    for (auto& v : voices)
        v.stop();
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
}

void SfxrEngine::clearVoiceMapping (int voiceIndex)
{
    const int note = voiceToNote[(size_t) voiceIndex];

    if (note >= 0 && note < 128 && noteToVoice[(size_t) note] == voiceIndex)
        noteToVoice[(size_t) note] = -1;

    voiceToNote[(size_t) voiceIndex] = -1;
}

int SfxrEngine::findFreeVoiceIndex()
{
    int oldest    = 0;
    uint32_t oldestAge = 0;
    bool found    = false;

    for (int i = 0; i < kNumVoices; i++)
    {
        if (! voices[(size_t) i].isActive())
            return i;

        if (! found || voices[(size_t) i].getAge() > oldestAge)
        {
            oldest    = i;
            oldestAge = voices[(size_t) i].getAge();
            found     = true;
        }
    }

    return oldest; // all busy -> steal the oldest
}

void SfxrEngine::noteOn (int midiNote, float velocity)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    if (monoMode)
    {
        // A single voice, always re-triggered by the newest note.
        voices[0].start (params, sampleRate, midiNote, velocity, oneShotMode);
        noteToVoice.fill (-1);
        voiceToNote.fill (-1);
        noteToVoice[(size_t) midiNote] = 0;
        voiceToNote[0] = midiNote;
        return;
    }

    const int idx = findFreeVoiceIndex();

    // Release any *other* voice already playing the same note so that rapid
    // retriggers don't leave orphaned (unreachable) voices behind.
    const int existing = noteToVoice[(size_t) midiNote];
    if (existing >= 0 && existing != idx && voices[(size_t) existing].isActive())
    {
        voices[(size_t) existing].noteOff();
        clearVoiceMapping (existing);
    }

    // The voice we are about to reuse may have been stolen from an older note
    // that is still held down. Drop that note's mapping now, otherwise its
    // eventual noteOff would release this new note instead.
    clearVoiceMapping (idx);

    voices[(size_t) idx].start (params, sampleRate, midiNote, velocity, oneShotMode);
    noteToVoice[(size_t) midiNote] = idx;
    voiceToNote[(size_t) idx] = midiNote;
}

void SfxrEngine::noteOff (int midiNote)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    if (monoMode)
    {
        voices[0].noteOff();
        noteToVoice.fill (-1);
        voiceToNote.fill (-1);
        return;
    }

    const int idx = noteToVoice[(size_t) midiNote];
    if (idx >= 0 && idx < kNumVoices)
    {
        voices[(size_t) idx].noteOff();
        clearVoiceMapping (idx);
    }
}

void SfxrEngine::allNotesOff()
{
    for (auto& v : voices)
        v.noteOff();
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
}

bool SfxrEngine::hasActiveVoices() const noexcept
{
    for (const auto& voice : voices)
        if (voice.isActive())
            return true;

    return false;
}

void SfxrEngine::render (juce::AudioBuffer<float>& audio, int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    // Should only ever happen if the host ignores its own maximumBlockSize;
    // growing here is still better than reading past the end of the buffer.
    if (voiceBuffer.getNumSamples() < numSamples)
        voiceBuffer.setSize (1, numSamples, false, true, true);

    const int numChannels = audio.getNumChannels();
    float* const mono = voiceBuffer.getWritePointer (0);

    for (int i = 0; i < kNumVoices; i++)
    {
        auto& v = voices[(size_t) i];

        if (! v.isActive())
            continue;

        const bool stillActive = v.render (mono, numSamples);

        for (int ch = 0; ch < numChannels; ch++)
            audio.addFrom (ch, startSample, mono, numSamples);

        // Reap the mapping as soon as the voice ends, so the slot cannot be
        // reassigned while a stale note still points at it.
        if (! stillActive)
            clearVoiceMapping (i);
    }
}
