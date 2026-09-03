// Offline test suite for the sfxr synthesis engine.
//
// These tests are deliberately independent of JUCE's plugin hosting so they can
// run headless in CI. Everything here is a property that must hold for the
// engine to behave as an instrument:
//
//   * pitch, envelope timing, vibrato rate, slides and duty sweep must remain
//     consistent across sample rates (the original constants target 44100 Hz)
//   * MIDI notes must transpose by equal temperament
//   * no parameter combination may ever emit a non-finite sample or exceed the
//     0 dBFS clamp
//   * .sfs files must round-trip
//
// Exits non-zero on the first failure category so CI fails loudly.

#include <JuceHeader.h>
#include "../Source/SfxrEngine/SfxrParams.h"
#include "../Source/SfxrEngine/SfxrVoice.h"
#include "../Source/SfxrEngine/SfxrEngine.h"
#include "../Source/SfxrEngine/SfxrAudioExporter.h"
#include "../Source/SfxrEngine/SfxrPresets.h"
#include "../Source/SfxrEngine/SfxrPresetFile.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace
{
    int failures = 0;
    int checks   = 0;

    void check (bool ok, const juce::String& what)
    {
        checks++;
        if (! ok)
        {
            failures++;
            std::printf ("  FAIL  %s\n", what.toRawUTF8());
        }
    }

    // Relative comparison, for quantities that are only equal up to the integer
    // truncation the engine does internally.
    void checkClose (double actual, double expected, double tolerance, const juce::String& what)
    {
        const double denom = std::abs (expected) > 1.0e-12 ? std::abs (expected) : 1.0;
        const double rel   = std::abs (actual - expected) / denom;
        checks++;
        if (! (rel <= tolerance))
        {
            failures++;
            std::printf ("  FAIL  %s: got %.6f, expected %.6f (rel err %.4f > %.4f)\n",
                         what.toRawUTF8(), actual, expected, rel, tolerance);
        }
    }

    void section (const char* name)
    {
        std::printf ("\n== %s ==\n", name);
    }

    struct Render
    {
        std::vector<float> samples;
        double sampleRate = 44100.0;

        int   nonFinite   = 0;
        float peak        = 0.0f;
        int   activeUntil = 0;   // last index with |sample| above the noise floor
    };

    // Renders one voice for a fixed duration in *seconds*, so results can be
    // compared directly across sample rates.
    Render renderVoice (const SfxrParams& p, double sampleRate, int note,
                        double seconds, bool oneShot = true,
                        double releaseAfterSeconds = -1.0)
    {
        Render r;
        r.sampleRate = sampleRate;

        SfxrVoice voice;
        voice.start (p, sampleRate, note, 1.0f, oneShot);

        const int total = (int) (sampleRate * seconds);
        r.samples.assign ((size_t) total, 0.0f);

        const int releaseAt = releaseAfterSeconds >= 0.0
                            ? (int) (sampleRate * releaseAfterSeconds) : -1;

        // Render in blocks so we exercise the same path the plugin uses.
        const int blockSize = 512;
        for (int i = 0; i < total; i += blockSize)
        {
            if (releaseAt >= 0 && i >= releaseAt && i - blockSize < releaseAt)
                voice.noteOff();

            const int n = juce::jmin (blockSize, total - i);
            voice.render (r.samples.data() + i, n);
        }

        for (int i = 0; i < total; i++)
        {
            const float s = r.samples[(size_t) i];
            if (! std::isfinite (s))
            {
                r.nonFinite++;
                continue;
            }
            r.peak = juce::jmax (r.peak, std::abs (s));
            if (std::abs (s) > 1.0e-5f)
                r.activeUntil = i;
        }

        return r;
    }

    // Fundamental frequency from mean zero-crossing rate over the sounding part.
    double measureFrequency (const Render& r)
    {
        int crossings = 0, last = 0, firstIdx = -1, lastIdx = -1;

        for (int i = 0; i <= r.activeUntil; i++)
        {
            const float s = r.samples[(size_t) i];
            if (std::abs (s) < 1.0e-5f)
                continue;

            const int sign = s > 0.0f ? 1 : -1;
            if (last != 0 && sign != last)
            {
                crossings++;
                if (firstIdx < 0) firstIdx = i;
                lastIdx = i;
            }
            last = sign;
        }

        if (crossings < 3 || lastIdx <= firstIdx)
            return 0.0;

        const double span = (lastIdx - firstIdx) / r.sampleRate;
        return (crossings - 1) / 2.0 / span;
    }

    // Sub-sample-accurate upward zero crossings. Plain integer crossings quantise
    // the carrier period to one output sample, which is far too coarse to measure
    // a modulated carrier at 44.1 kHz.
    std::vector<double> upwardCrossings (const Render& r)
    {
        std::vector<double> zc;

        for (int i = 1; i <= r.activeUntil; i++)
        {
            const float a = r.samples[(size_t) (i - 1)];
            const float b = r.samples[(size_t) i];

            if (a <= 0.0f && b > 0.0f)
            {
                const double frac = (double) -a / ((double) b - (double) a);
                zc.push_back ((i - 1 + frac) / r.sampleRate);
            }
        }

        return zc;
    }

    // Vibrato rate, measured as the modulation rate of the carrier's own period.
    double measureVibratoRate (const Render& r)
    {
        const auto zc = upwardCrossings (r);
        if (zc.size() < 16)
            return 0.0;

        std::vector<double> period, at;
        for (size_t i = 1; i < zc.size(); i++)
        {
            period.push_back (zc[i] - zc[i - 1]);
            at.push_back (zc[i]);
        }

        // Light smoothing: the period series is itself sampled irregularly, so a
        // few-tap moving average removes spurious mean crossings without moving
        // the modulation frequency.
        std::vector<double> smooth (period.size());
        const int halfWindow = 2;
        for (int i = 0; i < (int) period.size(); i++)
        {
            double sum = 0.0;
            int n = 0;
            for (int k = -halfWindow; k <= halfWindow; k++)
            {
                const int j = i + k;
                if (j >= 0 && j < (int) period.size()) { sum += period[(size_t) j]; n++; }
            }
            smooth[(size_t) i] = sum / n;
        }

        double mean = 0.0;
        for (auto x : smooth) mean += x;
        mean /= (double) smooth.size();

        int cycles = 0;
        double firstT = -1.0, lastT = -1.0;
        for (size_t i = 1; i < smooth.size(); i++)
        {
            if (smooth[i - 1] - mean <= 0.0 && smooth[i] - mean > 0.0)
            {
                cycles++;
                if (firstT < 0.0) firstT = at[i];
                lastT = at[i];
            }
        }

        if (cycles < 2 || lastT <= firstT)
            return 0.0;

        return (cycles - 1) / (lastT - firstT);
    }

    // Fundamental frequency measured over a time window, in seconds.
    double measureFrequencyBetween (const Render& r, double fromSeconds, double toSeconds)
    {
        Render window = r;
        const int from = juce::jlimit (0, (int) r.samples.size(), (int) (fromSeconds * r.sampleRate));
        const int to   = juce::jlimit (from, (int) r.samples.size(), (int) (toSeconds * r.sampleRate));

        window.samples.assign (r.samples.begin() + from, r.samples.begin() + to);
        window.activeUntil = (int) window.samples.size() - 1;

        return measureFrequency (window);
    }

    SfxrParams toneParams()
    {
        SfxrParams p;
        p.wave_type   = 2;      // sine: cleanest for frequency measurement
        p.base_freq   = 0.3f;
        p.env_attack  = 0.0f;
        p.env_sustain = 0.5f;   // 0.25 * 100000 / 44100 = 567 ms
        p.env_decay   = 0.1f;
        p.sound_vol   = 0.5f;
        return p;
    }

    const double kRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
}

