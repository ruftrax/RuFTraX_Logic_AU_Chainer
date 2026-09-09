#include "PluginEditor.h"

#include <cmath>

namespace
{
const auto black = juce::Colour(0xff0b0d0c);
const auto metal = juce::Colour(0xff171a18);
const auto metalLight = juce::Colour(0xff262a26);
const auto gold = juce::Colour(0xffb88a3d);
const auto goldBright = juce::Colour(0xffd2ad68);
const auto cream = juce::Colour(0xffead9b1);
const auto ivory = juce::Colour(0xffd9cfb6);
const auto woodDark = juce::Colour(0xff32170d);
const auto woodMid = juce::Colour(0xff6a341b);
const auto woodLight = juce::Colour(0xff9a5a2b);
const auto faultRed = juce::Colour(0xffcf4632);

void drawScrew(juce::Graphics& g, float x, float y, float radius)
{
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillEllipse(x - radius + 1.5f, y - radius + 2.0f,
                  radius * 2.0f, radius * 2.0f);

    juce::ColourGradient screwGradient(
        goldBright, x - radius, y - radius,
        juce::Colour(0xff55401f), x + radius, y + radius, false);

    g.setGradientFill(screwGradient);
    g.fillEllipse(x - radius, y - radius,
                  radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(0xff241b10));
    g.drawLine(x - radius * 0.5f, y,
               x + radius * 0.5f, y, 1.2f);
}

void drawWoodPanel(juce::Graphics& g,
                   juce::Rectangle<float> bounds,
                   int randomSeed)
{
    juce::ColourGradient woodGradient(
        woodLight, bounds.getX(), bounds.getY(),
        woodDark, bounds.getRight(), bounds.getBottom(), false);

    woodGradient.addColour(0.45, woodMid);
    g.setGradientFill(woodGradient);
    g.fillRect(bounds);

    juce::Random random(randomSeed);

    for (int line = 0; line < 125; ++line)
    {
        const float x = bounds.getX()
                      + random.nextFloat() * bounds.getWidth();
        const float width = 0.35f + random.nextFloat() * 1.4f;
        const float alpha = 0.04f + random.nextFloat() * 0.16f;

        g.setColour((line % 4 == 0 ? juce::Colours::black
                                    : goldBright)
                        .withAlpha(alpha));

        g.drawLine(x, bounds.getY(),
                   x + random.nextFloat() * 3.0f - 1.5f,
                   bounds.getBottom(), width);
    }

    g.setColour(juce::Colours::black.withAlpha(0.48f));
    g.drawRect(bounds, 2.0f);
}

void drawMeter(juce::Graphics& g,
               juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(gold.withAlpha(0.75f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    const auto meter = bounds.reduced(28.0f, 14.0f);

    g.setColour(juce::Colour(0xff030403));
    g.fillRoundedRectangle(meter, 2.0f);

    g.setColour(juce::Colour(0xff596154).withAlpha(0.78f));

    for (int tick = 0; tick <= 12; ++tick)
    {
        const float y = meter.getY()
                      + meter.getHeight() * (float) tick / 12.0f;

        const float tickWidth = tick % 2 == 0 ? 17.0f : 10.0f;

        g.drawLine(meter.getX() - tickWidth, y,
                   meter.getX() - 3.0f, y, 0.8f);

        g.drawLine(meter.getRight() + 3.0f, y,
                   meter.getRight() + tickWidth, y, 0.8f);
    }
}
}

RuFTraXChainerLookAndFeel::RuFTraXChainerLookAndFeel()
{
    setColour(juce::TextButton::textColourOffId, cream);
    setColour(juce::TextButton::textColourOnId, black);
    setColour(juce::Slider::textBoxTextColourId, cream);
    setColour(juce::Slider::textBoxBackgroundColourId, black);
    setColour(juce::Slider::textBoxOutlineColourId, gold.withAlpha(0.66f));
    setColour(juce::ComboBox::backgroundColourId, black);
    setColour(juce::ComboBox::textColourId, cream);
    setColour(juce::ComboBox::outlineColourId, gold);
    setColour(juce::PopupMenu::backgroundColourId, metal);
    setColour(juce::PopupMenu::textColourId, cream);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,
              juce::Colour(0xff4a3820));
}

juce::Font RuFTraXChainerLookAndFeel::getTextButtonFont(
    juce::TextButton&, int buttonHeight)
{
    return juce::Font(
        juce::FontOptions(
            juce::jlimit(10.0f, 18.0f,
                         (float) buttonHeight * 0.36f),
            juce::Font::bold));
}

void RuFTraXChainerLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool highlighted,
    bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool primary = button.getComponentID() == "Primary";
    const bool active = button.getToggleState();

    juce::Colour top;
    juce::Colour bottom;

    if (primary)
    {
        top = juce::Colour(0xfff0dfb5);
        bottom = juce::Colour(0xff9a7135);
    }
    else if (active)
    {
        top = goldBright;
        bottom = juce::Colour(0xff7a4d1d);
    }
    else
    {
        top = metalLight;
        bottom = black;
    }

