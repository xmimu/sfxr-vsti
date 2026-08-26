#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SfxrVstiAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit SfxrVstiAudioProcessorEditor (SfxrVstiAudioProcessor&);
    ~SfxrVstiAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void buildInterface();
    void addSectionHeader (const juce::String& text, int x, int y);
    juce::Slider* addSlider (const juce::String& paramID, const juce::String& label,
                             int x, int y, int barWidth, bool bipolar);
    void addWaveButton (const juce::String& text, int value, int x, int y);
    void updateWaveButtons();

    void randomize();
    void mutate();
    void generateCategory (PresetCategory c);
    void loadSfs();
    void saveSfs();

    SfxrVstiAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Random rng;

    std::unique_ptr<juce::LookAndFeel> lookAndFeel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    std::unique_ptr<juce::Component> waveformScope;
    std::unique_ptr<juce::MidiKeyboardComponent> midiKeyboard;

    juce::ToggleButton* waveButtons[4] = { nullptr, nullptr, nullptr, nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVstiAudioProcessorEditor)
};
