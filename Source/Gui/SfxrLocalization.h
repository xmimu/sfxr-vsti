#pragma once

#include <JuceHeader.h>

// UI language is a user preference, deliberately separate from APVTS/project
// state so opening a session never changes another user's interface language.
namespace SfxrLocalization
{
    enum class Language { automatic = 0, chinese, english };

    inline juce::PropertiesFile::Options settingsOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "SfxrVsti";
        options.filenameSuffix = ".ui-settings";
        options.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
       #endif
        return options;
    }

    inline juce::PropertiesFile& settings()
    {
        static juce::PropertiesFile file (settingsOptions());
        return file;
    }

    inline Language selectedLanguage()
    {
        return static_cast<Language> (juce::jlimit (0, 2, settings().getIntValue ("language", 0)));
    }

    inline bool isChinese()
    {
        const auto selected = selectedLanguage();
        return selected == Language::chinese
            || (selected == Language::automatic
                && juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("zh"));
    }

    inline juce::String text (const char* english, const char* chinese)
    {
        return isChinese() ? juce::String::fromUTF8 (chinese) : juce::String (english);
    }

    inline juce::ChangeBroadcaster& changes()
    {
        static juce::ChangeBroadcaster broadcaster;
        return broadcaster;
    }

    // JUCE's AudioDeviceSelectorComponent uses TRANS() internally rather than
    // the app's labels. Only the standalone app calls this: a plug-in must not
    // replace its host's process-wide JUCE translation mappings.
    inline void applyStandaloneJuceTranslations()
    {
        if (! isChinese())
        {
            juce::LocalisedStrings::setCurrentMappings (nullptr);
            return;
        }

        const auto translations = juce::String::fromUTF8 (R"translations(
language: Chinese
countries: cn tw hk sg

"Audio/MIDI Settings" = "音频/MIDI 设置"
"Output:" = "输出："
"Device:" = "设备："
"Input:" = "输入："
"Active output channels:" = "活动输出通道："
"Active input channels:" = "活动输入通道："
"Sample rate:" = "采样率："
"Audio buffer size:" = "音频缓冲区大小："
"Active MIDI inputs:" = "活动 MIDI 输入："
"MIDI Output:" = "MIDI 输出："
"Bluetooth MIDI" = "蓝牙 MIDI"
"Test" = "测试"
"none" = "无"
"No MIDI inputs available" = "没有可用的 MIDI 输入"
)translations");

        juce::LocalisedStrings::setCurrentMappings (
            new juce::LocalisedStrings (translations, false));
    }

    inline void setLanguage (Language language)
    {
        if (selectedLanguage() == language)
            return;

        settings().setValue ("language", static_cast<int> (language));
        settings().saveIfNeeded();
        changes().sendChangeMessage();
    }
}
