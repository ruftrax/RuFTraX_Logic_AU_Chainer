#include "HostEngine.h"

void HostEngine::processSlot (juce::AudioBuffer<float>& buffer,
                               float gainDb,
                               float pan,
                               float mix,
                               bool enabled,
                               bool bypassed)
{
    slot1Gain.process (buffer, gainDb, pan, mix, enabled, bypassed);
}
