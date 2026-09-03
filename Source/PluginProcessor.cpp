#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // One entry per continuous float parameter of SfxrParams, pairing its tree ID
    // with the struct member it fills. Used both to cache the raw atomics once in
    // the constructor and to fold them into an SfxrParams afterwards.
    struct ParamSlot
    {
        const char*            id;
        float SfxrParams::*    field;
    };

    constexpr ParamSlot kParamSlots[] =
    {
        { ParamID::base_freq,    &SfxrParams::base_freq    },
        { ParamID::freq_limit,   &SfxrParams::freq_limit   },
        { ParamID::freq_ramp,    &SfxrParams::freq_ramp    },
        { ParamID::freq_dramp,   &SfxrParams::freq_dramp   },
        { ParamID::duty,         &SfxrParams::duty         },
        { ParamID::duty_ramp,    &SfxrParams::duty_ramp    },

        { ParamID::vib_strength, &SfxrParams::vib_strength },
        { ParamID::vib_speed,    &SfxrParams::vib_speed    },
        { ParamID::vib_delay,    &SfxrParams::vib_delay    },

        { ParamID::env_attack,   &SfxrParams::env_attack   },
        { ParamID::env_sustain,  &SfxrParams::env_sustain  },
        { ParamID::env_decay,    &SfxrParams::env_decay    },
        { ParamID::env_punch,    &SfxrParams::env_punch    },

        { ParamID::lpf_resonance,&SfxrParams::lpf_resonance},
        { ParamID::lpf_freq,     &SfxrParams::lpf_freq     },
        { ParamID::lpf_ramp,     &SfxrParams::lpf_ramp     },
        { ParamID::hpf_freq,     &SfxrParams::hpf_freq     },
        { ParamID::hpf_ramp,     &SfxrParams::hpf_ramp     },

        { ParamID::pha_offset,   &SfxrParams::pha_offset   },
        { ParamID::pha_ramp,     &SfxrParams::pha_ramp     },

        { ParamID::repeat_speed, &SfxrParams::repeat_speed },

        { ParamID::arp_speed,    &SfxrParams::arp_speed    },
        { ParamID::arp_mod,      &SfxrParams::arp_mod      },

        { ParamID::master_vol,   &SfxrParams::sound_vol    },
    };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SfxrVstiAudioProcessor();
}

SfxrVstiAudioProcessor::SfxrVstiAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    // Cache the raw value pointers once. The audio thread then reads the params
    // by dereferencing these atomics instead of doing a string-keyed lookup (and
    // implicitly allocating a juce::String) on every block.
    static_assert (std::size (kParamSlots) == kNumFloatParams,
                   "kParamSlots must list every cached float parameter");

    for (size_t i = 0; i < std::size (kParamSlots); ++i)
        rawParams[i] = apvts.getRawParameterValue (kParamSlots[i].id);

    rawWaveType = apvts.getRawParameterValue (ParamID::wave_type);
    rawMono     = apvts.getRawParameterValue (ParamID::mono);
    rawOneShot  = apvts.getRawParameterValue (ParamID::one_shot);

    for (auto* p : rawParams)
    {
        jassert (p != nullptr);
        juce::ignoreUnused (p);
    }
    jassert (rawWaveType != nullptr && rawMono != nullptr && rawOneShot != nullptr);
}

SfxrVstiAudioProcessor::~SfxrVstiAudioProcessor()
{
}

const juce::String SfxrVstiAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

