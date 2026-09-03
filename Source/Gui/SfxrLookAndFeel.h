#pragma once

#include <JuceHeader.h>

// sfxr-styled LookAndFeel. The editor creates one instance and shares it with
// the styled alerts and the export dialog so every window matches.
class SfxrLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SfxrLookAndFeel();

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int, int, int buttonW, int, juce::ComboBox&) override;
};
