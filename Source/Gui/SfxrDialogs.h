#pragma once

#include <JuceHeader.h>

// Modal windows styled with a given LookAndFeel. AlertWindow's static show*()
// helpers always use the global default LookAndFeel, which would look out of
// place next to the rest of this sfxr-styled UI, so the windows are built here
// so callers can apply the same LookAndFeel as everything else.
namespace SfxrDialogs
{
    void showAlert (juce::LookAndFeel&, juce::MessageBoxIconType icon,
                    const juce::String& title, const juce::String& message,
                    juce::Component* associatedComponent);

    void showConfirm (juce::LookAndFeel&, juce::MessageBoxIconType icon,
                      const juce::String& title, const juce::String& message,
                      juce::Component* associatedComponent, std::function<void()> onConfirm);
}
