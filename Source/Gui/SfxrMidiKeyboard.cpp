#include "SfxrMidiKeyboard.h"
#include "../PluginProcessor.h"

bool SfxrMidiKeyboardComponent::isNoteEnabled (int midiNoteNumber) const noexcept
{
    return SfxrNoteRange::contains (midiNoteNumber);
}

void SfxrMidiKeyboardComponent::drawWhiteNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                                               bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour)
{
    if (! isNoteEnabled (midiNoteNumber))
    {
        g.setColour (disabledWhiteColour);
        g.fillRect (area);
        juce::MidiKeyboardComponent::drawWhiteNote (midiNoteNumber, g, area, false, false, lineColour, textColour);
        return;
    }

    if (midiNoteNumber == rootNote)
    {
        auto c = rootNoteColour;
        if (isDown) c = c.overlaidWith (findColour (keyDownOverlayColourId));
        if (isOver) c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));

        g.setColour (c);
        g.fillRect (area);
    }

    juce::MidiKeyboardComponent::drawWhiteNote (midiNoteNumber, g, area, isDown, isOver, lineColour, textColour);
}

void SfxrMidiKeyboardComponent::drawBlackNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                                               bool isDown, bool isOver, juce::Colour noteFillColour)
{
    if (! isNoteEnabled (midiNoteNumber))
    {
        g.setColour (disabledBlackColour);
        g.fillRect (area);

        const auto sideIndent = 1.0f / 8.0f;
        const auto topIndent  = 7.0f / 8.0f;
        g.setColour (disabledBlackColour.brighter (0.15f));
        g.fillRect (area.reduced (area.getWidth() * sideIndent, 0.0f)
                         .removeFromTop (area.getHeight() * topIndent));
        return;
    }

    juce::MidiKeyboardComponent::drawBlackNote (midiNoteNumber, g, area, isDown, isOver, noteFillColour);
}

bool SfxrMidiKeyboardComponent::mouseDownOnKey (int midiNoteNumber, const juce::MouseEvent& e)
{
    return isNoteEnabled (midiNoteNumber)
        && juce::MidiKeyboardComponent::mouseDownOnKey (midiNoteNumber, e);
}

bool SfxrMidiKeyboardComponent::mouseDraggedToKey (int midiNoteNumber, const juce::MouseEvent& e)
{
    return isNoteEnabled (midiNoteNumber)
        && juce::MidiKeyboardComponent::mouseDraggedToKey (midiNoteNumber, e);
}

void SfxrMidiKeyboardComponent::mouseUpOnKey (int midiNoteNumber, const juce::MouseEvent& e)
{
    if (isNoteEnabled (midiNoteNumber))
        juce::MidiKeyboardComponent::mouseUpOnKey (midiNoteNumber, e);
}

juce::String SfxrMidiKeyboardComponent::getWhiteNoteText (int midiNoteNumber)
{
    if (midiNoteNumber == rootNote)
        return "ROOT";

    return juce::MidiKeyboardComponent::getWhiteNoteText (midiNoteNumber);
}
