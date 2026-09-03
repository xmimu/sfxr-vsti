#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

#include <cstdio>

int main()
{
    // Needed to construct GUI components (fonts / look and feel). We never show
    // a window: creating and destroying the editor repeatedly is enough to leak
    // the widgets if buildInterface() did not take ownership of them.
    juce::ScopedJuceInitialiser_GUI guiInitialiser;

    std::printf ("SfxrVsti editor open/close leak test\n");

    // Warm up singletons (fonts etc.) so the steady-state count is meaningful.
    {
        SfxrVstiAudioProcessor p;
        std::unique_ptr<juce::AudioProcessorEditor> warm (p.createEditor());
    }

    // The historical bug: 77 components per editor leaked, visible only as
    // "*** Leaked" messages from JUCE's LeakedObjectDetector at exit. This
    // target is built with JUCE_ENABLE_LEAK_DETECTOR so any such leak prints
    // and the wrapper test greps for it.
    for (int round = 0; round < 15; ++round)
    {
        SfxrVstiAudioProcessor p;
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
    }

    std::printf ("15 open/close cycles done, no leaks expected at exit\n");
    return 0;
}