//==============================================================================
static void testPitchIsSampleRateIndependent()
{
    section ("pitch is sample-rate independent");

    const auto p = toneParams();
    const double reference = measureFrequency (renderVoice (p, 44100.0, 69, 0.5));

    check (reference > 100.0 && reference < 1000.0,
           "reference pitch is in a sane range (got " + juce::String (reference, 2) + " Hz)");

    for (double sr : kRates)
    {
        const double hz = measureFrequency (renderVoice (p, sr, 69, 0.5));
        std::printf ("  sr=%7.0f  %8.2f Hz\n", sr, hz);
        checkClose (hz, reference, 0.02, "pitch at " + juce::String (sr, 0) + " Hz");
    }
}

static void testMidiTransposition()
{
    section ("MIDI transposition follows equal temperament");

    const auto p = toneParams();

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const double root = measureFrequency (renderVoice (p, sr, 69, 0.5));

        checkClose (measureFrequency (renderVoice (p, sr, 81, 0.5)), root * 2.0, 0.02,
                    "+1 octave at " + juce::String (sr, 0));
        checkClose (measureFrequency (renderVoice (p, sr, 57, 0.5)), root * 0.5, 0.02,
                    "-1 octave at " + juce::String (sr, 0));
        checkClose (measureFrequency (renderVoice (p, sr, 76, 0.5)), root * std::pow (2.0, 7.0 / 12.0), 0.02,
                    "+7 semitones at " + juce::String (sr, 0));
    }
}