    if (!button.isEnabled())
    {
        top = top.withMultipliedAlpha(0.34f);
        bottom = bottom.withMultipliedAlpha(0.34f);
    }
    else if (down)
    {
        top = top.darker(0.25f);
        bottom = bottom.darker(0.28f);
    }
    else if (highlighted)
    {
        top = top.brighter(0.10f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(bounds.translated(2.0f, 3.0f), 5.0f);

    juce::ColourGradient gradient(
        top, bounds.getCentreX(), bounds.getY(),
        bottom, bounds.getCentreX(), bounds.getBottom(), false);

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(primary ? juce::Colour(0xff4c3518)
                        : gold.withAlpha(0.88f));
    g.drawRoundedRectangle(bounds, 5.0f,
                           highlighted ? 1.8f : 1.1f);

    g.setColour(cream.withAlpha(primary ? 0.18f : 0.08f));
    g.drawLine(bounds.getX() + 5.0f,
               bounds.getY() + 4.0f,
               bounds.getRight() - 5.0f,
               bounds.getY() + 4.0f,
               0.8f);
}

void RuFTraXChainerLookAndFeel::drawButtonText(
    juce::Graphics& g,
    juce::TextButton& button,
    bool,
    bool)
{
    const bool primary = button.getComponentID() == "Primary";

    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.setColour(primary
                    ? black.withAlpha(button.isEnabled() ? 1.0f : 0.42f)
                    : cream.withAlpha(button.isEnabled() ? 1.0f : 0.40f));

    g.drawFittedText(button.getButtonText(),
                     button.getLocalBounds().reduced(5, 2),
                     juce::Justification::centred,
                     1);
}

void RuFTraXChainerLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float position,
    float startAngle,
    float endAngle,
    juce::Slider& slider)
{
    const bool isPan = slider.getComponentID() == "Pan";
    const bool isInputOutput = slider.getComponentID() == "InputOutput";

    auto area = juce::Rectangle<float>(
        (float) x, (float) y,
        (float) width, (float) height);

    if (isPan)
    {
        const float labelHeight =
            juce::jlimit(8.0f, 13.0f, area.getHeight() * 0.22f);

        g.setFont(juce::Font(
            juce::FontOptions(labelHeight, juce::Font::bold)));
        g.setColour(cream.withAlpha(0.88f));

        auto labelArea = area.removeFromTop(labelHeight + 1.0f);

        g.drawText("L", labelArea.removeFromLeft(
                           labelArea.getWidth() / 3.0f),
                   juce::Justification::centredLeft);

        auto right = labelArea.removeFromRight(
            labelArea.getWidth() / 2.0f);

        g.drawText("C", labelArea,
                   juce::Justification::centred);
        g.drawText("R", right,
                   juce::Justification::centredRight);
    }

    const float maximumDiameter =
        isInputOutput ? 102.0f
                      : (isPan ? 42.0f : 48.0f);

    const float diameter = juce::jmin(
        maximumDiameter,
        juce::jmin(area.getWidth(),
                   area.getHeight()) - 4.0f);

    auto knob = area.withSizeKeepingCentre(
        diameter, diameter);

    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillEllipse(knob.translated(2.5f, 3.0f));

    if (isInputOutput)
    {
        juce::ColourGradient knobGradient(
            juce::Colour(0xfff1e4c7),
            knob.getX(), knob.getY(),
            juce::Colour(0xff7a715c),
            knob.getRight(), knob.getBottom(), false);

        knobGradient.addColour(0.48, ivory);

        g.setGradientFill(knobGradient);
        g.fillEllipse(knob);

        g.setColour(juce::Colour(0xff4a4336));
        g.drawEllipse(knob, 2.0f);
    }
    else
    {
        juce::ColourGradient knobGradient(
            juce::Colour(0xff373b37),
            knob.getX(), knob.getY(),
            juce::Colour(0xff090a09),
            knob.getRight(), knob.getBottom(), false);

        g.setGradientFill(knobGradient);
        g.fillEllipse(knob);

        g.setColour(gold.withAlpha(0.92f));
        g.drawEllipse(knob, isPan ? 1.5f : 1.0f);
    }

    const float angle =
        startAngle + position * (endAngle - startAngle);

    const float radius = knob.getWidth() * 0.5f;
    juce::Path pointer;

    pointer.addRoundedRectangle(
        -juce::jmax(1.2f, radius * 0.055f),
        -radius * 0.78f,
        juce::jmax(2.4f, radius * 0.11f),
        radius * (isInputOutput ? 0.52f : 0.42f),
        1.4f);

    g.setColour(isInputOutput ? black : cream);
    g.fillPath(
        pointer,
        juce::AffineTransform::rotation(angle)
            .translated(knob.getCentreX(),
                        knob.getCentreY()));

    if (isPan)
    {
        const float centreAngle =
            startAngle + 0.5f * (endAngle - startAngle);

        const auto centre = knob.getCentre();
        const auto outer = centre.getPointOnCircumference(
            radius + 3.0f, centreAngle);
        const auto inner = centre.getPointOnCircumference(
            radius - 2.0f, centreAngle);

        g.setColour(goldBright);
        g.drawLine(inner.x, inner.y,
                   outer.x, outer.y, 1.8f);

        g.setColour(goldBright.withAlpha(0.72f));
        g.fillEllipse(knob.withSizeKeepingCentre(
            diameter * 0.17f,
            diameter * 0.17f));
    }
}

void RuFTraXChainerLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool isButtonDown,
    int buttonX, int buttonY,
    int buttonW, int buttonH,
    juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(
        0.0f, 0.0f,
        (float) width, (float) height).reduced(1.0f);

