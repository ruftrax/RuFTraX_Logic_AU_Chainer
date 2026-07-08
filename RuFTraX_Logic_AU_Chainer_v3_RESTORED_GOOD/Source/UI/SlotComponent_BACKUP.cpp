#include "SlotComponent.h"

SlotComponent::SlotComponent(RuFTraXLogicChainerAudioProcessor& p, int slotIndex)
    : processor(p), index(slotIndex)
{
    for (auto* b : { &insertButton, &bypassButton, &editButton })
        addAndMakeVisible(*b);

    bypassButton.setClickingTogglesState(true);

    for (auto* s : { &mixSlider, &gainSlider, &panSlider })
    {
        addAndMakeVisible(*s);
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    }

    mixSlider.setTextValueSuffix("");
    gainSlider.setTextValueSuffix(" dB");

    if (index == 0)
    {
        bypassAttachment = std::make_unique<ButtonAttachment>(processor.apvts, "slot1Bypass", bypassButton);
        mixAttachment    = std::make_unique<SliderAttachment>(processor.apvts, "slot1Mix", mixSlider);
        gainAttachment   = std::make_unique<SliderAttachment>(processor.apvts, "slot1Gain", gainSlider);
        panAttachment    = std::make_unique<SliderAttachment>(processor.apvts, "slot1Pan", panSlider);
    }

    insertButton.onClick = [this]
    {
        if (index != 0)
            return;

        processor.loadPlaceholderPlugin(index, "Internal Gain / Pan");

        if (auto* enabled = processor.apvts.getParameter("slot1Enabled"))
            enabled->setValueNotifyingHost(1.0f);

        repaint();
    };
}

void SlotComponent::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().reduced(2);

    g.setColour(juce::Colour(24, 24, 24));
    g.fillRoundedRectangle(r.toFloat(), 5.0f);

    g.setColour(juce::Colour(95, 82, 50));
    g.drawRoundedRectangle(r.toFloat(), 5.0f, 1.5f);

    g.setColour(juce::Colours::whitesmoke);
    g.setFont(18);
    g.drawText("SLOT " + juce::String(index + 1), 14, 8, 90, 24, juce::Justification::centredLeft);

    g.setFont(15);
    g.drawText(processor.slots[(size_t) index].displayName, 115, 8, 230, 24, juce::Justification::centredLeft);

    g.setFont(11);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("MIX", 530, 5, 70, 16, juce::Justification::centred);
    g.drawText("GAIN", 620, 5, 70, 16, juce::Justification::centred);
    g.drawText("PAN", 710, 5, 70, 16, juce::Justification::centred);
}

void SlotComponent::resized()
{
    auto r = getLocalBounds().reduced(8);
    r.removeFromLeft(350);

    insertButton.setBounds(r.removeFromLeft(95).reduced(3));
    bypassButton.setBounds(r.removeFromLeft(90).reduced(3));
    editButton.setBounds(r.removeFromLeft(70).reduced(3));

    mixSlider.setBounds(r.removeFromLeft(90).reduced(4));
    gainSlider.setBounds(r.removeFromLeft(90).reduced(4));
    panSlider.setBounds(r.removeFromLeft(90).reduced(4));
}