static void testEnvelopeTimingIsSampleRateIndependent()
{
    section ("envelope duration is sample-rate independent");

    SfxrParams p = toneParams();
    p.env_attack  = 0.3f;
    p.env_sustain = 0.4f;
    p.env_decay   = 0.5f;

    // Expected total length, in seconds, from the original 44.1 kHz calibration.
    const double expected = (0.3 * 0.3 + 0.4 * 0.4 + 0.5 * 0.5) * 100000.0 / 44100.0;

    for (double sr : kRates)
    {
        const auto r = renderVoice (p, sr, 69, expected * 2.0);
        const double seconds = r.activeUntil / sr;
        std::printf ("  sr=%7.0f  %.4f s\n", sr, seconds);
        checkClose (seconds, expected, 0.02, "envelope length at " + juce::String (sr, 0));
    }
}

static void testVibratoRateIsSampleRateIndependent()
{
    section ("vibrato rate is sample-rate independent");

    SfxrParams p = toneParams();
    p.base_freq    = 0.7f;
    p.env_sustain  = 0.7f;
    p.env_decay    = 0.05f;
    p.vib_strength = 0.5f;
    p.vib_speed    = 0.4f;

    // vib_phase advances by vib_speed^2 * 0.01 radians per output sample and the
    // original is calibrated at 44100 Hz, so the rate the user hears must be:
    const double expected = std::pow ((double) p.vib_speed, 2.0) * 0.01 * 44100.0
                          / (2.0 * juce::MathConstants<double>::pi);
    std::printf ("  expected %.2f Hz\n", expected);

    for (double sr : kRates)
    {
        const double hz = measureVibratoRate (renderVoice (p, sr, 69, 1.2));
        std::printf ("  sr=%7.0f  %7.2f Hz\n", sr, hz);
        checkClose (hz, expected, 0.05, "vibrato rate at " + juce::String (sr, 0));
    }
}

static void testFrequencySlideIsSampleRateIndependent()
{
    section ("frequency slide is sample-rate independent");

    SfxrParams p = toneParams();
    p.base_freq   = 0.6f;
    p.freq_ramp   = -0.15f;   // gentle downward slide
    p.env_sustain = 0.7f;
    p.env_decay   = 0.05f;

    // fperiod is multiplied by fslide once per output sample, so after t seconds
    // the pitch must have dropped by fslide^(44100 * t) at every sample rate.
    const double fslide = 1.0 - std::pow ((double) p.freq_ramp, 3.0) * 0.01;
    const double t0 = 0.02, t1 = 0.32, windowLen = 0.05;
    const double expectedRatio = 1.0 / std::pow (fslide, 44100.0 * (t1 - t0));
    std::printf ("  expected f(%.2fs)/f(%.2fs) = %.4f\n", t1, t0, expectedRatio);

    for (double sr : kRates)
    {
        const auto r = renderVoice (p, sr, 69, 0.5);
        const double early = measureFrequencyBetween (r, t0, t0 + windowLen);
        const double late  = measureFrequencyBetween (r, t1, t1 + windowLen);

        check (early > 10.0 && late > 10.0,
               "slide is measurable at " + juce::String (sr, 0));

        if (early > 10.0 && late > 10.0)
        {
            const double ratio = late / early;
            std::printf ("  sr=%7.0f  %8.2f -> %8.2f Hz  ratio=%.4f\n", sr, early, late, ratio);
            checkClose (ratio, expectedRatio, 0.05, "slide ratio at " + juce::String (sr, 0));
        }
    }
}

static void testDutySweepIsSampleRateIndependent()
{
    section ("duty sweep is sample-rate independent");

    SfxrParams p = toneParams();
    p.wave_type   = 0;      // square
    p.duty        = 0.0f;   // square_duty starts at its 0.5 maximum
    p.duty_ramp   = 1.0f;   // positive ramp sweeps square_duty downwards
    p.env_sustain = 0.7f;
    p.env_decay   = 0.05f;
    p.lpf_freq    = 1.0f;   // keep the filter out of the way

    // The mean is useless here because the signal path DC-blocks, but the
    // *fraction of time the wave spends positive* still tracks square_duty.
    // square_duty -= 0.00005 per output sample at 44.1 kHz.
    // square_duty keeps falling across the measurement window, so the value we
    // should see is the one at the window's midpoint.
    const double at = 0.1, windowLen = 0.02;
    const double expectedDuty = 0.5 - 0.00005 * 44100.0 * (at + windowLen * 0.5);
    std::printf ("  expected duty at %.3fs = %.4f\n", at + windowLen * 0.5, expectedDuty);

    auto positiveFraction = [] (const Render& r, double fromSeconds, double toSeconds)
    {
        const int from = juce::jlimit (0, (int) r.samples.size(), (int) (fromSeconds * r.sampleRate));
        const int to   = juce::jlimit (from, (int) r.samples.size(), (int) (toSeconds * r.sampleRate));

        int positive = 0, total = 0;
        for (int i = from; i < to; i++)
        {
            if (r.samples[(size_t) i] > 0.0f) positive++;
            total++;
        }
        return total > 0 ? (double) positive / total : 0.0;
    };

    for (double sr : kRates)
    {
        const auto r = renderVoice (p, sr, 69, 0.3);
        const double duty = positiveFraction (r, at, at + windowLen);
        std::printf ("  sr=%7.0f  duty=%.4f\n", sr, duty);
        checkClose (duty, expectedDuty, 0.05, "swept duty at " + juce::String (sr, 0));
    }

    // Sanity: the sweep must actually be moving.
    const auto r = renderVoice (p, 44100.0, 69, 0.3);
    check (positiveFraction (r, 0.005, 0.025) > positiveFraction (r, 0.15, 0.17),
           "duty sweep moves in the expected direction");
}