    juce::ColourGradient gradient(
        metalLight, 0.0f, 0.0f,
        black, 0.0f, (float) height, false);

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(gold.withAlpha(0.88f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float centreX =
        (float) buttonX + (float) buttonW * 0.5f;
    const float centreY =
        (float) buttonY + (float) buttonH * 0.5f;

    arrow.startNewSubPath(
        centreX - 4.5f,
        centreY - (isButtonDown ? 1.0f : 2.0f));
    arrow.lineTo(centreX, centreY + 3.0f);
    arrow.lineTo(
        centreX + 4.5f,
        centreY - (isButtonDown ? 1.0f : 2.0f));

    g.setColour(goldBright);
    g.strokePath(arrow,
                 juce::PathStrokeType(1.6f));
}

juce::Font RuFTraXChainerLookAndFeel::getComboBoxFont(
    juce::ComboBox& box)
{
    return juce::Font(
        juce::FontOptions(
            juce::jlimit(10.0f, 16.0f,
                         (float) box.getHeight() * 0.40f),
            juce::Font::bold));
}


// A self-contained, independently-timed level meter. It reuses drawMeter()
// for the housing (drawn in its own 102x382 logical space, matching the
// footprint the main paint() used to draw statically) and adds a live peak
// bar on top, mapped through the same non-linear dB breakpoints as the
// printed scale beside it so the two line up.
class LevelMeter final : public juce::Component,
                         private juce::Timer
{
public:
    explicit LevelMeter(std::function<float()> levelProviderToUse)
        : levelProvider(std::move(levelProviderToUse))
    {
        startTimerHz(30);
    }

    ~LevelMeter() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        const float scaleX = (float) getWidth() / 102.0f;
        const float scaleY = (float) getHeight() / 382.0f;

        g.addTransform(
            juce::AffineTransform::scale(scaleX, scaleY));

        const juce::Rectangle<float> bounds(0.0f, 0.0f, 102.0f, 382.0f);
        drawMeter(g, bounds);

        const auto meter = bounds.reduced(28.0f, 14.0f);

        static const std::array<float, 9> dbSteps
            { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -60.0f };

        const float levelDb =
            juce::Decibels::gainToDecibels(
                juce::jmax(0.0f, displayedLevel), -100.0f);

        float fraction = 1.0f;

        if (levelDb >= dbSteps.front())
        {
            fraction = 0.0f;
        }
        else if (levelDb <= dbSteps.back())
        {
            fraction = 1.0f;
        }
        else
        {
            for (size_t step = 0; step + 1 < dbSteps.size(); ++step)
            {
                if (levelDb <= dbSteps[step] && levelDb >= dbSteps[step + 1])
                {
                    const float span = dbSteps[step] - dbSteps[step + 1];
                    const float within = (dbSteps[step] - levelDb) / span;
                    fraction = ((float) step + within)
                             / (float) (dbSteps.size() - 1);
                    break;
                }
            }
        }

        const float barTop = meter.getY() + meter.getHeight() * fraction;

        const auto barArea = juce::Rectangle<float>(
            meter.getX() + 2.0f, barTop,
            meter.getWidth() - 4.0f, meter.getBottom() - barTop);

        if (barArea.getHeight() > 0.5f)
        {
            juce::ColourGradient barGradient(
                juce::Colour(0xffd2ad68), meter.getX(), meter.getBottom(),
                juce::Colour(0xffe0503c), meter.getX(), meter.getY(), false);
            barGradient.addColour(0.72, juce::Colour(0xffd2ad68));
            barGradient.addColour(0.80, juce::Colour(0xffe0b23c));

            g.setGradientFill(barGradient);
            g.fillRect(barArea);
        }
    }

private:
    void timerCallback() override
    {
        const float newLevel = levelProvider ? levelProvider() : 0.0f;

        // Fast rise, slow fall - the classic peak-meter "ballistic" look,
        // driven by our own timer rather than the editor's slower 5 Hz
        // status-polling timer.
        displayedLevel = juce::jmax(newLevel, displayedLevel * 0.85f);
        repaint();
    }

    std::function<float()> levelProvider;
    float displayedLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

class HostedPluginWindow final : public juce::DocumentWindow
{
public:
    HostedPluginWindow(
                       RuFTraXLogicChainerAudioProcessor::HostedPluginPtr
                           pluginToUse,
                       const juce::String& title)
        : juce::DocumentWindow(
              title,
              juce::Colour(0xff171a18),
              juce::DocumentWindow::allButtons),
          hostedPlugin(std::move(pluginToUse))
    {
        jassert(juce::MessageManager::getInstance()
                    ->isThisTheMessageThread());
        jassert(hostedPlugin != nullptr);

        setUsingNativeTitleBar(true);
        setResizable(true, true);

        juce::AudioProcessorEditor* editor =
            hostedPlugin->createEditorAndMakeActive();

        if (editor == nullptr)
            editor =
                new juce::GenericAudioProcessorEditor(
                    *hostedPlugin);

        setContentOwned(editor, true);

        centreWithSize(
            juce::jmax(420, editor->getWidth()),
            juce::jmax(300, editor->getHeight()));

        setVisible(true);
        toFront(true);
    }

    ~HostedPluginWindow() override
    {
        // Destroy the native AU editor while its processor is still alive.
        // Complex Cocoa editors (including FabFilter and SSL) rely on this
        // ordering when their NSView hierarchy is detached.
        clearContentComponent();
    }

    bool represents(
        const RuFTraXLogicChainerAudioProcessor::HostedPluginPtr&
            plugin) const noexcept
    {
        return hostedPlugin.get() == plugin.get();
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    RuFTraXLogicChainerAudioProcessor::HostedPluginPtr
        hostedPlugin;
};

SlotRow::SlotRow(
    RuFTraXLogicChainerAudioProcessor& processor,
    int slotIndex,
    InsertCallback insertCallback,
    SlotCallback editCallback,
    SlotCallback removeCallback,
    SlotCallback retryCallback,
    ReorderCallback reorderCallback)
    : ownerProcessor(processor),
      index(slotIndex),
      onInsert(std::move(insertCallback)),
      onEdit(std::move(editCallback)),
      onRemove(std::move(removeCallback)),
      onRetry(std::move(retryCallback)),
      onReorder(std::move(reorderCallback))
{
    insert.setComponentID("Primary");
    bypass.setClickingTogglesState(true);
    retry.setComponentID("Primary");
    retry.setTooltip(
        "Attempt to recover this effect without reloading it from disk. "
        "If it keeps failing, use REMOVE and re-insert it.");

    insert.onClick = [this]
    {
        if (onInsert)
            onInsert(index, &insert);
    };

    edit.onClick = [this]
    {
        if (onEdit)
            onEdit(index);
    };

    remove.onClick = [this]
    {
        if (onRemove)
            onRemove(index);
    };

    retry.onClick = [this]
    {
        if (onRetry)
            onRetry(index);
    };

    for (auto* button :
         { &insert, &bypass, &edit, &retry, &remove })
        addAndMakeVisible(*button);

    retry.setVisible(false);

    // The row background (outside the buttons and knobs) is a drag handle:
    // press and drag it to reorder the chain.
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);

    auto setupKnob =
        [this](juce::Slider& slider,
               const juce::String& componentID,
               int textWidth)
        {
            slider.setComponentID(componentID);
            slider.setSliderStyle(
                juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setRotaryParameters(
                juce::MathConstants<float>::pi * 1.2f,
                juce::MathConstants<float>::pi * 2.8f,
                true);
            slider.setTextBoxStyle(
                juce::Slider::TextBoxBelow,
                false, textWidth, 17);
            addAndMakeVisible(slider);
        };

    setupKnob(mix, "Small", 48);
    setupKnob(gain, "Small", 52);
    setupKnob(pan, "Pan", 60);

    mix.setRange(0.0, 1.0, 0.001);
    mix.setDoubleClickReturnValue(true, 1.0);
    mix.textFromValueFunction =
        [](double value)
        {
            return juce::String(
                       (int) std::round(
                           value * 100.0))
                 + "%";
        };

    gain.setRange(-24.0, 24.0, 0.1);
    gain.setDoubleClickReturnValue(true, 0.0);
    gain.textFromValueFunction =
        [](double value)
        {
            return juce::String(value, 1)
                 + " dB";
        };

    pan.setRange(-1.0, 1.0, 0.01);
    pan.setDoubleClickReturnValue(true, 0.0);
    pan.textFromValueFunction =
        [](double value)
        {
            if (std::abs(value) < 0.005)
                return juce::String("C 0");

            const auto percent =
                (int) std::round(
                    std::abs(value) * 100.0);

            return juce::String(
                       value < 0.0 ? "L " : "R ")
                 + juce::String(percent);
        };

    const auto prefix =
        "slot" + juce::String(index + 1);

    bypassAttachment =
        std::make_unique<ButtonAttachment>(
            ownerProcessor.apvts,
            prefix + "Bypass",
            bypass);

    mixAttachment =
        std::make_unique<SliderAttachment>(
            ownerProcessor.apvts,
            prefix + "Mix",
            mix);

    gainAttachment =
        std::make_unique<SliderAttachment>(
            ownerProcessor.apvts,
            prefix + "Gain",
            gain);

    panAttachment =
        std::make_unique<SliderAttachment>(
            ownerProcessor.apvts,
            prefix + "Pan",
            pan);

    refresh();
}

void SlotRow::mouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 6)
        return;

    if (auto* container =
            juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (!container->isDragAndDropActive())
            container->startDragging(index, this);
    }
}

bool SlotRow::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.isInt() || details.description.isDouble();
}

void SlotRow::itemDragEnter(const SourceDetails&)
{
    dragHighlighted = true;
    repaint();
}

void SlotRow::itemDragExit(const SourceDetails&)
{
    dragHighlighted = false;
    repaint();
}

void SlotRow::itemDropped(const SourceDetails& details)
{
    dragHighlighted = false;
    const int sourceIndex = (int) details.description;

    if (sourceIndex != index && onReorder)
        onReorder(sourceIndex, index);

    repaint();
}

void SlotRow::refresh()
{
    const bool enabled =
        ownerProcessor.isSlotEnabled(index);
    const bool faulted =
        ownerProcessor.isSlotFaulted(index);

    edit.setEnabled(enabled);
    remove.setEnabled(enabled);

    edit.setVisible(!faulted);
    retry.setVisible(faulted);

    wasFaulted = faulted;

    repaint();
}

