#include "GainStage.h"

void GainStage::process (juce::AudioBuffer<float>& buffer,
                         float gainDb,
                         float pan,
                         float mix,
                         bool enabled,
                         bool bypassed)
{
    if (! enabled || bypassed)
        return;

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    const float gain = juce::Decibels::decibelsToGain (gainDb);
    buffer.applyGain (gain);

    if (buffer.getNumChannels() >= 2)
    {
        const float leftPan  = pan > 0.0f ? 1.0f - pan : 1.0f;
        const float rightPan = pan < 0.0f ? 1.0f + pan : 1.0f;

        buffer.applyGain (0, 0, buffer.getNumSamples(), leftPan);
        buffer.applyGain (1, 0, buffer.getNumSamples(), rightPan);
    }

    mix = juce::jlimit (0.0f, 1.0f, mix);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wetData = buffer.getWritePointer (ch);
        auto* dryData = dry.getReadPointer (ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            wetData[i] = dryData[i] + (wetData[i] - dryData[i]) * mix;
    }
}