static void testNoNonFiniteOutput()
{
    section ("output is always finite and clamped");

    // Zero-length envelope stages used to divide by zero and emit NaN.
    struct Edge { const char* name; float attack, sustain, decay; };
    const Edge edges[] = {
        { "decay = 0",            0.0f, 0.3f, 0.0f },
        { "sustain = 0",          0.0f, 0.0f, 0.3f },
        { "attack = 0",           0.0f, 0.3f, 0.3f },
        { "all envelope stages 0", 0.0f, 0.0f, 0.0f },
    };

    for (const auto& e : edges)
    {
        SfxrParams p = toneParams();
        p.env_attack = e.attack; p.env_sustain = e.sustain; p.env_decay = e.decay;

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        {
            const auto r = renderVoice (p, sr, 69, 0.5);
            check (r.nonFinite == 0, juce::String (e.name) + " at " + juce::String (sr, 0)
                                     + " Hz produced " + juce::String (r.nonFinite) + " non-finite samples");
        }
    }

    // Same thing via the sustain-mode release path.
    {
        SfxrParams p = toneParams();
        p.env_decay = 0.0f;
        const auto r = renderVoice (p, 44100.0, 69, 0.5, /*oneShot*/ false, /*release at*/ 0.1);
        check (r.nonFinite == 0, "sustain-mode release with decay = 0 produced "
                                 + juce::String (r.nonFinite) + " non-finite samples");
    }

    // Broad sweep over the whole parameter space, including every preset
    // generator and the randomiser.
    {
        juce::Random rng (20240827);
        int nonFinite = 0, overs = 0, cases = 0;

        auto exercise = [&] (const SfxrParams& p, double sr, int note)
        {
            const auto r = renderVoice (p, sr, note, 0.35);
            nonFinite += r.nonFinite;
            if (r.peak > 1.0f) overs++;
            cases++;
        };

        for (int i = 0; i < (int) PresetCategory::Count; i++)
            for (int rep = 0; rep < 40; rep++)
            {
                SfxrParams p;
                generatePreset (p, (PresetCategory) i, rng);
                exercise (p, rep % 2 ? 48000.0 : 44100.0, rng.nextInt (128));
            }

        for (int rep = 0; rep < 300; rep++)
        {
            SfxrParams p;
            randomize (p, rng);
            for (int m = 0; m < 3; m++) mutate (p, rng);
            exercise (p, rep % 3 == 0 ? 96000.0 : (rep % 3 == 1 ? 48000.0 : 44100.0), rng.nextInt (128));
        }

        // The extremes of the MIDI range, explicitly: note 0 stretches every
        // period by 2^5.75 and note 127 compresses it into the period floor.
        for (int i = 0; i < (int) PresetCategory::Count; i++)
        {
            SfxrParams p;
            generatePreset (p, (PresetCategory) i, rng);
            for (int note : { 0, 1, 12, 24, 100, 120, 126, 127 })
                for (double sr : { 44100.0, 96000.0 })
                    exercise (p, sr, note);
        }

        std::printf ("  swept %d parameter sets\n", cases);
        check (nonFinite == 0, "parameter sweep produced " + juce::String (nonFinite)
                               + " non-finite samples");
        check (overs == 0, juce::String (overs) + " parameter sets exceeded the 0 dBFS clamp");
    }
}

static void testOutputLevelMatchesOriginal()
{
    section ("output level matches the original WAV export");

    SfxrParams p = toneParams();
    p.wave_type = 0;          // square, so the peak is predictable
    p.sound_vol = 0.5f;
    p.env_attack = 0.0f; p.env_sustain = 0.5f; p.env_decay = 0.1f;

    const auto r = renderVoice (p, 44100.0, 69, 0.5);

    // ssample = raw * 2 * sound_vol * 0.2, and a unity square peaks at 1.0 after
    // the phaser sums two copies of a +/-0.5 wave.
    check (r.peak > 0.15f && r.peak < 0.25f,
           "square peak near 0.2 (got " + juce::String (r.peak, 4) + ")");

    p.sound_vol = 1.0f;
    const auto loud = renderVoice (p, 44100.0, 69, 0.5);
    checkClose (loud.peak, r.peak * 2.0, 0.05, "output level scales linearly");
}