void SlotRow::paint(juce::Graphics& g)
{
    const float scaleX =
        (float) getWidth() / 1000.0f;
    const float scaleY =
        (float) getHeight() / 70.0f;

    g.addTransform(
        juce::AffineTransform::scale(
            scaleX, scaleY));

    auto panel = juce::Rectangle<float>(
        1.0f, 1.0f, 998.0f, 68.0f);

    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.fillRoundedRectangle(
        panel.translated(1.5f, 2.0f), 4.0f);

    juce::ColourGradient gradient(
        metalLight, panel.getX(), panel.getY(),
        black, panel.getRight(), panel.getBottom(),
        false);

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(panel, 4.0f);

    g.setColour(gold.withAlpha(0.62f));
    g.drawRoundedRectangle(panel, 4.0f, 1.0f);

    const bool enabled =
        ownerProcessor.isSlotEnabled(index);
    const bool bypassed =
        ownerProcessor.isSlotBypassed(index);
    const bool allBypassed =
        ownerProcessor.isGlobalBypassed();
    const bool faulted =
        ownerProcessor.isSlotFaulted(index);

    juce::Colour ledColour = juce::Colour(0xff474b46);

    if (faulted)
        ledColour = faultRed;
    else if (enabled && !bypassed && !allBypassed)
        ledColour = goldBright;

    g.setColour(ledColour);
    g.fillEllipse(17.0f, 25.0f, 14.0f, 14.0f);

    g.setColour(cream);
    g.setFont(juce::Font(
        juce::FontOptions(19.0f, juce::Font::bold)));

    g.drawText(juce::String(index + 1),
               37, 0, 35, 70,
               juce::Justification::centred);

    g.setFont(juce::Font(
        juce::FontOptions(15.5f, juce::Font::bold)));
    g.drawText("SLOT " + juce::String(index + 1),
               76, 0, 92, 70,
               juce::Justification::centredLeft);

    juce::String statusText;
    juce::Colour statusColour = cream.withAlpha(0.84f);

    if (faulted)
    {
        statusText = "FAULT - click RETRY";
        statusColour = faultRed;
    }
    else
    {
        statusText = ownerProcessor.getSlotDisplayName(index);

        if (enabled)
        {
            const int cpuPercent = juce::roundToInt(
                ownerProcessor.getSlotCpuUsage(index) * 100.0f);
            statusText << "  |  " << cpuPercent << "% cpu";
        }
    }

    g.setColour(statusColour);
    g.setFont(juce::Font(
        juce::FontOptions(13.5f)));

    g.drawFittedText(
        statusText,
        165, 0, 205, 70,
        juce::Justification::centredLeft,
        1);

    g.setColour(gold.withAlpha(0.76f));
    g.setFont(juce::Font(
        juce::FontOptions(8.5f, juce::Font::bold)));

    g.drawText("MIX", 748, 2, 62, 12,
               juce::Justification::centred);
    g.drawText("GAIN", 815, 2, 62, 12,
               juce::Justification::centred);
    g.drawText("PAN", 890, 1, 94, 12,
               juce::Justification::centred);

    if (dragHighlighted)
    {
        g.setColour(goldBright);
        g.drawRoundedRectangle(panel.reduced(1.0f), 4.0f, 3.0f);
    }
}

void SlotRow::resized()
{
    const float scaleX =
        (float) getWidth() / 1000.0f;
    const float scaleY =
        (float) getHeight() / 70.0f;

    auto logicalBounds =
        [scaleX, scaleY](int x, int y,
                         int width, int height)
        {
            return juce::Rectangle<int>(
                juce::roundToInt(
                    (float) x * scaleX),
                juce::roundToInt(
                    (float) y * scaleY),
                juce::jmax(
                    1,
                    juce::roundToInt(
                        (float) width * scaleX)),
                juce::jmax(
                    1,
                    juce::roundToInt(
                        (float) height * scaleY)));
        };

    insert.setBounds(
        logicalBounds(376, 14, 102, 42));
    bypass.setBounds(
        logicalBounds(486, 14, 72, 42));
    edit.setBounds(
        logicalBounds(564, 14, 60, 42));
    retry.setBounds(
        logicalBounds(564, 14, 60, 42));
    remove.setBounds(
        logicalBounds(632, 14, 110, 42));

    mix.setBounds(
        logicalBounds(748, 10, 62, 58));
    gain.setBounds(
        logicalBounds(815, 10, 62, 58));
    pan.setBounds(
        logicalBounds(886, 8, 102, 61));
}

