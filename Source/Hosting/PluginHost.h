#pragma once
#include <array>
#include "PluginInstance.h"

class PluginHost
{
public:
    static constexpr int numSlots = 8;

    PluginInstance& getSlot(int index);
    const PluginInstance& getSlot(int index) const;

    void setSlotName(int index, const juce::String& name);
    void clearSlot(int index);

    void prepare(double sampleRate, int blockSize);
    void processSlot(int index, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    bool slotHasHostedPlugin(int index) const;

private:
    std::array<PluginInstance, numSlots> slots;
};
