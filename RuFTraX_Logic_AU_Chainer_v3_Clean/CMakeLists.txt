cmake_minimum_required(VERSION 3.22)
project(RuFTraX_Logic_AU_Chainer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put JUCE in a sibling folder or set JUCE_DIR before configuring:
# cmake -B build -S . -DJUCE_DIR=/path/to/JUCE
if(NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Set JUCE_DIR to your JUCE folder, e.g. -DJUCE_DIR=/Users/mike/JUCE")
endif()
add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(RuFTraXLogicChainer
    COMPANY_NAME "RuFTraX"
    PLUGIN_MANUFACTURER_CODE RfTx
    PLUGIN_CODE RfLC
    FORMATS AU Standalone
    PRODUCT_NAME "RuFTraX Logic Chainer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(RuFTraXLogicChainer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/GainStage.cpp
    Source/Host/HostEngine.cpp
    Source/UI/SlotComponent.cpp
    Source/Hosting/AUHost.cpp
    Source/UI/SlotComponent.cpp
    Source/Hosting/AUHost.cpp
)

target_compile_definitions(RuFTraXLogicChainer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(RuFTraXLogicChainer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
