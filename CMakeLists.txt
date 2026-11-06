cmake_minimum_required(VERSION 3.22)
project(CustomFreeverb VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE develop branch
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        develop
)
FetchContent_MakeAvailable(JUCE)

# Define Plugin Target
juce_add_plugin(CustomFreeverb
    COMPANY_NAME "BobbyQooks"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Qook
    PLUGIN_CODE Fvrb
    FORMATS VST3 AU
)

# Instruct CMake to generate JuceHeader.h
juce_generate_juce_header(CustomFreeverb)

# Explicitly pass macro to compiler to bypass VST2 automation check
target_compile_definitions(CustomFreeverb PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Add Source Files
target_sources(CustomFreeverb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/CombFilter.h
    Source/AllPassFilter.h
    Source/EarlyReflections.h
)

# Link JUCE Modules
target_link_libraries(CustomFreeverb PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)