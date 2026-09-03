#pragma once

#include <JuceHeader.h>

// A MidiKeyboardComponent that shows the full 88-key range, highlights the
// root note and greys out + disables every key outside the valid range.
class SfxrMidiKeyboardComponent : public juce::MidiKeyboardComponent
{
public:
    // Note 69 plays the Start Frequency parameter unmodified, so it is the
    // root of the transposition -- not a 440 Hz concert A. Labelling it "A4"
    // would imply a pitch the synth does not actually produce.
    static constexpr int rootNote = 69;

    using juce::MidiKeyboardComponent::MidiKeyboardComponent;

    bool isNoteEnabled (int midiNoteNumber) const noexcept;

    void drawWhiteNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour) override;

    void drawBlackNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour noteFillColour) override;

    bool mouseDownOnKey (int midiNoteNumber, const juce::MouseEvent&) override;
    bool mouseDraggedToKey (int midiNoteNumber, const juce::MouseEvent&) override;
    void mouseUpOnKey (int midiNoteNumber, const juce::MouseEvent&) override;

    juce::String getWhiteNoteText (int midiNoteNumber) override;

private:
    const juce::Colour rootNoteColour      { 0xFFE8A030 };
    const juce::Colour disabledWhiteColour { 0xFFC8C0B8 };
    const juce::Colour disabledBlackColour { 0xFF908880 };
};
