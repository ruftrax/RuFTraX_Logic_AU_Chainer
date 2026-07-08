#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Host/HostEngine.h"

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

    void loadPlaceholderPlugin (int index, const juce::String& name);
    void clearSlot (int index);

    float getInputLevel() const  { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

private:
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    HostEngine hostEngine;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RuFTraXLogicChainerAudioProcessor)
};
