#include "PluginInstance.h"

void PluginInstance::setDisplayName(const juce::String& name)
{
    displayName = name;
}

juce::String PluginInstance::getDisplayName() const
{
    return displayName;
}

void PluginInstance::setEnabled(bool shouldBeEnabled)
{
    enabled = shouldBeEnabled;
}

bool PluginInstance::isEnabled() const
{
    return enabled;
}
