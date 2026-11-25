cmake_minimum_required(VERSION 3.22)
project(PsyGoaSynth VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# JUCE is downloaded automatically at configure time (needs internet once).
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PsyGoaSynth
    COMPANY_NAME "PsyGoaLabs"
    PLUGIN_MANUFACTURER_CODE PsGo
    PLUGIN_CODE PgSy
    PRODUCT_NAME "PsyGoa Synth"
    FORMATS VST3 Standalone
    VST3_CATEGORIES Instrument Synth
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(PsyGoaSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Params.cpp
    Source/Presets.cpp)

target_include_directories(PsyGoaSynth PRIVATE Source)

target_compile_definitions(PsyGoaSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(PsyGoaSynth
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional: DSP unit test (no JUCE needed)  ->  cmake -DPSY_BUILD_TESTS=ON
option(PSY_BUILD_TESTS "Build DSP sanity test" OFF)
if(PSY_BUILD_TESTS)
    add_executable(psy_dsp_test tests/dsp_test.cpp)
endif()
