cmake_minimum_required(VERSION 3.22)
project(Boostor VERSION 0.0.14)

add_subdirectory(JUCE)

juce_add_plugin(Boostor
    COMPANY_NAME "boostor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Bstr
    PLUGIN_CODE Bstr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "boostor")

target_sources(Boostor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

juce_add_binary_data(BoostorBinaryData
    SOURCES
        Assets/conjius-avatar-transparent-bg.png
        Assets/boostor-logo-orange.png
        Assets/Inconsolata-Bold.ttf
        Assets/Inconsolata-Regular.ttf)

target_compile_definitions(Boostor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Boostor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        BoostorBinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Tests
enable_testing()

juce_add_console_app(BoostorTests PRODUCT_NAME "BoostorTests")

target_sources(BoostorTests
    PRIVATE
        Tests/GainTest.cpp
        Source/PluginProcessor.cpp
        Source/KnobDesign.h)

target_compile_definitions(BoostorTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_UNIT_TESTS=1
        BOOSTOR_TESTS=1)

target_link_libraries(BoostorTests
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Screenshot CLI ---
# Headless renderer for the plugin editor — used by CI to generate the README image.
add_executable(BoostorScreenshot
    Tools/Screenshot.cpp)

target_include_directories(BoostorScreenshot PRIVATE Source)

target_link_libraries(BoostorScreenshot
    PRIVATE
        Boostor
        BoostorBinaryData
        juce::juce_audio_utils
        juce::juce_gui_basics)
