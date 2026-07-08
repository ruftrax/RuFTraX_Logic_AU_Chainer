#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/GainStage.h"

class HostEngine
{
public:
    void processSlot1 (juce::AudioBuffer<float>& buffer,
                       float gainDb,
                       float pan,
                       float mix,
                       bool enabled,
                       bool bypassed);

private:
    GainStage slot1Gain;
};
