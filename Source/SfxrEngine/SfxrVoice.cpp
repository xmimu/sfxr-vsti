#include "SfxrVoice.h"

namespace
{
    constexpr double PI = 3.14159265358979323846;
    constexpr float  kOutputGain = 0.2f;   // matches the original WAV export level

    // Progress through an envelope stage, in [0, 1].
    //
    // A stage can legitimately have zero length (e.g. DECAY TIME at 0), in which
    // case the original sfxr evaluated 0/0 and emitted a NaN. That is harmless
    // when the result is quantised into a WAV file, but a NaN reaching a DAW's
    // mix bus is not -- and the clamp below cannot catch it, because every
    // comparison against a NaN is false. A zero-length stage is over as soon as
    // it starts, so treating its progress as 0 gives the intended full-scale
    // value for the single sample before the stage advances.
    inline float envProgress (int time, int length) noexcept
    {
        return length > 0 ? (float) time / (float) length : 0.0f;
    }
}

SfxrVoice::SfxrVoice()
{
    juce::zeromem (phaser_buffer, sizeof (phaser_buffer));
    juce::zeromem (noise_buffer, sizeof (noise_buffer));
}

void SfxrVoice::start (const SfxrParams& p, double sr, int note,
                       float vel, bool oneShotMode)
{
    params      = p;
    sampleRate  = sr;
    midiNote    = note;
    velocity    = vel;
    oneShot     = oneShotMode;
    released    = false;
    age         = 0;
    reset (false);
}

void SfxrVoice::noteOff()
{
    if (!oneShot)
        released = true;
}

void SfxrVoice::stop() noexcept
{
    playing  = false;
    released = true;
}

