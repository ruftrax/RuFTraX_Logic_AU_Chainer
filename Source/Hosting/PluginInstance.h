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

    void setHostedPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
    bool hasHostedPlugin() const;
    bool isProcessingHostedPlugin() const { return enabled && hostedPlugin != nullptr; }

    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void clear();

private:
    juce::String displayName { "<Empty Slot>" };
    bool enabled { false };
    std::unique_ptr<juce::AudioPluginInstance> hostedPlugin;
};
