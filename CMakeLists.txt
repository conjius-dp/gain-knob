cmake_minimum_required(VERSION 3.22)
project(GainKnob VERSION 0.0.1)

add_subdirectory(JUCE)

juce_add_plugin(GainKnob
    COMPANY_NAME "GainKnob"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Gknb
    PLUGIN_CODE Gknb
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "GainKnob")

target_sources(GainKnob
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

juce_add_binary_data(GainKnobBinaryData
    SOURCES
        Assets/conjius-avatar-transparent-bg.png
        Assets/Inconsolata-Bold.ttf
        Assets/Inconsolata-Regular.ttf)

target_compile_definitions(GainKnob
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GainKnob
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        GainKnobBinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Tests
enable_testing()

juce_add_console_app(GainKnobTests PRODUCT_NAME "GainKnobTests")

target_sources(GainKnobTests
    PRIVATE
        Tests/GainTest.cpp
        Source/PluginProcessor.cpp
        Source/KnobDesign.h)

target_compile_definitions(GainKnobTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_UNIT_TESTS=1
        GAINKNOB_TESTS=1)

target_link_libraries(GainKnobTests
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
