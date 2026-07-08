#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"

class SlotComponent : public juce::Component
{
public:
    SlotComponent(RuFTraXLogicChainerAudioProcessor& p, int slotIndex);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    RuFTraXLogicChainerAudioProcessor& processor;
    int index;

    juce::TextButton insertButton {"+ INSERT"};
    juce::TextButton bypassButton {"BYPASS"};
    juce::TextButton editButton {"EDIT"};
    juce::TextButton removeButton {"REMOVE"};

    juce::Slider mixSlider;
    juce::Slider gainSlider;
    juce::Slider panSlider;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> panAttachment;
};