static void testSustainModeHolds()
{
    section ("sustain mode holds until note-off");

    SfxrParams p = toneParams();
    p.env_attack = 0.0f; p.env_sustain = 0.1f; p.env_decay = 0.1f;

    // One-shot: stops on its own well before 2 s.
    const auto shot = renderVoice (p, 44100.0, 69, 2.0, true);
    check (shot.activeUntil / 44100.0 < 0.2, "one-shot note ends by itself");

    // Sustain: still sounding at 1 s, then stops shortly after the release.
    const auto held = renderVoice (p, 44100.0, 69, 2.0, false, 1.0);
    check (held.activeUntil / 44100.0 > 0.9, "sustained note is still sounding at 1 s");
    check (held.activeUntil / 44100.0 < 1.3, "sustained note ends after release");
}

static void testAudioExport()
{
    section ("offline audio export");

    SfxrParams p = toneParams();
    p.env_attack = 0.0f;
    p.env_sustain = 0.1f;
    p.env_decay = 0.1f;

    for (const int sampleRate : { 44100, 48000, 88200, 96000, 192000 })
    {
        const auto audio = SfxrAudioExporter::renderPreview (p, false, true, sampleRate);
        check (audio.getNumChannels() == 1, "offline export is mono");
        check (audio.getNumSamples() > 0 && audio.getNumSamples() < sampleRate * 2,
               "one-shot export ends naturally at " + juce::String (sampleRate) + " Hz");

        for (int i = 0; i < audio.getNumSamples(); ++i)
            check (std::isfinite (audio.getSample (0, i)), "offline export sample is finite");
    }

    const auto sustained = SfxrAudioExporter::renderPreview (p, false, false, 44100);
    check (sustained.getNumSamples() == 441000, "sustained export is bounded to 10 seconds");

    const auto audio = SfxrAudioExporter::renderPreview (p, false, true, 48000);
    const auto directory = juce::File::getSpecialLocation (juce::File::tempDirectory);
    const auto baseName = "SfxrVstiExportTest-" + juce::String (juce::Random::getSystemRandom().nextInt());

    for (const int bitDepth : { 16, 24, 32 })
    {
        const auto file = directory.getChildFile (baseName + "-" + juce::String (bitDepth) + ".wav");
        SfxrAudioExporter::Options options;
        options.sampleRate = 48000;
        options.wavBitDepth = bitDepth;
        juce::String error;
        check (SfxrAudioExporter::writeFile (file, audio, options, error), "WAV export succeeds");

        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        check (reader != nullptr, "WAV export can be read");
        if (reader != nullptr)
        {
            check (reader->sampleRate == 48000.0, "WAV sample rate is retained");
            check (reader->numChannels == 1, "WAV remains mono");
            check ((int) reader->bitsPerSample == bitDepth, "WAV bit depth is retained");
        }
        file.deleteFile();
    }

    const auto ogg = directory.getChildFile (baseName + ".ogg");
    SfxrAudioExporter::Options options;
    options.format = SfxrAudioExporter::Format::ogg;
    options.sampleRate = 48000;
    options.oggQualityIndex = 4;
    juce::String error;
    check (SfxrAudioExporter::writeFile (ogg, audio, options, error), "OGG export succeeds");

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (ogg));
    check (reader != nullptr, "OGG export can be read");
    if (reader != nullptr)
    {
        check (reader->sampleRate == 48000.0, "OGG sample rate is retained");
        check (reader->numChannels == 1, "OGG remains mono");
    }
    ogg.deleteFile();
}

static void testOriginalPresetSemantics()
{
    section ("preset operations match original semantics");

    juce::Random rng (1234);
    for (int i = 0; i < (int) PresetCategory::Count; i++)
    {
        SfxrParams p;
        p.sound_vol = 0.23f;
        generatePreset (p, (PresetCategory) i, rng);
        check (p.sound_vol == 0.23f,
               "generator " + juce::String (i) + " preserves Output Level");
    }

    SfxrParams immediate = toneParams();
    immediate.vib_strength = 0.7f;
    immediate.vib_speed = 0.6f;
    immediate.vib_delay = 0.0f;

    SfxrParams delayed = immediate;
    delayed.vib_delay = 1.0f;

    const auto a = renderVoice (immediate, 44100.0, 69, 0.5);
    const auto b = renderVoice (delayed, 44100.0, 69, 0.5);
    check (a.samples != b.samples, "vib_delay extension changes the vibrato fade-in");
}

