#include "SfxrDialogs.h"

namespace SfxrDialogs
{
    void showAlert (juce::LookAndFeel& lf, juce::MessageBoxIconType icon,
                    const juce::String& title, const juce::String& message,
                    juce::Component* associatedComponent)
    {
        auto* alert = new juce::AlertWindow (title, message, icon, associatedComponent);
        alert->setLookAndFeel (&lf);
        alert->addButton ("OK", 0, juce::KeyPress (juce::KeyPress::returnKey));
        alert->enterModalState (true, juce::ModalCallbackFunction::create ([alert] (int)
        {
            std::unique_ptr<juce::AlertWindow> deleter (alert);
        }), false);
    }

    void showConfirm (juce::LookAndFeel& lf, juce::MessageBoxIconType icon,
                      const juce::String& title, const juce::String& message,
                      juce::Component* associatedComponent, std::function<void()> onConfirm)
    {
        auto* alert = new juce::AlertWindow (title, message, icon, associatedComponent);
        alert->setLookAndFeel (&lf);
        alert->addButton ("Replace", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        alert->enterModalState (true, juce::ModalCallbackFunction::create ([alert, onConfirm] (int result)
        {
            std::unique_ptr<juce::AlertWindow> deleter (alert);
            if (result == 1)
                onConfirm();
        }), false);
    }
}
