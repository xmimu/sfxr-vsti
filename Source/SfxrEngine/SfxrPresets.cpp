#include "SfxrPresets.h"

namespace
{
    // Original helpers: rnd(n) -> nextInt(n+1), frnd(range) -> nextFloat()*range
    int   irnd (juce::Random& r, int n)     { return r.nextInt (n + 1); }
    float frnd (juce::Random& r, float range){ return r.nextFloat() * range; }
}

const char* presetCategoryName (PresetCategory c)
{
    switch (c)
    {
        case PresetCategory::PickupCoin: return "PICKUP/COIN";
        case PresetCategory::LaserShoot: return "LASER/SHOOT";
        case PresetCategory::Explosion:  return "EXPLOSION";
        case PresetCategory::Powerup:    return "POWERUP";
        case PresetCategory::HitHurt:    return "HIT/HURT";
        case PresetCategory::Jump:       return "JUMP";
        case PresetCategory::BlipSelect: return "BLIP/SELECT";
        case PresetCategory::Count:      break;
    }
    return "";
}

void generatePreset (SfxrParams& p, PresetCategory c, juce::Random& r)
{
    p.resetDefaults();

    switch (c)
    {
        case PresetCategory::PickupCoin:
        {
            p.base_freq   = 0.4f + frnd (r, 0.5f);
            p.env_attack  = 0.0f;
            p.env_sustain = frnd (r, 0.1f);
            p.env_decay   = 0.1f + frnd (r, 0.4f);
            p.env_punch   = 0.3f + frnd (r, 0.3f);
            if (irnd (r, 1))
            {
                p.arp_speed = 0.5f + frnd (r, 0.2f);
                p.arp_mod   = 0.2f + frnd (r, 0.4f);
            }
            break;
        }
        case PresetCategory::LaserShoot:
        {
            p.wave_type = irnd (r, 2);
            if (p.wave_type == 2 && irnd (r, 1))
                p.wave_type = irnd (r, 1);
            p.base_freq  = 0.5f + frnd (r, 0.5f);
            p.freq_limit = p.base_freq - 0.2f - frnd (r, 0.6f);
            if (p.freq_limit < 0.2f) p.freq_limit = 0.2f;
            p.freq_ramp = -0.15f - frnd (r, 0.2f);
            if (irnd (r, 2) == 0)
            {
                p.base_freq  = 0.3f + frnd (r, 0.6f);
                p.freq_limit = frnd (r, 0.1f);
                p.freq_ramp  = -0.35f - frnd (r, 0.3f);
            }
            if (irnd (r, 1))
            {
                p.duty      = frnd (r, 0.5f);
                p.duty_ramp = frnd (r, 0.2f);
            }
            else
            {
                p.duty      = 0.4f + frnd (r, 0.5f);
                p.duty_ramp = -frnd (r, 0.7f);
            }
            p.env_attack  = 0.0f;
            p.env_sustain = 0.1f + frnd (r, 0.2f);
            p.env_decay   = frnd (r, 0.4f);
            if (irnd (r, 1))
                p.env_punch = frnd (r, 0.3f);
            if (irnd (r, 2) == 0)
            {
                p.pha_offset = frnd (r, 0.2f);
                p.pha_ramp   = -frnd (r, 0.2f);
            }
            if (irnd (r, 1))
                p.hpf_freq = frnd (r, 0.3f);
            break;
        }
        case PresetCategory::Explosion:
        {
            p.wave_type = 3;
            if (irnd (r, 1))
            {
                p.base_freq  = 0.1f + frnd (r, 0.4f);
                p.freq_ramp  = -0.1f + frnd (r, 0.4f);
            }
            else
            {
                p.base_freq  = 0.2f + frnd (r, 0.7f);
                p.freq_ramp  = -0.2f - frnd (r, 0.2f);
            }
            p.base_freq *= p.base_freq;
            if (irnd (r, 4) == 0)
                p.freq_ramp = 0.0f;
            if (irnd (r, 2) == 0)
                p.repeat_speed = 0.3f + frnd (r, 0.5f);
            p.env_attack  = 0.0f;
            p.env_sustain = 0.1f + frnd (r, 0.3f);
            p.env_decay   = frnd (r, 0.5f);
            if (irnd (r, 1) == 0)
            {
                p.pha_offset = -0.3f + frnd (r, 0.9f);
                p.pha_ramp   = -frnd (r, 0.3f);
            }
            p.env_punch = 0.2f + frnd (r, 0.6f);
            if (irnd (r, 1))
            {
                p.vib_strength = frnd (r, 0.7f);
                p.vib_speed    = frnd (r, 0.6f);
            }
            if (irnd (r, 2) == 0)
            {
                p.arp_speed = 0.6f + frnd (r, 0.3f);
                p.arp_mod   = 0.8f - frnd (r, 1.6f);
            }
            break;
        }
        case PresetCategory::Powerup:
        {
            if (irnd (r, 1))
                p.wave_type = 1;
            else
                p.duty = frnd (r, 0.6f);
            if (irnd (r, 1))
            {
                p.base_freq    = 0.2f + frnd (r, 0.3f);
                p.freq_ramp    = 0.1f + frnd (r, 0.4f);
                p.repeat_speed = 0.4f + frnd (r, 0.4f);
            }
            else
            {
                p.base_freq = 0.2f + frnd (r, 0.3f);
                p.freq_ramp = 0.05f + frnd (r, 0.2f);
                if (irnd (r, 1))
                {
                    p.vib_strength = frnd (r, 0.7f);
                    p.vib_speed    = frnd (r, 0.6f);
                }
            }
            p.env_attack  = 0.0f;
            p.env_sustain = frnd (r, 0.4f);
            p.env_decay   = 0.1f + frnd (r, 0.4f);
            break;
        }
        case PresetCategory::HitHurt:
        {
            p.wave_type = irnd (r, 2);
            if (p.wave_type == 2)
                p.wave_type = 3;
            if (p.wave_type == 0)
                p.duty = frnd (r, 0.6f);
            p.base_freq  = 0.2f + frnd (r, 0.6f);
            p.freq_ramp  = -0.3f - frnd (r, 0.4f);
            p.env_attack = 0.0f;
            p.env_sustain = frnd (r, 0.1f);
            p.env_decay  = 0.1f + frnd (r, 0.2f);
            if (irnd (r, 1))
                p.hpf_freq = frnd (r, 0.3f);
            break;
        }
        case PresetCategory::Jump:
        {
            p.wave_type  = 0;
            p.duty       = frnd (r, 0.6f);
            p.base_freq  = 0.3f + frnd (r, 0.3f);
            p.freq_ramp  = 0.1f + frnd (r, 0.2f);
            p.env_attack = 0.0f;
            p.env_sustain = 0.1f + frnd (r, 0.3f);
            p.env_decay  = 0.1f + frnd (r, 0.2f);
            if (irnd (r, 1))
                p.hpf_freq = frnd (r, 0.3f);
            if (irnd (r, 1))
                p.lpf_freq = 1.0f - frnd (r, 0.6f);
            break;
        }
        case PresetCategory::BlipSelect:
        {
            p.wave_type  = irnd (r, 1);
            if (p.wave_type == 0)
                p.duty = frnd (r, 0.6f);
            p.base_freq  = 0.2f + frnd (r, 0.4f);
            p.env_attack = 0.0f;
            p.env_sustain = 0.1f + frnd (r, 0.1f);
            p.env_decay  = frnd (r, 0.2f);
            p.hpf_freq   = 0.1f;
            break;
        }
        case PresetCategory::Count:
            break;
    }

    p.foldIntoDomain();
}