RuFTraXLogicChainerAudioProcessorEditor::
RuFTraXLogicChainerAudioProcessorEditor(
    RuFTraXLogicChainerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      ownerProcessor(p)
{
    setLookAndFeel(&chainerLookAndFeel);
    setResizable(false, false);

    title.setText(
        "RuFTraX PLUG-IN CHAINER",
        juce::dontSendNotification);
    title.setColour(
        juce::Label::textColourId,
        goldBright);

    subtitle.setText(
        "VERSION 6.0 CLASSIC CONSOLE UI  |  "
        "AU + VST3 + STANDALONE",
        juce::dontSendNotification);
    subtitle.setColour(
        juce::Label::textColourId,
        cream.withAlpha(0.86f));

    pluginStatus.setText(
        ownerProcessor.getAudioUnitScanStatus(),
        juce::dontSendNotification);
    pluginStatus.setColour(
        juce::Label::textColourId,
        goldBright.withAlpha(0.88f));

    chainStatus.setJustificationType(
        juce::Justification::centredRight);
    chainStatus.setColour(
        juce::Label::textColourId,
        cream.withAlpha(0.72f));

    scaleLabel.setText(
        "VIEW SIZE",
        juce::dontSendNotification);
    scaleLabel.setJustificationType(
        juce::Justification::centredRight);
    scaleLabel.setColour(
        juce::Label::textColourId,
        cream.withAlpha(0.86f));

    scaleSelector.addItem("Fit Screen", 1);
    scaleSelector.addItem("50%", 2);
    scaleSelector.addItem("67%", 3);
    scaleSelector.addItem("75%", 4);
    scaleSelector.addItem("100%", 5);

    save.setComponentID("Primary");
    load.setComponentID("Primary");
    testTone.setComponentID("Primary");

    save.setTooltip("Save the current chain as a named preset.");
    load.setTooltip("Load or delete a saved chain preset.");
    undo.setTooltip("Undo the last insert, remove or reorder.");
    redo.setTooltip("Redo the last undone chain edit.");

    testTone.setClickingTogglesState(true);
    testTone.setTooltip(
        "Injects a 440 Hz tone at -18 dBFS into the chain input, so you "
        "can audition the chain without external source audio.");

    bypassAll.setClickingTogglesState(true);

    for (auto* component :
         { static_cast<juce::Component*>(&title),
           static_cast<juce::Component*>(&subtitle),
           static_cast<juce::Component*>(&pluginStatus),
           static_cast<juce::Component*>(&chainStatus),
           static_cast<juce::Component*>(&scaleLabel),
           static_cast<juce::Component*>(&scaleSelector),
           static_cast<juce::Component*>(&save),
           static_cast<juce::Component*>(&load),
           static_cast<juce::Component*>(&undo),
           static_cast<juce::Component*>(&redo),
           static_cast<juce::Component*>(&bypassAll),
           static_cast<juce::Component*>(&testTone) })
        addAndMakeVisible(*component);

    auto setupInputOutput =
        [this](juce::Slider& slider)
        {
            slider.setComponentID("InputOutput");
            slider.setSliderStyle(
                juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setRotaryParameters(
                juce::MathConstants<float>::pi * 1.2f,
                juce::MathConstants<float>::pi * 2.8f,
                true);
            slider.setRange(-24.0, 24.0, 0.1);
            slider.setValue(0.0);
            slider.setDoubleClickReturnValue(true, 0.0);
            slider.setTextBoxStyle(
                juce::Slider::TextBoxBelow,
                false, 72, 20);
            slider.textFromValueFunction =
                [](double value)
                {
                    return juce::String(value, 1);
                };
            addAndMakeVisible(slider);
        };

    setupInputOutput(inputGain);
    setupInputOutput(outputGain);

    inputMeter = std::make_unique<LevelMeter>(
        [this] { return ownerProcessor.getInputLevel(); });
    outputMeter = std::make_unique<LevelMeter>(
        [this] { return ownerProcessor.getOutputLevel(); });

    addAndMakeVisible(*inputMeter);
    addAndMakeVisible(*outputMeter);

    for (int slot = 0; slot < slotCount; ++slot)
    {
        rows[(size_t) slot] =
            std::make_unique<SlotRow>(
                ownerProcessor,
                slot,
                [this](int slotIndex,
                       juce::Component* source)
                {
                    requestPluginMenu(
                        slotIndex,
                        source);
                },
                [this](int slotIndex)
                {
                    openPluginEditor(slotIndex);
                },
                [this](int slotIndex)
                {
                    removePlugin(slotIndex);
                },
                [this](int slotIndex)
                {
                    retrySlot(slotIndex);
                },
                [this](int fromIndex, int toIndex)
                {
                    reorderSlots(fromIndex, toIndex);
                });

        addAndMakeVisible(
            *rows[(size_t) slot]);
    }

    inputAttachment =
        std::make_unique<SliderAttachment>(
            ownerProcessor.apvts,
            "inputGain",
            inputGain);

    outputAttachment =
        std::make_unique<SliderAttachment>(
            ownerProcessor.apvts,
            "outputGain",
            outputGain);

    // This button writes directly to a processor-level atomic flag.  It does
    // not rely on a GUI attachment or host round-trip before audio bypasses.
    bypassAll.onClick = [this]
    {
        const bool shouldBypass =
            bypassAll.getToggleState();

        ownerProcessor.setGlobalBypassed(shouldBypass);
        pluginStatus.setText(
            shouldBypass
                ? "All processing bypassed - original input is passing."
                : "Audio Unit chain active.",
            juce::dontSendNotification);
    };

    testTone.onClick = [this]
    {
        ownerProcessor.setTestToneEnabled(testTone.getToggleState());
    };

    save.onClick = [this] { promptAndSaveChain(); };
    load.onClick = [this] { showLoadChainMenu(); };

    undo.onClick = [this]
    {
        ownerProcessor.undoChainEdit();
        for (auto& row : rows)
            if (row != nullptr) row->refresh();
        updateUndoRedoButtons();
    };

    redo.onClick = [this]
    {
        ownerProcessor.redoChainEdit();
        for (auto& row : rows)
            if (row != nullptr) row->refresh();
        updateUndoRedoButtons();
    };

    juce::PropertiesFile::Options options;
    options.applicationName =
        "RuFTraX Plug-In Chainer";
    options.filenameSuffix = "settings";
    options.folderName = "RuFTraX";
    options.osxLibrarySubFolder =
        "Application Support";
    options.storageFormat =
        juce::PropertiesFile::storeAsXML;

    settings =
        std::make_unique<juce::PropertiesFile>(
            options);

    currentScaleId = juce::jlimit(
        1, 5,
        settings->getIntValue(
            "logicChainerViewSize", 1));

    scaleSelector.setSelectedId(
        currentScaleId,
        juce::dontSendNotification);

    scaleSelector.onChange =
        [this]
        {
            applyScale(
                scaleSelector.getSelectedId(),
                true);
        };

    updateUndoRedoButtons();
    updateChainStatusReadout();

    applyScale(currentScaleId, false);
    startTimerHz(5);
}

RuFTraXLogicChainerAudioProcessorEditor::
~RuFTraXLogicChainerAudioProcessorEditor()
{
    stopTimer();

    for (int slot = 0; slot < slotCount; ++slot)
        closePluginEditor(slot);

    if (settings != nullptr)
        settings->saveIfNeeded();

    setLookAndFeel(nullptr);
}

float RuFTraXLogicChainerAudioProcessorEditor::
calculateFitScale() const
{
    const auto* display =
        juce::Desktop::getInstance()
            .getDisplays()
            .getPrimaryDisplay();

    if (display == nullptr)
        return 0.75f;

    const auto available =
        display->userBounds.reduced(50, 100);

    const float widthScale =
        (float) available.getWidth()
        / (float) designWidth;

    const float heightScale =
        (float) available.getHeight()
        / (float) designHeight;

    return juce::jlimit(
        0.40f, 1.0f,
        juce::jmin(widthScale, heightScale));
}

void RuFTraXLogicChainerAudioProcessorEditor::
timerCallback()
{
    bypassAll.setToggleState(
        ownerProcessor.isGlobalBypassed(),
        juce::dontSendNotification);

    testTone.setToggleState(
        ownerProcessor.isTestToneEnabled(),
        juce::dontSendNotification);

    pluginStatus.setText(
        ownerProcessor.getAudioUnitScanStatus(),
        juce::dontSendNotification);

    for (auto& row : rows)
    {
        if (row != nullptr)
            row->refresh();
    }

    updateUndoRedoButtons();
    updateChainStatusReadout();

    if (pendingMenuSlot >= 0
        && ownerProcessor.isAudioUnitScanFinished())
    {
        const int slot = pendingMenuSlot;
        pendingMenuSlot = -1;

        if (ownerProcessor.didAudioUnitScanFail())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Audio Unit Browser",
                ownerProcessor.getAudioUnitScanStatus());
        }
        else
        {
            showPluginMenu(slot);
        }
    }
}

void RuFTraXLogicChainerAudioProcessorEditor::
updateUndoRedoButtons()
{
    const bool canUndo = ownerProcessor.canUndoChainEdit();
    const bool canRedo = ownerProcessor.canRedoChainEdit();

    undo.setEnabled(canUndo);
    redo.setEnabled(canRedo);

    undo.setTooltip(
        canUndo
            ? ("Undo: " + ownerProcessor.getUndoDescription())
            : juce::String("Nothing to undo."));

    redo.setTooltip(
        canRedo
            ? ("Redo: " + ownerProcessor.getRedoDescription())
            : juce::String("Nothing to redo."));
}

void RuFTraXLogicChainerAudioProcessorEditor::
updateChainStatusReadout()
{
    int totalLatencySamples = 0;

    for (int slot = 0; slot < slotCount; ++slot)
        totalLatencySamples += ownerProcessor.getSlotLatencySamples(slot);

    juce::String text;
    text << "LATENCY " << totalLatencySamples << " SMP   |   SIDECHAIN "
         << (ownerProcessor.isSidechainConnected() ? "CONNECTED" : "OFF");

    chainStatus.setText(text, juce::dontSendNotification);
}

void RuFTraXLogicChainerAudioProcessorEditor::
requestPluginMenu(
    int slotIndex,
    juce::Component*)
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return;

    ownerProcessor.startAudioUnitScan();

    if (!ownerProcessor.isAudioUnitScanFinished())
    {
        pendingMenuSlot = slotIndex;

        pluginStatus.setText(
            "Reading the macOS Audio Unit registry. "
            "The Insert menu will open automatically...",
            juce::dontSendNotification);

        return;
    }

    if (ownerProcessor.didAudioUnitScanFail())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Audio Unit Browser",
            ownerProcessor.getAudioUnitScanStatus());
        return;
    }

    showPluginMenu(slotIndex);
}

