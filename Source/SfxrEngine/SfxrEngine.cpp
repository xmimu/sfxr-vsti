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
    clearHeldNotes();

    for (auto& v : voices)
        v.noteOff();
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
}

void SfxrEngine::reset()
{
    clearHeldNotes();
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
        monoNoteOn (midiNote, velocity);
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

    // If the stolen voice is still sounding, fade its tail out over a few
    // milliseconds before switching to the new note, so the takeover does not
    // cut the old waveform mid-cycle and click.
    auto& voice = voices[(size_t) idx];
    if (voice.isActive())
        voice.requestStealRestart (params, sampleRate, midiNote, velocity, oneShotMode);
    else
        voice.start (params, sampleRate, midiNote, velocity, oneShotMode);

    noteToVoice[(size_t) midiNote] = idx;
    voiceToNote[(size_t) idx] = midiNote;
}

void SfxrEngine::noteOff (int midiNote)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    if (monoMode)
    {
        monoNoteOff (midiNote);
        return;
    }

    const int idx = noteToVoice[(size_t) midiNote];
    if (idx >= 0 && idx < kNumVoices)
    {
        voices[(size_t) idx].noteOff();
        clearVoiceMapping (idx);
    }
}

void SfxrEngine::monoNoteOn (int midiNote, float velocity)
{
    // Dedupe so a re-press of an already held note becomes the most recent.
    for (int i = 0; i < heldCount; ++i)
        if (heldStack[(size_t) i] == midiNote)
        {
            for (int j = i; j < heldCount - 1; ++j)
            {
                heldStack[(size_t) j] = heldStack[(size_t) (j + 1)];
                heldVel[(size_t) j]   = heldVel[(size_t) (j + 1)];
            }
            --heldCount;
            break;
        }

    heldStack[(size_t) heldCount] = midiNote;
    heldVel[(size_t) heldCount]   = velocity;
    ++heldCount;

    // A single voice, always retriggered by the newest note.
    voices[0].start (params, sampleRate, midiNote, velocity, oneShotMode);
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
    noteToVoice[(size_t) midiNote] = 0;
    voiceToNote[0] = midiNote;
}

void SfxrEngine::monoNoteOff (int midiNote)
{
    // Notes that were never started are ignored (paired filtering upstream).
    int found = -1;
    for (int i = 0; i < heldCount; ++i)
        if (heldStack[(size_t) i] == midiNote)
        {
            found = i;
            break;
        }
    if (found < 0)
        return;

    for (int j = found; j < heldCount - 1; ++j)
    {
        heldStack[(size_t) j] = heldStack[(size_t) (j + 1)];
        heldVel[(size_t) j]   = heldVel[(size_t) (j + 1)];
    }
    --heldCount;

    // Nothing held any more: release the voice normally.
    if (heldCount == 0)
    {
        voices[0].noteOff();
        noteToVoice.fill (-1);
        voiceToNote.fill (-1);
        return;
    }

    // Releasing an older note while a newer one still rings must not stop the
    // voice (this is the legato case that used to kill the ringing note).
    if (voiceToNote[0] != midiNote)
        return;

    // The sounding note was released but others are still held: fall back to
    // the newest one by retriggering it, because the voice pitch is fixed when
    // it is started.
    const int   fallback = heldStack[(size_t) (heldCount - 1)];
    const float vel      = heldVel[(size_t) (heldCount - 1)];
    voices[0].start (params, sampleRate, fallback, vel, oneShotMode);
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
    noteToVoice[(size_t) fallback] = 0;
    voiceToNote[0] = fallback;
}

void SfxrEngine::allNotesOff()
{
    clearHeldNotes();
    for (auto& v : voices)
        v.noteOff();
    noteToVoice.fill (-1);
    voiceToNote.fill (-1);
}

void SfxrEngine::allSoundOff()
{
    clearHeldNotes();
    for (auto& v : voices)
        v.stop();
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

    // voiceBuffer is sized once in prepare(). If a host ever hands us a block
    // larger than it advertised, cut the request into pieces that fit instead
    // of growing the buffer -- allocating on the audio thread is not allowed.
    const int chunk = voiceBuffer.getNumSamples();
    int offset = 0;

    while (offset < numSamples)
    {
        const int n = juce::jmin (chunk, numSamples - offset);
        renderChunk (audio, startSample + offset, n);
        offset += n;
    }
}

void SfxrEngine::renderChunk (juce::AudioBuffer<float>& audio, int startSample, int numSamples)
{
    const int numChannels = audio.getNumChannels();
    float* const mono = voiceBuffer.getWritePointer (0);

    // Polyphony headroom contract: every voice is individually clamped to +/-1,
    // so N simultaneous voices could sum to N. Keep a single voice untouched
    // (identical output to the original) and reserve headroom for chords by
    // scaling the bus down by the number of voices that are actually sounding.
    int activeVoices = 0;
    for (int i = 0; i < kNumVoices; i++)
        if (voices[(size_t) i].isActive())
            activeVoices++;

    const float busScale = activeVoices > 1 ? 1.0f / (float) activeVoices : 1.0f;

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

    if (busScale != 1.0f)
        for (int ch = 0; ch < numChannels; ch++)
            audio.applyGain (ch, startSample, numSamples, busScale);
}
