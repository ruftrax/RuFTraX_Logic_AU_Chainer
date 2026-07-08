#include "AUHost.h"
#include <AudioToolbox/AudioToolbox.h>

static juce::String fourCCToString(OSType code)
{
    char chars[5];
    chars[0] = (char)((code >> 24) & 0xff);
    chars[1] = (char)((code >> 16) & 0xff);
    chars[2] = (char)((code >> 8) & 0xff);
    chars[3] = (char)(code & 0xff);
    chars[4] = 0;
    return juce::String(chars);
}

juce::Array<AUPluginInfo> AUHost::getInstalledEffects()
{
    juce::Array<AUPluginInfo> results;

    AudioComponentDescription desc {};
    desc.componentType = kAudioUnitType_Effect;
    desc.componentSubType = 0;
    desc.componentManufacturer = 0;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = nullptr;

    while ((comp = AudioComponentFindNext(comp, &desc)) != nullptr)
    {
        AudioComponentDescription found {};
        AudioComponentGetDescription(comp, &found);

        CFStringRef nameRef = nullptr;
        juce::String name = "Unknown AU";

        if (AudioComponentCopyName(comp, &nameRef) == noErr && nameRef != nullptr)
        {
            name = juce::String::fromCFString(nameRef);
            CFRelease(nameRef);
        }

        AUPluginInfo info;
        info.name = name;
        info.manufacturer = fourCCToString(found.componentManufacturer);
        info.typeCode = fourCCToString(found.componentType);
        info.subTypeCode = fourCCToString(found.componentSubType);
        info.manufacturerCode = fourCCToString(found.componentManufacturer);

        results.add(info);
    }

    struct AUPluginSorter
    {
        int compareElements(const AUPluginInfo& a, const AUPluginInfo& b) const
        {
            return a.name.compareIgnoreCase(b.name);
        }
    };

    AUPluginSorter sorter;
    results.sort(sorter);

    return results;
}
