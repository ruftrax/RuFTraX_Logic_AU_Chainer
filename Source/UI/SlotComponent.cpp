#include "SlotComponent.h"
#include "../Hosting/AUHost.h"

SlotComponent::SlotComponent(RuFTraXLogicChainerAudioProcessor& p, int slotIndex)
    : processor(p), index(slotIndex)
{
    for (auto* b : { &insertButton, &bypassButton, &editButton, &removeButton })
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

    auto prefix = "slot" + juce::String(index + 1);

    bypassAttachment = std::make_unique<ButtonAttachment>(processor.apvts, prefix + "Bypass", bypassButton);
    mixAttachment    = std::make_unique<SliderAttachment>(processor.apvts, prefix + "Mix", mixSlider);
    gainAttachment   = std::make_unique<SliderAttachment>(processor.apvts, prefix + "Gain", gainSlider);
    panAttachment    = std::make_unique<SliderAttachment>(processor.apvts, prefix + "Pan", panSlider);

    insertButton.onClick = [this]
    {
        auto plugins = AUHost::getInstalledEffects();

        juce::PopupMenu menu;
        int itemId = 1;

        for (const auto& plugin : plugins)
            menu.addItem(itemId++, plugin.name);

        menu.showMenuAsync(juce::PopupMenu::Options(),
            [this, plugins](int result)
            {
                if (result <= 0)
                    return;

                const int selectedIndex = result - 1;

                if (! juce::isPositiveAndBelow(selectedIndex, plugins.size()))
                    return;

                const auto& plugin = plugins.getReference(selectedIndex);

                processor.loadPlaceholderPlugin(index, plugin.name);

                auto enabledParam = "slot" + juce::String(index + 1) + "Enabled";
                if (auto* enabled = processor.apvts.getParameter(enabledParam))
                    enabled->setValueNotifyingHost(1.0f);

                repaint();
            });
    };

    editButton.onClick = [this]
    {
        auto* window = new juce::DocumentWindow(
            "RuFTraX Hosted Plug-in",
            juce::Colours::black,
            juce::DocumentWindow::closeButton);

        auto* label = new juce::Label();
        label->setText("Slot " + juce::String(index + 1) + "\n" + processor.getSlotName(index) + "\n\nNative editor will open here", juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setColour(juce::Label::backgroundColourId, juce::Colour(30, 30, 30));

        window->setUsingNativeTitleBar(true);
        window->setContentOwned(label, true);
        window->centreWithSize(520, 260);
        window->setVisible(true);
    };

    removeButton.onClick = [this]
    {
        processor.clearSlot(index);

        auto enabledParam = "slot" + juce::String(index + 1) + "Enabled";
        if (auto* enabled = processor.apvts.getParameter(enabledParam))
            enabled->setValueNotifyingHost(0.0f);

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

    const bool active = processor.isSlotActive(index);

    g.setColour(active ? juce::Colours::limegreen : juce::Colour(75, 75, 75));
    g.fillEllipse(14, 16, 12, 12);

    g.setColour(juce::Colours::whitesmoke);
    g.setFont(18);
    g.drawText("SLOT " + juce::String(index + 1), 34, 8, 90, 24, juce::Justification::centredLeft);

    g.setFont(16);
    g.setColour(active ? juce::Colours::white : juce::Colours::lightgrey);
    g.drawText(processor.getSlotName(index), 130, 8, 250, 24, juce::Justification::centredLeft);

    auto signal = juce::Rectangle<int>(390, 18, 90, 12);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(signal.toFloat(), 3.0f);
    float slotLevel = 0.0f;
    if (index == 0)
        slotLevel = processor.getSlot1Level();

    slotLevel = juce::jlimit(0.0f, 1.0f, slotLevel * 6.0f);
    int activeWidth = juce::jmax(4, (int)(signal.getWidth() * slotLevel));

    g.setColour(slotLevel > 0.85f ? juce::Colours::red : (active ? juce::Colours::limegreen : juce::Colour(45, 45, 45)));
    g.fillRoundedRectangle(signal.withWidth(active ? activeWidth : 12).toFloat(), 3.0f);
    g.setColour(juce::Colours::darkgrey);
    g.drawRoundedRectangle(signal.toFloat(), 3.0f, 1.0f);

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
    removeButton.setBounds(r.removeFromLeft(85).reduced(3));

    mixSlider.setBounds(r.removeFromLeft(90).reduced(4));
    gainSlider.setBounds(r.removeFromLeft(90).reduced(4));
    panSlider.setBounds(r.removeFromLeft(90).reduced(4));
}