juce::AudioProcessorValueTreeState::ParameterLayout SfxrVstiAudioProcessor::createLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    // sfxr treats every one of these as a continuous float. The convenience
    // AudioParameterFloat constructor would quantise them to a 0.01 interval,
    // which is coarse enough to audibly change a sound loaded from a .sfs file
    // (a base_freq of 0.4372 would snap to 0.44), so specify the ranges
    // explicitly with no snapping.
    auto addFloat = [&layout] (const juce::String& id, const juce::String& name,
                               float minValue, float maxValue, float defaultValue)
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            id, name, NormalisableRange<float> (minValue, maxValue), defaultValue));
    };

    layout.add (std::make_unique<AudioParameterChoice> (ParamID::wave_type, "Waveform",
                                                       StringArray { "Square", "Sawtooth", "Sine", "Noise" }, 0));

    addFloat (ParamID::env_attack, "Attack Time", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::env_sustain, "Sustain Time", 0.0f, 1.0f, 0.3f);
    addFloat (ParamID::env_punch, "Sustain Punch", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::env_decay, "Decay Time", 0.0f, 1.0f, 0.4f);

    addFloat (ParamID::base_freq, "Start Frequency", 0.0f, 1.0f, 0.3f);
    addFloat (ParamID::freq_limit, "Min Frequency", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::freq_ramp, "Slide", -1.0f, 1.0f, 0.0f);
    addFloat (ParamID::freq_dramp, "Delta Slide", -1.0f, 1.0f, 0.0f);

    addFloat (ParamID::vib_strength, "Vibrato Depth", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::vib_speed, "Vibrato Speed", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::vib_delay, "Vibrato Delay", 0.0f, 1.0f, 0.0f);

    addFloat (ParamID::arp_mod, "Change Amount", -1.0f, 1.0f, 0.0f);
    addFloat (ParamID::arp_speed, "Change Speed", 0.0f, 1.0f, 0.0f);

    addFloat (ParamID::duty, "Square Duty", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::duty_ramp, "Duty Sweep", -1.0f, 1.0f, 0.0f);

    addFloat (ParamID::repeat_speed, "Repeat Speed", 0.0f, 1.0f, 0.0f);

    addFloat (ParamID::pha_offset, "Phaser Offset", -1.0f, 1.0f, 0.0f);
    addFloat (ParamID::pha_ramp, "Phaser Sweep", -1.0f, 1.0f, 0.0f);

    addFloat (ParamID::lpf_freq, "LP Cutoff", 0.0f, 1.0f, 1.0f);
    addFloat (ParamID::lpf_ramp, "LP Cutoff Sweep", -1.0f, 1.0f, 0.0f);
    addFloat (ParamID::lpf_resonance, "LP Resonance", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::hpf_freq, "HP Cutoff", 0.0f, 1.0f, 0.0f);
    addFloat (ParamID::hpf_ramp, "HP Cutoff Sweep", -1.0f, 1.0f, 0.0f);

    addFloat (ParamID::master_vol, "Output Level", 0.0f, 1.0f, 0.5f);

    layout.add (std::make_unique<AudioParameterBool> (ParamID::mono,     "Mono",    false));
    layout.add (std::make_unique<AudioParameterBool> (ParamID::one_shot, "One-Shot", true));

    return layout;
}

void SfxrVstiAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);

    // Drop anything the oscilloscope still holds from the previous run so the
    // display doesn't start with stale audio.
    scopeFifo.reset();
}

void SfxrVstiAudioProcessor::releaseResources()
{
    engine.reset();
}

bool SfxrVstiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

double SfxrVstiAudioProcessor::getTailLengthSeconds() const
{
    // Conservative upper bound on how long any voice can still sound after the
    // last note event: attack + sustain + decay, each lasting at most
    // env_x^2 * 100000 samples at 44.1 kHz. Decay alone under-reports one-shots
    // (whose attack and sustain stages still have to finish), which made hosts
    // truncate the sound when bouncing or freezing.
    const float attack  = apvts.getRawParameterValue (ParamID::env_attack)->load();
    const float sustain = apvts.getRawParameterValue (ParamID::env_sustain)->load();
    const float decay   = apvts.getRawParameterValue (ParamID::env_decay)->load();
    return (double) (attack * attack + sustain * sustain + decay * decay) * 100000.0 / 44100.0;
}

SfxrParams SfxrVstiAudioProcessor::readParams() const
{
    SfxrParams p;

    for (size_t i = 0; i < std::size (kParamSlots); ++i)
        p.*(kParamSlots[i].field) = rawParams[i]->load();

    p.wave_type = juce::roundToInt (rawWaveType->load());

    return p;
}

SfxrParams SfxrVstiAudioProcessor::pullParamsForAudio()
{
    const int gen = committedGen.load (std::memory_order_acquire);

    if (gen != lastSeenGen)
    {
        // A preset batch has been committed since the last block. The slot was
        // fully written before the generation was published (see applyParams), so
        // this is a consistent whole-preset snapshot, not a param-by-param read.
        lastSeenGen = gen;
        lastAudioParams = committed[(size_t) (gen % 3)];
    }
    else if (applying.load (std::memory_order_acquire))
    {
        // A batch is half-written right now: reading the tree would see a mix of
        // old and new values. Reuse the previous block's consistent snapshot.
    }
    else
    {
        // Steady state: read the live tree (covers single-parameter automation).
        lastAudioParams = readParams();
    }

    return lastAudioParams;
}

bool SfxrVstiAudioProcessor::readBoolParam (const char* id) const
{
    if (std::strcmp (id, ParamID::mono) == 0)
        return rawMono->load() >= 0.5f;

    if (std::strcmp (id, ParamID::one_shot) == 0)
        return rawOneShot->load() >= 0.5f;

    jassertfalse;
    return false;
}

