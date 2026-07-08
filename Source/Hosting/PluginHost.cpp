#include "PluginHost.h"

PluginInstance& PluginHost::getSlot(int index)
{
    return slots[(size_t) juce::jlimit(0, numSlots - 1, index)];
}

const PluginInstance& PluginHost::getSlot(int index) const
{
    return slots[(size_t) juce::jlimit(0, numSlots - 1, index)];
}

void PluginHost::setSlotName(int index, const juce::String& name)
{
    auto& slot = getSlot(index);
    slot.setDisplayName(name);
    slot.setEnabled(true);
}

void PluginHost::clearSlot(int index)
{
    getSlot(index).clear();
}

void PluginHost::prepare(double sampleRate, int blockSize)
{
    for (auto& slot : slots)
        slot.prepare(sampleRate, blockSize);
}

void PluginHost::processSlot(int index, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    getSlot(index).process(buffer, midi);
}

bool PluginHost::slotHasHostedPlugin(int index) const
{
    return getSlot(index).isProcessingHostedPlugin();
}


