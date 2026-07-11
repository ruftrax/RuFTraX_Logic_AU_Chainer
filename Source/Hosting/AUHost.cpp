#include "AUHost.h"

#include <AudioToolbox/AudioToolbox.h>

static juce::String fourCCToString(OSType code)
{
    char chars[5] {};

    chars[0] = static_cast<char>((code >> 24) & 0xff);
    chars[1] = static_cast<char>((code >> 16) & 0xff);
    chars[2] = static_cast<char>((code >> 8)  & 0xff);
    chars[3] = static_cast<char>(code & 0xff);

    return juce::String::fromUTF8(chars, 4);
}

juce::Array<AUPluginInfo> AUHost::getInstalledEffects()
{
    juce::Array<AUPluginInfo> results;
    juce::AudioUnitPluginFormat auFormat;

    AudioComponentDescription searchDescription {};
    searchDescription.componentType = kAudioUnitType_Effect;

    AudioComponent component = nullptr;

    while ((component = AudioComponentFindNext(component,
                                                &searchDescription)) != nullptr)
    {
        AudioComponentDescription found {};
        AudioComponentGetDescription(component, &found);

        CFStringRef nameReference = nullptr;
        juce::String displayName { "Unknown AU" };

        if (AudioComponentCopyName(component, &nameReference) == noErr
            && nameReference != nullptr)
        {
            displayName = juce::String::fromCFString(nameReference);
            CFRelease(nameReference);
        }

        AUPluginInfo info;
        info.name = displayName;
        info.manufacturer = fourCCToString(found.componentManufacturer);
        info.typeCode = fourCCToString(found.componentType);
        info.subTypeCode = fourCCToString(found.componentSubType);
        info.manufacturerCode = fourCCToString(found.componentManufacturer);

        const auto identifier =
            "AudioUnit:Effect/"
            + info.subTypeCode
            + ","
            + info.manufacturerCode;

        juce::OwnedArray<juce::PluginDescription> descriptions;
        auFormat.findAllTypesForFile(descriptions, identifier);

        if (! descriptions.isEmpty())
        {
            info.description = *descriptions.getFirst();
            info.hasValidDescription = true;

            // Prefer JUCE's fully resolved names and identifier.
            if (info.description.name.isNotEmpty())
                info.name = info.description.name;

            if (info.description.manufacturerName.isNotEmpty())
                info.manufacturer = info.description.manufacturerName;
        }

        results.add(info);
    }

    struct Sorter
    {
        int compareElements(const AUPluginInfo& first,
                            const AUPluginInfo& second) const
        {
            return first.name.compareIgnoreCase(second.name);
        }
    };

    Sorter sorter;
    results.sort(sorter);

    return results;
}