static void testPresetFileRoundTrip()
{
    section (".sfs round-trip");

    auto file = juce::File::createTempFile (".sfs");
    juce::Random rng (99);

    bool allMatch = true;
    for (int i = 0; i < (int) PresetCategory::Count; i++)
    {
        SfxrParams written;
        generatePreset (written, (PresetCategory) i, rng);

        if (! SfxrPresetFile::save (file, written))
        {
            check (false, "save failed for category " + juce::String (i));
            continue;
        }

        SfxrParams read;
        if (! SfxrPresetFile::load (file, read))
        {
            check (false, "load failed for category " + juce::String (i));
            continue;
        }

        // Compare every serialized parameter, not just a representative subset.
        auto eq = [] (float a, float b) { return std::abs (a - b) < 1.0e-6f; };
        const bool same = read.wave_type == written.wave_type
                       && eq (read.sound_vol, written.sound_vol)
                       && eq (read.base_freq, written.base_freq)
                       && eq (read.freq_limit, written.freq_limit)
                       && eq (read.freq_ramp, written.freq_ramp)
                       && eq (read.freq_dramp, written.freq_dramp)
                       && eq (read.duty, written.duty)
                       && eq (read.duty_ramp, written.duty_ramp)
                       && eq (read.vib_strength, written.vib_strength)
                       && eq (read.vib_speed, written.vib_speed)
                       && eq (read.vib_delay, written.vib_delay)
                       && eq (read.env_attack, written.env_attack)
                       && eq (read.env_sustain, written.env_sustain)
                       && eq (read.env_decay, written.env_decay)
                       && eq (read.env_punch, written.env_punch)
                       && eq (read.lpf_resonance, written.lpf_resonance)
                       && eq (read.lpf_freq, written.lpf_freq)
                       && eq (read.lpf_ramp, written.lpf_ramp)
                       && eq (read.hpf_freq, written.hpf_freq)
                       && eq (read.hpf_ramp, written.hpf_ramp)
                       && eq (read.pha_offset, written.pha_offset)
                       && eq (read.pha_ramp, written.pha_ramp)
                       && eq (read.repeat_speed, written.repeat_speed)
                       && eq (read.arp_speed, written.arp_speed)
                       && eq (read.arp_mod, written.arp_mod);
        if (! same)
            allMatch = false;
    }

    check (allMatch, "all preset categories survive a save/load cycle");

    // Non-finite values are not meaningful parameters and can otherwise reach
    // float-to-int conversions in the renderer.
    SfxrParams nonFinite;
    nonFinite.base_freq = std::numeric_limits<float>::quiet_NaN();
    check (SfxrPresetFile::save (file, nonFinite), "NaN fixture can be written");
    SfxrParams rejected;
    check (! SfxrPresetFile::load (file, rejected), ".sfs containing NaN is rejected");

    // A truncated file must be rejected rather than silently half-loaded.
    file.replaceWithData ("\x66\x00\x00\x00\x01", 5);
    SfxrParams dummy;
    dummy.base_freq = 0.77f;
    check (! SfxrPresetFile::load (file, dummy), "truncated .sfs file is rejected");
    check (dummy.base_freq == 0.77f, "failed load leaves the destination unchanged");

    // So must a bogus version.
    file.replaceWithData ("\xff\xff\x00\x00", 4);
    check (! SfxrPresetFile::load (file, dummy), "unknown .sfs version is rejected");

    file.deleteFile();
}


//==============================================================================
static void testSampleAccurateNoteOnset()
{
    section ("note onset is sample-accurate within a block");

    const double sr = 44100.0;
    const int    blockSize = 512;

    // Mirrors what processBlock does: render up to the event, apply it, then
    // render the rest of the block.
    auto onsetIndexForEventAt = [&] (int eventPos)
    {
        SfxrEngine engine;
        engine.prepare (sr, blockSize);

        SfxrParams p = toneParams();
        p.wave_type = 0;         // square: non-zero immediately
        p.base_freq = 0.5f;
        engine.setParams (p);
        engine.setOneShot (true);

        juce::AudioBuffer<float> buffer (2, blockSize);
        buffer.clear();

        engine.render (buffer, 0, eventPos);
        engine.noteOn (69, 1.0f);
        engine.render (buffer, eventPos, blockSize - eventPos);

        const float* out = buffer.getReadPointer (0);
        for (int i = 0; i < blockSize; i++)
            if (std::abs (out[i]) > 1.0e-6f)
                return i;

        return -1;
    };

    for (int eventPos : { 0, 1, 37, 200, 511 })
    {
        const int onset = onsetIndexForEventAt (eventPos);
        std::printf ("  event at %3d -> first non-zero sample %3d\n", eventPos, onset);
        check (onset == eventPos, "onset for an event at sample " + juce::String (eventPos)
                                  + " (got " + juce::String (onset) + ")");
    }

    // render() must be additive and must not touch anything outside its range.
    {
        SfxrEngine engine;
        engine.prepare (sr, blockSize);
        engine.setParams (toneParams());
        engine.setOneShot (true);

        juce::AudioBuffer<float> buffer (2, blockSize);
        buffer.clear();
        for (int i = 0; i < blockSize; i++)
            buffer.setSample (0, i, 1.0f);

        engine.noteOn (69, 1.0f);
        engine.render (buffer, 100, 200);

        bool outsideUntouched = true;
        for (int i = 0; i < 100; i++)
            if (buffer.getSample (0, i) != 1.0f) outsideUntouched = false;
        for (int i = 300; i < blockSize; i++)
            if (buffer.getSample (0, i) != 1.0f) outsideUntouched = false;

        check (outsideUntouched, "render() leaves samples outside [start, start+n) alone");
    }
}

