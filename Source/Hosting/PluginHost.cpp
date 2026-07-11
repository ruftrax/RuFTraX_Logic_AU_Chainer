#include "PluginHost.h"

PluginHost::PluginHost()
{
    formatManager.addFormat(
        std::make_unique<juce::AudioUnitPluginFormat>());
}

bool PluginHost::isValidSlot(int index) const
{
    return juce::isPositiveAndBelow(index, numSlots);
}

PluginInstance& PluginHost::getSlot(int index)
{
    jassert(isValidSlot(index));
    return slots[(size_t) juce::jlimit(0, numSlots - 1, index)];
}

const PluginInstance& PluginHost::getSlot(int index) const
{
    jassert(isValidSlot(index));
    return slots[(size_t) juce::jlimit(0, numSlots - 1, index)];
}

void PluginHost::setSlotName(int index, const juce::String& name)
{
    if (! isValidSlot(index))
        return;

    auto& slot = getSlot(index);
    slot.setDisplayName(name);
    slot.setEnabled(true);
}

void PluginHost::clearSlot(int index)
{
    if (isValidSlot(index))
        getSlot(index).clear();
}

void PluginHost::prepare(double sampleRate, int blockSize)
{
    for (auto& slot : slots)
        slot.prepare(sampleRate, blockSize);
}

void PluginHost::processSlot(int index,
                             juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midi)
{
    if (isValidSlot(index))
        getSlot(index).process(buffer, midi);
}

bool PluginHost::slotHasHostedPlugin(int index) const
{
    return isValidSlot(index)
        && getSlot(index).isProcessingHostedPlugin();
}

bool PluginHost::loadAudioUnit(int index,
                               const AUPluginInfo& info,
                               double sampleRate,
                               int blockSize,
                               juce::String& errorMessage)
{
    if (! isValidSlot(index))
    {
        errorMessage = "Invalid insert slot.";
        return false;
    }

    if (! info.hasValidDescription)
    {
        errorMessage = "Audio Unit description is invalid.";
        return false;
    }

    auto instance = formatManager.createPluginInstance(
        info.description,
        sampleRate,
        blockSize,
        errorMessage);

    if (instance == nullptr)
    {
        if (errorMessage.isEmpty())
            errorMessage = "JUCE could not create the Audio Unit.";

        return false;
    }

    instance->setPlayConfigDetails(2, 2, sampleRate, blockSize);
    instance->prepareToPlay(sampleRate, blockSize);

    auto& slot = getSlot(index);
    slot.setHostedPlugin(std::move(instance));
    slot.setDisplayName(info.name);

    return true;
}

bool PluginHost::openEditor(int index, juce::String& errorMessage)
{
    if (! isValidSlot(index))
    {
        errorMessage = "Invalid insert slot.";
        return false;
    }

    return getSlot(index).openEditorWindow(errorMessage);
}
