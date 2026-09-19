#include <JuceHeader.h>
#include "Gui/SfxrLookAndFeel.h"
#include "Gui/SfxrLocalization.h"

// This file is compiled for every plug-in format, but a JUCEApplication exists
// only in the standalone wrapper. Keeping the application code in this guard
// ensures the VST3 and AU targets continue to use their normal entry points.
#if JucePlugin_Build_Standalone

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
    constexpr int kAudioMidiSettingsCommand = 1;
    constexpr int kLanguageAutoCommand = 2;
    constexpr int kLanguageChineseCommand = 3;
    constexpr int kLanguageEnglishCommand = 4;

    class SfxrStandaloneWindow final : public juce::StandaloneFilterWindow,
                                       private juce::MenuBarModel
    {
    public:
        SfxrStandaloneWindow (const juce::String& title,
                              juce::Colour backgroundColour,
                              std::unique_ptr<juce::StandalonePluginHolder> pluginHolderIn)
            : juce::StandaloneFilterWindow (title, backgroundColour, std::move (pluginHolderIn))
        {
           #if JUCE_MAC
            // macOS applications use the system menu bar rather than an
            // in-window bar.
            juce::MenuBarModel::setMacMainMenu (this);
           #else
            setMenuBar (this);
           #endif
        }

        ~SfxrStandaloneWindow() override
        {
           #if JUCE_MAC
            if (juce::MenuBarModel::getMacMainMenu() == this)
                juce::MenuBarModel::setMacMainMenu (nullptr);
           #endif
        }

    private:
        juce::StringArray getMenuBarNames() override
        {
            return { "Settings" };
        }

        juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String&) override
        {
            juce::PopupMenu menu;

            if (menuIndex == 0)
            {
                menu.addItem (kAudioMidiSettingsCommand,
                              SfxrLocalization::text ("Audio & MIDI Devices...", "音频与 MIDI 设备..."));
                juce::PopupMenu languageMenu;
                const auto selected = SfxrLocalization::selectedLanguage();
                languageMenu.addItem (kLanguageAutoCommand,
                                      SfxrLocalization::text ("Automatic", "自动"), true,
                                      selected == SfxrLocalization::Language::automatic);
                languageMenu.addItem (kLanguageChineseCommand,
                                      SfxrLocalization::text ("Chinese", "中文"), true,
                                      selected == SfxrLocalization::Language::chinese);
                languageMenu.addItem (kLanguageEnglishCommand, "English", true,
                                      selected == SfxrLocalization::Language::english);
                menu.addSubMenu (SfxrLocalization::text ("Language", "语言"), languageMenu);
            }

            return menu;
        }

        void menuItemSelected (int menuItemID, int) override
        {
            if (menuItemID == kAudioMidiSettingsCommand)
            {
                // JUCE's standalone settings panel is bound to this window's
                // AudioDeviceManager. It offers explicit audio input/output
                // device choices and selectable MIDI input devices, and saves
                // the chosen setup in the application's settings file.
                handleMenuResult (1);
            }
            else if (menuItemID >= kLanguageAutoCommand && menuItemID <= kLanguageEnglishCommand)
            {
                SfxrLocalization::setLanguage (
                    static_cast<SfxrLocalization::Language> (menuItemID - kLanguageAutoCommand));
                SfxrLocalization::applyStandaloneJuceTranslations();
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrStandaloneWindow)
    };

    class SfxrStandaloneApplication final : public juce::JUCEApplication
    {
    public:
        SfxrStandaloneApplication()
        {
            // The editor owns its own instance, while JUCE creates the device
            // settings dialog internally. Make this matching look-and-feel the
            // standalone default so that dialog and all of its child controls
            // use the same palette as the editor.
            juce::LookAndFeel::setDefaultLookAndFeel (&standaloneLookAndFeel);
            SfxrLocalization::applyStandaloneJuceTranslations();

            juce::PropertiesFile::Options options;
            options.applicationName = JucePlugin_Name;
            options.filenameSuffix = ".settings";
            options.osxLibrarySubFolder = "Application Support";

           #if JUCE_LINUX || JUCE_BSD
            options.folderName = "~/.config";
           #endif

            appProperties.setStorageParameters (options);
        }

        const juce::String getApplicationName() override { return JucePlugin_Name; }
        const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
        bool moreThanOneInstanceAllowed() override { return true; }
        void anotherInstanceStarted (const juce::String&) override {}

        void initialise (const juce::String&) override
        {
            if (juce::Desktop::getInstance().getDisplays().displays.isEmpty())
            {
                // Keep audio working in headless use, matching JUCE's default
                // standalone application behaviour.
                headlessPluginHolder = createPluginHolder();
                return;
            }

            mainWindow = std::make_unique<SfxrStandaloneWindow> (
                getApplicationName(),
                juce::LookAndFeel::getDefaultLookAndFeel().findColour (
                    juce::ResizableWindow::backgroundColourId),
                createPluginHolder());
            mainWindow->setVisible (true);
        }

        void shutdown() override
        {
            headlessPluginHolder = nullptr;
            mainWindow = nullptr;
            appProperties.saveIfNeeded();

            juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        }

        void systemRequestedQuit() override
        {
            if (headlessPluginHolder != nullptr)
                headlessPluginHolder->savePluginState();

            if (mainWindow != nullptr)
                mainWindow->getPluginHolder()->savePluginState();

            if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
            {
                juce::Timer::callAfterDelay (100, []
                {
                    if (auto* app = juce::JUCEApplicationBase::getInstance())
                        app->systemRequestedQuit();
                });
            }
            else
            {
                quit();
            }
        }

    private:
        std::unique_ptr<juce::StandalonePluginHolder> createPluginHolder()
        {
           #ifdef JucePlugin_PreferredChannelConfigurations
            constexpr juce::StandalonePluginHolder::PluginInOuts channels[]
            {
                JucePlugin_PreferredChannelConfigurations
            };
            const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfiguration (
                channels, juce::numElementsInArray (channels));
           #else
            const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfiguration;
           #endif

            return std::make_unique<juce::StandalonePluginHolder> (
                appProperties.getUserSettings(), false, juce::String(), nullptr, channelConfiguration, false);
        }

        juce::ApplicationProperties appProperties;
        SfxrLookAndFeel standaloneLookAndFeel;
        std::unique_ptr<SfxrStandaloneWindow> mainWindow;
        std::unique_ptr<juce::StandalonePluginHolder> headlessPluginHolder;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfxrStandaloneApplication)
    };
}

// The JUCE standalone wrapper calls this factory when
// JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP is enabled.
juce::JUCEApplicationBase* juce_CreateApplication();

juce::JUCEApplicationBase* juce_CreateApplication()
{
    return new SfxrStandaloneApplication();
}

#endif
