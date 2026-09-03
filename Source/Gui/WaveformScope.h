#pragma once

#include <JuceHeader.h>

class SfxrVstiAudioProcessor;

// Live oscilloscope fed from the audio thread through the processor's lock-free
// FIFO (readScope). Polls at 30 Hz and paints a scrolling trace.
class WaveformScope : public juce::Component, private juce::Timer
{
public:
    explicit WaveformScope (SfxrVstiAudioProcessor&);
    ~WaveformScope() override;

    void timerCallback() override;
    void paint (juce::Graphics&) override;

private:
    static constexpr int displaySize = 8192;
    SfxrVstiAudioProcessor& processor;
    std::vector<float> displayBuffer;
    std::vector<float> tempBuffer;
};