static void testStolenVoiceDoesNotReleaseTheWrongNote()
{
    section ("a stolen voice does not get released by the old note's note-off");

    const double sr = 44100.0;
    const int    blockSize = 512;

    SfxrEngine engine;
    engine.prepare (sr, blockSize);

    SfxrParams p = toneParams();
    p.wave_type   = 0;
    p.env_attack  = 0.0f;
    p.env_sustain = 0.3f;
    p.env_decay   = 0.3f;   // 0.09 * 100000 / 44100 = 204 ms of decay
    engine.setParams (p);
    engine.setOneShot (false);      // sustain: a held note rings until note-off

    juce::AudioBuffer<float> buffer (2, blockSize);

    // Renders and discards, to let releasing voices finish decaying.
    auto settle = [&] (int blocks)
    {
        for (int b = 0; b < blocks; b++)
        {
            buffer.clear();
            engine.render (buffer, 0, blockSize);
        }
    };

    // Peak over the blocks *after* settling, i.e. what is still being held.
    auto sustainedPeak = [&] (int blocks)
    {
        float peak = 0.0f;
        for (int b = 0; b < blocks; b++)
        {
            buffer.clear();
            engine.render (buffer, 0, blockSize);
            peak = juce::jmax (peak, buffer.getMagnitude (0, 0, blockSize));
        }
        return peak;
    };

    // Fill the pool, then steal with a ninth note.
    for (int n = 0; n < SfxrEngine::kNumVoices; n++)
    {
        engine.noteOn (60 + n, 1.0f);
        settle (1);
    }
    const int stealingNote = 60 + SfxrEngine::kNumVoices;
    engine.noteOn (stealingNote, 1.0f);   // steals the oldest voice
    settle (1);

    // Release every note except the one that did the stealing. If the stolen
    // voice were still mapped to note 60, releasing 60 would kill it too.
    for (int n = 0; n < SfxrEngine::kNumVoices; n++)
        engine.noteOff (60 + n);

    settle (40);                                   // ~0.46 s: decays are over
    const float held = sustainedPeak (20);
    std::printf ("  still sounding after the others decayed: %.4f\n", held);
    check (held > 0.01f, "the held note survives its predecessor's note-off");

    // And once it is released too, everything really does stop.
    engine.noteOff (stealingNote);
    settle (40);                                   // let its own decay finish
    const float after = sustainedPeak (20);
    std::printf ("  after releasing it as well:              %.4f\n", after);
    check (after < 1.0e-5f, "releasing the last note silences the engine");
}

