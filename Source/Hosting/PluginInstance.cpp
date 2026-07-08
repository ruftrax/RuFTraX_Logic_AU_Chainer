#include "PluginInstance.h"

void PluginInstance::setDisplayName(const juce::String& name) { displayName = name; }
juce::String PluginInstance::getDisplayName() const { return displayName; }

void PluginInstance::setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
bool PluginInstance::isEnabled() const { return enabled; }

void PluginInstance::setHostedPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    hostedPlugin = std::move(plugin);
    enabled = hostedPlugin != nullptr;
}

bool PluginInstance::hasHostedPlugin() const
{
    return hostedPlugin != nullptr;
}

void PluginInstance::prepare(double sampleRate, int blockSize)
{
    if (hostedPlugin != nullptr)
        hostedPlugin->prepareToPlay(sampleRate, blockSize);
}

void PluginInstance::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (enabled && hostedPlugin != nullptr)
        hostedPlugin->processBlock(buffer, midi);
}

void PluginInstance::clear()
{
    if (hostedPlugin != nullptr)
        hostedPlugin->releaseResources();

    hostedPlugin.reset();
    displayName = "<Empty Slot>";
    enabled = false;
}
