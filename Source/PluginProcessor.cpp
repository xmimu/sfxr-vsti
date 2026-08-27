#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SfxrVstiAudioProcessor();
}

SfxrVstiAudioProcessor::SfxrVstiAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    keyboardState.addListener (this);
}

SfxrVstiAudioProcessor::~SfxrVstiAudioProcessor()
{
    keyboardState.removeListener (this);
}

const juce::String SfxrVstiAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

juce::AudioProcessorValueTreeState::ParameterLayout SfxrVstiAudioProcessor::createLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParamID::wave_type, "Waveform",
                                                       StringArray { "Square", "Sawtooth", "Sine", "Noise" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::env_attack,  "Attack Time",    0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::env_sustain, "Sustain Time",   0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::env_punch,   "Sustain Punch",  0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::env_decay,   "Decay Time",     0.0f, 1.0f, 0.4f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::base_freq,   "Start Frequency", 0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::freq_limit,  "Min Frequency",   0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::freq_ramp,   "Slide",          -1.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::freq_dramp,  "Delta Slide",    -1.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::vib_strength, "Vibrato Depth",  0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::vib_speed,    "Vibrato Speed",  0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::vib_delay,    "Vibrato Delay",  0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::arp_mod,    "Change Amount", -1.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::arp_speed,  "Change Speed",   0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::duty,      "Square Duty",    0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::duty_ramp, "Duty Sweep",     -1.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::repeat_speed, "Repeat Speed", 0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::pha_offset, "Phaser Offset", -1.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::pha_ramp,   "Phaser Sweep",  -1.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::lpf_freq,      "LP Cutoff",        0.0f, 1.0f, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::lpf_ramp,      "LP Cutoff Sweep", -1.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::lpf_resonance, "LP Resonance",     0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::hpf_freq,      "HP Cutoff",        0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParamID::hpf_ramp,      "HP Cutoff Sweep", -1.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (ParamID::master_vol, "Output Level", 0.0f, 1.0f, 0.5f));

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
    // After a note-off the decay stage is the longest thing that can still be
    // sounding: env_decay^2 * 100000 samples, calibrated at 44.1 kHz. Reporting
    // 0 here makes hosts cut the tail short when bouncing or freezing.
    const float decay = apvts.getRawParameterValue (ParamID::env_decay)->load();
    return (double) decay * (double) decay * 100000.0 / 44100.0;
}

SfxrParams SfxrVstiAudioProcessor::readParams() const
{
    SfxrParams p;

    p.wave_type = juce::roundToInt (apvts.getRawParameterValue (ParamID::wave_type)->load());

    p.base_freq  = apvts.getRawParameterValue (ParamID::base_freq)->load();
    p.freq_limit = apvts.getRawParameterValue (ParamID::freq_limit)->load();
    p.freq_ramp  = apvts.getRawParameterValue (ParamID::freq_ramp)->load();
    p.freq_dramp = apvts.getRawParameterValue (ParamID::freq_dramp)->load();
    p.duty       = apvts.getRawParameterValue (ParamID::duty)->load();
    p.duty_ramp  = apvts.getRawParameterValue (ParamID::duty_ramp)->load();

    p.vib_strength = apvts.getRawParameterValue (ParamID::vib_strength)->load();
    p.vib_speed    = apvts.getRawParameterValue (ParamID::vib_speed)->load();
    p.vib_delay    = apvts.getRawParameterValue (ParamID::vib_delay)->load();

    p.env_attack  = apvts.getRawParameterValue (ParamID::env_attack)->load();
    p.env_sustain = apvts.getRawParameterValue (ParamID::env_sustain)->load();
    p.env_decay   = apvts.getRawParameterValue (ParamID::env_decay)->load();
    p.env_punch   = apvts.getRawParameterValue (ParamID::env_punch)->load();

    p.lpf_resonance = apvts.getRawParameterValue (ParamID::lpf_resonance)->load();
    p.lpf_freq      = apvts.getRawParameterValue (ParamID::lpf_freq)->load();
    p.lpf_ramp      = apvts.getRawParameterValue (ParamID::lpf_ramp)->load();
    p.hpf_freq      = apvts.getRawParameterValue (ParamID::hpf_freq)->load();
    p.hpf_ramp      = apvts.getRawParameterValue (ParamID::hpf_ramp)->load();

    p.pha_offset = apvts.getRawParameterValue (ParamID::pha_offset)->load();
    p.pha_ramp   = apvts.getRawParameterValue (ParamID::pha_ramp)->load();

    p.repeat_speed = apvts.getRawParameterValue (ParamID::repeat_speed)->load();

    p.arp_speed = apvts.getRawParameterValue (ParamID::arp_speed)->load();
    p.arp_mod   = apvts.getRawParameterValue (ParamID::arp_mod)->load();

    p.sound_vol = apvts.getRawParameterValue (ParamID::master_vol)->load();

    return p;
}

