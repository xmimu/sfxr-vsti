cmake_minimum_required(VERSION 3.24)

project(SfxrVsti VERSION 1.1.0)

enable_testing()

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Disable extras" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

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
        Source/SfxrEngine/SfxrVoice.cpp
        Source/SfxrEngine/SfxrEngine.cpp
        Source/SfxrEngine/SfxrPresets.cpp
        Source/SfxrEngine/SfxrPresetFile.cpp
)

juce_generate_juce_header(SfxrVsti)

target_compile_features(SfxrVsti PRIVATE cxx_std_17)

# Required for the Standalone build.
target_link_libraries(SfxrVsti PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
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
)
target_compile_features(SfxrRenderTest PRIVATE cxx_std_17)
target_link_libraries(SfxrRenderTest PRIVATE juce::juce_audio_basics)

add_test(NAME EngineTests COMMAND SfxrRenderTest)
