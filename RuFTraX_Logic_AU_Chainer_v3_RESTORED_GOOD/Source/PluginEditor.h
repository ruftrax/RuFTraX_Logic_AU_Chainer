#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/SlotComponent.h"

class SSLLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SSLLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

class RuFTraXLogicChainerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                     private juce::Timer
{
public:
    explicit RuFTraXLogicChainerAudioProcessorEditor (RuFTraXLogicChainerAudioProcessor&);
    ~RuFTraXLogicChainerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    RuFTraXLogicChainerAudioProcessor& processor;
    SSLLookAndFeel sslLnf;

    juce::Label title, subtitle;
    juce::Slider inputGain, outputGain;
    juce::TextButton save {"SAVE"}, load {"LOAD"}, bypassAll {"BYPASS ALL"}, testTone {"TEST TONE"};

    std::array<std::unique_ptr<SlotComponent>, 8> slots;
};
