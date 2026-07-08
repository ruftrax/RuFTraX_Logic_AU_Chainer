#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class SSLLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SSLLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

class SlotRow : public juce::Component
{
public:
    SlotRow(RuFTraXLogicChainerAudioProcessor& p, int slotIndex);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    RuFTraXLogicChainerAudioProcessor& processor;
    int index;
    juce::TextButton load {"+ LOAD AU"}, bypass {"BYPASS"}, edit {"EDIT"};
    juce::Slider mix, gain, pan;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> panAttachment;
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
    juce::TextButton save {"SAVE"}, load {"LOAD"}, bypassAll {"BYPASS ALL"};
    std::array<std::unique_ptr<SlotRow>, 8> rows;
};