void RuFTraXLogicChainerAudioProcessorEditor::
showPluginMenu(int slotIndex)
{
    currentMenuPlugins =
        ownerProcessor.getAvailableAudioUnits();

    if (currentMenuPlugins.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Audio Unit Browser",
            "No compatible Audio Unit effects were found.");
        return;
    }

    juce::PopupMenu menu;

    juce::KnownPluginList::addToMenu(
        menu,
        currentMenuPlugins,
        juce::KnownPluginList::sortByManufacturer);

    const auto options =
        juce::PopupMenu::Options()
            .withTargetComponent(
                rows[(size_t) slotIndex].get());

    const auto safeThis =
        juce::Component::SafePointer<
            RuFTraXLogicChainerAudioProcessorEditor>(
                this);

    menu.showMenuAsync(
        options,
        [safeThis, slotIndex](int result)
        {
            if (safeThis == nullptr
                || result == 0)
                return;

            const int pluginIndex =
                juce::KnownPluginList::
                    getIndexChosenByMenu(
                        safeThis->currentMenuPlugins,
                        result);

            if (!juce::isPositiveAndBelow(
                    pluginIndex,
                    safeThis->currentMenuPlugins.size()))
                return;

            const auto description =
                safeThis->currentMenuPlugins[
                    pluginIndex];

            safeThis->closePluginEditor(
                slotIndex);

            safeThis->pluginStatus.setText(
                "Loading "
                    + description.manufacturerName
                    + " - "
                    + description.name
                    + "...",
                juce::dontSendNotification);

            safeThis->ownerProcessor
                .loadAudioUnitInSlot(
                    slotIndex,
                    description,
                    [safeThis, slotIndex]
                    (bool success,
                     const juce::String& message)
                    {
                        if (safeThis == nullptr)
                            return;

                        safeThis->pluginStatus.setText(
                            message,
                            juce::dontSendNotification);

                        if (!success)
                        {
                            juce::AlertWindow::
                                showMessageBoxAsync(
                                    juce::MessageBoxIconType::
                                        WarningIcon,
                                    "Audio Unit Load Failed",
                                    message);
                        }

                        if (safeThis->rows[
                                (size_t) slotIndex]
                            != nullptr)
                        {
                            safeThis->rows[
                                (size_t) slotIndex]
                                ->refresh();
                        }

                        safeThis->updateUndoRedoButtons();
                    });
        });
}

void RuFTraXLogicChainerAudioProcessorEditor::
openPluginEditor(int slotIndex)
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return;

    auto plugin =
        ownerProcessor.getHostedPluginHandle(
            slotIndex);

    auto& pluginWindow =
        pluginWindows[(size_t) slotIndex];

    if (pluginWindow != nullptr
        && pluginWindow->represents(plugin))
    {
        pluginWindow->setVisible(true);

        pluginWindow->toFront(true);
        return;
    }

    // A preset restore may have replaced this slot while an old editor was
    // hidden.  Drop that window before opening the current plug-in.
    pluginWindow.reset();

    if (plugin == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Hosted Plug-in Editor",
            "Load an Audio Unit into this slot first.");
        return;
    }

    pluginWindow =
        std::make_unique<HostedPluginWindow>(
            std::move(plugin),
            "Slot "
                + juce::String(slotIndex + 1)
                + " - "
                + ownerProcessor.getSlotDisplayName(
                    slotIndex));
}

void RuFTraXLogicChainerAudioProcessorEditor::
closePluginEditor(int slotIndex)
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return;

    pluginWindows[(size_t) slotIndex].reset();
}

void RuFTraXLogicChainerAudioProcessorEditor::
removePlugin(int slotIndex)
{
    closePluginEditor(slotIndex);
    ownerProcessor.clearSlot(slotIndex);

    pluginStatus.setText(
        "Slot "
            + juce::String(slotIndex + 1)
            + " cleared.",
        juce::dontSendNotification);

    if (rows[(size_t) slotIndex] != nullptr)
        rows[(size_t) slotIndex]->refresh();

    updateUndoRedoButtons();
}

void RuFTraXLogicChainerAudioProcessorEditor::
retrySlot(int slotIndex)
{
    ownerProcessor.retrySlot(slotIndex);

    if (rows[(size_t) slotIndex] != nullptr)
        rows[(size_t) slotIndex]->refresh();

    pluginStatus.setText(
        ownerProcessor.isSlotFaulted(slotIndex)
            ? ("Slot " + juce::String(slotIndex + 1)
                   + " could not be recovered. Try REMOVE and re-insert it.")
            : ("Slot " + juce::String(slotIndex + 1) + " recovered."),
        juce::dontSendNotification);
}

void RuFTraXLogicChainerAudioProcessorEditor::
reorderSlots(int fromIndex, int toIndex)
{
    // Open hosted-plug-in editor windows are keyed by slot index, which a
    // reorder invalidates; close them rather than risk a window pointing at
    // the wrong plug-in.
    for (int slot = 0; slot < slotCount; ++slot)
        closePluginEditor(slot);

    ownerProcessor.moveSlot(fromIndex, toIndex);

    for (auto& row : rows)
        if (row != nullptr) row->refresh();

    updateUndoRedoButtons();

    pluginStatus.setText(
        "Moved Slot " + juce::String(fromIndex + 1)
            + " to Slot " + juce::String(toIndex + 1) + ".",
        juce::dontSendNotification);
}

