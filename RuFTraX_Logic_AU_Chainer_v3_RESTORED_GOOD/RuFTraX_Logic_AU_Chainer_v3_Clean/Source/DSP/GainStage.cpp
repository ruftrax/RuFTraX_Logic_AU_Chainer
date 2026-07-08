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
    dry.makeCopyOf(buffer);

    const float gain = juce::Decibels::decibelsToGain(gainDb);
    buffer.applyGain(gain);

    pan = juce::jlimit(-1.0f, 1.0f, pan);

    if (buffer.getNumChannels() >= 2)
    {
        float leftGain  = 1.0f;
        float rightGain = 1.0f;

        if (pan < 0.0f)
            rightGain = 1.0f + pan;   // pan -1 = right muted

        if (pan > 0.0f)
            leftGain = 1.0f - pan;    // pan +1 = left muted

        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }

    mix = juce::jlimit(0.0f, 1.0f, mix);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dryData = dry.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            wet[i] = dryData[i] + (wet[i] - dryData[i]) * mix;
    }
}
