#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class PluginInstance
{
public:
    PluginInstance() = default;

    void setDisplayName(const juce::String& name);
    juce::String getDisplayName() const;

    void setEnabled(bool shouldBeEnabled);
    bool isEnabled() const;

private:
    juce::String displayName { "<Empty Slot>" };
    bool enabled { false };
};
