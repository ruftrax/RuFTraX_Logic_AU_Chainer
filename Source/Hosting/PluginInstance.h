#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class PluginInstance
{
public:
    PluginInstance() = default;
    ~PluginInstance();

    void setDisplayName(const juce::String& name);
    juce::String getDisplayName() const;

    void setEnabled(bool shouldBeEnabled);
    bool isEnabled() const;

    void setHostedPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
    bool hasHostedPlugin() const;
    bool isProcessingHostedPlugin() const;

    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    bool openEditorWindow(juce::String& errorMessage);
    void clear();

private:
    juce::String displayName { "<Empty Slot>" };
    bool enabled { false };

    std::unique_ptr<juce::AudioPluginInstance> hostedPlugin;
    std::unique_ptr<juce::Component> editorWindow;
};
