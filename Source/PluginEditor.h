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
    void parentHierarchyChanged() override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void buildInterface();
    void addSectionHeader (const juce::String& text, int x, int y);
    juce::Slider* addSlider (const juce::String& paramID, const juce::String& label,
                             int x, int y, int barWidth);
    void addWaveButton (const juce::String& text, int value, int x, int y);
    void updateWaveButtons();

    // Takes ownership of a freshly created widget and shows it.
    template <typename ComponentType>
    ComponentType* own (ComponentType* c)
    {
        widgets.add (c);
        addAndMakeVisible (c);
        return c;
    }

    void randomize();
    void mutate();
    void generateCategory (PresetCategory c);
    void loadSfs();
    void saveSfs();
    void exportAudio();
    void exportAudio (int formatId, int sampleRateId, int encodingId, const juce::File& target);

    SfxrVstiAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Random rng;

    std::unique_ptr<juce::LookAndFeel> lookAndFeel;

    // Owns every widget created by the addXxx() helpers below. Declared before
    // the attachments so that it is destroyed *after* them -- an attachment
    // dereferences its Slider/Button in its destructor.
    juce::OwnedArray<juce::Component> widgets;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    std::unique_ptr<juce::Component> waveformScope;
    std::unique_ptr<juce::MidiKeyboardComponent> midiKeyboard;

    juce::ToggleButton* waveButtons[4] = { nullptr, nullptr, nullptr, nullptr };
    bool configuredStandaloneTitleBar = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVstiAudioProcessorEditor)
};
