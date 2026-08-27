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

    // Brings an out-of-domain parameter set back into the ranges documented
    // above, preserving the sound wherever that is possible.
    //
    // This is needed because the original sfxr's Randomize/Mutate freely violate
    // their own domain -- p_env_decay, for instance, is assigned frnd(2) - 1,
    // i.e. [-1, 1] for a parameter that is conceptually [0, 1] -- and it happily
    // writes those values into a .sfs file. Simply clamping them to 0 is wrong:
    // the synth squares most unipolar parameters, so a negative value there
    // sounds exactly like its magnitude, whereas 0 usually means "off".
    //
    // Three groups, in decreasing order of fidelity:
    //
    //  1. squared by the synth -> abs() reproduces the original sound exactly
    //  2. sign-significant, but clamping happens to land on the same sound
    //     (see the comments below for why)
    //  3. not representable in the documented range at all -> clamp to the
    //     nearest value and accept the difference
    void foldIntoDomain()
    {
        const auto uni = [] (float v) { return juce::jlimit (0.0f, 1.0f, v); };
        const auto bi  = [] (float v) { return juce::jlimit (-1.0f, 1.0f, v); };

        // --- group 1: squared by the synth, so |v| is exactly equivalent ---
        base_freq     = std::abs (base_freq);       // 100 / (f*f + 0.001)
        vib_speed     = std::abs (vib_speed);       // pow(v, 2) * 0.01
        vib_delay     = std::abs (vib_delay);       // v * v * 100000
        env_attack    = std::abs (env_attack);      // a * a * 100000
        env_sustain   = std::abs (env_sustain);     // s * s * 100000
        env_decay     = std::abs (env_decay);       // d * d * 100000
        lpf_resonance = std::abs (lpf_resonance);   // pow(r, 2) * 20
        hpf_freq      = std::abs (hpf_freq);        // pow(f, 2) * 0.1

        // --- group 2: clamping is already sound-preserving ---
        //  duty:         square_duty = 0.5 - duty*0.5, and the render loop clamps
        //                square_duty to 0.5, which is what duty = 0 produces
        //  vib_strength: vib_amp = strength*0.5, and a non-positive vib_amp skips
        //                the vibrato entirely, exactly as 0 does
        //  lpf_freq:     pow(f, 3) keeps the sign, so a negative cutoff gives a
        //                negative fltw that the render loop clamps to 0 -- the
        //                filter then outputs silence, same as lpf_freq = 0
        //  freq_limit:   squared, but its *sign* also decides whether the voice
        //                stops at the limit, so abs() would change behaviour.
        //                No generator ever makes it negative, so plain clamping
        //                is the safe choice for untrusted input.

        // --- group 3: no equivalent inside the range ---
        //  base_freq > 1:  clamped to 1, which can lower the pitch by up to an
        //                  octave. Randomize reaches 1.5 via pow(x,3) + 0.5
        //  env_punch < 0:  a negative punch ramps *up* to unity instead of down
        //                  from it; not expressible, so 0 (no punch) is used
        //  repeat_speed<0: gives a repeat slower than any in-range value, and 0
        //                  is the original's "off" sentinel, so the repeat is
        //                  lost. Those periods exceed 0.45 s and rarely fire
        //                  within a typical sfxr sound
        //  arp_speed < 0:  clamping to 0 yields the slowest in-range arpeggio,
        //                  which is the nearest representable behaviour

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
