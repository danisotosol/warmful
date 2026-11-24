# WARMFUL — behavioural replica of a four-module vintage multi-effect pedal.
# SPDX-License-Identifier: CC-BY-NC-SA-4.0
cmake_minimum_required (VERSION 3.25)
project (WARMFUL VERSION 0.3.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif ()

# Windows: link the C++ runtime statically so the installer needs no VC++ redistributable.
set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option (WARMFUL_BUILD_PLUGIN "Build the JUCE plug-in (VST3/Standalone/LV2/AU)" ON)
option (WARMFUL_BUILD_TOOLS "Build the offline renderer / DSP test-suite" ON)

# ---- DSP core: plain C++20, no framework ---------------------------------------------------------
add_library (warmful_dsp STATIC
    source/dsp/Capture.cpp
    source/dsp/Engine.cpp
    source/dsp/FactoryPresets.cpp
    source/dsp/Gesture.cpp
    source/dsp/Module.cpp
    source/dsp/effects/character/CharacterFactory.cpp
    source/dsp/effects/character/Drive.cpp
    source/dsp/effects/character/Fuzz.cpp
    source/dsp/effects/character/Howl.cpp
    source/dsp/effects/character/Sweeten.cpp
    source/dsp/effects/character/Swell.cpp
    source/dsp/effects/movement/Doubler.cpp
    source/dsp/effects/movement/MovementFactory.cpp
    source/dsp/effects/movement/Phaser.cpp
    source/dsp/effects/movement/Pitch.cpp
    source/dsp/effects/movement/Tremolo.cpp
    source/dsp/effects/movement/Vibrato.cpp
    source/dsp/effects/diffusion/Cascade.cpp
    source/dsp/effects/diffusion/Collage.cpp
    source/dsp/effects/diffusion/DiffusionFactory.cpp
    source/dsp/effects/diffusion/Reels.cpp
    source/dsp/effects/diffusion/Reverse.cpp
    source/dsp/effects/diffusion/Space.cpp
    source/dsp/effects/texture/Broken.cpp
    source/dsp/effects/texture/Cassette.cpp
    source/dsp/effects/texture/Filter.cpp
    source/dsp/effects/texture/Interference.cpp
    source/dsp/effects/texture/Squash.cpp
    source/dsp/effects/texture/TextureFactory.cpp)
target_include_directories (warmful_dsp PUBLIC source/dsp)
if (MSVC)
    target_compile_options (warmful_dsp PRIVATE /W4)
else ()
    target_compile_options (warmful_dsp PRIVATE -Wall -Wextra)
endif ()

# ---- Offline renderer + tests ---------------------------------------------------------------------
if (WARMFUL_BUILD_TOOLS)
    add_executable (warmful_render tools/render/main.cpp)
    target_link_libraries (warmful_render PRIVATE warmful_dsp)
    enable_testing ()
    add_test (NAME dsp COMMAND warmful_render test)
endif ()

# ---- JUCE plug-in ---------------------------------------------------------------------------------
if (WARMFUL_BUILD_PLUGIN)
    include (FetchContent)
    # Offline/local builds: -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable (JUCE)

    set (WARMFUL_FORMATS VST3 Standalone)
    if (APPLE)
        list (APPEND WARMFUL_FORMATS AU)
    elseif (UNIX)
        list (APPEND WARMFUL_FORMATS LV2)
    endif ()

    juce_add_plugin (WARMFUL
        PRODUCT_NAME "WARMFUL"
        COMPANY_NAME "WARMFUL"
        COMPANY_WEBSITE "https://github.com/danisotosol/WARMFUL"
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png"
        BUNDLE_ID "org.warmful.warmful"
        PLUGIN_MANUFACTURER_CODE Wrmf
        PLUGIN_CODE Wrm1
        FORMATS ${WARMFUL_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Distortion Modulation Delay Reverb
        AU_MAIN_TYPE kAudioUnitType_MusicEffect
        LV2URI "urn:warmful:warmful"
        DESCRIPTION "Four-module vintage multi-effect")

    target_sources (WARMFUL PRIVATE
        source/plugin/Parameters.cpp
        source/plugin/PluginEditor.cpp
        source/plugin/PluginProcessor.cpp
        source/plugin/PresetBank.cpp
        source/plugin/ui/Controls.cpp
        source/plugin/ui/Panels.cpp)
    target_include_directories (WARMFUL PRIVATE source/plugin)

    target_compile_definitions (WARMFUL PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries (WARMFUL
        PRIVATE
            warmful_dsp
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif ()
