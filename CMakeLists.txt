cmake_minimum_required(VERSION 3.22)

project(Quill VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(Quill
    COMPANY_NAME "Quill"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Qulc
    PLUGIN_CODE Qll1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Quill")

target_sources(Quill
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/CompilerLocator.cpp
        Source/CompileService.cpp)

target_include_directories(Quill
    PRIVATE
        shared)

target_compile_definitions(Quill
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Quill
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
