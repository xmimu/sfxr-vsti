#include "SfxrVoice.h"

namespace
{
    constexpr double PI = 3.14159265358979323846;
    constexpr float  kOutputGain = 0.2f;   // matches the original WAV export level
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

void SfxrVoice::reset (bool restart)
{
    if (!restart)
        phase = 0;

    const double srScale   = sampleRate / 44100.0;
    const double noteScale = std::pow (2.0, -(midiNote - 69) / 12.0);

    fperiod    = 100.0 / (params.base_freq * params.base_freq + 0.001) * noteScale;
    period     = (int) fperiod;
    fmaxperiod = 100.0 / (params.freq_limit * params.freq_limit + 0.001) * noteScale;
    fslide     = 1.0 - std::pow ((double) params.freq_ramp, 3.0) * 0.01;
    fdslide    = -std::pow ((double) params.freq_dramp, 3.0) * 0.000001;
    square_duty  = 0.5f - params.duty * 0.5f;
    square_slide = -params.duty_ramp * 0.00005f;

    if (params.arp_mod >= 0.0f)
        arp_mod = 1.0 - std::pow ((double) params.arp_mod, 2.0) * 0.9;
    else
        arp_mod = 1.0 + std::pow ((double) params.arp_mod, 2.0) * 10.0;
    arp_time  = 0;
    arp_limit = (int) (std::pow (1.0f - params.arp_speed, 2.0f) * 20000.0 * srScale + 32);
    if (params.arp_speed == 1.0f)
        arp_limit = 0;

    if (!restart)
    {
        // filter
        fltp   = 0.0f;
        fltdp  = 0.0f;
        fltw   = std::pow (params.lpf_freq, 3.0f) * 0.1f;
        fltw_d = 1.0f + params.lpf_ramp * 0.0001f;
        fltdmp = 5.0f / (1.0f + std::pow (params.lpf_resonance, 2.0f) * 20.0f) * (0.01f + fltw);
        if (fltdmp > 0.8f)
            fltdmp = 0.8f;
        fltphp  = 0.0f;
        flthp   = std::pow (params.hpf_freq, 2.0f) * 0.1f;
        flthp_d = 1.0 + params.hpf_ramp * 0.0003f;

        // vibrato
        vib_phase = 0.0f;
        vib_speed = std::pow (params.vib_speed, 2.0f) * 0.01f * (float) srScale;
        vib_amp   = params.vib_strength * 0.5f;
        vib_delay_len = (int) (params.vib_delay * params.vib_delay * 100000.0 * srScale);
        vib_delay_time = 0;

        // envelope
        env_vol   = 0.0f;
        env_stage = 0;
        env_time  = 0;
        env_length[0] = (int) (params.env_attack * params.env_attack * 100000.0 * srScale);
        env_length[1] = (int) (params.env_sustain * params.env_sustain * 100000.0 * srScale);
        env_length[2] = (int) (params.env_decay * params.env_decay * 100000.0 * srScale);

        // phaser
        fphase  = std::pow (params.pha_offset, 2.0f) * 1020.0f;
        if (params.pha_offset < 0.0f) fphase = -fphase;
        fdphase = std::pow (params.pha_ramp, 2.0f) * 1.0f;
        if (params.pha_ramp < 0.0f) fdphase = -fdphase;
        iphase = std::abs ((int) fphase);
        ipp    = 0;
        juce::zeromem (phaser_buffer, sizeof (phaser_buffer));

        // noise
        for (int i = 0; i < 32; i++)
            noise_buffer[i] = rng.nextFloat() * 2.0f - 1.0f;

        // repeat
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
        if (period < 8)
            period = 8;
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
            env_vol = (float) env_time / env_length[0];
        else if (env_stage == 1)
        {
            if (!oneShot && !released)
                env_vol = 1.0f;
            else
                env_vol = 1.0f + std::pow (1.0f - (float) env_time / env_length[1], 1.0f) * 2.0f * p_env_punch;
        }
        else if (env_stage == 2)
            env_vol = 1.0f - (float) env_time / env_length[2];

        // phaser step
        fphase += fdphase;
        iphase  = std::abs ((int) fphase);
        if (iphase > 1023)
            iphase = 1023;

        if (flthp_d != 0.0f)
        {
            flthp *= flthp_d;
            if (flthp < 0.00001f) flthp = 0.00001f;
            if (flthp > 0.1f)     flthp = 0.1f;
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
            if (fltw < 0.0f) fltw = 0.0f;
            if (fltw > 0.1f) fltw = 0.1f;
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
            phaser_buffer[ipp & 1023] = sample;
            sample += phaser_buffer[(ipp - iphase + 1024) & 1023];
            ipp = (ipp + 1) & 1023;

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
