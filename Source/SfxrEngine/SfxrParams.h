#pragma once

#include <JuceHeader.h>

// Parameter ID constants (shared between processor, editor and engine).
namespace ParamID
{
    const juce::String wave_type     = "wave_type";

    const juce::String base_freq     = "base_freq";
    const juce::String freq_limit    = "freq_limit";
    const juce::String freq_ramp     = "freq_ramp";
    const juce::String freq_dramp    = "freq_dramp";
    const juce::String duty          = "duty";
    const juce::String duty_ramp     = "duty_ramp";

    const juce::String vib_strength  = "vib_strength";
    const juce::String vib_speed     = "vib_speed";
    const juce::String vib_delay     = "vib_delay";

    const juce::String env_attack    = "env_attack";
    const juce::String env_sustain   = "env_sustain";
    const juce::String env_decay     = "env_decay";
    const juce::String env_punch     = "env_punch";

    const juce::String lpf_resonance = "lpf_resonance";
    const juce::String lpf_freq      = "lpf_freq";
    const juce::String lpf_ramp      = "lpf_ramp";
    const juce::String hpf_freq      = "hpf_freq";
    const juce::String hpf_ramp      = "hpf_ramp";

    const juce::String pha_offset    = "pha_offset";
    const juce::String pha_ramp      = "pha_ramp";

    const juce::String repeat_speed  = "repeat_speed";

    const juce::String arp_speed     = "arp_speed";
    const juce::String arp_mod       = "arp_mod";

    const juce::String master_vol    = "master_vol";
    const juce::String mono          = "mono";
    const juce::String one_shot      = "one_shot";
}

// The full sfxr parameter set in the same domain as the original code.
// Unipolar parameters are in [0, 1]; bipolar parameters are in [-1, 1].
struct SfxrParams
{
    int   wave_type     = 0;

    float base_freq     = 0.3f;
    float freq_limit    = 0.0f;
    float freq_ramp     = 0.0f;   // bipolar
    float freq_dramp    = 0.0f;   // bipolar
    float duty          = 0.0f;
    float duty_ramp     = 0.0f;   // bipolar

    float vib_strength  = 0.0f;
    float vib_speed     = 0.0f;
    float vib_delay     = 0.0f;

    float env_attack    = 0.0f;
    float env_sustain   = 0.3f;
    float env_decay     = 0.4f;
    float env_punch     = 0.0f;

    float lpf_resonance = 0.0f;
    float lpf_freq      = 1.0f;
    float lpf_ramp      = 0.0f;   // bipolar
    float hpf_freq      = 0.0f;
    float hpf_ramp      = 0.0f;   // bipolar

    float pha_offset    = 0.0f;   // bipolar
    float pha_ramp      = 0.0f;   // bipolar

    float repeat_speed  = 0.0f;

    float arp_speed     = 0.0f;
    float arp_mod       = 0.0f;   // bipolar

    float sound_vol     = 0.5f;   // 0..1 output level

    // Matches ResetParams() in the original sfxr.
    void resetDefaults()
    {
        *this = SfxrParams {};
    }

    // The original UI runs every parameter through Slider() before playing a
    // generated sound. Slider clamps unipolar values to [0, 1] and bipolar
    // values to [-1, 1], even though Randomize/Mutate can generate values beyond
    // those ranges. Apply the same pass before values enter the parameter tree.
    void clampToDomain()
    {
        const auto uni = [] (float v) { return juce::jlimit (0.0f, 1.0f, v); };
        const auto bi  = [] (float v) { return juce::jlimit (-1.0f, 1.0f, v); };

        base_freq    = uni (base_freq);
        freq_limit   = uni (freq_limit);
        duty         = uni (duty);
        vib_strength = uni (vib_strength);
        vib_speed    = uni (vib_speed);
        vib_delay    = uni (vib_delay);
        env_attack   = uni (env_attack);
        env_sustain  = uni (env_sustain);
        env_decay    = uni (env_decay);
        env_punch    = uni (env_punch);
        lpf_freq     = uni (lpf_freq);
        lpf_resonance = uni (lpf_resonance);
        hpf_freq     = uni (hpf_freq);
        repeat_speed = uni (repeat_speed);
        arp_speed    = uni (arp_speed);
        sound_vol    = uni (sound_vol);

        freq_ramp  = bi (freq_ramp);
        freq_dramp = bi (freq_dramp);
        duty_ramp  = bi (duty_ramp);
        lpf_ramp   = bi (lpf_ramp);
        hpf_ramp   = bi (hpf_ramp);
        pha_offset = bi (pha_offset);
        pha_ramp   = bi (pha_ramp);
        arp_mod    = bi (arp_mod);

        wave_type = juce::jlimit (0, 3, wave_type);
    }
};
