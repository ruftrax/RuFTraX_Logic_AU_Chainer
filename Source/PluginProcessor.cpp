#include "PluginProcessor.h"
#include "PluginEditor.h"

RuFTraXLogicChainerAudioProcessor::RuFTraXLogicChainerAudioProcessor()
: AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RuFTraXLogicChainerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("inputGain", "Input Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("bypassAll", "Bypass All", false));
    for (int i = 0; i < 8; ++i)
    {
        auto prefix = "slot" + juce::String(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "Enabled", "Slot " + juce::String(i + 1) + " Enabled", false));
        params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "Bypass", "Slot " + juce::String(i + 1) + " Bypass", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Mix", "Slot " + juce::String(i + 1) + " Mix", 0.0f, 1.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Gain", "Slot " + juce::String(i + 1) + " Gain", -24.0f, 24.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Pan", "Slot " + juce::String(i + 1) + " Pan", -1.0f, 1.0f, 0.0f));
    }
    return { params.begin(), params.end() };
}

void RuFTraXLogicChainerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    testTonePhase = 0.0;
    pluginHost.prepare(sampleRate, samplesPerBlock);
}
void RuFTraXLogicChainerAudioProcessor::releaseResources() {}

bool RuFTraXLogicChainerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RuFTraXLogicChainerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (testToneEnabled.load())
    {
        const double freq = 1000.0;
        const double phaseInc = juce::MathConstants<double>::twoPi * freq / currentSampleRate;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float v = 0.15f * std::sin(testTonePhase);
            testTonePhase += phaseInc;

            if (testTonePhase >= juce::MathConstants<double>::twoPi)
                testTonePhase -= juce::MathConstants<double>::twoPi;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, sample, v);
        }
    }

    auto inGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load());
    auto outGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load());

    inputLevel.store(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));

    buffer.applyGain(inGain);

    const bool bypassAll = apvts.getRawParameterValue("bypassAll")->load() > 0.5f;

    if (! bypassAll)
    {
        for (int i = 0; i < 8; ++i)
        {
            auto prefix = "slot" + juce::String(i + 1);

            const bool enabled = apvts.getRawParameterValue(prefix + "Enabled")->load() > 0.5f;
            const bool bypass  = apvts.getRawParameterValue(prefix + "Bypass")->load() > 0.5f;
            const float mix    = apvts.getRawParameterValue(prefix + "Mix")->load();
            const float gain   = apvts.getRawParameterValue(prefix + "Gain")->load();
            const float pan    = apvts.getRawParameterValue(prefix + "Pan")->load();

            if (enabled && ! bypass && pluginHost.slotHasHostedPlugin(i))
                pluginHost.processSlot(i, buffer, midi);
            else
                if (enabled && ! bypass && pluginHost.slotHasHostedPlugin(i))
                pluginHost.processSlot(i, buffer, midi);
            else
                hostEngine.processSlot(buffer, gain, pan, mix, enabled, bypass);
        }
    }

    buffer.applyGain(outGain);

    outputLevelL.store(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));

    if (buffer.getNumChannels() > 1)
        outputLevelR.store(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
    else
        outputLevelR.store(outputLevelL.load());

    outputLevel.store((outputLevelL.load() + outputLevelR.load()) * 0.5f);
}

void RuFTraXLogicChainerAudioProcessor::loadPlaceholderPlugin (int index, const juce::String& name)
{
    if (juce::isPositiveAndBelow(index, (int) slots.size()))
    {
        slots[(size_t) index].enabled = true;
        slots[(size_t) index].displayName = name;
        pluginHost.setSlotName(index, name);
    }
}

void RuFTraXLogicChainerAudioProcessor::clearSlot (int index)
{
    if (juce::isPositiveAndBelow(index, (int) slots.size()))
    {
        slots[(size_t) index] = {};
        pluginHost.clearSlot(index);
    }
}


void RuFTraXLogicChainerAudioProcessor::saveTemporaryPreset()
{
    temporaryPreset = apvts.copyState();

    juce::ValueTree slotTree("Slots");
    for (int i = 0; i < 8; ++i)
    {
        juce::ValueTree s("Slot");
        s.setProperty("index", i, nullptr);
        s.setProperty("name", slots[(size_t)i].displayName, nullptr);
        s.setProperty("enabled", slots[(size_t)i].enabled, nullptr);
        slotTree.addChild(s, -1, nullptr);
    }

    temporaryPreset.addChild(slotTree, -1, nullptr);
}

void RuFTraXLogicChainerAudioProcessor::loadTemporaryPreset()
{
    if (! temporaryPreset.isValid())
        return;

    apvts.replaceState(temporaryPreset);

    auto slotTree = temporaryPreset.getChildWithName("Slots");
    if (slotTree.isValid())
    {
        for (int i = 0; i < 8; ++i)
        {
            auto s = slotTree.getChild(i);
            if (s.isValid())
            {
                slots[(size_t)i].displayName = s.getProperty("name", "<Empty Slot>").toString();
                slots[(size_t)i].enabled = (bool) s.getProperty("enabled", false);
            }
        }
    }
}











void RuFTraXLogicChainerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::ValueTree slotTree("Slots");
    for (int i = 0; i < 8; ++i)
    {
        juce::ValueTree s("Slot");
        s.setProperty("index", i, nullptr);
        s.setProperty("name", slots[(size_t)i].displayName, nullptr);
        s.setProperty("enabled", slots[(size_t)i].enabled, nullptr);
        slotTree.addChild(s, -1, nullptr);
    }
    state.addChild(slotTree, -1, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RuFTraXLogicChainerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* RuFTraXLogicChainerAudioProcessor::createEditor()
{
    return new RuFTraXLogicChainerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RuFTraXLogicChainerAudioProcessor();
}