// Every magic constant in the original sfxr is calibrated for 44100 Hz. To get
// identical output at any other sample rate each one has to be rescaled
// according to what it actually represents:
//
//   * a length or period measured in samples          -> * srScale
//   * a first-order per-sample rate (x += a, x *= 1+a) -> a / srScale
//   * a second-order per-sample rate (a += b)          -> b / srScale^2
//
// Note that the oscillator, the two filters and the phaser all run at the
// supersample rate (8 * sampleRate) while the envelopes, slides and vibrato run
// at the output sample rate. srScale is the same for both because the 8x
// supersampling factor is fixed.
void SfxrVoice::reset (bool restart)
{
    if (!restart)
        phase = 0;

    srScale = sampleRate / 44100.0;
    const double noteScale = std::pow (2.0, -(midiNote - 69) / 12.0);

    // Oscillator period, measured in supersamples -> scales with the rate.
    fperiod    = 100.0 / (params.base_freq * params.base_freq + 0.001) * noteScale * srScale;
    period     = (int) fperiod;
    fmaxperiod = 100.0 / (params.freq_limit * params.freq_limit + 0.001) * noteScale * srScale;

    // fperiod *= fslide once per output sample -> first order.
    fslide  = 1.0 - std::pow ((double) params.freq_ramp, 3.0) * 0.01 / srScale;
    // fslide += fdslide once per output sample -> second order.
    fdslide = -std::pow ((double) params.freq_dramp, 3.0) * 0.000001 / (srScale * srScale);

    square_duty  = 0.5f - params.duty * 0.5f;
    // square_duty += square_slide once per output sample -> first order.
    square_slide = (float) (-params.duty_ramp * 0.00005 / srScale);

    if (params.arp_mod >= 0.0f)
        arp_mod = 1.0 - std::pow ((double) params.arp_mod, 2.0) * 0.9;
    else
        arp_mod = 1.0 + std::pow ((double) params.arp_mod, 2.0) * 10.0;
    arp_time  = 0;
    arp_limit = (int) (std::pow (1.0f - params.arp_speed, 2.0f) * 20000.0 * srScale + 32);
    if (params.arp_speed == 1.0f)
        arp_limit = 0;

    // Clamp limits that the render loop applies to per-supersample coefficients.
    minPeriod = juce::jmax (8, (int) (8.0 * srScale));
    fltwMax   = (float) (0.1 / srScale);
    flthpMin  = (float) (0.00001 / srScale);
    flthpMax  = (float) (0.1 / srScale);
    iphaseMax = juce::jmin (kPhaserBufferSize - 1, (int) (1023.0 * srScale));

    if (!restart)
    {
        // filter -- fltw, flthp and fltdmp are all applied once per supersample,
        // so all three are first-order rates.
        fltp   = 0.0f;
        fltdp  = 0.0f;
        const double fltw44 = std::pow ((double) params.lpf_freq, 3.0) * 0.1;
        fltw   = (float) (fltw44 / srScale);
        fltw_d = (float) (1.0 + params.lpf_ramp * 0.0001 / srScale);
        fltdmp = (float) (5.0 / (1.0 + std::pow ((double) params.lpf_resonance, 2.0) * 20.0)
                              * (0.01 + fltw44) / srScale);
        if (fltdmp > 0.8f)
            fltdmp = 0.8f;
        fltphp  = 0.0f;
        flthp   = (float) (std::pow ((double) params.hpf_freq, 2.0) * 0.1 / srScale);
        flthp_d = (float) (1.0 + params.hpf_ramp * 0.0003 / srScale);

        // vibrato -- vib_phase advances once per output sample -> first order.
        vib_phase = 0.0f;
        vib_speed = (float) (std::pow ((double) params.vib_speed, 2.0) * 0.01 / srScale);
        vib_amp   = params.vib_strength * 0.5f;
        vib_delay_len = (int) (params.vib_delay * params.vib_delay * 100000.0 * srScale);
        vib_delay_time = 0;

        // envelope -- lengths in output samples. A stage may legitimately be zero
        // samples long, in which case the render loop skips straight past it; the
        // division is guarded there rather than padding the length here, so that
        // non-degenerate sounds stay bit-identical to the original.
        env_vol   = 0.0f;
        env_stage = 0;
        env_time  = 0;
        env_length[0] = (int) (params.env_attack  * params.env_attack  * 100000.0 * srScale);
        env_length[1] = (int) (params.env_sustain * params.env_sustain * 100000.0 * srScale);
        env_length[2] = (int) (params.env_decay   * params.env_decay   * 100000.0 * srScale);

        // phaser -- fphase is a delay length in supersamples (scales with the
        // rate), while fdphase sweeps that length once per output sample, which
        // already works out to a rate-independent change in delay *time*.
        fphase  = (float) (std::pow ((double) params.pha_offset, 2.0) * 1020.0 * srScale);
        if (params.pha_offset < 0.0f) fphase = -fphase;
        fdphase = std::pow (params.pha_ramp, 2.0f) * 1.0f;
        if (params.pha_ramp < 0.0f) fdphase = -fdphase;
        iphase = juce::jmin (iphaseMax, std::abs ((int) fphase));
        ipp    = 0;
        juce::zeromem (phaser_buffer, sizeof (phaser_buffer));

        // noise
        for (int i = 0; i < 32; i++)
            noise_buffer[i] = rng.nextFloat() * 2.0f - 1.0f;

        // repeat -- a length in output samples.
        rep_time  = 0;
        rep_limit = (int) (std::pow (1.0f - params.repeat_speed, 2.0f) * 20000.0 * srScale + 32);
        if (params.repeat_speed == 0.0f)
            rep_limit = 0;
    }

    // snapshot the handful of params the render loop reads directly
    wave_type    = params.wave_type;
    p_freq_limit = params.freq_limit;
    p_env_punch  = params.env_punch;
    p_lpf_freq   = params.lpf_freq;
    sound_vol    = params.sound_vol;

    playing = true;
}

