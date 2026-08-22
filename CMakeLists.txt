cmake_minimum_required(VERSION 3.22)
project(RuFTraX_PlugIn_Chainer VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Set JUCE_DIR to your JUCE folder, e.g. -DJUCE_DIR=/Users/mike/JUCE")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

if(APPLE)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
endif()

juce_add_plugin(RuFTraXLogicChainer
    BUNDLE_ID "com.ruftrax.pluginchainer"
    COMPANY_NAME "RuFTraX"
    PLUGIN_MANUFACTURER_CODE RfTx
    PLUGIN_CODE RfLC
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "RuFTraX Plug-In Chainer"
    PLUGIN_NAME "RuFTraX Plug-In Chainer"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/AppIcon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/AppIcon.png"
    DESCRIPTION "Eight-slot plug-in chainer for Logic Pro and compatible macOS DAWs"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGINHOST_AU TRUE
)

target_sources(RuFTraXLogicChainer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# PLUGINHOST_AU above is a real juce_add_plugin() argument (it also links the
# CoreAudioKit/AudioUnit frameworks on Apple platforms), but there is no
# equivalent PLUGINHOST_VST3 argument - VST3 hosting is just a plain compile
# definition, since JUCE bundles the VST3 SDK itself and hosting a VST3
# needs no extra framework linking on any platform.
target_compile_definitions(RuFTraXLogicChainer PUBLIC
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RuFTraXLogicChainer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_dsp
    ${AUDIOTOOLBOX_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
