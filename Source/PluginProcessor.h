#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Host/HostEngine.h"
#include "Hosting/PluginHost.h"

struct ChainSlot
{
    bool enabled = false;
    bool bypassed = false;
    float mix = 1.0f;
    float gainDb = 0.0f;
    float pan = 0.0f;
    juce::String displayName = "<Empty Slot>";
};

class RuFTraXLogicChainerAudioProcessor final : public juce::AudioProcessor
{
public:
    RuFTraXLogicChainerAudioProcessor();
    ~RuFTraXLogicChainerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RuFTraX Logic Chainer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::array<ChainSlot, 8> slots;
    PluginHost pluginHost;

    void loadPlaceholderPlugin (int index, const juce::String& name);
    void clearSlot (int index);
    juce::String getSlotName(int index) const
    {
        return pluginHost.getSlot(index).getDisplayName();
    }

    bool isSlotActive(int index) const
    {
        return pluginHost.getSlot(index).isEnabled();
    }

    float getInputLevel() const  { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getOutputLevelL() const { return outputLevelL.load(); }
    float getOutputLevelR() const { return outputLevelR.load(); }
    float getSlot1Level() const  { return slot1Level.load(); }
    void setTestToneEnabled(bool shouldBeEnabled) { testToneEnabled.store(shouldBeEnabled); }

    void saveTemporaryPreset();
    void loadTemporaryPreset();



private:
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> outputLevelL { 0.0f };
    std::atomic<float> outputLevelR { 0.0f };
    std::atomic<float> slot1Level { 0.0f };
    std::atomic<bool> testToneEnabled { false };
    double testTonePhase { 0.0 };
    double currentSampleRate { 44100.0 };
    juce::ValueTree temporaryPreset;
    HostEngine hostEngine;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RuFTraXLogicChainerAudioProcessor)
};
