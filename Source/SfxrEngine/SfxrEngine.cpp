#include "SfxrEngine.h"

SfxrEngine::SfxrEngine()
{
    noteToVoice.fill (-1);

    // Give each voice a distinct noise seed so polyphonic noise waveforms don't
    // end up perfectly correlated.
    for (int i = 0; i < kNumVoices; i++)
        voices[(size_t) i].setSeed (0x5f3759df + i);
}

void SfxrEngine::prepare (double sr)
{
    sampleRate = sr;
}

void SfxrEngine::setParams (const SfxrParams& p)
{
    const juce::ScopedLock sl (voiceLock);
    params = p;
}

void SfxrEngine::setMono (bool mono)
{
    const juce::ScopedLock sl (voiceLock);

    if (monoMode == mono)
        return;

    monoMode = mono;

    if (monoMode)
    {
        for (auto& v : voices)
            v.noteOff();
        noteToVoice.fill (-1);
    }
}

void SfxrEngine::setOneShot (bool oneShot)
{
    const juce::ScopedLock sl (voiceLock);
    oneShotMode = oneShot;
}

SfxrVoice* SfxrEngine::findFreeVoice()
{
    SfxrVoice* oldest = nullptr;
    uint32_t   oldestAge = 0;

    for (auto& v : voices)
    {
        if (!v.isActive())
            return &v;

        if (oldest == nullptr || v.getAge() > oldestAge)
        {
            oldest    = &v;
            oldestAge = v.getAge();
        }
    }

    return oldest; // all busy -> steal the oldest
}

void SfxrEngine::noteOn (int midiNote, float velocity)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    const juce::ScopedLock sl (voiceLock);

    if (monoMode)
    {
        // A single voice, always re-triggered by the newest note.
        voices[0].start (params, sampleRate, midiNote, velocity, oneShotMode);
        noteToVoice.fill (-1);
        noteToVoice[midiNote] = 0;
        return;
    }

    if (SfxrVoice* v = findFreeVoice())
    {
        // Release any voice already playing the same note so that rapid
        // retriggers don't leave orphaned (unreachable) voices behind.
        const int existing = noteToVoice[midiNote];
        if (existing >= 0 && existing < kNumVoices && voices[(size_t) existing].isActive())
            voices[(size_t) existing].noteOff();

        v->start (params, sampleRate, midiNote, velocity, oneShotMode);
        noteToVoice[midiNote] = (int) (v - &voices[0]);
    }
}

void SfxrEngine::noteOff (int midiNote)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    const juce::ScopedLock sl (voiceLock);

    if (monoMode)
    {
        voices[0].noteOff();
        noteToVoice.fill (-1);
        return;
    }

    const int idx = noteToVoice[midiNote];
    if (idx >= 0)
    {
        voices[idx].noteOff();
        noteToVoice[midiNote] = -1;
    }
}

void SfxrEngine::allNotesOff()
{
    const juce::ScopedLock sl (voiceLock);

    for (auto& v : voices)
        v.noteOff();
    noteToVoice.fill (-1);
}

void SfxrEngine::process (juce::AudioBuffer<float>& audio)
{
    const int numSamples = audio.getNumSamples();
    audio.clear();

    if (numSamples == 0)
        return;

    juce::AudioBuffer<float> mono (1, numSamples);
    mono.clear();

    const juce::ScopedLock sl (voiceLock);

    for (auto& v : voices)
    {
        if (!v.isActive())
            continue;

        v.render (mono.getWritePointer (0), numSamples);

        for (int ch = 0; ch < audio.getNumChannels(); ch++)
            audio.addFrom (ch, 0, mono.getReadPointer (0), numSamples);
    }
}
