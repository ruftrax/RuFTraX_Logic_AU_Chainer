#include "PluginEditor.h"

SSLLookAndFeel::SSLLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, juce::Colours::silver);
    setColour(juce::TextButton::buttonColourId, juce::Colour(32, 32, 32));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(140, 90, 20));
    setColour(juce::TextButton::textColourOffId, juce::Colours::whitesmoke);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void SSLLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float start, float end, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(5);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto angle = start + pos * (end - start);

    g.setColour(juce::Colour(18, 18, 18));
    g.fillEllipse(bounds);
    g.setColour(juce::Colour(120, 120, 120));
    g.drawEllipse(bounds, 2.0f);
    g.setColour(juce::Colours::silver);
    g.fillEllipse(bounds.reduced(radius * 0.22f));

    juce::Path p;
    p.addRectangle(-1.5f, -radius, 3.0f, radius * 0.55f);
    g.setColour(juce::Colours::black);
    g.fillPath(p, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

RuFTraXLogicChainerAudioProcessorEditor::RuFTraXLogicChainerAudioProcessorEditor (RuFTraXLogicChainerAudioProcessor& p)
: AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel(&sslLnf);

    title.setText("RuFTraX SSL CHAINER", juce::dontSendNotification);
    title.setFont(juce::Font(32, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);

    subtitle.setText("VERSION 2 SSL UI - INTERNAL DSP CORE", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(title);
    addAndMakeVisible(subtitle);

    for (auto* b : { &save, &load, &bypassAll, &testTone })
        addAndMakeVisible(*b);

    save.onClick = [this]
    {
        processor.saveTemporaryPreset();
    };

    load.onClick = [this]
    {
        processor.loadTemporaryPreset();
        repaint();
    };

    save.onClick = [this]
    {
        processor.saveTemporaryPreset();
    };

    load.onClick = [this]
    {
        processor.loadTemporaryPreset();
        repaint();
    };

    save.onClick = [this]
    {
        processor.saveTemporaryPreset();
    };

    load.onClick = [this]
    {
        processor.loadTemporaryPreset();
        repaint();
    };

    bypassAll.setClickingTogglesState(true);
    bypassAll.onClick = [this]
    {
        const bool state = bypassAll.getToggleState();

        if (auto* bypass = processor.apvts.getParameter("bypassAll"))
            bypass->setValueNotifyingHost(state ? 1.0f : 0.0f);

        for (int i = 0; i < 8; ++i)
        {
            auto paramName = "slot" + juce::String(i + 1) + "Bypass";
            if (auto* slotBypass = processor.apvts.getParameter(paramName))
                slotBypass->setValueNotifyingHost(state ? 1.0f : 0.0f);
        }
    };

    testTone.setClickingTogglesState(true);
    testTone.onClick = [this]
    {
        processor.setTestToneEnabled(testTone.getToggleState());
    };

    for (auto* s : { &inputGain, &outputGain })
    {
        addAndMakeVisible(*s);
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setRange(-24, 24, 0.1);
        s->setValue(0);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    }

    for (int i = 0; i < 8; ++i)
    {
        slots[(size_t)i] = std::make_unique<SlotComponent>(processor, i);
        addAndMakeVisible(*slots[(size_t)i]);
    }

    startTimerHz(24);
    setSize(1180, 760);
}

RuFTraXLogicChainerAudioProcessorEditor::~RuFTraXLogicChainerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void RuFTraXLogicChainerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(12, 12, 12));

    auto outer = getLocalBounds().reduced(10);
    g.setColour(juce::Colour(34, 34, 32));
    g.fillRoundedRectangle(outer.toFloat(), 8.0f);

    g.setColour(juce::Colour(120, 95, 48));
    g.drawRoundedRectangle(outer.toFloat(), 8.0f, 2.0f);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRect(0, 92, getWidth(), 5);

    g.setColour(juce::Colours::whitesmoke);
    g.setFont(15);
    g.drawText("INPUT", 25, 122, 140, 24, juce::Justification::centred);
    g.drawText("OUTPUT", getWidth() - 165, 122, 140, 24, juce::Justification::centred);

    auto drawMeter = [&g](juce::Rectangle<int> meter, float level)
    {
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(meter.toFloat(), 4.0f);

        level = juce::jlimit(0.0f, 1.0f, level * 6.0f);
        auto fill = meter.withTrimmedTop((int)(meter.getHeight() * (1.0f - level)));

        g.setColour(level > 0.85f ? juce::Colours::red : juce::Colours::limegreen);
        g.fillRoundedRectangle(fill.toFloat(), 4.0f);

        g.setColour(juce::Colours::darkgrey);
        g.drawRoundedRectangle(meter.toFloat(), 4.0f, 1.0f);
    };

    drawMeter(juce::Rectangle<int>(70, 280, 38, 330), processor.getInputLevel());
    drawMeter(juce::Rectangle<int>(getWidth() - 118, 280, 18, 330), processor.getOutputLevelL());
    drawMeter(juce::Rectangle<int>(getWidth() - 94, 280, 18, 330), processor.getOutputLevelR());
}

void RuFTraXLogicChainerAudioProcessorEditor::resized()
{
    title.setBounds(32, 20, 460, 38);
    subtitle.setBounds(34, 58, 520, 24);

    save.setBounds(720, 28, 70, 30);
    load.setBounds(800, 28, 70, 30);
    bypassAll.setBounds(890, 28, 120, 30);
    testTone.setBounds(1020, 28, 120, 30);

    inputGain.setBounds(35, 150, 110, 110);
    outputGain.setBounds(getWidth() - 145, 150, 110, 110);

    int top = 112;
    int left = 185;
    int width = getWidth() - 370;
    int h = 76;

    for (int i = 0; i < 8; ++i)
        slots[(size_t)i]->setBounds(left, top + i * h, width, h - 6);
}

void RuFTraXLogicChainerAudioProcessorEditor::timerCallback()
{
    repaint();
}