void RuFTraXLogicChainerAudioProcessorEditor::
promptAndSaveChain()
{
    saveChainPrompt = std::make_unique<juce::AlertWindow>(
        "Save Chain",
        "Name this chain preset:",
        juce::MessageBoxIconType::NoIcon);

    saveChainPrompt->addTextEditor("name", "My Chain");
    saveChainPrompt->addButton(
        "Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    saveChainPrompt->addButton(
        "Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    const auto safeThis =
        juce::Component::SafePointer<
            RuFTraXLogicChainerAudioProcessorEditor>(this);

    saveChainPrompt->enterModalState(
        true,
        juce::ModalCallbackFunction::create(
            [safeThis](int result)
            {
                if (safeThis == nullptr)
                    return;

                if (result == 1)
                {
                    const auto name =
                        safeThis->saveChainPrompt
                            ->getTextEditorContents("name");

                    const bool ok =
                        safeThis->ownerProcessor.saveChainPreset(name);

                    safeThis->pluginStatus.setText(
                        ok
                            ? ("Chain \"" + name + "\" saved.")
                            : juce::String(
                                  "Could not save the chain - "
                                  "check the name and try again."),
                        juce::dontSendNotification);
                }

                safeThis->saveChainPrompt.reset();
            }),
        false);
}

void RuFTraXLogicChainerAudioProcessorEditor::
showLoadChainMenu()
{
    const auto names = ownerProcessor.getSavedChainPresetNames();
    const auto factoryNames = ownerProcessor.getFactoryChainNames();

    juce::PopupMenu menu;

    if (!factoryNames.isEmpty())
    {
        juce::PopupMenu factoryMenu;
        int factoryId = 2000;

        for (const auto& name : factoryNames)
            factoryMenu.addItem(factoryId++, name);

        menu.addSubMenu("Factory Chains (built-in)", factoryMenu);
        menu.addSeparator();
    }

    if (names.isEmpty())
    {
        menu.addItem(1, "No saved chains yet - use SAVE first", false);
    }
    else
    {
        int itemId = 1;

        for (const auto& name : names)
            menu.addItem(itemId++, name);

        juce::PopupMenu deleteMenu;
        itemId = 1000;

        for (const auto& name : names)
            deleteMenu.addItem(itemId++, name);

        menu.addSeparator();
        menu.addSubMenu("Delete a saved chain...", deleteMenu);
    }

    const auto safeThis =
        juce::Component::SafePointer<
            RuFTraXLogicChainerAudioProcessorEditor>(this);

    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(&load),
        [safeThis, names, factoryNames](int result)
        {
            if (safeThis == nullptr || result == 0)
                return;

            if (result >= 2000)
            {
                const int factoryIndex = result - 2000;

                if (!juce::isPositiveAndBelow(factoryIndex, factoryNames.size()))
                    return;

                safeThis->confirmAndLoadFactoryChain(factoryIndex);
                return;
            }

            if (result >= 1000)
            {
                const int deleteIndex = result - 1000;

                if (!juce::isPositiveAndBelow(deleteIndex, names.size()))
                    return;

                safeThis->ownerProcessor.deleteChainPreset(
                    names[deleteIndex]);

                safeThis->pluginStatus.setText(
                    "Deleted chain \"" + names[deleteIndex] + "\".",
                    juce::dontSendNotification);

                return;
            }

            const int loadIndex = result - 1;

            if (!juce::isPositiveAndBelow(loadIndex, names.size()))
                return;

            for (int slot = 0; slot < slotCount; ++slot)
                safeThis->closePluginEditor(slot);

            const auto file =
                safeThis->ownerProcessor.getChainPresetsDirectory()
                    .getChildFile(names[loadIndex] + ".rfxchain");

            safeThis->ownerProcessor.loadChainPreset(file);

            safeThis->pluginStatus.setText(
                "Loading chain \"" + names[loadIndex] + "\"...",
                juce::dontSendNotification);

            for (auto& row : safeThis->rows)
                if (row != nullptr) row->refresh();

            safeThis->updateUndoRedoButtons();
        });
}

void RuFTraXLogicChainerAudioProcessorEditor::
confirmAndLoadFactoryChain(int chainIndex)
{
    const auto factoryNames = ownerProcessor.getFactoryChainNames();

    if (!juce::isPositiveAndBelow(chainIndex, factoryNames.size()))
        return;

    const auto chainName = factoryNames[chainIndex];

    factoryChainPrompt = std::make_unique<juce::AlertWindow>(
        "Load Factory Chain",
        "Load the built-in \"" + chainName
            + "\" chain? This replaces all 8 slots.",
        juce::MessageBoxIconType::WarningIcon);

    factoryChainPrompt->addButton(
        "Load", 1, juce::KeyPress(juce::KeyPress::returnKey));
    factoryChainPrompt->addButton(
        "Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    const auto safeThis =
        juce::Component::SafePointer<
            RuFTraXLogicChainerAudioProcessorEditor>(this);

    factoryChainPrompt->enterModalState(
        true,
        juce::ModalCallbackFunction::create(
            [safeThis, chainIndex, chainName](int result)
            {
                if (safeThis == nullptr)
                    return;

                if (result == 1)
                {
                    for (int slot = 0; slot < slotCount; ++slot)
                        safeThis->closePluginEditor(slot);

                    safeThis->pluginStatus.setText(
                        "Loading factory chain \"" + chainName + "\"...",
                        juce::dontSendNotification);

                    safeThis->ownerProcessor.loadFactoryChain(
                        chainIndex,
                        [safeThis, chainName]
                        {
                            if (safeThis == nullptr)
                                return;

                            safeThis->pluginStatus.setText(
                                "Factory chain \"" + chainName + "\" loaded.",
                                juce::dontSendNotification);

                            for (auto& row : safeThis->rows)
                                if (row != nullptr) row->refresh();

                            safeThis->updateUndoRedoButtons();
                        });
                }

                safeThis->factoryChainPrompt.reset();
            }),
        false);
}

void RuFTraXLogicChainerAudioProcessorEditor::
applyScale(int selectedId,
           bool saveSelection)
{
    currentScaleId =
        juce::jlimit(1, 5, selectedId);

    switch (currentScaleId)
    {
        case 1:
            uiScale = calculateFitScale();
            break;
        case 2:
            uiScale = 0.50f;
            break;
        case 3:
            uiScale = 0.67f;
            break;
        case 4:
            uiScale = 0.75f;
            break;
        case 5:
            uiScale = 1.00f;
            break;
        default:
            uiScale = 0.75f;
            break;
    }

    updateScaledFonts();

    setSize(
        juce::roundToInt(
            (float) designWidth * uiScale),
        juce::roundToInt(
            (float) designHeight * uiScale));

    if (saveSelection
        && settings != nullptr)
    {
        settings->setValue(
            "logicChainerViewSize",
            currentScaleId);
        settings->saveIfNeeded();
    }

    resized();
    repaint();
}

juce::Rectangle<int>
RuFTraXLogicChainerAudioProcessorEditor::
scaledBounds(int x, int y,
             int width, int height) const
{
    return {
        juce::roundToInt((float) x * uiScale),
        juce::roundToInt((float) y * uiScale),
        juce::jmax(
            1,
            juce::roundToInt(
                (float) width * uiScale)),
        juce::jmax(
            1,
            juce::roundToInt(
                (float) height * uiScale))
    };
}

void RuFTraXLogicChainerAudioProcessorEditor::
updateScaledFonts()
{
    title.setFont(
        juce::Font(
            juce::FontOptions(
                34.0f * uiScale,
                juce::Font::bold)));

    subtitle.setFont(
        juce::Font(
            juce::FontOptions(
                13.0f * uiScale,
                juce::Font::bold)));

    pluginStatus.setFont(
        juce::Font(
            juce::FontOptions(
                10.0f * uiScale,
                juce::Font::bold)));

    chainStatus.setFont(
        juce::Font(
            juce::FontOptions(
                9.5f * uiScale,
                juce::Font::bold)));

    scaleLabel.setFont(
        juce::Font(
            juce::FontOptions(
                10.0f * uiScale,
                juce::Font::bold)));

    inputGain.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        juce::jmax(
            38,
            juce::roundToInt(
                72.0f * uiScale)),
        juce::jmax(
            12,
            juce::roundToInt(
                20.0f * uiScale)));

    outputGain.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        juce::jmax(
            38,
            juce::roundToInt(
                72.0f * uiScale)),
        juce::jmax(
            12,
            juce::roundToInt(
                20.0f * uiScale)));
}