bool SfxrVoice::render (float* buffer, int numSamples)
{
    age++;

    for (int i = 0; i < numSamples; i++)
    {
        if (!playing)
        {
            buffer[i] = 0.0f;
            continue;
        }

        rep_time++;
        if (rep_limit != 0 && rep_time >= rep_limit)
        {
            rep_time = 0;
            reset (true);
        }

        // frequency envelopes / arpeggios
        arp_time++;
        if (arp_limit != 0 && arp_time >= arp_limit)
        {
            arp_limit = 0;
            fperiod *= arp_mod;
        }
        fslide += fdslide;
        fperiod *= fslide;
        if (fperiod > fmaxperiod)
        {
            fperiod = fmaxperiod;
            if (p_freq_limit > 0.0f)
                playing = false;
        }

        float rfperiod = (float) fperiod;

        // vibrato (with the delay the original never implemented)
        float vibCurrent = vib_amp;
        if (vib_delay_len > 0 && vib_delay_time < vib_delay_len)
        {
            vib_delay_time++;
            vibCurrent = vib_amp * ((float) vib_delay_time / (float) vib_delay_len);
        }
        if (vibCurrent > 0.0f)
        {
            vib_phase += vib_speed;
            rfperiod = (float) (fperiod * (1.0 + std::sin (vib_phase) * vibCurrent));
        }

        period = (int) rfperiod;
        if (period < minPeriod)
            period = minPeriod;
        square_duty += square_slide;
        if (square_duty < 0.0f)  square_duty = 0.0f;
        if (square_duty > 0.5f)  square_duty = 0.5f;

        // volume envelope
        env_time++;
        if (!oneShot && !released && env_stage == 1)
        {
            if (env_time > env_length[1])
                env_time = env_length[1];
        }
        else if (env_time > env_length[env_stage])
        {
            env_time = 0;
            env_stage++;
            if (env_stage == 3)
                playing = false;
        }

        if (env_stage == 0)
            env_vol = envProgress (env_time, env_length[0]);
        else if (env_stage == 1)
        {
            if (!oneShot && !released)
                env_vol = 1.0f;
            else
                env_vol = 1.0f + std::pow (1.0f - envProgress (env_time, env_length[1]), 1.0f) * 2.0f * p_env_punch;
        }
        else if (env_stage == 2)
            env_vol = 1.0f - envProgress (env_time, env_length[2]);

        // phaser step
        fphase += fdphase;
        iphase  = std::abs ((int) fphase);
        if (iphase > iphaseMax)
            iphase = iphaseMax;

        if (flthp_d != 0.0f)
        {
            flthp *= flthp_d;
            if (flthp < flthpMin) flthp = flthpMin;
            if (flthp > flthpMax) flthp = flthpMax;
        }

        float ssample = 0.0f;
        for (int si = 0; si < 8; si++) // 8x supersampling
        {
            float sample = 0.0f;
            phase++;
            if (phase >= period)
            {
                phase %= period;
                if (wave_type == 3)
                    for (int j = 0; j < 32; j++)
                        noise_buffer[j] = rng.nextFloat() * 2.0f - 1.0f;
            }

            // base waveform
            const float fp = (float) phase / period;
            switch (wave_type)
            {
                case 0: sample = (fp < square_duty) ? 0.5f : -0.5f; break;   // square
                case 1: sample = 1.0f - fp * 2; break;                       // sawtooth
                case 2: sample = (float) std::sin (fp * 2.0 * PI); break;    // sine
                case 3: sample = noise_buffer[phase * 32 / period]; break;   // noise
            }

            // low-pass filter
            const float pp = fltp;
            fltw *= fltw_d;
            if (fltw < 0.0f)     fltw = 0.0f;
            if (fltw > fltwMax)  fltw = fltwMax;
            if (p_lpf_freq != 1.0f)
            {
                fltdp += (sample - fltp) * fltw;
                fltdp -= fltdp * fltdmp;
            }
            else
            {
                fltp  = sample;
                fltdp = 0.0f;
            }
            fltp += fltdp;

            // high-pass filter
            fltphp += fltp - pp;
            fltphp -= fltphp * flthp;
            sample = fltphp;

            // phaser
            phaser_buffer[ipp & kPhaserBufferMask] = sample;
            sample += phaser_buffer[(ipp - iphase + kPhaserBufferSize) & kPhaserBufferMask];
            ipp = (ipp + 1) & kPhaserBufferMask;

            // final accumulation and envelope application
            ssample += sample * env_vol;
        }
        ssample = ssample / 8.0f;
        ssample *= 2.0f * sound_vol;
        ssample *= velocity;
        ssample *= kOutputGain;

        if (ssample > 1.0f)  ssample = 1.0f;
        if (ssample < -1.0f) ssample = -1.0f;

        buffer[i] = ssample;
    }

    return playing;
}
