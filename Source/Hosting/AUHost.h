#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

struct AUPluginInfo
{
    juce::String name;
    juce::String manufacturer;
    juce::String typeCode;
    juce::String subTypeCode;
    juce::String manufacturerCode;

    juce::PluginDescription description;
    bool hasValidDescription { false };
};

class AUHost
{
public:
    static juce::Array<AUPluginInfo> getInstalledEffects();
};
