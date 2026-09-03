#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SfxrLookAndFeel;
class SfxrEditorActions;
class WaveformScope;
class SfxrMidiKeyboardComponent;

// Thin view layer: builds the fixed pixel layout, binds widgets to parameters
// and forwards button presses to SfxrEditorActions. All async dialog flow is
// anchored here because this component is the natural lifetime anchor for the
// file choosers and export window it launches.
class SfxrVstiAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::AudioProcessorValueTreeState::Listener,
                                     private juce::AsyncUpdater
{
public:
    explicit SfxrVstiAudioProcessorEditor (SfxrVstiAudioProcessor&);
    ~SfxrVstiAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override {}
    void parentHierarchyChanged() override;

private:
    void handleAsyncUpdate() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // ---- view construction ----
    void buildInterface();
    void buildTopBar();
    void buildGeneratorColumn();
    void buildSettingsColumns();
    void buildWaveformArea();
    void buildKeyboardArea();

    void addSectionHeader (const juce::String& text, int x, int y);
    juce::Slider* addSlider (const juce::String& paramID, const juce::String& label,
                             int x, int y, int barWidth);
    void addWaveButton (const juce::String& text, int value, int x, int y);
    void updateWaveButtons();

    // ---- user actions (file choosers / export dialog; execution is delegated
    //      to SfxrEditorActions) ----
    void loadSfs();
    void saveSfs();
    void exportAudio();
    void performExport (int formatId, int sampleRateId, int encodingId, const juce::File& target);

    // Takes ownership of a freshly created widget and shows it.
    template <typename ComponentType>
    ComponentType* own (ComponentType* c)
    {
        widgets.add (c);
        addAndMakeVisible (c);
        return c;
    }

    SfxrVstiAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<SfxrEditorActions> actions;

    std::unique_ptr<SfxrLookAndFeel> lookAndFeel;

    // Owns every widget created by the addXxx() helpers below. Declared before
    // the attachments so that it is destroyed *after* them -- an attachment
    // dereferences its Slider/Button in its destructor.
    juce::OwnedArray<juce::Component> widgets;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    std::unique_ptr<WaveformScope> waveformScope;
    std::unique_ptr<SfxrMidiKeyboardComponent> midiKeyboard;

    juce::ToggleButton* waveButtons[4] = { nullptr, nullptr, nullptr, nullptr };
    bool configuredStandaloneTitleBar = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrVstiAudioProcessorEditor)
};
