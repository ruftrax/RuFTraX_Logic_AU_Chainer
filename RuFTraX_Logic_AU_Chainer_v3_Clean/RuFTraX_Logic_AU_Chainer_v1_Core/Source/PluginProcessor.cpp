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

void RuFTraXLogicChainerAudioProcessor::prepareToPlay (double, int) {}
void RuFTraXLogicChainerAudioProcessor::releaseResources() {}

bool RuFTraXLogicChainerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RuFTraXLogicChainerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto inGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load());
    auto outGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load());

    inputLevel.store(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));

    buffer.applyGain(inGain);

    const bool bypassAll = apvts.getRawParameterValue("bypassAll")->load() > 0.5f;

    if (! bypassAll)
    {
        const bool slot1Enabled = apvts.getRawParameterValue("slot1Enabled")->load() > 0.5f;
        const bool slot1Bypass  = apvts.getRawParameterValue("slot1Bypass")->load() > 0.5f;
        const float slot1Mix    = apvts.getRawParameterValue("slot1Mix")->load();
        const float slot1Gain   = apvts.getRawParameterValue("slot1Gain")->load();
        const float slot1Pan    = apvts.getRawParameterValue("slot1Pan")->load();

        hostEngine.processSlot1(buffer, slot1Gain, slot1Pan, slot1Mix, slot1Enabled, slot1Bypass);
    }

    buffer.applyGain(outGain);

    outputLevel.store(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
}

void RuFTraXLogicChainerAudioProcessor::loadPlaceholderPlugin (int index, const juce::String& name)
{
    if (juce::isPositiveAndBelow(index, (int) slots.size()))
    {
        slots[(size_t) index].enabled = true;
        slots[(size_t) index].displayName = name;
    }
}

void RuFTraXLogicChainerAudioProcessor::clearSlot (int index)
{
    if (juce::isPositiveAndBelow(index, (int) slots.size()))
        slots[(size_t) index] = {};
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
