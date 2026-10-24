cmake_minimum_required(VERSION 3.22)

project(MatrixForge VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Static runtime so the .vst3 / .exe run on any Windows PC without the VC++ redistributable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(MF_BUILD_TESTS "Build the offline render test" ON)
option(MF_COPY_AFTER_BUILD "Copy the VST3 into the system plug-in folder after building" OFF)

# Use a local JUCE checkout with -DMF_JUCE_PATH=..., otherwise download JUCE automatically.
if(DEFINED MF_JUCE_PATH)
    add_subdirectory(${MF_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MatrixForge
    COMPANY_NAME "MatrixForge Audio"
    PRODUCT_NAME "MatrixForge"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${MF_COPY_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Mfga
    PLUGIN_CODE Mfx1
    VST3_CATEGORIES Instrument Synth
    FORMATS VST3 Standalone)

set(MF_SOURCES
    Source/Params.cpp
    Source/Presets.cpp
    Source/dsp/Synth.cpp
    Source/dsp/ArpSeq.cpp
    Source/dsp/FxChain.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/ForgeLookAndFeel.cpp
    Source/ui/ModMatrixComponent.cpp
    Source/ui/SeqComponent.cpp)

target_sources(MatrixForge PRIVATE ${MF_SOURCES})
target_include_directories(MatrixForge PRIVATE Source)

set(MF_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_definitions(MatrixForge PUBLIC ${MF_DEFINITIONS})

set(MF_JUCE_MODULES
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp)

target_link_libraries(MatrixForge
    PRIVATE ${MF_JUCE_MODULES}
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(MF_BUILD_TESTS)
    juce_add_console_app(MatrixForgeTests PRODUCT_NAME "MatrixForgeTests")
    target_sources(MatrixForgeTests PRIVATE Tests/RenderTests.cpp ${MF_SOURCES})
    target_include_directories(MatrixForgeTests PRIVATE Source)
    target_compile_definitions(MatrixForgeTests PRIVATE ${MF_DEFINITIONS} JucePlugin_Name="MatrixForge"
        JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(MatrixForgeTests PRIVATE ${MF_JUCE_MODULES} juce::juce_recommended_config_flags)
endif()
