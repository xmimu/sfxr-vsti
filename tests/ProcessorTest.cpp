#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

#include <cstdio>
#include <cmath>

namespace
{
    int checks = 0, failures = 0;

    void check (bool ok, const char* name)
    {
        checks++;
        if (! ok)
        {
            failures++;
            std::printf ("  FAIL  %s\n", name);
        }
    }

    float blockPeak (const juce::AudioBuffer<float>& b)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ch++)
            peak = juce::jmax (peak, b.getMagnitude (ch, 0, b.getNumSamples()));
        return peak;
    }
}

int main()
{
    std::printf ("SfxrVsti processor integration tests\n");

    const double sr = 44100.0;
    const int    block = 512;

    // ---- construction / layout ----
    {
        SfxrVstiAudioProcessor proc;
        check (proc.getNumPrograms() == 1 + (int) PresetCategory::Count, "program count covers init + generators");
        check (proc.getProgramName (0) == "Init", "program 0 is Init");
        check (proc.acceptsMidi() && ! proc.producesMidi(), "MIDI direction flags");
        check (proc.getTailLengthSeconds() > 0.0, "tail length is reported");
    }

    // ---- a real processBlock renders a note ----
    {
        SfxrVstiAudioProcessor proc;
        proc.prepareToPlay (sr, block);

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), 0);

        juce::AudioBuffer<float> audio (2, block);
        audio.clear();
        proc.processBlock (audio, midi);

        const float peak = blockPeak (audio);
        std::printf ("  note-on peak: %.4f\n", peak);
        check (std::isfinite (peak), "processBlock output is finite");
        check (peak > 0.01f, "a note-on produces audio");
    }

    // ---- CC120 (all sound off) silences a sustained note through processBlock ----
    {
        SfxrVstiAudioProcessor proc;
        proc.prepareToPlay (sr, block);

        auto setParam = [&] (const char* id, float v01)
        {
            if (auto* p = proc.getAPVTS().getParameter (id))
                p->setValueNotifyingHost (v01);
        };
        setParam (ParamID::one_shot, 0.0f);   // sustain: holds until note-off/panic

        juce::AudioBuffer<float> audio (2, block);

        juce::MidiBuffer on;
        on.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), 0);
        proc.processBlock (audio, on);
        check (blockPeak (audio) > 0.01f, "sustained note is sounding");

        audio.clear();
        juce::MidiBuffer panic;
        panic.addEvent (juce::MidiMessage::allSoundOff (1), 0);   // CC120
        proc.processBlock (audio, panic);
        const float after = blockPeak (audio);
        std::printf ("  peak right after CC120: %.6f\n", after);
        check (after < 1.0e-5f, "CC120 all-sound-off silences the note immediately");
    }

    // ---- deterministic programs: re-selecting an index reproduces the params ----
    {
        SfxrVstiAudioProcessor proc;
        proc.setCurrentProgram (4);
        const auto first = proc.readParams();
        proc.setCurrentProgram (4);
        const auto second = proc.readParams();

        const float* a = reinterpret_cast<const float*> (&first);
        const float* b = reinterpret_cast<const float*> (&second);
        bool same = true;
        for (int i = 0; i < (int) (sizeof (SfxrParams) / sizeof (float)); i++)
            same = same && std::abs (a[i] - b[i]) < 1.0e-6f;
        check (same && first.wave_type == second.wave_type, "program selection is deterministic");
    }

    // ---- state round trip: params + current program survive ----
    {
        SfxrVstiAudioProcessor a;
        a.setCurrentProgram (2);

        SfxrParams p;
        juce::Random rng (1234);
        generatePreset (p, PresetCategory::LaserShoot, rng);
        a.applyParams (p);

        juce::MemoryBlock state;
        a.getStateInformation (state);

        SfxrVstiAudioProcessor b;
        b.setStateInformation (state.getData(), (int) state.getSize());

        check (b.getCurrentProgram() == 2, "current program survives state restore");
        check (a.getParameters().size() == b.getParameters().size(), "parameter count survives");

        const auto pa = a.readParams();
        const auto pb = b.readParams();
        const float* fa = reinterpret_cast<const float*> (&pa);
        const float* fb = reinterpret_cast<const float*> (&pb);
        bool same = true;
        for (int i = 0; i < (int) (sizeof (SfxrParams) / sizeof (float)); i++)
            same = same && std::abs (fa[i] - fb[i]) < 1.0e-6f;
        check (same && pa.wave_type == pb.wave_type, "parameters survive state round trip");
    }

    // ---- parameters are continuous floats (no 0.01 quantisation) ----
    {
        SfxrVstiAudioProcessor proc;

        auto* param = proc.getAPVTS().getParameter (ParamID::base_freq);
        check (param != nullptr, "base_freq parameter exists");
        if (param != nullptr)
        {
            param->setValueNotifyingHost (param->convertTo0to1 (0.4372f));
            const float got = proc.readParams().base_freq;
            std::printf ("  base_freq after writing 0.4372: %.6f\n", got);
            check (std::abs (got - 0.4372f) < 1.0e-5f, "parameters are not quantised to 0.01");
        }
    }

    // ---- oversized blocks do not allocate / glitch through the processor ----
    {
        SfxrVstiAudioProcessor proc;
        proc.prepareToPlay (sr, block);   // advertises 512

        juce::AudioBuffer<float> audio (2, 4096);
        audio.clear();

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        proc.processBlock (audio, midi);

        const float peak = blockPeak (audio);
        std::printf ("  oversized processBlock peak: %.4f\n", peak);
        check (std::isfinite (peak), "oversized processBlock output is finite");
        check (peak > 0.01f && peak <= 1.0001f, "oversized processBlock is bounded and audible");
    }

    std::printf ("\n%d checks, %d failure(s)\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
