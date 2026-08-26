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
};
