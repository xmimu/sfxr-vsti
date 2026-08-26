#include <JuceHeader.h>
#include "../Source/SfxrEngine/SfxrParams.h"
#include "../Source/SfxrEngine/SfxrVoice.h"

#include <cstdio>
#include <vector>

static void reportNote (const SfxrParams& p, int note)
{
    SfxrVoice voice;
    voice.start (p, 44100.0, note, 1.0f, true);

    const int numSamples = 44100; // 1 second
    std::vector<float> buffer (numSamples);
    voice.render (buffer.data(), numSamples);

    float peak = 0.0f;
    double sumSq = 0.0;
    int signChanges = 0;
    int lastSign = (buffer[0] >= 0.0f) ? 1 : -1;

    for (int i = 0; i < numSamples; i++)
    {
        const float s = buffer[i];
        peak = std::max (peak, std::abs (s));
        sumSq += (double) s * (double) s;

        const int sign = (s >= 0.0f) ? 1 : -1;
        if (sign != lastSign)
            signChanges++;
        lastSign = sign;
    }

    const double rms = std::sqrt (sumSq / numSamples);
    std::printf ("note %3d : peak=%.4f  rms=%.5f  zeroCrossings=%d  (~%.1f Hz)\n",
                 note, peak, rms, signChanges, signChanges / 2.0);
}

int main()
{
    SfxrParams p;
    p.resetDefaults();
    p.wave_type  = 0;    // square
    p.base_freq  = 0.3f;
    p.env_attack = 0.0f;
    p.env_sustain = 0.2f;
    p.env_decay  = 0.3f;
    p.sound_vol  = 0.5f;

    std::printf ("Square, base_freq=0.3 (A4 == note 69, no transpose):\n");
    reportNote (p, 69);
    reportNote (p, 81);  // +1 octave
    reportNote (p, 57);  // -1 octave

    SfxrParams saw = p;
    saw.wave_type = 1;   // sawtooth
    std::printf ("\nSawtooth:\n");
    reportNote (saw, 69);

    return 0;
}
