#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <functional>
#include <memory>

class RuFTraXChainerLookAndFeel final
    : public juce::LookAndFeel_V4
{
public:
    RuFTraXChainerLookAndFeel();

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour&, bool highlighted,
                              bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool highlighted, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&,
                                 int buttonHeight) override;

    void drawRotarySlider(juce::Graphics&, int x, int y,
                          int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider&) override;

    void drawComboBox(juce::Graphics&, int width, int height,
                      bool isButtonDown,
                      int buttonX, int buttonY,
                      int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
};

class SlotRow final : public juce::Component,
                      public juce::DragAndDropTarget
{
public:
    using InsertCallback =
        std::function<void(int, juce::Component*)>;
    using SlotCallback =
        std::function<void(int)>;
    using ReorderCallback =
        std::function<void(int fromIndex, int toIndex)>;

    SlotRow(RuFTraXLogicChainerAudioProcessor&,
            int slotIndex,
            InsertCallback insertCallback,
            SlotCallback editCallback,
            SlotCallback removeCallback,
            SlotCallback retryCallback,
            ReorderCallback reorderCallback);

    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();

    void mouseDrag(const juce::MouseEvent&) override;

    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDragEnter(const SourceDetails&) override;
    void itemDragExit(const SourceDetails&) override;
    void itemDropped(const SourceDetails&) override;

private:
    RuFTraXLogicChainerAudioProcessor& ownerProcessor;
    int index = 0;

    InsertCallback onInsert;
    SlotCallback onEdit;
    SlotCallback onRemove;
    SlotCallback onRetry;
    ReorderCallback onReorder;

    bool dragHighlighted = false;
    bool wasFaulted = false;

    juce::TextButton insert { "+ INSERT" };
    juce::TextButton bypass { "BYPASS" };
    juce::TextButton edit { "EDIT" };
    juce::TextButton retry { "RETRY" };
    juce::TextButton remove { "REMOVE" };

    juce::Slider mix;
    juce::Slider gain;
    juce::Slider pan;

    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> panAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotRow)
};

class HostedPluginWindow;
class LevelMeter;

class RuFTraXLogicChainerAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      public juce::DragAndDropContainer,
      private juce::Timer
{
public:
    explicit RuFTraXLogicChainerAudioProcessorEditor(
        RuFTraXLogicChainerAudioProcessor&);
    ~RuFTraXLogicChainerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int designWidth = 1400;
    static constexpr int designHeight = 900;
    static constexpr int slotCount =
        RuFTraXLogicChainerAudioProcessor::slotCount;

    void timerCallback() override;

    void requestPluginMenu(int slotIndex,
                           juce::Component* sourceComponent);
    void showPluginMenu(int slotIndex);
    void openPluginEditor(int slotIndex);
    void closePluginEditor(int slotIndex);
    void removePlugin(int slotIndex);
    void retrySlot(int slotIndex);
    void reorderSlots(int fromIndex, int toIndex);

    void promptAndSaveChain();
    void showLoadChainMenu();
    void confirmAndLoadFactoryChain(int chainIndex);
    void updateUndoRedoButtons();
    void updateChainStatusReadout();

    void applyScale(int selectedId, bool saveSelection);
    float calculateFitScale() const;
    juce::Rectangle<int> scaledBounds(int x, int y,
                                      int width, int height) const;
    void updateScaledFonts();

    RuFTraXLogicChainerAudioProcessor& ownerProcessor;
    RuFTraXChainerLookAndFeel chainerLookAndFeel;

    juce::Label title;
    juce::Label subtitle;
    juce::Label pluginStatus;
    juce::Label chainStatus;
    juce::Label scaleLabel;
    juce::ComboBox scaleSelector;

    juce::Slider inputGain;
    juce::Slider outputGain;

    juce::TextButton save { "SAVE" };
    juce::TextButton load { "LOAD" };
    juce::TextButton undo { "UNDO" };
    juce::TextButton redo { "REDO" };
    juce::TextButton bypassAll { "BYPASS ALL" };
    juce::TextButton testTone { "TEST TONE" };

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<SliderAttachment> inputAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    std::array<std::unique_ptr<SlotRow>, slotCount> rows;
    std::array<std::unique_ptr<HostedPluginWindow>, slotCount>
        pluginWindows;

    std::unique_ptr<LevelMeter> inputMeter;
    std::unique_ptr<LevelMeter> outputMeter;

    juce::Array<juce::PluginDescription> currentMenuPlugins;
    int pendingMenuSlot = -1;

    std::unique_ptr<juce::AlertWindow> saveChainPrompt;
    std::unique_ptr<juce::AlertWindow> factoryChainPrompt;

    std::unique_ptr<juce::PropertiesFile> settings;
    float uiScale = 1.0f;
    int currentScaleId = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        RuFTraXLogicChainerAudioProcessorEditor)
};
