#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class GainStage
{
public:
    void process (juce::AudioBuffer<float>& buffer,
                  float gainDb,
                  float pan,
                  float mix,
                  bool enabled,
                  bool bypassed);
};
