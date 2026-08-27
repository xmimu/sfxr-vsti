#include "SfxrPresetFile.h"

namespace SfxrPresetFile
{
    bool load (const juce::File& file, SfxrParams& p)
    {
        juce::FileInputStream in (file);
        if (!in.openedOk())
            return false;

        // The original UI calls ResetParams() before loading. Decode into a
        // fresh object so fields absent from v100/v101 use those defaults, and
        // so a short read never partially mutates the caller's object.
        SfxrParams loaded;

        bool allOk = true;
        auto readInt   = [&] (int& v)   { const bool ok = in.read (&v, sizeof (v)) == (int) sizeof (v); allOk &= ok; return ok; };
        auto readFloat = [&] (float& v) { const bool ok = in.read (&v, sizeof (v)) == (int) sizeof (v); allOk &= ok; return ok; };
        auto readBool  = [&] (bool& v)  { const bool ok = in.read (&v, sizeof (v)) == (int) sizeof (v); allOk &= ok; return ok; };

        int version = 0;
        if (!readInt (version))
            return false;
        if (version != 100 && version != 101 && version != 102)
            return false;

        int wave = 0;
        if (!readInt (wave))
            return false;
        if (wave < 0 || wave > 3)
            return false;
        loaded.wave_type = wave;

        loaded.sound_vol = 0.5f;
        if (version == 102)
            readFloat (loaded.sound_vol);

        readFloat (loaded.base_freq);
        readFloat (loaded.freq_limit);
        readFloat (loaded.freq_ramp);
        if (version >= 101)
            readFloat (loaded.freq_dramp);
        readFloat (loaded.duty);
        readFloat (loaded.duty_ramp);

        readFloat (loaded.vib_strength);
        readFloat (loaded.vib_speed);
        readFloat (loaded.vib_delay);

        readFloat (loaded.env_attack);
        readFloat (loaded.env_sustain);
        readFloat (loaded.env_decay);
        readFloat (loaded.env_punch);

        bool filter_on = false;
        readBool (filter_on); // unused

        readFloat (loaded.lpf_resonance);
        readFloat (loaded.lpf_freq);
        readFloat (loaded.lpf_ramp);
        readFloat (loaded.hpf_freq);
        readFloat (loaded.hpf_ramp);

        readFloat (loaded.pha_offset);
        readFloat (loaded.pha_ramp);

        readFloat (loaded.repeat_speed);

        if (version >= 101)
        {
            readFloat (loaded.arp_speed);
            readFloat (loaded.arp_mod);
        }

        if (! allOk)
            return false;

        const float values[] = {
            loaded.sound_vol, loaded.base_freq, loaded.freq_limit,
            loaded.freq_ramp, loaded.freq_dramp, loaded.duty, loaded.duty_ramp,
            loaded.vib_strength, loaded.vib_speed, loaded.vib_delay,
            loaded.env_attack, loaded.env_sustain, loaded.env_decay, loaded.env_punch,
            loaded.lpf_resonance, loaded.lpf_freq, loaded.lpf_ramp,
            loaded.hpf_freq, loaded.hpf_ramp, loaded.pha_offset, loaded.pha_ramp,
            loaded.repeat_speed, loaded.arp_speed, loaded.arp_mod
        };

        for (const float value : values)
            if (! std::isfinite (value))
                return false;

        // Match the original UI's Slider() pass after loading.
        loaded.clampToDomain();
        p = loaded;

        return true;
    }

    bool save (const juce::File& file, const SfxrParams& p)
    {
        juce::FileOutputStream out (file);
        if (!out.openedOk())
            return false;

        // FileOutputStream positions itself at the end of an existing file, so
        // without this an overwrite would append a second record and load()
        // would keep reading the old sound back.
        if (! out.setPosition (0) || ! out.truncate().wasOk())
            return false;

        bool ok = true;
        auto writeInt   = [&] (int v)   { ok = ok && out.write (&v, sizeof (v)); };
        auto writeFloat = [&] (float v) { ok = ok && out.write (&v, sizeof (v)); };
        auto writeBool  = [&] (bool v)  { ok = ok && out.write (&v, sizeof (v)); };

        writeInt (102);
        writeInt (p.wave_type);
        writeFloat (p.sound_vol);

        writeFloat (p.base_freq);
        writeFloat (p.freq_limit);
        writeFloat (p.freq_ramp);
        writeFloat (p.freq_dramp);
        writeFloat (p.duty);
        writeFloat (p.duty_ramp);

        writeFloat (p.vib_strength);
        writeFloat (p.vib_speed);
        writeFloat (p.vib_delay);

        writeFloat (p.env_attack);
        writeFloat (p.env_sustain);
        writeFloat (p.env_decay);
        writeFloat (p.env_punch);

        writeBool (false); // filter_on (unused in the synth)

        writeFloat (p.lpf_resonance);
        writeFloat (p.lpf_freq);
        writeFloat (p.lpf_ramp);
        writeFloat (p.hpf_freq);
        writeFloat (p.hpf_ramp);

        writeFloat (p.pha_offset);
        writeFloat (p.pha_ramp);

        writeFloat (p.repeat_speed);

        writeFloat (p.arp_speed);
        writeFloat (p.arp_mod);

        // Flush explicitly so a full disk or a read-only volume is reported here
        // rather than being swallowed by the destructor.
        out.flush();

        return ok && out.getStatus().wasOk();
    }
}