void SfxrVstiAudioProcessor::applyParams (const SfxrParams& p, bool withGesture)
{
    // The tree parameters that a preset write covers, in write order.
    const char* const ids[] =
    {
        ParamID::wave_type, ParamID::base_freq,  ParamID::freq_limit,
        ParamID::freq_ramp, ParamID::freq_dramp, ParamID::duty,
        ParamID::duty_ramp, ParamID::vib_strength, ParamID::vib_speed,
        ParamID::vib_delay, ParamID::env_attack, ParamID::env_sustain,
        ParamID::env_decay, ParamID::env_punch,  ParamID::lpf_resonance,
        ParamID::lpf_freq,  ParamID::lpf_ramp,   ParamID::hpf_freq,
        ParamID::hpf_ramp,  ParamID::pha_offset, ParamID::pha_ramp,
        ParamID::repeat_speed, ParamID::arp_speed, ParamID::arp_mod,
        ParamID::master_vol,
    };

    // One gesture wrapping the whole batch rather than one per parameter, so a
    // host sees a single preset edit instead of a stream of unrelated writes.
    if (withGesture)
        for (auto* id : ids)
            if (auto* param = apvts.getParameter (id))
                param->beginChangeGesture();

    // Publishing order matters: set applying *before* touching the tree so the
    // audio thread stops reading live values while the batch is half-written.
    applying.store (true, std::memory_order_release);

    auto setValue = [this] (const juce::String& id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    setValue (ParamID::wave_type,  (float) p.wave_type);
    setValue (ParamID::base_freq,  p.base_freq);
    setValue (ParamID::freq_limit, p.freq_limit);
    setValue (ParamID::freq_ramp,  p.freq_ramp);
    setValue (ParamID::freq_dramp, p.freq_dramp);
    setValue (ParamID::duty,       p.duty);
    setValue (ParamID::duty_ramp,  p.duty_ramp);

    setValue (ParamID::vib_strength, p.vib_strength);
    setValue (ParamID::vib_speed,    p.vib_speed);
    setValue (ParamID::vib_delay,    p.vib_delay);

    setValue (ParamID::env_attack,  p.env_attack);
    setValue (ParamID::env_sustain, p.env_sustain);
    setValue (ParamID::env_decay,   p.env_decay);
    setValue (ParamID::env_punch,   p.env_punch);

    setValue (ParamID::lpf_resonance, p.lpf_resonance);
    setValue (ParamID::lpf_freq,      p.lpf_freq);
    setValue (ParamID::lpf_ramp,      p.lpf_ramp);
    setValue (ParamID::hpf_freq,      p.hpf_freq);
    setValue (ParamID::hpf_ramp,      p.hpf_ramp);

    setValue (ParamID::pha_offset, p.pha_offset);
    setValue (ParamID::pha_ramp,   p.pha_ramp);

    setValue (ParamID::repeat_speed, p.repeat_speed);

    setValue (ParamID::arp_speed, p.arp_speed);
    setValue (ParamID::arp_mod,   p.arp_mod);

    setValue (ParamID::master_vol, p.sound_vol);

    // Commit the whole struct into the next triple-buffer slot, then publish the
    // new generation. The acquire in pullParamsForAudio orders this publish after
    // the copy, and the slot is not reused for three generations.
    const int newGen = committedGen.load (std::memory_order_relaxed) + 1;
    committed[(size_t) (newGen % 3)] = p;
    committedGen.store (newGen, std::memory_order_release);
    applying.store (false, std::memory_order_release);

    if (withGesture)
        for (auto* id : ids)
            if (auto* param = apvts.getParameter (id))
                param->endChangeGesture();
}

const juce::String SfxrVstiAudioProcessor::getProgramName (int index)
{
    if (index <= 0)
        return "Init";

    const auto category = (PresetCategory) (index - 1);
    return index - 1 < (int) PresetCategory::Count ? presetCategoryName (category) : juce::String();
}

void SfxrVstiAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    currentProgram.store (index);

    SfxrParams p;

    if (index > 0)
    {
        // Seeded from the program index, so selecting a program always gives the
        // same sound. The generators are randomised by nature, but a host may
        // call this while restoring a session -- if the result varied, that
        // would silently overwrite the user's saved parameters with a different
        // sound. RANDOMIZE and the category buttons in the editor stay random.
        juce::Random rng (0x5f3759df + index);
        generatePreset (p, (PresetCategory) (index - 1), rng);
    }

    applyParams (p, /*withGesture*/ false);
}

