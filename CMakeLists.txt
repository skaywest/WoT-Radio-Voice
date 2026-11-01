cmake_minimum_required(VERSION 3.22)
project(WoTRadioVoice VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(WoTRadioVoice
    COMPANY_NAME "FieldRadioAudio"
    BUNDLE_ID "com.fieldradioaudio.wotradiovoice"
    PLUGIN_MANUFACTURER_CODE FldR
    PLUGIN_CODE WtRv
    FORMATS VST3
    PRODUCT_NAME "WoT Radio Voice"
    DESCRIPTION "Configurable radio voice effect"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(WoTRadioVoice PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(WoTRadioVoice PRIVATE cxx_std_17)

target_compile_definitions(WoTRadioVoice PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(WoTRadioVoice PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