//==============================================================================
static void testDomainClampingMatchesOriginalUi()
{
    section ("domain clamping matches the original UI");

    // Randomize and Mutate can exceed their sliders' ranges. In the original,
    // visible Slider() controls clamp later in the same frame before PlaySample;
    // vib_delay uses the plugin extension's unipolar range.
    struct Field { const char* name; float SfxrParams::* member; };
    const Field unipolar[] = {
        { "base_freq",     &SfxrParams::base_freq },
        { "freq_limit",    &SfxrParams::freq_limit },
        { "duty",          &SfxrParams::duty },
        { "vib_strength",  &SfxrParams::vib_strength },
        { "vib_speed",     &SfxrParams::vib_speed },
        { "vib_delay",     &SfxrParams::vib_delay },
        { "env_attack",    &SfxrParams::env_attack },
        { "env_sustain",   &SfxrParams::env_sustain },
        { "env_decay",     &SfxrParams::env_decay },
        { "env_punch",     &SfxrParams::env_punch },
        { "lpf_freq",      &SfxrParams::lpf_freq },
        { "lpf_resonance", &SfxrParams::lpf_resonance },
        { "hpf_freq",      &SfxrParams::hpf_freq },
        { "repeat_speed",  &SfxrParams::repeat_speed },
        { "arp_speed",     &SfxrParams::arp_speed },
        { "sound_vol",     &SfxrParams::sound_vol },
    };

    for (const auto& f : unipolar)
    {
        SfxrParams low;
        low.*(f.member) = -0.4f;
        low.clampToDomain();
        check (low.*(f.member) == 0.0f, juce::String (f.name) + " clamps negative values to zero");

        SfxrParams high;
        high.*(f.member) = 1.4f;
        high.clampToDomain();
        check (high.*(f.member) == 1.0f, juce::String (f.name) + " clamps values above one");
    }

    const Field bipolar[] = {
        { "freq_ramp",  &SfxrParams::freq_ramp },
        { "freq_dramp", &SfxrParams::freq_dramp },
        { "duty_ramp",  &SfxrParams::duty_ramp },
        { "lpf_ramp",   &SfxrParams::lpf_ramp },
        { "hpf_ramp",   &SfxrParams::hpf_ramp },
        { "pha_offset", &SfxrParams::pha_offset },
        { "pha_ramp",   &SfxrParams::pha_ramp },
        { "arp_mod",    &SfxrParams::arp_mod },
    };

    for (const auto& f : bipolar)
    {
        SfxrParams low;
        low.*(f.member) = -1.4f;
        low.clampToDomain();
        check (low.*(f.member) == -1.0f, juce::String (f.name) + " clamps below minus one");

        SfxrParams high;
        high.*(f.member) = 1.4f;
        high.clampToDomain();
        check (high.*(f.member) == 1.0f, juce::String (f.name) + " clamps above one");
    }

    SfxrParams lowWave;
    lowWave.wave_type = -1;
    lowWave.clampToDomain();
    check (lowWave.wave_type == 0, "wave_type clamps below zero");

    SfxrParams highWave;
    highWave.wave_type = 4;
    highWave.clampToDomain();
    check (highWave.wave_type == 3, "wave_type clamps above three");

    // Whatever the generators produce must now be inside the documented domain,
    // so the parameter tree has nothing left to clamp.
    {
        juce::Random rng (777);
        int violations = 0, sets = 0;

        auto verify = [&] (const SfxrParams& p)
        {
            auto uni = [&] (float v) { if (v < 0.0f || v > 1.0f) violations++; };
            auto bi  = [&] (float v) { if (v < -1.0f || v > 1.0f) violations++; };
            uni (p.base_freq);  uni (p.freq_limit);   uni (p.duty);
            uni (p.vib_strength); uni (p.vib_speed);  uni (p.vib_delay);
            uni (p.env_attack); uni (p.env_sustain);  uni (p.env_decay);
            uni (p.env_punch);  uni (p.lpf_freq);     uni (p.lpf_resonance);
            uni (p.hpf_freq);   uni (p.repeat_speed); uni (p.arp_speed);
            uni (p.sound_vol);
            bi (p.freq_ramp);   bi (p.freq_dramp);    bi (p.duty_ramp);
            bi (p.lpf_ramp);    bi (p.hpf_ramp);      bi (p.pha_offset);
            bi (p.pha_ramp);    bi (p.arp_mod);
            if (p.wave_type < 0 || p.wave_type > 3) violations++;
            sets++;
        };

        for (int i = 0; i < 500; i++)
        {
            SfxrParams p;
            randomize (p, rng);
            verify (p);
            for (int m = 0; m < 20; m++) { mutate (p, rng); verify (p); }
        }
        for (int i = 0; i < (int) PresetCategory::Count; i++)
            for (int rep = 0; rep < 50; rep++)
            {
                SfxrParams p;
                generatePreset (p, (PresetCategory) i, rng);
                verify (p);
            }

        std::printf ("  checked %d generated parameter sets\n", sets);
        check (violations == 0, juce::String (violations) + " generated values fell outside the domain");
    }
}

//==============================================================================
int main()
{
    std::printf ("SfxrVsti engine tests\n");

    testPitchIsSampleRateIndependent();
    testMidiTransposition();
    testEnvelopeTimingIsSampleRateIndependent();
    testVibratoRateIsSampleRateIndependent();
    testFrequencySlideIsSampleRateIndependent();
    testDutySweepIsSampleRateIndependent();
    testNoNonFiniteOutput();
    testOutputLevelMatchesOriginal();
    testSustainModeHolds();
    testAudioExport();
    testOriginalPresetSemantics();
    testPresetFileRoundTrip();
    testSampleAccurateNoteOnset();
    testStolenVoiceDoesNotReleaseTheWrongNote();
    testDomainClampingMatchesOriginalUi();

    std::printf ("\n%d checks, %d failure(s)\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