void SfxrVstiAudioProcessor::playPreview()
{
    // Queue the note on the keyboard state rather than calling into the engine.
    // processBlock merges these into the incoming MIDI, so the note is triggered
    // on the audio thread with the parameter values of that block -- and the
    // on-screen keyboard lights up for free.
    keyboardState.noteOn (1, kPreviewNote, 1.0f);

    if (! readBoolParam (ParamID::one_shot))
        startTimer (400);
}

void SfxrVstiAudioProcessor::timerCallback()
{
    stopTimer();
    keyboardState.noteOff (1, kPreviewNote, 0.0f);
}

void SfxrVstiAudioProcessor::pushScope (const float* data, int numSamples)
{
    const int n = juce::jmin (numSamples, scopeFifo.getFreeSpace());
    if (n <= 0)
        return;

    const auto block = scopeFifo.write (n);
    if (block.blockSize1 > 0)
        std::memcpy (&scopeData[(size_t) block.startIndex1], data, (size_t) block.blockSize1 * sizeof (float));
    if (block.blockSize2 > 0)
        std::memcpy (&scopeData[(size_t) block.startIndex2], data + block.blockSize1, (size_t) block.blockSize2 * sizeof (float));
}

int SfxrVstiAudioProcessor::readScope (float* dest, int maxSamples)
{
    const auto block = scopeFifo.read (maxSamples);
    if (block.blockSize1 > 0)
        std::memcpy (dest, &scopeData[(size_t) block.startIndex1], (size_t) block.blockSize1 * sizeof (float));
    if (block.blockSize2 > 0)
        std::memcpy (dest + block.blockSize1, &scopeData[(size_t) block.startIndex2], (size_t) block.blockSize2 * sizeof (float));
    return block.blockSize1 + block.blockSize2;
}

void SfxrVstiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Read current parameters into the engine. pullParamsForAudio() returns a
    // whole-preset snapshot whenever a preset write is in flight, so notes can
    // never be locked onto a half-updated mix of old and new values.
    engine.setParams (pullParamsForAudio());
    engine.setMono (readBoolParam (ParamID::mono));
    engine.setOneShot (readBoolParam (ParamID::one_shot));

    // Merge anything the on-screen keyboard has queued into the incoming MIDI
    // and update the key highlighting. Doing it this way means UI notes take the
    // same path as host notes and the engine is only ever touched from here.
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    // Render in segments split on the MIDI event positions, so note timing is
    // sample-accurate instead of being quantised to the block size.
    int position = 0;

    for (const auto metadata : midiMessages)
    {
        const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition);

        if (eventPos > position)
        {
            engine.render (buffer, position, eventPos - position);
            position = eventPos;
        }

        handleMidiEvent (metadata.getMessage());
    }

    engine.render (buffer, position, numSamples - position);

    midiMessages.clear();

    // Feed the oscilloscope from the first output channel.
    if (buffer.getNumChannels() > 0)
        pushScope (buffer.getReadPointer (0), numSamples);
}

void SfxrVstiAudioProcessor::handleMidiEvent (const juce::MidiMessage& msg)
{
    // Note-ons and note-offs are filtered together, so a note that was never
    // started cannot be released either. All-notes-off is never filtered.
    if (msg.isNoteOn())
    {
        if (SfxrNoteRange::contains (msg.getNoteNumber()))
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    }
    else if (msg.isNoteOff())
    {
        if (SfxrNoteRange::contains (msg.getNoteNumber()))
            engine.noteOff (msg.getNoteNumber());
    }
    else if (msg.isAllSoundOff())
    {
        // CC120 All Sound Off: a panic. Must cut one-shots immediately, which
        // a normal release (allNotesOff) cannot do.
        engine.allSoundOff();
    }
    else if (msg.isAllNotesOff())
    {
        // CC123 All Notes Off: release held notes, let anything already
        // released or one-shot ring out naturally.
        engine.allNotesOff();
    }
}

juce::AudioProcessorEditor* SfxrVstiAudioProcessor::createEditor()
{
    return new SfxrVstiAudioProcessorEditor (*this);
}

bool SfxrVstiAudioProcessor::hasEditor() const
{
    return true;
}

void SfxrVstiAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("currentProgram", currentProgram.load(), nullptr);

    if (std::unique_ptr<juce::XmlElement> xml { state.createXml() })
        copyXmlToBinary (*xml, destData);
}

void SfxrVstiAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        const auto state = juce::ValueTree::fromXml (*xml);

        // Restore the parameters directly. Deliberately *not* via
        // setCurrentProgram: the saved parameters are the truth, and the program
        // index is only remembered so the host's menu shows the right entry.
        apvts.replaceState (state);
        currentProgram.store (juce::jlimit (0, getNumPrograms() - 1,
                                            (int) state.getProperty ("currentProgram", 0)));
    }
}