void randomize (SfxrParams& p, juce::Random& r)
{
    p.base_freq  = std::pow (frnd (r, 2.0f) - 1.0f, 2.0f);
    if (irnd (r, 1))
        p.base_freq = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f) + 0.5f;
    p.freq_limit = 0.0f;
    p.freq_ramp  = std::pow (frnd (r, 2.0f) - 1.0f, 5.0f);
    if (p.base_freq > 0.7f && p.freq_ramp > 0.2f)
        p.freq_ramp = -p.freq_ramp;
    if (p.base_freq < 0.2f && p.freq_ramp < -0.05f)
        p.freq_ramp = -p.freq_ramp;
    p.freq_dramp = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    p.duty       = frnd (r, 2.0f) - 1.0f;
    p.duty_ramp  = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    p.vib_strength = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    p.vib_speed    = frnd (r, 2.0f) - 1.0f;
    p.vib_delay    = frnd (r, 2.0f) - 1.0f;
    p.env_attack   = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    p.env_sustain  = std::pow (frnd (r, 2.0f) - 1.0f, 2.0f);
    p.env_decay    = frnd (r, 2.0f) - 1.0f;
    p.env_punch    = std::pow (frnd (r, 0.8f), 2.0f);
    if (p.env_attack + p.env_sustain + p.env_decay < 0.2f)
    {
        p.env_sustain += 0.2f + frnd (r, 0.3f);
        p.env_decay   += 0.2f + frnd (r, 0.3f);
    }
    p.lpf_resonance = frnd (r, 2.0f) - 1.0f;
    p.lpf_freq      = 1.0f - std::pow (frnd (r, 1.0f), 3.0f);
    p.lpf_ramp      = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    if (p.lpf_freq < 0.1f && p.lpf_ramp < -0.05f)
        p.lpf_ramp = -p.lpf_ramp;
    p.hpf_freq = std::pow (frnd (r, 1.0f), 5.0f);
    p.hpf_ramp = std::pow (frnd (r, 2.0f) - 1.0f, 5.0f);
    p.pha_offset = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    p.pha_ramp   = std::pow (frnd (r, 2.0f) - 1.0f, 3.0f);
    p.repeat_speed = frnd (r, 2.0f) - 1.0f;
    p.arp_speed = frnd (r, 2.0f) - 1.0f;
    p.arp_mod   = frnd (r, 2.0f) - 1.0f;

    // The assignments above are a literal port and deliberately overshoot the
    // documented ranges, exactly as the original does. Fold them back in a way
    // that keeps the sound rather than letting the parameter tree clamp them.
    p.foldIntoDomain();
}

void mutate (SfxrParams& p, juce::Random& r)
{
    if (irnd (r, 1)) p.base_freq    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.freq_ramp    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.freq_dramp   += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.duty         += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.duty_ramp    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.vib_strength += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.vib_speed    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.vib_delay    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.env_attack   += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.env_sustain  += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.env_decay    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.env_punch    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.lpf_resonance += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.lpf_freq     += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.lpf_ramp     += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.hpf_freq     += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.hpf_ramp     += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.pha_offset   += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.pha_ramp     += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.repeat_speed += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.arp_speed    += frnd (r, 0.1f) - 0.05f;
    if (irnd (r, 1)) p.arp_mod      += frnd (r, 0.1f) - 0.05f;

    // Repeated nudges drift out of range over time.
    p.foldIntoDomain();
}
