#include "PluginEditor.h"

SSLLookAndFeel::SSLLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, juce::Colours::silver);
    setColour(juce::TextButton::buttonColourId, juce::Colour(32, 32, 32));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(140, 90, 20));
    setColour(juce::TextButton::textColourOffId, juce::Colours::whitesmoke);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void SSLLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(5);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto angle = start + pos * (end - start);
    g.setColour(juce::Colour(20, 20, 20)); g.fillEllipse(bounds);
    g.setColour(juce::Colours::darkgrey); g.drawEllipse(bounds, 2.0f);
    g.setColour(juce::Colours::silver); g.fillEllipse(bounds.reduced(radius * 0.22f));
    juce::Path p; p.addRectangle(-1.5f, -radius, 3.0f, radius * 0.55f);
    g.setColour(juce::Colours::black);
    g.fillPath(p, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

RuFTraXLogicChainerAudioProcessorEditor::RuFTraXLogicChainerAudioProcessorEditor (RuFTraXLogicChainerAudioProcessor& p)
: AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel(&sslLnf);
    title.setText("RuFTraX LOGIC CHAINER", juce::dontSendNotification); title.setFont(juce::Font(30, juce::Font::bold)); title.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
    subtitle.setText("SSL CONSOLE STYLE AU PLUG-IN CHAINER FOR LOGIC PRO", juce::dontSendNotification); subtitle.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(title); addAndMakeVisible(subtitle);
    for (auto* b : { &save, &load, &bypassAll }) addAndMakeVisible(*b);
    for (auto* s : { &inputGain, &outputGain }) { addAndMakeVisible(*s); s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s->setRange(-24,24,.1); s->setValue(0); s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20); }
    for (int i=0;i<8;++i) { rows[(size_t)i] = std::make_unique<SlotComponent>(processor, i); addAndMakeVisible(*rows[(size_t)i]); }

    startTimerHz(24);
    setSize (1180, 760);
}
RuFTraXLogicChainerAudioProcessorEditor::~RuFTraXLogicChainerAudioProcessorEditor(){ setLookAndFeel(nullptr); }
void RuFTraXLogicChainerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(14, 14, 14));
    auto r = getLocalBounds().reduced(10);
    g.setColour(juce::Colour(38, 38, 36)); g.fillRoundedRectangle(r.toFloat(), 6);
    g.setColour(juce::Colour(106, 92, 60)); g.drawRoundedRectangle(r.toFloat(), 6, 2);
    g.setColour(juce::Colours::black.withAlpha(.35f)); g.fillRect(0, 88, getWidth(), 4);
    g.setColour(juce::Colours::whitesmoke); g.setFont(15);
    g.drawText("INPUT", 25, 120, 140, 24, juce::Justification::centred);
    g.drawText("OUTPUT", getWidth()-165, 120, 140, 24, juce::Justification::centred);

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
    drawMeter(juce::Rectangle<int>(getWidth() - 108, 280, 38, 330), processor.getOutputLevel());
}
void RuFTraXLogicChainerAudioProcessorEditor::resized()
{
    title.setBounds(28, 18, 450, 34); subtitle.setBounds(30, 50, 520, 24);
    save.setBounds(720, 24, 70, 30); load.setBounds(800, 24, 70, 30); bypassAll.setBounds(890, 24, 110, 30);
    inputGain.setBounds(35, 150, 110, 110); outputGain.setBounds(getWidth()-145, 150, 110, 110);
    int top = 105; int left = 170; int width = getWidth() - 340; int h = 74;
    for (int i=0;i<8;++i) rows[(size_t)i]->setBounds(left, top + i*h, width, h-4);
}


void RuFTraXLogicChainerAudioProcessorEditor::timerCallback()
{
    repaint();
}