void RuFTraXLogicChainerAudioProcessorEditor::
paint(juce::Graphics& g)
{
    g.fillAll(black);
    g.addTransform(
        juce::AffineTransform::scale(uiScale));

    drawWoodPanel(
        g,
        { 0.0f, 0.0f, 52.0f,
          (float) designHeight },
        0x1970);

    drawWoodPanel(
        g,
        { (float) designWidth - 52.0f,
          0.0f, 52.0f,
          (float) designHeight },
        0x1971);

    auto chassis = juce::Rectangle<float>(
        48.0f, 8.0f,
        (float) designWidth - 96.0f,
        (float) designHeight - 16.0f);

    juce::ColourGradient chassisGradient(
        metalLight,
        chassis.getX(), chassis.getY(),
        black,
        chassis.getRight(),
        chassis.getBottom(),
        false);

    g.setGradientFill(chassisGradient);
    g.fillRoundedRectangle(chassis, 8.0f);

    g.setColour(gold.withAlpha(0.80f));
    g.drawRoundedRectangle(
        chassis.reduced(5.0f),
        7.0f, 1.4f);

    juce::Random metalTexture(0x5150);

    for (int line = 0; line < 170; ++line)
    {
        const float y =
            chassis.getY()
            + metalTexture.nextFloat()
              * chassis.getHeight();

        g.setColour(
            (line % 4 == 0 ? cream
                           : juce::Colours::black)
                .withAlpha(
                    0.012f
                    + metalTexture.nextFloat()
                      * 0.030f));

        g.drawLine(
            chassis.getX() + 8.0f,
            y,
            chassis.getRight() - 8.0f,
            y,
            0.6f);
    }

    auto headerPanel =
        juce::Rectangle<float>(
            66.0f, 22.0f,
            1268.0f, 104.0f);

    g.setColour(
        juce::Colours::black.withAlpha(0.42f));
    g.fillRoundedRectangle(
        headerPanel.translated(2.0f, 3.0f),
        5.0f);

    g.setColour(metal);
    g.fillRoundedRectangle(
        headerPanel, 5.0f);

    g.setColour(gold.withAlpha(0.78f));
    g.drawRoundedRectangle(
        headerPanel, 5.0f, 1.0f);

    auto inputPanel =
        juce::Rectangle<float>(
            67.0f, 145.0f,
            136.0f, 614.0f);

    auto outputPanel =
        juce::Rectangle<float>(
            1197.0f, 145.0f,
            136.0f, 614.0f);

    for (const auto panel :
         { inputPanel, outputPanel })
    {
        g.setColour(
            juce::Colours::black
                .withAlpha(0.58f));
        g.fillRoundedRectangle(
            panel.translated(2.0f, 3.0f),
            5.0f);

        g.setColour(metal);
        g.fillRoundedRectangle(
            panel, 5.0f);

        g.setColour(gold.withAlpha(0.72f));
        g.drawRoundedRectangle(
            panel, 5.0f, 1.0f);
    }

    g.setColour(cream);
    g.setFont(juce::Font(
        juce::FontOptions(
            14.0f,
            juce::Font::bold)));

    g.drawText(
        "INPUT",
        67, 163, 136, 24,
        juce::Justification::centred);

    g.drawText(
        "OUTPUT",
        1197, 163, 136, 24,
        juce::Justification::centred);

    g.setColour(cream.withAlpha(0.68f));
    g.setFont(juce::Font(
        juce::FontOptions(
            9.0f,
            juce::Font::bold)));

    const std::array<const char*, 9>
        meterLabels
        {
            "+12", "+6", "0", "-6",
            "-12", "-18", "-24",
            "-36", "-60"
        };

    for (int label = 0;
         label < (int) meterLabels.size();
         ++label)
    {
        const int y =
            354 + label * 43;

        g.drawText(
            meterLabels[(size_t) label],
            72, y, 30, 14,
            juce::Justification::centredRight);

        g.drawText(
            meterLabels[(size_t) label],
            1297, y, 30, 14,
            juce::Justification::centredLeft);
    }

    auto footer =
        juce::Rectangle<float>(
            66.0f, 785.0f,
            1268.0f, 79.0f);

    g.setColour(
        juce::Colours::black.withAlpha(0.50f));
    g.fillRoundedRectangle(
        footer.translated(2.0f, 3.0f),
        5.0f);

    g.setColour(metal);
    g.fillRoundedRectangle(footer, 5.0f);

    g.setColour(gold.withAlpha(0.72f));
    g.drawRoundedRectangle(
        footer, 5.0f, 1.0f);

    g.setColour(goldBright);
    g.setFont(juce::Font(
        juce::FontOptions(
            24.0f,
            juce::Font::bold)));

    g.drawText(
        "RuFTraX Plug-In Chainer",
        footer.toNearestInt(),
        juce::Justification::centred);

    g.setColour(gold.withAlpha(0.70f));
    g.drawLine(
        360.0f, 825.0f,
        532.0f, 825.0f, 1.0f);
    g.drawLine(
        868.0f, 825.0f,
        1040.0f, 825.0f, 1.0f);

    for (const auto point :
         { juce::Point<float>(72.0f, 28.0f),
           juce::Point<float>(1328.0f, 28.0f),
           juce::Point<float>(72.0f, 858.0f),
           juce::Point<float>(1328.0f, 858.0f),
           juce::Point<float>(75.0f, 153.0f),
           juce::Point<float>(195.0f, 153.0f),
           juce::Point<float>(1205.0f, 153.0f),
           juce::Point<float>(1325.0f, 153.0f) })
    {
        drawScrew(
            g, point.x, point.y, 6.0f);
    }
}

void RuFTraXLogicChainerAudioProcessorEditor::
resized()
{
    title.setBounds(
        scaledBounds(
            94, 36, 590, 44));

    subtitle.setBounds(
        scaledBounds(
            96, 74, 570, 22));

    pluginStatus.setBounds(
        scaledBounds(
            96, 96, 620, 18));

    chainStatus.setBounds(
        scaledBounds(
            722, 94, 350, 20));

    save.setBounds(
        scaledBounds(
            780, 43, 78, 42));

    load.setBounds(
        scaledBounds(
            864, 43, 78, 42));

    undo.setBounds(
        scaledBounds(
            948, 43, 78, 42));

    redo.setBounds(
        scaledBounds(
            1032, 43, 78, 42));

    bypassAll.setBounds(
        scaledBounds(
            1116, 43, 100, 42));

    testTone.setBounds(
        scaledBounds(
            1222, 43, 98, 42));

    scaleLabel.setBounds(
        scaledBounds(
            1080, 91, 90, 23));

    scaleSelector.setBounds(
        scaledBounds(
            1180, 89, 142, 28));

    inputGain.setBounds(
        scaledBounds(
            80, 192, 110, 125));

    outputGain.setBounds(
        scaledBounds(
            1210, 192, 110, 125));

    if (inputMeter != nullptr)
        inputMeter->setBounds(scaledBounds(84, 340, 102, 382));

    if (outputMeter != nullptr)
        outputMeter->setBounds(scaledBounds(1214, 340, 102, 382));

    constexpr int rowX = 215;
    constexpr int rowY = 145;
    constexpr int rowWidth = 970;
    constexpr int rowHeight = 70;
    constexpr int rowStep = 76;

    for (int row = 0; row < slotCount; ++row)
    {
        if (rows[(size_t) row] != nullptr)
        {
            rows[(size_t) row]->setBounds(
                scaledBounds(
                    rowX,
                    rowY + row * rowStep,
                    rowWidth,
                    rowHeight));
        }
    }
}
