#include "SfxrPresetFile.h"

namespace SfxrPresetFile
{
    bool load (const juce::File& file, SfxrParams& p)
    {
        juce::FileInputStream in (file);
        if (!in.openedOk())
            return false;

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
        p.wave_type = wave;

        p.sound_vol = 0.5f;
        if (version == 102)
            readFloat (p.sound_vol);

        readFloat (p.base_freq);
        readFloat (p.freq_limit);
        readFloat (p.freq_ramp);
        if (version >= 101)
            readFloat (p.freq_dramp);
        readFloat (p.duty);
        readFloat (p.duty_ramp);

        readFloat (p.vib_strength);
        readFloat (p.vib_speed);
        readFloat (p.vib_delay);

        readFloat (p.env_attack);
        readFloat (p.env_sustain);
        readFloat (p.env_decay);
        readFloat (p.env_punch);

        bool filter_on = false;
        readBool (filter_on); // unused

        readFloat (p.lpf_resonance);
        readFloat (p.lpf_freq);
        readFloat (p.lpf_ramp);
        readFloat (p.hpf_freq);
        readFloat (p.hpf_ramp);

        readFloat (p.pha_offset);
        readFloat (p.pha_ramp);

        readFloat (p.repeat_speed);

        if (version >= 101)
        {
            readFloat (p.arp_speed);
            readFloat (p.arp_mod);
        }

        if (! allOk)
            return false;

        // Fold into the documented domain. The original sfxr writes whatever
        // Randomize left in its globals, so real .sfs files do contain values
        // outside these ranges; foldIntoDomain() reproduces the sound they had
        // there instead of flattening them to zero.
        p.foldIntoDomain();

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
