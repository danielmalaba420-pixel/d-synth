cmake_minimum_required(VERSION 3.22)

# Mac: build for Apple Silicon and Intel, macOS 11+ (must be set before project())
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(DSYNTH VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(DSynth
    COMPANY_NAME "Danova"
    COMPANY_WEBSITE "https://danova.cd"
    BUNDLE_ID "com.danova.dsynth"
    PLUGIN_MANUFACTURER_CODE Dnva
    PLUGIN_CODE Dsyn
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS AU VST3 Standalone
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    PRODUCT_NAME "D-SYNTH")

juce_add_binary_data(DSynthAssets SOURCES
    Assets/walnut.jpg
    Assets/Michroma-Regular.ttf
    Assets/BarlowCondensed-SemiBold.ttf
    Assets/BarlowCondensed-Bold.ttf
    Assets/ShareTechMono-Regular.ttf)

target_sources(DSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(DSynth PRIVATE Source Source/Engine)

target_compile_definitions(DSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(DSynth
    PRIVATE
        DSynthAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional: renders the editor to a PNG for visual checks (cmake -DDSYNTH_SNAPSHOT=ON)
option(DSYNTH_SNAPSHOT "Build the editor snapshot tool" OFF)
if(DSYNTH_SNAPSHOT)
    juce_add_console_app(DSynthSnapshot PRODUCT_NAME "DSynthSnapshot")
    target_sources(DSynthSnapshot PRIVATE tools/snapshot/main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(DSynthSnapshot PRIVATE Source Source/Engine)
    target_compile_definitions(DSynthSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="D-SYNTH")
    target_link_libraries(DSynthSnapshot PRIVATE DSynthAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
