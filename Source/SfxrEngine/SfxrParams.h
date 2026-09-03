#pragma once

#include <JuceHeader.h>

// Parameter ID constants (shared between processor, editor and engine).
// Plain C strings rather than juce::String so a header include costs nothing:
// every TU that pulls this in used to get its own copy of 27 String objects
// plus their static initialisation. JUCE APIs accept const char* implicitly.
namespace ParamID
{
    inline constexpr const char* wave_type     = "wave_type";

    inline constexpr const char* base_freq     = "base_freq";
    inline constexpr const char* freq_limit    = "freq_limit";
    inline constexpr const char* freq_ramp     = "freq_ramp";
    inline constexpr const char* freq_dramp    = "freq_dramp";
    inline constexpr const char* duty          = "duty";
    inline constexpr const char* duty_ramp     = "duty_ramp";

    inline constexpr const char* vib_strength  = "vib_strength";
    inline constexpr const char* vib_speed     = "vib_speed";
    inline constexpr const char* vib_delay     = "vib_delay";

    inline constexpr const char* env_attack    = "env_attack";
    inline constexpr const char* env_sustain   = "env_sustain";
    inline constexpr const char* env_decay     = "env_decay";
    inline constexpr const char* env_punch     = "env_punch";

    inline constexpr const char* lpf_resonance = "lpf_resonance";
    inline constexpr const char* lpf_freq      = "lpf_freq";
    inline constexpr const char* lpf_ramp      = "lpf_ramp";
    inline constexpr const char* hpf_freq      = "hpf_freq";
    inline constexpr const char* hpf_ramp      = "hpf_ramp";

    inline constexpr const char* pha_offset    = "pha_offset";
    inline constexpr const char* pha_ramp      = "pha_ramp";

    inline constexpr const char* repeat_speed  = "repeat_speed";

    inline constexpr const char* arp_speed     = "arp_speed";
    inline constexpr const char* arp_mod       = "arp_mod";

    inline constexpr const char* master_vol    = "master_vol";
    inline constexpr const char* mono          = "mono";
    inline constexpr const char* one_shot      = "one_shot";
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