bool SfxrVstiAudioProcessor::readBoolParam (const juce::String& id) const
{
    return apvts.getRawParameterValue (id)->load() >= 0.5f;
}

void SfxrVstiAudioProcessor::applyParams (const SfxrParams& p)
{
    auto setParam = [this] (const juce::String& id, float value)
    {
        if (auto* param = apvts.getParameter (id))
        {
            // Wrap in a gesture so hosts treat the whole preset change as a
            // single edit rather than a stream of unrelated automation writes.
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (value));
            param->endChangeGesture();
        }
    };

    setParam (ParamID::wave_type,  (float) p.wave_type);
    setParam (ParamID::base_freq,  p.base_freq);
    setParam (ParamID::freq_limit, p.freq_limit);
    setParam (ParamID::freq_ramp,  p.freq_ramp);
    setParam (ParamID::freq_dramp, p.freq_dramp);
    setParam (ParamID::duty,       p.duty);
    setParam (ParamID::duty_ramp,  p.duty_ramp);

    setParam (ParamID::vib_strength, p.vib_strength);
    setParam (ParamID::vib_speed,    p.vib_speed);
    setParam (ParamID::vib_delay,    p.vib_delay);

    setParam (ParamID::env_attack,  p.env_attack);
    setParam (ParamID::env_sustain, p.env_sustain);
    setParam (ParamID::env_decay,   p.env_decay);
    setParam (ParamID::env_punch,   p.env_punch);

    setParam (ParamID::lpf_resonance, p.lpf_resonance);
    setParam (ParamID::lpf_freq,      p.lpf_freq);
    setParam (ParamID::lpf_ramp,      p.lpf_ramp);
    setParam (ParamID::hpf_freq,      p.hpf_freq);
    setParam (ParamID::hpf_ramp,      p.hpf_ramp);

    setParam (ParamID::pha_offset, p.pha_offset);
    setParam (ParamID::pha_ramp,   p.pha_ramp);

    setParam (ParamID::repeat_speed, p.repeat_speed);

    setParam (ParamID::arp_speed, p.arp_speed);
    setParam (ParamID::arp_mod,   p.arp_mod);

    setParam (ParamID::master_vol, p.sound_vol);
}

void SfxrVstiAudioProcessor::playPreview()
{
    // Sync the engine with the latest parameters before triggering, otherwise
    // the preview would play with the previous block's (stale) parameters.
    engine.setParams (readParams());
    engine.setMono (readBoolParam (ParamID::mono));
    engine.setOneShot (readBoolParam (ParamID::one_shot));

    engine.noteOn (69, 1.0f);

    if (! readBoolParam (ParamID::one_shot))
        startTimer (400);
}

void SfxrVstiAudioProcessor::timerCallback()
{
    stopTimer();
    engine.noteOff (69);
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

void SfxrVstiAudioProcessor::handleNoteOn (juce::MidiKeyboardState*, int, int note, float velocity)
{
    if (SfxrNoteRange::contains (note))
        engine.noteOn (note, velocity);
}

void SfxrVstiAudioProcessor::handleNoteOff (juce::MidiKeyboardState*, int, int note, float)
{
    if (SfxrNoteRange::contains (note))
        engine.noteOff (note);
}

void SfxrVstiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Read current parameters into the engine.
    engine.setParams (readParams());
    engine.setMono (readBoolParam (ParamID::mono));
    engine.setOneShot (readBoolParam (ParamID::one_shot));

    // Route MIDI through the keyboard state so that incoming notes both
    // trigger the engine and light up the on-screen keyboard.
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            keyboardState.allNotesOff (0);
            engine.allNotesOff();
        }
        else
        {
            keyboardState.processNextMidiEvent (msg);
        }
    }
    midiMessages.clear();

    engine.process (buffer);

    // Feed the oscilloscope from the first output channel.
    if (buffer.getNumChannels() > 0)
        pushScope (buffer.getReadPointer (0), buffer.getNumSamples());
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
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SfxrVstiAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}
