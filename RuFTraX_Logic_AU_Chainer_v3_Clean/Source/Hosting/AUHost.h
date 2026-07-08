#pragma once
#include <juce_core/juce_core.h>

struct AUPluginInfo
{
    juce::String name;
    juce::String manufacturer;
    juce::String typeCode;
    juce::String subTypeCode;
    juce::String manufacturerCode;
};

class AUHost
{
public:
    static juce::Array<AUPluginInfo> getInstalledEffects();
};
