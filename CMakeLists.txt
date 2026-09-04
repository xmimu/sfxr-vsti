cmake_minimum_required(VERSION 3.24)

project(SfxrVsti VERSION 1.2.0 DESCRIPTION "Retro video-game sound-effect synthesizer (a port of sfxr) as a VST3/AU/Standalone plugin" HOMEPAGE_URL https://github.com/xmimu/sfxr-vsti)

enable_testing()

# Fixed macOS deployment target so local builds match the CI baseline
# (macOS 14 runner + CMAKE_OSX_ARCHITECTURES universal).
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Disable extras" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)

include(FetchContent)

# Pin JUCE to the exact 8.0.15 commit rather than the (movable) tag name, so
# the dependency cannot silently change between builds.
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

# None of our formats use the web browser or cURL, so drop those JUCE
# dependencies (this also trims the Linux package list).
add_compile_definitions(JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

# Cross-platform formats: VST3 + Standalone everywhere; AU only on Apple.
set(SFXR_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SFXR_FORMATS AU)
endif()

juce_add_plugin(
    SfxrVsti
    COMPANY_NAME            "SfxrVsti"
    PLUGIN_MANUFACTURER_CODE "Sfxr"
    PLUGIN_CODE              "Vsti"
    FORMATS                 ${SFXR_FORMATS}
    PRODUCT_NAME            "SfxrVsti"
    VERSION                 ${PROJECT_VERSION}
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(SfxrVsti
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Gui/SfxrLookAndFeel.cpp
        Source/Gui/SfxrDialogs.cpp
        Source/Gui/WaveformScope.cpp
        Source/Gui/SfxrMidiKeyboard.cpp
        Source/Gui/ExportOptionsComponent.cpp
        Source/Gui/EditorActions.cpp
        Source/SfxrEngine/SfxrVoice.cpp
        Source/SfxrEngine/SfxrEngine.cpp
        Source/SfxrEngine/SfxrPresets.cpp
        Source/SfxrEngine/SfxrPresetFile.cpp
        Source/SfxrEngine/SfxrAudioExporter.cpp
)

juce_generate_juce_header(SfxrVsti)

target_compile_features(SfxrVsti PRIVATE cxx_std_17)

# Required for the Standalone build.
target_link_libraries(SfxrVsti PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

# We never shipped a VST2 version, so the VST3 does not need to be a drop-in
# replacement and can use the modern parameter ID scheme.
target_compile_definitions(SfxrVsti PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

# Offline engine test suite (verifies the DSP independently of any DAW).
juce_add_console_app(SfxrRenderTest
    PRODUCT_NAME "SfxrRenderTest"
)
juce_generate_juce_header(SfxrRenderTest)
target_sources(SfxrRenderTest
    PRIVATE
        tests/RenderTest.cpp
        Source/SfxrEngine/SfxrVoice.cpp
        Source/SfxrEngine/SfxrEngine.cpp
        Source/SfxrEngine/SfxrPresets.cpp
        Source/SfxrEngine/SfxrPresetFile.cpp
        Source/SfxrEngine/SfxrAudioExporter.cpp
)
target_compile_features(SfxrRenderTest PRIVATE cxx_std_17)
target_link_libraries(SfxrRenderTest PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

# A few assertions intentionally compare exact float values (bit-identical
# round trips / "unchanged after failed load"), which -Wfloat-equal flags.
if(NOT MSVC)
    target_compile_options(SfxrRenderTest PRIVATE -Wno-float-equal)
endif()

add_test(NAME EngineTests COMMAND SfxrRenderTest)

# Processor-level integration test: instantiates the real AudioProcessor (same
# translation units as the plugin, minus a host) and drives processBlock with
# MIDI, exercising state save/restore, programs, CC120/123 and oversized blocks.
# This gives CI a chance to load the actual plugin logic, not just the engine.
juce_add_console_app(SfxrPluginTest
    PRODUCT_NAME "SfxrPluginTest"
)
juce_generate_juce_header(SfxrPluginTest)
target_sources(SfxrPluginTest
    PRIVATE
        tests/ProcessorTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Gui/SfxrLookAndFeel.cpp
        Source/Gui/SfxrDialogs.cpp
        Source/Gui/WaveformScope.cpp
        Source/Gui/SfxrMidiKeyboard.cpp
        Source/Gui/ExportOptionsComponent.cpp
        Source/Gui/EditorActions.cpp
        Source/SfxrEngine/SfxrVoice.cpp
        Source/SfxrEngine/SfxrEngine.cpp
        Source/SfxrEngine/SfxrPresets.cpp
        Source/SfxrEngine/SfxrPresetFile.cpp
        Source/SfxrEngine/SfxrAudioExporter.cpp
)
target_compile_features(SfxrPluginTest PRIVATE cxx_std_17)
# These macros normally come from the plugin's own build; provide them so the
# shared sources (which mention JucePlugin_Name / JucePlugin_VersionString) and
# the free createPluginFilter() entry point compile in a console app. The
# version string is taken from project(VERSION) so it cannot drift.
target_compile_definitions(SfxrPluginTest PRIVATE
    JucePlugin_Name="SfxrVsti"
    JucePlugin_VersionString="${PROJECT_VERSION}"
)
target_link_libraries(SfxrPluginTest PRIVATE
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME PluginTests COMMAND SfxrPluginTest)

# GUI open/close leak regression: opens and closes the editor 15 times and
# greps JUCE's LeakedObjectDetector output for any "Leaked" messages. Built
# with JUCE_ENABLE_LEAK_DETECTOR even in Release so the check is meaningful.
juce_add_console_app(SfxrGuiTest
    PRODUCT_NAME "SfxrGuiTest"
)
juce_generate_juce_header(SfxrGuiTest)
target_sources(SfxrGuiTest
    PRIVATE
        tests/GuiLeakTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Gui/SfxrLookAndFeel.cpp
        Source/Gui/SfxrDialogs.cpp
        Source/Gui/WaveformScope.cpp
        Source/Gui/SfxrMidiKeyboard.cpp
        Source/Gui/ExportOptionsComponent.cpp
        Source/Gui/EditorActions.cpp
        Source/SfxrEngine/SfxrVoice.cpp
        Source/SfxrEngine/SfxrEngine.cpp
        Source/SfxrEngine/SfxrPresets.cpp
        Source/SfxrEngine/SfxrPresetFile.cpp
        Source/SfxrEngine/SfxrAudioExporter.cpp
)
target_compile_features(SfxrGuiTest PRIVATE cxx_std_17)
target_compile_definitions(SfxrGuiTest PRIVATE
    JUCE_ENABLE_LEAK_DETECTOR=1
    JucePlugin_Name="SfxrVsti"
    JucePlugin_VersionString="${PROJECT_VERSION}"
)
target_link_libraries(SfxrGuiTest PRIVATE
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# The leak detector prints at exit; run through a wrapper that turns any
# "Leaked" report into a test failure. Windows runners have no bash by default,
# so only register it on POSIX (it is also the platform where the historical
# 77-component leak was reproduced).
if(NOT WIN32)
    add_test(NAME GuiLeakTest
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gui_leak_test.sh $<TARGET_FILE:SfxrGuiTest>)
endif()
