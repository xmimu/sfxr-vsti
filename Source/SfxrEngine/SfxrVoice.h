#pragma once

#include <JuceHeader.h>
#include "SfxrParams.h"

// One polyphonic voice. This is a faithful port of the sfxr synthesis core
// (ResetSample + SynthSample in the original main.cpp), with the following
// changes to make it usable as an instrument:
//   - per-voice state so it can be polyphonic
//   - fully sample-rate independent: every constant in the original is
//     calibrated for 44100 Hz, so each one is rescaled by srScale = sr/44100
//     according to what it represents (see reset() for the details)
//   - MIDI note maps to a transposition around the root note (note 69 plays
//     the unmodified Start Frequency)
//   - velocity scales the output gain
//   - optional sustain mode (when oneShot is false the note holds until noteOff)
//   - the original p_vib_delay parameter is now actually used (vibrato fades in
//     after the delay rather than being applied immediately)
class SfxrVoice
{
public:
    // The phaser delay line is indexed in supersamples (8 per output sample).
    // The original used 1024 entries at 44100 Hz, i.e. a maximum delay of
    // 1023/(8*44100) = 2.9 ms. To keep that delay *time* available at higher
    // sample rates the line has to grow proportionally; 8192 covers everything
    // up to 352.8 kHz. Must stay a power of two (the index wraps with a mask).
    static constexpr int kPhaserBufferSize = 8192;
    static constexpr int kPhaserBufferMask = kPhaserBufferSize - 1;

    SfxrVoice();

    void start (const SfxrParams& params, double sampleRate,
                int midiNote, float velocity, bool oneShot);

    void noteOff();

    // Seeds the internal noise generator (used by the engine to decorrelate voices).
    void setSeed (int seed) { rng = juce::Random (seed); }

    // Renders into a mono buffer. Samples after the voice has finished are
    // zeroed. Returns true while the voice is still active.
    bool render (float* buffer, int numSamples);

    bool isActive() const noexcept { return playing; }
    int  getNote() const noexcept    { return midiNote; }
    uint32_t getAge() const noexcept { return age; }

private:
    void reset (bool restart);

    // ---- per-voice synth state (mirrors the globals in original main.cpp) ----
    bool   playing     = false;
    int    phase       = 0;
    double fperiod     = 0.0;
    double fmaxperiod  = 0.0;
    double fslide      = 0.0;
    double fdslide     = 0.0;
    int    period      = 0;
    float  square_duty = 0.0f;
    float  square_slide= 0.0f;

    int    env_stage   = 0;
    int    env_time    = 0;
    int    env_length[3] {};
    float  env_vol     = 0.0f;

    float  fphase      = 0.0f;
    float  fdphase     = 0.0f;
    int    iphase      = 0;
    float  phaser_buffer[kPhaserBufferSize] {};
    int    ipp         = 0;

    float  noise_buffer[32] {};

    float  fltp        = 0.0f;
    float  fltdp       = 0.0f;
    float  fltw        = 0.0f;
    float  fltw_d      = 0.0f;
    float  fltdmp      = 0.0f;
    float  fltphp      = 0.0f;
    float  flthp       = 0.0f;
    float  flthp_d     = 0.0f;

    float  vib_phase   = 0.0f;
    float  vib_speed   = 0.0f;
    float  vib_amp     = 0.0f;
    int    vib_delay_len = 0;
    int    vib_delay_time = 0;

    int    rep_time    = 0;
    int    rep_limit   = 0;

    int    arp_time    = 0;
    int    arp_limit   = 0;
    double arp_mod     = 0.0;

    // ---- params sampled at start, used during render ----
    int    wave_type   = 0;
    float  p_freq_limit= 0.0f;
    float  p_env_punch = 0.0f;
    float  p_lpf_freq  = 1.0f;
    float  sound_vol   = 0.5f;
    float  velocity    = 1.0f;
    bool   oneShot     = true;
    bool   released    = false;

    // ---- sample-rate dependent limits, recomputed in reset() ----
    // The original hard-codes these as bare numbers because it only ever ran at
    // 44100 Hz. They are all expressed in normalised (per-supersample) units, so
    // they have to track srScale to keep clamping at the same frequency.
    double srScale     = 1.0;
    int    minPeriod   = 8;
    float  fltwMax     = 0.1f;
    float  flthpMin    = 0.00001f;
    float  flthpMax    = 0.1f;
    int    iphaseMax   = 1023;

    // ---- retained to support repeat-speed re-trigger ----
    SfxrParams params;
    double sampleRate  = 44100.0;
    int    midiNote    = 69;
    uint32_t age       = 0;

    juce::Random rng { 0x5f3759df };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVoice)
};
