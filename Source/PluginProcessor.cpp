#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>

#if JUCE_MAC
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace
{

#if JUCE_MAC
juce::String fourCCToString(OSType value)
{
    char text[5]
    {
        static_cast<char>((value >> 24) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>(value & 0xff),
        0
    };

    return juce::String::fromUTF8(text, 4);
}

juce::String audioUnitTypeLabel(OSType type)
{
    switch (type)
    {
        case kAudioUnitType_Effect:
            return "Effect";

        case kAudioUnitType_MusicEffect:
            return "MusicEffect";

        default:
            return "Effect";
    }
}

juce::String makeAudioUnitIdentifier(
    const AudioComponentDescription& description)
{
    return "AudioUnit:"
         + audioUnitTypeLabel(description.componentType)
         + "/"
         + fourCCToString(description.componentType)
         + ","
         + fourCCToString(description.componentSubType)
         + ","
         + fourCCToString(description.componentManufacturer);
}

juce::String copyAudioComponentName(AudioComponent component)
{
    CFStringRef componentName = nullptr;

    if (AudioComponentCopyName(component, &componentName) != noErr
        || componentName == nullptr)
        return {};

    char utf8[1024] {};

    const bool converted =
        CFStringGetCString(
            componentName,
            utf8,
            sizeof(utf8),
            kCFStringEncodingUTF8);

    CFRelease(componentName);

    return converted
        ? juce::String::fromUTF8(utf8)
        : juce::String();
}

juce::String formatAudioUnitVersion(UInt32 version)
{
    const auto major = (version >> 16) & 0xffff;
    const auto minor = (version >> 8) & 0xff;
    const auto patch = version & 0xff;

    return juce::String(major)
         + "."
         + juce::String(minor)
         + "."
         + juce::String(patch);
}

void addRegistryComponentsOfType(
    OSType componentType,
    juce::KnownPluginList& destination)
{
    AudioComponentDescription query {};
    query.componentType = componentType;
    query.componentSubType = 0;
    query.componentManufacturer = 0;
    query.componentFlags = 0;
    query.componentFlagsMask = 0;

    AudioComponent component = nullptr;

    while ((component = AudioComponentFindNext(
                component,
                &query)) != nullptr)
    {
        AudioComponentDescription componentDescription {};

        if (AudioComponentGetDescription(
                component,
                &componentDescription) != noErr)
            continue;

        const auto fullName =
            copyAudioComponentName(component).trim();

        if (fullName.isEmpty())
            continue;

        juce::String manufacturer;
        juce::String pluginName;

        const int separator =
            fullName.indexOfChar(':');

        if (separator >= 0)
        {
            manufacturer =
                fullName.substring(0, separator).trim();

            pluginName =
                fullName.substring(separator + 1).trim();
        }
        else
        {
            manufacturer =
                fourCCToString(
                    componentDescription.componentManufacturer);

            pluginName = fullName;
        }

        if (pluginName.isEmpty())
            pluginName = fullName;

        if (pluginName.containsIgnoreCase(
                "RuFTraX Logic Chainer")
            || pluginName.containsIgnoreCase(
                "RuFTraX Plug-In Chainer"))
            continue;

        juce::PluginDescription description;
        description.name = pluginName;
        description.descriptiveName = pluginName;
        description.manufacturerName = manufacturer;
        description.pluginFormatName = "AudioUnit";
        description.category =
            audioUnitTypeLabel(componentType);
        description.fileOrIdentifier =
            makeAudioUnitIdentifier(componentDescription);
        description.isInstrument = false;
        description.numInputChannels = 2;
        description.numOutputChannels = 2;
        description.uniqueId =
            description.fileOrIdentifier.hashCode();
        description.deprecatedUid = description.uniqueId;
        description.lastInfoUpdateTime =
            juce::Time::getCurrentTime();

        UInt32 version = 0;

        if (AudioComponentGetVersion(
                component,
                &version) == noErr)
        {
            description.version =
                formatAudioUnitVersion(version);
        }

        destination.addType(description);
    }
}
#endif

}

RuFTraXLogicChainerAudioProcessor::
RuFTraXLogicChainerAudioProcessor()
    : juce::AudioProcessor(
          BusesProperties()
              .withInput(
                  "Input",
                  juce::AudioChannelSet::stereo(),
                  true)
              .withInput(
                  "Sidechain",
                  juce::AudioChannelSet::stereo(),
                  false)
              .withOutput(
                  "Output",
                  juce::AudioChannelSet::stereo(),
                  true)),
      juce::Thread("RuFTraX Audio Unit Scanner"),
      apvts(*this, nullptr, "PARAMETERS",
            createParameterLayout())
{
    for (auto& generation : slotLoadGenerations)
        generation.store(0);

    for (auto& faulted : slotProcessingFaults)
        faulted.store(false);

    for (auto& usage : slotCpuUsage)
        usage.store(0.0f);

    globalBypassRequested.store(
        apvts.getRawParameterValue("bypassAll")->load()
            >= 0.5f,
        std::memory_order_relaxed);
    apvts.addParameterListener("bypassAll", this);

    // Audio Units are macOS/iOS-only; everywhere else, host VST3 plugins
    // instead. Both paths land in the same cross-platform formatManager,
    // so the slot-loading, scanning and hosting code below is unchanged.
#if JUCE_MAC
    formatManager.addFormat(
        std::make_unique<juce::AudioUnitPluginFormat>());

    setScanStatus(
        scanNotStarted,
        "Click + INSERT to browse installed Audio Units.");
#else
    formatManager.addFormat(
        std::make_unique<juce::VST3PluginFormat>());

    setScanStatus(
        scanNotStarted,
        "Click + INSERT to browse installed VST3 plug-ins.");
#endif
}

RuFTraXLogicChainerAudioProcessor::
~RuFTraXLogicChainerAudioProcessor()
{
    shuttingDown.store(true);
    apvts.removeParameterListener("bypassAll", this);

    {
        const juce::ScopedLock lock(asyncLifetime->lock);
        asyncLifetime->alive = false;
    }

    signalThreadShouldExit();
    stopThread(5000);
    releaseResources();

    const juce::SpinLock::ScopedLockType lock(
        hostedPluginLock);

    for (auto& plugin : hostedPlugins)
        plugin.reset();
}

juce::AudioProcessorValueTreeState::ParameterLayout
RuFTraXLogicChainerAudioProcessor::createParameterLayout()
{
    std::vector<
        std::unique_ptr<juce::RangedAudioParameter>>
        parameters;

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "inputGain",
            "Input Gain",
            -24.0f,
            24.0f,
            0.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "outputGain",
            "Output Gain",
            -24.0f,
            24.0f,
            0.0f));

    parameters.push_back(
        std::make_unique<juce::AudioParameterBool>(
            "bypassAll",
            "Bypass All",
            false));

    for (int slot = 0; slot < slotCount; ++slot)
    {
        const auto prefix =
            "slot" + juce::String(slot + 1);

        parameters.push_back(
            std::make_unique<juce::AudioParameterBool>(
                prefix + "Enabled",
                "Slot " + juce::String(slot + 1)
                    + " Enabled",
                false));

        parameters.push_back(
            std::make_unique<juce::AudioParameterBool>(
                prefix + "Bypass",
                "Slot " + juce::String(slot + 1)
                    + " Bypass",
                false));

        parameters.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                prefix + "Mix",
                "Slot " + juce::String(slot + 1)
                    + " Mix",
                0.0f,
                1.0f,
                1.0f));

        parameters.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                prefix + "Gain",
                "Slot " + juce::String(slot + 1)
                    + " Gain",
                -24.0f,
                24.0f,
                0.0f));

        parameters.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                prefix + "Pan",
                "Slot " + juce::String(slot + 1)
                    + " Pan",
                -1.0f,
                1.0f,
                0.0f));
    }

    return { parameters.begin(), parameters.end() };
}

void RuFTraXLogicChainerAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    const double safeSampleRate =
        sampleRate > 0.0 ? sampleRate : 44100.0;

    const int safeBlockSize =
        juce::jmax(1, samplesPerBlock);

    currentSampleRate.store(safeSampleRate);
    currentBlockSize.store(safeBlockSize);
    testTonePhase = 0.0;

    dryBuffer.setSize(
        juce::jmax(2, getTotalNumOutputChannels()),
        safeBlockSize,
        false,
        false,
        true);

    // Snapshot the hosted plug-in pointers under a brief lock, then call
    // into each plug-in's (potentially slow) prepareToPlay() outside the
    // lock. Holding a spin lock across third-party preparation work would
    // otherwise stall the message thread and burn CPU busy-waiting, and
    // could starve the audio thread's try-lock in processHostedChain for
    // the whole duration.
    std::array<HostedPluginPtr, slotCount> snapshot;
    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
        snapshot = hostedPlugins;
    }

    std::array<juce::AudioBuffer<float>, slotCount> preparedBuffers;
    std::array<bool, slotCount> preparedOk {};
    preparedOk.fill(true);

    for (int slot = 0; slot < slotCount; ++slot)
    {
        auto* plugin = snapshot[(size_t) slot].get();

        if (plugin == nullptr)
            continue;

        juce::String errorMessage;
        preparedOk[(size_t) slot] =
            prepareHostedPlugin(
                *plugin,
                preparedBuffers[(size_t) slot],
                errorMessage);
    }

    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);

        for (int slot = 0; slot < slotCount; ++slot)
        {
            // Only commit the prepared buffer if the same instance is still
            // the one loaded - a load, clear or reorder that happened while
            // we were preparing wins.
            if (hostedPlugins[(size_t) slot].get()
                != snapshot[(size_t) slot].get())
                continue;

            if (snapshot[(size_t) slot] != nullptr)
            {
                pluginBuffers[(size_t) slot] =
                    std::move(preparedBuffers[(size_t) slot]);
                slotProcessingFaults[(size_t) slot]
                    .store(!preparedOk[(size_t) slot]);
            }
        }
    }

    isPrepared.store(true);
    updateReportedLatency();
}

void RuFTraXLogicChainerAudioProcessor::releaseResources()
{
    isPrepared.store(false);

    std::array<HostedPluginPtr, slotCount> snapshot;
    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
        snapshot = hostedPlugins;
    }

    for (auto& plugin : snapshot)
    {
        if (plugin != nullptr)
            plugin->releaseResources();
    }
}

bool RuFTraXLogicChainerAudioProcessor::
isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output =
        layouts.getMainOutputChannelSet();

    const auto input =
        layouts.getMainInputChannelSet();

    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

    if (input != output)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.inputBuses.getReference(1);

        if (!sidechain.isDisabled()
            && sidechain != juce::AudioChannelSet::mono()
            && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void RuFTraXLogicChainerAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainIn = getBusBuffer(buffer, true, 0);
    auto mainOut = getBusBuffer(buffer, false, 0);

    for (int channel = mainIn.getNumChannels();
         channel < mainOut.getNumChannels();
         ++channel)
    {
        mainOut.clear(
            channel,
            0,
            mainOut.getNumSamples());
    }

    // This is a hard, processor-level bypass.  It deliberately runs before
    // the input/output gains, the test tone and any hosted plug-in.  The
    // original host buffer therefore passes through untouched whenever the
    // front-panel switch, automation, or restored state requests Bypass All.
    if (globalBypassRequested.load(std::memory_order_relaxed))
    {
        const float passthroughLevel =
            mainOut.getMagnitude(0, mainOut.getNumSamples());
        inputPeakLevel.store(passthroughLevel, std::memory_order_relaxed);
        outputPeakLevel.store(passthroughLevel, std::memory_order_relaxed);
        return;
    }

    if (testToneEnabled.load(std::memory_order_relaxed))
        injectTestTone(mainOut);

    const float inputGain =
        juce::Decibels::decibelsToGain(
            apvts.getRawParameterValue(
                "inputGain")->load());

    const float outputGain =
        juce::Decibels::decibelsToGain(
            apvts.getRawParameterValue(
                "outputGain")->load());

    mainOut.applyGain(inputGain);
    inputPeakLevel.store(
        mainOut.getMagnitude(0, mainOut.getNumSamples()),
        std::memory_order_relaxed);

    juce::AudioBuffer<float> sidechainView;
    const juce::AudioBuffer<float>* sidechainPtr = nullptr;

    if (getBusCount(true) > 1
        && getBus(true, 1) != nullptr
        && getBus(true, 1)->isEnabled())
    {
        sidechainView = getBusBuffer(buffer, true, 1);
        sidechainPtr = &sidechainView;
    }

    processHostedChain(mainOut, sidechainPtr, midi);
    mainOut.applyGain(outputGain);
    outputPeakLevel.store(
        mainOut.getMagnitude(0, mainOut.getNumSamples()),
        std::memory_order_relaxed);
}

void RuFTraXLogicChainerAudioProcessor::
decaySlotCpuUsage(int slotIndex) noexcept
{
    const float previous =
        slotCpuUsage[(size_t) slotIndex]
            .load(std::memory_order_relaxed);

    if (previous > 0.0005f)
    {
        slotCpuUsage[(size_t) slotIndex].store(
            previous * 0.8f,
            std::memory_order_relaxed);
    }
    else if (previous != 0.0f)
    {
        slotCpuUsage[(size_t) slotIndex].store(
            0.0f,
            std::memory_order_relaxed);
    }
}

void RuFTraXLogicChainerAudioProcessor::
processHostedChain(
    juce::AudioBuffer<float>& mainBuffer,
    const juce::AudioBuffer<float>* sidechainBuffer,
    juce::MidiBuffer& midi)
{
    if (mainBuffer.getNumSamples()
        > dryBuffer.getNumSamples())
        return;

    const juce::SpinLock::ScopedTryLockType lock(
        hostedPluginLock);

    if (!lock.isLocked())
        return;

    const double sampleRate =
        currentSampleRate.load(std::memory_order_relaxed);
    const double blockSeconds =
        sampleRate > 0.0
            ? (double) mainBuffer.getNumSamples() / sampleRate
            : 0.0;

    for (int slot = 0; slot < slotCount; ++slot)
    {
        auto* plugin =
            hostedPlugins[(size_t) slot].get();

        if (plugin == nullptr)
        {
            decaySlotCpuUsage(slot);
            continue;
        }

        if (slotProcessingFaults[(size_t) slot].load()
            || plugin->isSuspended())
        {
            decaySlotCpuUsage(slot);
            continue;
        }

        const auto prefix =
            "slot" + juce::String(slot + 1);

        const bool enabled =
            apvts.getRawParameterValue(
                prefix + "Enabled")->load() >= 0.5f;

        const bool bypassed =
            apvts.getRawParameterValue(
                prefix + "Bypass")->load() >= 0.5f;

        if (!enabled || bypassed)
        {
            decaySlotCpuUsage(slot);
            continue;
        }

        const int channels =
            juce::jmin(
                mainBuffer.getNumChannels(),
                dryBuffer.getNumChannels());

        for (int channel = 0;
             channel < channels;
             ++channel)
        {
            dryBuffer.copyFrom(
                channel,
                0,
                mainBuffer,
                channel,
                0,
                mainBuffer.getNumSamples());
        }

        auto& pluginBuffer =
            pluginBuffers[(size_t) slot];

        const int pluginInputs =
            plugin->getTotalNumInputChannels();
        const int pluginOutputs =
            plugin->getTotalNumOutputChannels();
        const int requiredChannels =
            juce::jmax(pluginInputs, pluginOutputs);

        if (requiredChannels <= 0
            || pluginBuffer.getNumChannels() < requiredChannels
            || pluginBuffer.getNumSamples() < mainBuffer.getNumSamples())
        {
            // A transient host block-size surprise, not a broken plug-in -
            // skip only this block. The next prepareToPlay (or a change
            // that stops triggering the mismatch) recovers automatically,
            // instead of the slot being silenced for the rest of the
            // session.
            decaySlotCpuUsage(slot);
            continue;
        }

        pluginBuffer.clear(0, mainBuffer.getNumSamples());

        const int pluginMainInputChannels =
            plugin->getChannelCountOfBus(true, 0);

        const int copiedInputs = juce::jmin(
            juce::jmin(mainBuffer.getNumChannels(), pluginMainInputChannels),
            pluginBuffer.getNumChannels());

        for (int channel = 0; channel < copiedInputs; ++channel)
        {
            pluginBuffer.copyFrom(
                channel, 0,
                mainBuffer, channel, 0,
                mainBuffer.getNumSamples());
        }

        // If the hosted plug-in exposes a second input bus and our own
        // optional sidechain bus is connected, feed it through rather than
        // always leaving it silent.
        if (sidechainBuffer != nullptr
            && plugin->getBusCount(true) > 1)
        {
            const int pluginSideInputChannels =
                plugin->getChannelCountOfBus(true, 1);

            const int sideCopied = juce::jmin(
                juce::jmin(sidechainBuffer->getNumChannels(),
                          pluginSideInputChannels),
                pluginBuffer.getNumChannels() - pluginMainInputChannels);

            for (int channel = 0; channel < sideCopied; ++channel)
            {
                pluginBuffer.copyFrom(
                    pluginMainInputChannels + channel, 0,
                    *sidechainBuffer, channel, 0,
                    mainBuffer.getNumSamples());
            }
        }

        // Extra AU input channels beyond the main and sidechain buses
        // remain silent instead of reading past the host's buffers.
        juce::AudioBuffer<float> processingBuffer(
            pluginBuffer.getArrayOfWritePointers(),
            requiredChannels,
            mainBuffer.getNumSamples());

        const auto cpuStart =
            juce::Time::getHighResolutionTicks();

        try
        {
            plugin->setPlayHead(getPlayHead());
            plugin->processBlock(processingBuffer, midi);
        }
        catch (const std::exception&)
        {
            slotProcessingFaults[(size_t) slot].store(true);
            decaySlotCpuUsage(slot);
            continue;
        }
        catch (...)
        {
            slotProcessingFaults[(size_t) slot].store(true);
            decaySlotCpuUsage(slot);
            continue;
        }

        if (blockSeconds > 0.0)
        {
            const auto cpuEnd =
                juce::Time::getHighResolutionTicks();
            const double elapsed =
                juce::Time::highResolutionTicksToSeconds(
                    cpuEnd - cpuStart);
            const float instantaneous =
                (float) juce::jlimit(0.0, 4.0, elapsed / blockSeconds);
            const float previous =
                slotCpuUsage[(size_t) slot]
                    .load(std::memory_order_relaxed);

            // A light one-pole smoothing pass keeps the meter from
            // flickering block-to-block while still tracking real changes.
            slotCpuUsage[(size_t) slot].store(
                previous + (instantaneous - previous) * 0.2f,
                std::memory_order_relaxed);
        }

        if (pluginOutputs <= 0)
            continue;

        for (int channel = 0;
             channel < mainBuffer.getNumChannels();
             ++channel)
        {
            const int sourceChannel =
                juce::jmin(channel, pluginOutputs - 1);

            mainBuffer.copyFrom(
                channel, 0,
                processingBuffer, sourceChannel, 0,
                mainBuffer.getNumSamples());
        }

        const float mix =
            juce::jlimit(
                0.0f,
                1.0f,
                apvts.getRawParameterValue(
                    prefix + "Mix")->load());

        const float gain =
            juce::Decibels::decibelsToGain(
                apvts.getRawParameterValue(
                    prefix + "Gain")->load());

        const float pan =
            juce::jlimit(
                -1.0f,
                1.0f,
                apvts.getRawParameterValue(
                    prefix + "Pan")->load());

        float leftGain = gain;
        float rightGain = gain;

        if (pan > 0.0f)
            leftGain *= 1.0f - pan;
        else if (pan < 0.0f)
            rightGain *= 1.0f + pan;

        if (mainBuffer.getNumChannels() > 0)
            mainBuffer.applyGain(
                0,
                0,
                mainBuffer.getNumSamples(),
                leftGain);

        if (mainBuffer.getNumChannels() > 1)
            mainBuffer.applyGain(
                1,
                0,
                mainBuffer.getNumSamples(),
                rightGain);

        const float dryAmount = 1.0f - mix;

        for (int channel = 0;
             channel < channels;
             ++channel)
        {
            auto* wet =
                mainBuffer.getWritePointer(channel);

            const auto* dry =
                dryBuffer.getReadPointer(channel);

            for (int sample = 0;
                 sample < mainBuffer.getNumSamples();
                 ++sample)
            {
                wet[sample] =
                    wet[sample] * mix
                    + dry[sample] * dryAmount;
            }
        }
    }
}

void RuFTraXLogicChainerAudioProcessor::
injectTestTone(juce::AudioBuffer<float>& mainBuffer)
{
    constexpr double frequencyHz = 440.0;
    constexpr float amplitude = 0.1259f; // -18 dBFS

    const double sampleRate =
        currentSampleRate.load(std::memory_order_relaxed);

    if (sampleRate <= 0.0)
        return;

    const double phaseIncrement =
        juce::MathConstants<double>::twoPi
        * frequencyHz / sampleRate;

    const int numSamples = mainBuffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float value =
            amplitude * (float) std::sin(testTonePhase);

        testTonePhase += phaseIncrement;

        for (int channel = 0;
             channel < mainBuffer.getNumChannels();
             ++channel)
        {
            mainBuffer.setSample(channel, sample, value);
        }
    }

    if (testTonePhase > juce::MathConstants<double>::twoPi)
    {
        testTonePhase =
            std::fmod(
                testTonePhase,
                juce::MathConstants<double>::twoPi);
    }
}

bool RuFTraXLogicChainerAudioProcessor::
prepareHostedPlugin(
    juce::AudioPluginInstance& plugin,
    juce::AudioBuffer<float>& scratchBuffer,
    juce::String& errorMessage)
{
    try
    {
        const double sampleRate = currentSampleRate.load();
        const int blockSize = currentBlockSize.load();
        const bool sidechainConnected = isSidechainConnected();

        const auto originalLayout = plugin.getBusesLayout();
        auto requestedLayout = originalLayout;

        if (!requestedLayout.inputBuses.isEmpty())
        {
            requestedLayout.inputBuses.getReference(0) =
                getChannelLayoutOfBus(true, 0);

            for (int bus = 1;
                 bus < requestedLayout.inputBuses.size();
                 ++bus)
            {
                if (bus == 1 && sidechainConnected)
                {
                    // Route our own optional sidechain bus into the hosted
                    // plug-in's second input bus, if it can accept it,
                    // instead of always silencing it.
                    requestedLayout.inputBuses.getReference(bus) =
                        getChannelLayoutOfBus(true, 1);
                }
                else
                {
                    requestedLayout.inputBuses.getReference(bus) =
                        juce::AudioChannelSet::disabled();
                }
            }
        }

        if (!requestedLayout.outputBuses.isEmpty())
        {
            requestedLayout.outputBuses.getReference(0) =
                getChannelLayoutOfBus(false, 0);

            for (int bus = 1;
                 bus < requestedLayout.outputBuses.size();
                 ++bus)
            {
                requestedLayout.outputBuses.getReference(bus) =
                    juce::AudioChannelSet::disabled();
            }
        }

        if (requestedLayout != originalLayout
            && !plugin.setBusesLayout(requestedLayout))
        {
            // Some Audio Units require their published layout.  Keep it and
            // provide enough scratch channels instead of forcing an unsafe
            // stereo configuration.
            plugin.setBusesLayout(originalLayout);
        }

        plugin.setProcessingPrecision(
            juce::AudioProcessor::singlePrecision);
        plugin.setNonRealtime(isNonRealtime());
        plugin.setRateAndBufferSizeDetails(sampleRate, blockSize);
        plugin.prepareToPlay(sampleRate, blockSize);

        const int pluginInputs =
            plugin.getTotalNumInputChannels();
        const int pluginOutputs =
            plugin.getTotalNumOutputChannels();

        if (pluginInputs <= 0 || pluginOutputs <= 0)
        {
            errorMessage =
                "The Audio Unit does not provide a usable audio layout.";
            plugin.releaseResources();
            return false;
        }

        const int scratchChannels = juce::jmax(
            juce::jmax(pluginInputs, pluginOutputs),
            juce::jmax(getTotalNumInputChannels(),
                       getTotalNumOutputChannels()));

        scratchBuffer.setSize(
            scratchChannels,
            blockSize,
            false,
            true,
            false);
        scratchBuffer.clear();
        errorMessage.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        errorMessage =
            "The Audio Unit could not be prepared: "
            + juce::String(exception.what());
    }
    catch (...)
    {
        errorMessage =
            "The Audio Unit could not be prepared safely.";
    }

    return false;
}

void RuFTraXLogicChainerAudioProcessor::
startAudioUnitScan()
{
    int expected = scanNotStarted;

    if (scanState.compare_exchange_strong(
            expected,
            scanRunning))
    {
        setScanStatus(
            scanRunning,
            "Scanning installed Audio Unit effects...");

        startThread();
    }
}

void RuFTraXLogicChainerAudioProcessor::run()
{
#if JUCE_MAC
    juce::KnownPluginList registryPlugins;

    // AudioComponentFindNext reads macOS's Audio Unit registry and metadata.
    // It does not instantiate each plug-in, so unlicensed products cannot
    // open activation windows merely because the browser is being populated.
    addRegistryComponentsOfType(
        kAudioUnitType_Effect,
        registryPlugins);

    if (threadShouldExit())
        return;

    addRegistryComponentsOfType(
        kAudioUnitType_MusicEffect,
        registryPlugins);

    if (threadShouldExit())
        return;

    registryPlugins.sort(
        juce::KnownPluginList::sortByManufacturer,
        true);

    const int pluginCount =
        registryPlugins.getNumTypes();

    {
        const juce::ScopedLock lock(
            pluginListLock);

        knownPlugins.clear();

        for (const auto& description :
             registryPlugins.getTypes())
        {
            knownPlugins.addType(description);
        }
    }

    if (pluginCount <= 0)
    {
        setScanStatus(
            scanFailed,
            "No Audio Unit effects are registered with macOS.");
        return;
    }

    setScanStatus(
        scanReady,
        juce::String(pluginCount)
            + " Audio Unit effects ready. "
              "No plug-ins were opened during discovery.");
#else
    setScanStatus(
        scanFailed,
        "Audio Unit browsing is available only on macOS.");
#endif
}

void RuFTraXLogicChainerAudioProcessor::
setScanStatus(
    ScanState state,
    const juce::String& message)
{
    {
        const juce::ScopedLock lock(
            pluginListLock);
        scanStatus = message;
    }

    scanState.store(state);
}

void RuFTraXLogicChainerAudioProcessor::
parameterChanged(
    const juce::String& parameterID,
    float newValue)
{
    if (parameterID == "bypassAll")
    {
        globalBypassRequested.store(
            newValue >= 0.5f,
            std::memory_order_relaxed);
    }
}

bool RuFTraXLogicChainerAudioProcessor::
isAudioUnitScanFinished() const noexcept
{
    const int state = scanState.load();

    return state == scanReady
        || state == scanFailed;
}

bool RuFTraXLogicChainerAudioProcessor::
didAudioUnitScanFail() const noexcept
{
    return scanState.load() == scanFailed;
}

juce::String RuFTraXLogicChainerAudioProcessor::
getAudioUnitScanStatus() const
{
    const juce::ScopedLock lock(
        pluginListLock);

    return scanStatus;
}

juce::Array<juce::PluginDescription>
RuFTraXLogicChainerAudioProcessor::
getAvailableAudioUnits() const
{
    const juce::ScopedLock lock(
        pluginListLock);

    return knownPlugins.getTypes();
}

void RuFTraXLogicChainerAudioProcessor::
loadAudioUnitInSlot(
    int slotIndex,
    const juce::PluginDescription& description,
    LoadCallback callback,
    bool recordUndo,
    bool notifyLatency)
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
    {
        if (callback)
            callback(
                false,
                "Invalid slot number.");
        return;
    }

    const SlotSnapshot before =
        recordUndo ? captureSlotSnapshot(slotIndex) : SlotSnapshot {};

    const std::uint64_t loadGeneration =
        slotLoadGenerations[(size_t) slotIndex]
            .fetch_add(1)
        + 1;

    ChainSlot previousSlot;
    const auto lifetime = asyncLifetime;

    {
        const juce::ScopedLock lock(
            slotInfoLock);

        previousSlot = slots[(size_t) slotIndex];
        slots[(size_t) slotIndex].displayName =
            "Loading " + description.name + "...";
    }

    formatManager.createPluginInstanceAsync(
        description,
        currentSampleRate.load(),
        currentBlockSize.load(),
        [this,
         slotIndex,
         loadGeneration,
         lifetime,
         previousSlot,
         description,
         before,
         recordUndo,
         notifyLatency,
         callback = std::move(callback)]
        (std::unique_ptr<juce::AudioPluginInstance>
             instance,
         const juce::String& errorMessage) mutable
        {
            const juce::ScopedLock lifetimeLock(
                lifetime->lock);

            if (!lifetime->alive)
                return;

            if (shuttingDown.load()
                || slotLoadGenerations[(size_t) slotIndex].load()
                    != loadGeneration)
                return;

            if (instance == nullptr)
            {
                {
                    const juce::ScopedLock lock(
                        slotInfoLock);

                    slots[(size_t) slotIndex] = previousSlot;
                }

                setBoolParameter(
                    slotParameterID(
                        slotIndex,
                        "Enabled"),
                    previousSlot.enabled);

                if (callback)
                {
                    callback(
                        false,
                        errorMessage.isNotEmpty()
                            ? errorMessage
                            : "The Audio Unit could not be loaded.");
                }

                return;
            }

            juce::AudioBuffer<float> newPluginBuffer;
            juce::String preparationError;

            if (!prepareHostedPlugin(
                    *instance,
                    newPluginBuffer,
                    preparationError))
            {
                {
                    const juce::ScopedLock lock(
                        slotInfoLock);
                    slots[(size_t) slotIndex] = previousSlot;
                }

                setBoolParameter(
                    slotParameterID(slotIndex, "Enabled"),
                    previousSlot.enabled);

                if (callback)
                {
                    callback(
                        false,
                        preparationError.isNotEmpty()
                            ? preparationError
                            : "The Audio Unit could not be prepared safely.");
                }

                return;
            }

            if (shuttingDown.load()
                || slotLoadGenerations[(size_t) slotIndex].load()
                    != loadGeneration)
            {
                instance->releaseResources();
                return;
            }

            HostedPluginPtr newPlugin(instance.release());
            HostedPluginPtr oldPlugin;

            {
                const juce::SpinLock::ScopedLockType lock(
                    hostedPluginLock);

                oldPlugin = std::move(
                    hostedPlugins[(size_t) slotIndex]);
                hostedPlugins[(size_t) slotIndex] =
                    std::move(newPlugin);
                pluginBuffers[(size_t) slotIndex] =
                    std::move(newPluginBuffer);
                slotProcessingFaults[(size_t) slotIndex]
                    .store(false);
            }

            if (oldPlugin != nullptr)
                oldPlugin->releaseResources();

            slotCpuUsage[(size_t) slotIndex]
                .store(0.0f, std::memory_order_relaxed);

            {
                const juce::ScopedLock lock(
                    slotInfoLock);

                auto& slot =
                    slots[(size_t) slotIndex];

                slot.enabled = true;
                slot.displayName =
                    description.name;
                slot.description =
                    description;
                slot.hasDescription = true;
            }

            setBoolParameter(
                slotParameterID(
                    slotIndex,
                    "Enabled"),
                true);

            setBoolParameter(
                slotParameterID(
                    slotIndex,
                    "Bypass"),
                false);

            if (notifyLatency)
                updateReportedLatency();

            if (recordUndo)
            {
                ChainEditRecord record;
                record.kind = ChainEditKind::insert;
                record.description =
                    "Insert " + description.name
                    + " in Slot " + juce::String(slotIndex + 1);
                record.slotIndex = slotIndex;
                record.before = before;
                record.after = captureSlotSnapshot(slotIndex);
                pushChainEdit(std::move(record));
            }

            if (callback)
            {
                callback(
                    true,
                    description.manufacturerName
                        + " - "
                        + description.name
                        + " loaded in Slot "
                        + juce::String(
                            slotIndex + 1)
                        + ".");
            }
        });
}

void RuFTraXLogicChainerAudioProcessor::
clearSlot(int slotIndex, bool recordUndo, bool notifyLatency)
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return;

    const bool hadPlugin =
        getHostedPluginHandle(slotIndex) != nullptr;

    const SlotSnapshot before =
        (recordUndo && hadPlugin)
            ? captureSlotSnapshot(slotIndex)
            : SlotSnapshot {};

    slotLoadGenerations[(size_t) slotIndex]
        .fetch_add(1);

    HostedPluginPtr removedPlugin;

    {
        const juce::SpinLock::ScopedLockType lock(
            hostedPluginLock);

        removedPlugin = std::move(
            hostedPlugins[(size_t) slotIndex]);
        pluginBuffers[(size_t) slotIndex]
            .setSize(0, 0);
        slotProcessingFaults[(size_t) slotIndex]
            .store(false);
    }

    if (removedPlugin != nullptr)
        removedPlugin->releaseResources();

    slotCpuUsage[(size_t) slotIndex]
        .store(0.0f, std::memory_order_relaxed);

    {
        const juce::ScopedLock lock(
            slotInfoLock);

        slots[(size_t) slotIndex] = {};
    }

    setBoolParameter(
        slotParameterID(slotIndex, "Enabled"),
        false);

    setBoolParameter(
        slotParameterID(slotIndex, "Bypass"),
        false);

    if (notifyLatency)
        updateReportedLatency();

    if (recordUndo && hadPlugin)
    {
        ChainEditRecord record;
        record.kind = ChainEditKind::remove;
        record.description =
            "Remove "
            + (before.hasDescription
                   ? before.description.name
                   : juce::String("plug-in"))
            + " from Slot " + juce::String(slotIndex + 1);
        record.slotIndex = slotIndex;
        record.before = before;
        record.after = captureSlotSnapshot(slotIndex);
        pushChainEdit(std::move(record));
    }
}

bool RuFTraXLogicChainerAudioProcessor::
isSlotEnabled(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return false;

    const auto id =
        slotParameterID(slotIndex, "Enabled");

    return apvts.getRawParameterValue(id)->load()
        >= 0.5f;
}

bool RuFTraXLogicChainerAudioProcessor::
isSlotBypassed(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return false;

    const auto id =
        slotParameterID(slotIndex, "Bypass");

    return apvts.getRawParameterValue(id)->load()
        >= 0.5f;
}

bool RuFTraXLogicChainerAudioProcessor::
isSlotFaulted(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(slotIndex, slotCount))
        return false;

    return slotProcessingFaults[(size_t) slotIndex].load();
}

float RuFTraXLogicChainerAudioProcessor::
getSlotCpuUsage(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(slotIndex, slotCount))
        return 0.0f;

    return slotCpuUsage[(size_t) slotIndex]
        .load(std::memory_order_relaxed);
}

int RuFTraXLogicChainerAudioProcessor::
getSlotLatencySamples(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(slotIndex, slotCount))
        return 0;

    const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
    auto* plugin = hostedPlugins[(size_t) slotIndex].get();
    return plugin != nullptr ? plugin->getLatencySamples() : 0;
}

void RuFTraXLogicChainerAudioProcessor::
retrySlot(int slotIndex)
{
    if (!juce::isPositiveAndBelow(slotIndex, slotCount))
        return;

    HostedPluginPtr plugin;
    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
        plugin = hostedPlugins[(size_t) slotIndex];
    }

    if (plugin == nullptr)
        return;

    juce::AudioBuffer<float> newBuffer;
    juce::String error;
    const bool ok = prepareHostedPlugin(*plugin, newBuffer, error);

    if (!ok)
        return;

    const juce::SpinLock::ScopedLockType lock(hostedPluginLock);

    // Only apply if the same instance is still loaded in this slot - a
    // reload or clear may have happened while we were preparing.
    if (hostedPlugins[(size_t) slotIndex].get() == plugin.get())
    {
        pluginBuffers[(size_t) slotIndex] = std::move(newBuffer);
        slotProcessingFaults[(size_t) slotIndex].store(false);
    }
}

bool RuFTraXLogicChainerAudioProcessor::
isGlobalBypassed() const
{
    return globalBypassRequested.load(
        std::memory_order_relaxed);
}

void RuFTraXLogicChainerAudioProcessor::
setGlobalBypassed(bool shouldBypass)
{
    globalBypassRequested.store(
        shouldBypass,
        std::memory_order_relaxed);
    setBoolParameter("bypassAll", shouldBypass);
}

juce::String RuFTraXLogicChainerAudioProcessor::
getSlotDisplayName(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return "<Empty Slot>";

    const juce::ScopedLock lock(
        slotInfoLock);

    return slots[(size_t) slotIndex].displayName;
}

RuFTraXLogicChainerAudioProcessor::HostedPluginPtr
RuFTraXLogicChainerAudioProcessor::
getHostedPluginHandle(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(
            slotIndex,
            slotCount))
        return {};

    const juce::SpinLock::ScopedLockType lock(
        hostedPluginLock);

    return hostedPlugins[(size_t) slotIndex];
}

bool RuFTraXLogicChainerAudioProcessor::
applyHostedPluginState(
    int slotIndex,
    const juce::MemoryBlock& state)
{
    if (!juce::isPositiveAndBelow(slotIndex, slotCount)
        || state.getSize() == 0)
        return false;

    const juce::SpinLock::ScopedLockType lock(
        hostedPluginLock);

    auto& plugin = hostedPlugins[(size_t) slotIndex];

    if (plugin == nullptr)
        return false;

    try
    {
        plugin->setStateInformation(
            state.getData(),
            (int) state.getSize());
        return true;
    }
    catch (...)
    {
        slotProcessingFaults[(size_t) slotIndex].store(true);
        return false;
    }
}

void RuFTraXLogicChainerAudioProcessor::
moveSlot(int fromIndex, int toIndex, bool recordUndo)
{
    if (!juce::isPositiveAndBelow(fromIndex, slotCount)
        || !juce::isPositiveAndBelow(toIndex, slotCount)
        || fromIndex == toIndex)
        return;

    std::array<SlotSnapshot, slotCount> beforeParams;

    for (int i = 0; i < slotCount; ++i)
    {
        beforeParams[(size_t) i].enabled = isSlotEnabled(i);
        beforeParams[(size_t) i].bypass = isSlotBypassed(i);
        beforeParams[(size_t) i].mix =
            apvts.getRawParameterValue(slotParameterID(i, "Mix"))->load();
        beforeParams[(size_t) i].gain =
            apvts.getRawParameterValue(slotParameterID(i, "Gain"))->load();
        beforeParams[(size_t) i].pan =
            apvts.getRawParameterValue(slotParameterID(i, "Pan"))->load();
    }

    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);

        auto movedPlugin = std::move(hostedPlugins[(size_t) fromIndex]);
        auto movedBuffer = std::move(pluginBuffers[(size_t) fromIndex]);
        const bool movedFault =
            slotProcessingFaults[(size_t) fromIndex].load();
        const float movedCpu =
            slotCpuUsage[(size_t) fromIndex].load();

        if (fromIndex < toIndex)
        {
            for (int i = fromIndex; i < toIndex; ++i)
            {
                hostedPlugins[(size_t) i] =
                    std::move(hostedPlugins[(size_t) i + 1]);
                pluginBuffers[(size_t) i] =
                    std::move(pluginBuffers[(size_t) i + 1]);
                slotProcessingFaults[(size_t) i].store(
                    slotProcessingFaults[(size_t) i + 1].load());
                slotCpuUsage[(size_t) i].store(
                    slotCpuUsage[(size_t) i + 1].load());
            }
        }
        else
        {
            for (int i = fromIndex; i > toIndex; --i)
            {
                hostedPlugins[(size_t) i] =
                    std::move(hostedPlugins[(size_t) i - 1]);
                pluginBuffers[(size_t) i] =
                    std::move(pluginBuffers[(size_t) i - 1]);
                slotProcessingFaults[(size_t) i].store(
                    slotProcessingFaults[(size_t) i - 1].load());
                slotCpuUsage[(size_t) i].store(
                    slotCpuUsage[(size_t) i - 1].load());
            }
        }

        hostedPlugins[(size_t) toIndex] = std::move(movedPlugin);
        pluginBuffers[(size_t) toIndex] = std::move(movedBuffer);
        slotProcessingFaults[(size_t) toIndex].store(movedFault);
        slotCpuUsage[(size_t) toIndex].store(movedCpu);

        // A reorder changes what "the same plug-in" means at every affected
        // index, so any load still in flight for one of those slots must no
        // longer be allowed to land.
        for (int i = 0; i < slotCount; ++i)
            slotLoadGenerations[(size_t) i].fetch_add(1);
    }

    {
        const juce::ScopedLock lock(slotInfoLock);

        auto movedSlot = slots[(size_t) fromIndex];

        if (fromIndex < toIndex)
        {
            for (int i = fromIndex; i < toIndex; ++i)
                slots[(size_t) i] = slots[(size_t) i + 1];
        }
        else
        {
            for (int i = fromIndex; i > toIndex; --i)
                slots[(size_t) i] = slots[(size_t) i - 1];
        }

        slots[(size_t) toIndex] = movedSlot;
    }

    std::array<SlotSnapshot, slotCount> afterParams;

    if (fromIndex < toIndex)
    {
        for (int i = fromIndex; i < toIndex; ++i)
            afterParams[(size_t) i] = beforeParams[(size_t) i + 1];
    }
    else
    {
        for (int i = fromIndex; i > toIndex; --i)
            afterParams[(size_t) i] = beforeParams[(size_t) i - 1];
    }

    afterParams[(size_t) toIndex] = beforeParams[(size_t) fromIndex];

    const int lo = juce::jmin(fromIndex, toIndex);
    const int hi = juce::jmax(fromIndex, toIndex);

    for (int i = lo; i <= hi; ++i)
    {
        applySlotParameterValues(i, afterParams[(size_t) i]);
    }

    updateReportedLatency();

    if (recordUndo)
    {
        ChainEditRecord record;
        record.kind = ChainEditKind::reorder;
        record.description =
            "Move Slot " + juce::String(fromIndex + 1)
            + " to Slot " + juce::String(toIndex + 1);
        record.fromIndex = fromIndex;
        record.toIndex = toIndex;
        pushChainEdit(std::move(record));
    }
}

bool RuFTraXLogicChainerAudioProcessor::
canUndoChainEdit() const
{
    return !undoStack.empty();
}

bool RuFTraXLogicChainerAudioProcessor::
canRedoChainEdit() const
{
    return !redoStack.empty();
}

juce::String RuFTraXLogicChainerAudioProcessor::
getUndoDescription() const
{
    return undoStack.empty() ? juce::String() : undoStack.back().description;
}

juce::String RuFTraXLogicChainerAudioProcessor::
getRedoDescription() const
{
    return redoStack.empty() ? juce::String() : redoStack.back().description;
}

void RuFTraXLogicChainerAudioProcessor::
undoChainEdit()
{
    if (undoStack.empty())
        return;

    ChainEditRecord record = undoStack.back();
    undoStack.pop_back();

    if (record.kind == ChainEditKind::reorder)
        moveSlot(record.toIndex, record.fromIndex, false);
    else
        restoreSlotFromSnapshot(record.slotIndex, record.before, false, nullptr);

    redoStack.push_back(record);

    if (redoStack.size() > maxUndoDepth)
        redoStack.erase(redoStack.begin());
}

void RuFTraXLogicChainerAudioProcessor::
redoChainEdit()
{
    if (redoStack.empty())
        return;

    ChainEditRecord record = redoStack.back();
    redoStack.pop_back();

    if (record.kind == ChainEditKind::reorder)
        moveSlot(record.fromIndex, record.toIndex, false);
    else
        restoreSlotFromSnapshot(record.slotIndex, record.after, false, nullptr);

    undoStack.push_back(record);
}

juce::File RuFTraXLogicChainerAudioProcessor::
getChainPresetsDirectory() const
{
    auto directory =
        juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("RuFTraX")
            .getChildFile("Plug-In Chainer Chains");

    directory.createDirectory();
    return directory;
}

bool RuFTraXLogicChainerAudioProcessor::
saveChainPreset(const juce::String& name)
{
    const auto trimmedName = name.trim();

    if (trimmedName.isEmpty())
        return false;

    const auto file =
        getChainPresetsDirectory().getChildFile(
            juce::File::createLegalFileName(trimmedName)
                + ".rfxchain");

    juce::MemoryBlock state;
    getStateInformation(state);

    return file.replaceWithData(state.getData(), state.getSize());
}

void RuFTraXLogicChainerAudioProcessor::
loadChainPreset(const juce::File& file, std::function<void()> onComplete)
{
    if (!file.existsAsFile())
    {
        if (onComplete)
            onComplete();
        return;
    }

    juce::MemoryBlock state;

    if (!file.loadFileAsData(state))
    {
        if (onComplete)
            onComplete();
        return;
    }

    setStateInformation(state.getData(), (int) state.getSize());

    if (onComplete)
        onComplete();
}

bool RuFTraXLogicChainerAudioProcessor::
deleteChainPreset(const juce::String& name)
{
    const auto file =
        getChainPresetsDirectory().getChildFile(
            juce::File::createLegalFileName(name) + ".rfxchain");

    return !file.existsAsFile() || file.deleteFile();
}

juce::StringArray RuFTraXLogicChainerAudioProcessor::
getSavedChainPresetNames() const
{
    juce::StringArray names;

    for (const auto& file :
         getChainPresetsDirectory().findChildFiles(
             juce::File::findFiles, false, "*.rfxchain"))
    {
        names.add(file.getFileNameWithoutExtension());
    }

    names.sort(true);
    return names;
}

namespace
{
// One slot of a built-in factory chain: enough to build a PluginDescription
// that JUCE's AudioUnitPluginFormat can resolve directly, without browsing
// the installed-plug-in menu. componentSubType/componentManufacturer are
// exact four-character codes (including any trailing spaces a short code
// pads with) read from this Mac's own installed Audio Unit registry.
struct FactoryChainSlot
{
    const char* pluginName;
    const char* manufacturerName;
    bool isMusicEffect;
    const char* componentSubType;
    const char* componentManufacturer;
    float mix;
    float gain;
    float pan;
};

struct FactoryChain
{
    const char* name;
    std::vector<FactoryChainSlot> slots;
};

const std::vector<FactoryChain>& getFactoryChains()
{
    static const std::vector<FactoryChain> chains
    {
        {
            "Lead Vocal",
            {
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "soothe2",       "oeksound",      false, "SthB", "OekS", 1.0f, 0.0f, 0.0f },
                { "DeEsser (s)",   "Waves",         false, "DSAS", "ksWV", 1.0f, 0.0f, 0.0f },
                { "White 2A",      "IK Multimedia", false, "Tr16", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Black 76",      "IK Multimedia", false, "Tr15", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "EQP-1A",        "IK Multimedia", false, "Tr09", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Tape Machine 440", "IK Multimedia", false, "Tr40", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "SSL Native FlexVerb", "SSL",      false, "SNFV", "SSLN", 0.18f, 0.0f, 0.0f },
            }
        },
        {
            "Background Vocals",
            {
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "DeEsser (s)",   "Waves",         false, "DSAS", "ksWV", 1.0f, 0.0f, 0.0f },
                { "Renaissance Axx (s)", "Waves",    false, "RAXS", "ksWV", 1.0f, 0.0f, 0.0f },
                { "Black 76",      "IK Multimedia", false, "Tr15", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "MicroShift",    "Soundtoys",     false, "MCS ", "SToy", 1.0f, 0.0f, 0.0f },
                { "EQ-73",         "IK Multimedia", false, "Tr26", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "SSL Native FlexVerb", "SSL",      false, "SNFV", "SSLN", 0.30f, 0.0f, 0.0f },
            }
        },
        {
            "Kick",
            {
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "Black 76",      "IK Multimedia", false, "Tr15", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Saturator X",   "IK Multimedia", false, "Tr34", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "Stealth Limiter", "IK Multimedia", false, "Tr32", "Ikmm", 1.0f, 0.0f, 0.0f },
            }
        },
        {
            "Snare",
            {
                { "Renaissance Axx (s)", "Waves",    false, "RAXS", "ksWV", 1.0f, 0.0f, 0.0f },
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "Black 76",      "IK Multimedia", false, "Tr15", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Decapitator",   "Soundtoys",     false, "DEC ", "SToy", 1.0f, 0.0f, 0.0f },
                { "CSR Room",      "IK Multimedia", false, "Tr20", "Ikmm", 0.20f, 0.0f, 0.0f },
                { "Stealth Limiter", "IK Multimedia", false, "Tr32", "Ikmm", 1.0f, 0.0f, 0.0f },
            }
        },
        {
            "Bass",
            {
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "Opto Comp",     "IK Multimedia", false, "Tr07", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Bass One",      "IK Multimedia", false, "Tr58", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "Stealth Limiter", "IK Multimedia", false, "Tr32", "Ikmm", 1.0f, 0.0f, 0.0f },
            }
        },
        {
            "Lead Instrument",
            {
                { "Pro-Q 4",       "FabFilter",     true,  "FQ4p", "FabF", 1.0f, 0.0f, 0.0f },
                { "Opto Comp",     "IK Multimedia", false, "Tr07", "Ikmm", 1.0f, 0.0f, 0.0f },
                { "Decapitator",   "Soundtoys",     false, "DEC ", "SToy", 1.0f, 0.0f, 0.0f },
                { "TB_Enhancer_v1", "ToneBoosters",  false, "T1EN", "TONB", 1.0f, 0.0f, 0.0f },
                { "SSL Native FlexVerb", "SSL",      false, "SNFV", "SSLN", 0.20f, 0.0f, 0.0f },
            }
        },
    };

    return chains;
}
}

juce::StringArray RuFTraXLogicChainerAudioProcessor::
getFactoryChainNames() const
{
    juce::StringArray names;

    for (const auto& chain : getFactoryChains())
        names.add(chain.name);

    return names;
}

void RuFTraXLogicChainerAudioProcessor::
loadFactoryChain(int chainIndex, std::function<void()> onComplete)
{
    const auto& chains = getFactoryChains();

    if (!juce::isPositiveAndBelow(chainIndex, (int) chains.size()))
    {
        if (onComplete)
            onComplete();
        return;
    }

    const auto& chain = chains[(size_t) chainIndex];

    // Clear every slot first so a shorter factory chain does not leave a
    // stale plug-in behind from whatever was loaded before it. Latency
    // notifications are suppressed here (see below) and folded into one
    // update after the whole chain has finished loading.
    for (int slot = 0; slot < slotCount; ++slot)
        clearSlot(slot, false, false);

    const int slotsToLoad =
        juce::jmin((int) chain.slots.size(), slotCount);

    if (slotsToLoad <= 0)
    {
        updateReportedLatency();
        if (onComplete)
            onComplete();
        return;
    }

    // Audio Unit instantiation is not guaranteed reentrant across different
    // vendors: several of these plug-ins share licensing/init code (iLok
    // and similar) that is not safe to run concurrently on more than one
    // background thread at once. Firing off all of a chain's loads
    // simultaneously was found to reliably crash the audio thread later,
    // inside AudioUnitRender, on real third-party plug-ins. Loading one
    // slot fully before starting the next avoids that - the chain takes a
    // little longer to fully appear, but each instantiation is isolated.
    //
    // Each setLatencySamples() call can make the host (including our own
    // Standalone target) tear down and rebuild its audio callback graph.
    // Calling that once per slot while a dozen more slots are still being
    // cleared and loaded raced prepareToPlay()/releaseResources() against
    // this function badly enough to corrupt the heap and crash later inside
    // an unrelated plug-in's render call - notifyLatency=false keeps every
    // intermediate step quiet, and updateReportedLatency() is called
    // exactly once, after the whole chain has finished.
    auto loadNext =
        std::make_shared<std::function<void(int)>>();

    *loadNext =
        [this, &chain, slotsToLoad, onComplete, loadNext](int slot)
        {
            if (slot >= slotsToLoad)
            {
                updateReportedLatency();
                if (onComplete)
                    onComplete();
                return;
            }

            const auto& slotDef = chain.slots[(size_t) slot];

            juce::PluginDescription description;
            description.name = slotDef.pluginName;
            description.descriptiveName = slotDef.pluginName;
            description.manufacturerName = slotDef.manufacturerName;
            description.pluginFormatName = "AudioUnit";
            description.category =
                slotDef.isMusicEffect ? "MusicEffect" : "Effect";
            description.fileOrIdentifier =
                juce::String("AudioUnit:")
                + (slotDef.isMusicEffect ? "MusicEffect/" : "Effect/")
                + (slotDef.isMusicEffect ? "aumf" : "aufx")
                + "," + juce::String(slotDef.componentSubType)
                + "," + juce::String(slotDef.componentManufacturer);
            description.isInstrument = false;
            description.numInputChannels = 2;
            description.numOutputChannels = 2;
            description.uniqueId = description.fileOrIdentifier.hashCode();
            description.deprecatedUid = description.uniqueId;
            description.lastInfoUpdateTime = juce::Time::getCurrentTime();

            const float mix = slotDef.mix;
            const float gain = slotDef.gain;
            const float pan = slotDef.pan;

            loadAudioUnitInSlot(
                slot,
                description,
                [this, slot, mix, gain, pan, loadNext]
                (bool success, const juce::String&)
                {
                    if (success)
                    {
                        setFloatParameter(slotParameterID(slot, "Mix"), mix);
                        setFloatParameter(slotParameterID(slot, "Gain"), gain);
                        setFloatParameter(slotParameterID(slot, "Pan"), pan);
                    }

                    (*loadNext)(slot + 1);
                },
                false,
                false);
        };

    (*loadNext)(0);
}

bool RuFTraXLogicChainerAudioProcessor::
isSidechainConnected() const
{
    return getBusCount(true) > 1
        && getBus(true, 1) != nullptr
        && getBus(true, 1)->isEnabled();
}

void RuFTraXLogicChainerAudioProcessor::
setTestToneEnabled(bool shouldPlay)
{
    testToneEnabled.store(shouldPlay, std::memory_order_relaxed);
}

bool RuFTraXLogicChainerAudioProcessor::
isTestToneEnabled() const noexcept
{
    return testToneEnabled.load(std::memory_order_relaxed);
}

float RuFTraXLogicChainerAudioProcessor::
getInputLevel() const noexcept
{
    return inputPeakLevel.load(std::memory_order_relaxed);
}

float RuFTraXLogicChainerAudioProcessor::
getOutputLevel() const noexcept
{
    return outputPeakLevel.load(std::memory_order_relaxed);
}

void RuFTraXLogicChainerAudioProcessor::
setBoolParameter(
    const juce::String& parameterID,
    bool value)
{
    if (auto* parameter =
            apvts.getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(
            value ? 1.0f : 0.0f);
    }
}

void RuFTraXLogicChainerAudioProcessor::
setFloatParameter(
    const juce::String& parameterID,
    float value)
{
    if (auto* parameter = apvts.getParameter(parameterID))
    {
        const auto range = apvts.getParameterRange(parameterID);
        parameter->setValueNotifyingHost(range.convertTo0to1(value));
    }
}

juce::String RuFTraXLogicChainerAudioProcessor::
slotParameterID(
    int slotIndex,
    const juce::String& suffix) const
{
    return "slot"
         + juce::String(slotIndex + 1)
         + suffix;
}

SlotSnapshot RuFTraXLogicChainerAudioProcessor::
captureSlotSnapshot(int slotIndex) const
{
    SlotSnapshot snapshot;

    if (!juce::isPositiveAndBelow(slotIndex, slotCount))
        return snapshot;

    snapshot.enabled = isSlotEnabled(slotIndex);
    snapshot.bypass = isSlotBypassed(slotIndex);
    snapshot.mix =
        apvts.getRawParameterValue(slotParameterID(slotIndex, "Mix"))->load();
    snapshot.gain =
        apvts.getRawParameterValue(slotParameterID(slotIndex, "Gain"))->load();
    snapshot.pan =
        apvts.getRawParameterValue(slotParameterID(slotIndex, "Pan"))->load();

    {
        const juce::ScopedLock lock(slotInfoLock);
        const auto& slot = slots[(size_t) slotIndex];
        snapshot.hasDescription = slot.hasDescription;
        snapshot.description = slot.description;
    }

    HostedPluginPtr plugin;
    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
        plugin = hostedPlugins[(size_t) slotIndex];
    }

    if (plugin != nullptr)
        plugin->getStateInformation(snapshot.pluginState);

    return snapshot;
}

void RuFTraXLogicChainerAudioProcessor::
applySlotParameterValues(int slotIndex, const SlotSnapshot& snapshot)
{
    setBoolParameter(slotParameterID(slotIndex, "Enabled"), snapshot.enabled);
    setBoolParameter(slotParameterID(slotIndex, "Bypass"), snapshot.bypass);
    setFloatParameter(slotParameterID(slotIndex, "Mix"), snapshot.mix);
    setFloatParameter(slotParameterID(slotIndex, "Gain"), snapshot.gain);
    setFloatParameter(slotParameterID(slotIndex, "Pan"), snapshot.pan);
}

void RuFTraXLogicChainerAudioProcessor::
restoreSlotFromSnapshot(
    int slotIndex,
    const SlotSnapshot& snapshot,
    bool recordUndo,
    std::function<void()> onComplete)
{
    if (!snapshot.hasDescription)
    {
        clearSlot(slotIndex, recordUndo);
        if (onComplete)
            onComplete();
        return;
    }

    bool sameIdentity = false;
    {
        const juce::ScopedLock lock(slotInfoLock);
        const auto& current = slots[(size_t) slotIndex];
        sameIdentity =
            current.hasDescription
            && current.description.fileOrIdentifier
                == snapshot.description.fileOrIdentifier;
    }

    if (sameIdentity)
    {
        applyHostedPluginState(slotIndex, snapshot.pluginState);
        applySlotParameterValues(slotIndex, snapshot);
        if (onComplete)
            onComplete();
        return;
    }

    loadAudioUnitInSlot(
        slotIndex,
        snapshot.description,
        [this, slotIndex, snapshot, onComplete]
        (bool success, const juce::String&)
        {
            if (success)
            {
                applyHostedPluginState(slotIndex, snapshot.pluginState);
                applySlotParameterValues(slotIndex, snapshot);
            }

            if (onComplete)
                onComplete();
        },
        recordUndo);
}

void RuFTraXLogicChainerAudioProcessor::
pushChainEdit(ChainEditRecord record)
{
    undoStack.push_back(std::move(record));

    if (undoStack.size() > maxUndoDepth)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

bool RuFTraXLogicChainerAudioProcessor::
isSlotActiveForRouting(int slotIndex) const
{
    if (!juce::isPositiveAndBelow(slotIndex, slotCount))
        return false;

    if (slotProcessingFaults[(size_t) slotIndex].load())
        return false;

    return isSlotEnabled(slotIndex) && !isSlotBypassed(slotIndex);
}

void RuFTraXLogicChainerAudioProcessor::
updateReportedLatency()
{
    int totalLatency = 0;
    double maxTail = 0.0;

    std::array<HostedPluginPtr, slotCount> snapshot;
    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
        snapshot = hostedPlugins;
    }

    for (int i = 0; i < slotCount; ++i)
    {
        if (!isSlotActiveForRouting(i))
            continue;

        if (auto* plugin = snapshot[(size_t) i].get())
        {
            totalLatency += plugin->getLatencySamples();
            maxTail = juce::jmax(maxTail, plugin->getTailLengthSeconds());
        }
    }

    lastKnownTailSeconds.store(maxTail, std::memory_order_relaxed);
    setLatencySamples(totalLatency);
}

double RuFTraXLogicChainerAudioProcessor::
getTailLengthSeconds() const
{
    return lastKnownTailSeconds.load(std::memory_order_relaxed);
}

void RuFTraXLogicChainerAudioProcessor::
getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::ValueTree slotTree("Slots");

    // Snapshot the hosted plug-in pointers under a brief lock, then read
    // each one's state outside the lock. This guarantees every loaded
    // plug-in's state is captured even if the audio thread is mid-block
    // when a save happens, rather than silently skipping every slot on a
    // single failed try-lock.
    std::array<HostedPluginPtr, slotCount> pluginsSnapshot;
    {
        const juce::SpinLock::ScopedLockType lock(hostedPluginLock);
        pluginsSnapshot = hostedPlugins;
    }

    for (int slotIndex = 0;
         slotIndex < slotCount;
         ++slotIndex)
    {
        juce::ValueTree slotState("Slot");
        slotState.setProperty(
            "index",
            slotIndex,
            nullptr);

        {
            const juce::ScopedLock lock(
                slotInfoLock);

            const auto& slot =
                slots[(size_t) slotIndex];

            slotState.setProperty(
                "name",
                slot.displayName,
                nullptr);

            if (slot.hasDescription)
            {
                if (auto xml =
                        slot.description.createXml())
                {
                    slotState.setProperty(
                        "descriptionXml",
                        xml->toString(
                            juce::XmlElement::
                                TextFormat()
                                    .singleLine()),
                        nullptr);
                }
            }
        }

        if (auto* plugin = pluginsSnapshot[(size_t) slotIndex].get())
        {
            juce::MemoryBlock pluginState;
            plugin->getStateInformation(
                pluginState);

            slotState.setProperty(
                "pluginState",
                pluginState.toBase64Encoding(),
                nullptr);
        }

        slotTree.addChild(
            slotState,
            -1,
            nullptr);
    }

    state.addChild(
        slotTree,
        -1,
        nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void RuFTraXLogicChainerAudioProcessor::
setStateInformation(
    const void* data,
    int sizeInBytes)
{
    auto xmlState =
        getXmlFromBinary(
            data,
            sizeInBytes);

    if (xmlState == nullptr
        || !xmlState->hasTagName(
            apvts.state.getType()))
        return;

    auto state =
        juce::ValueTree::fromXml(
            *xmlState);

    auto slotsTree =
        state.getChildWithName("Slots");

    if (slotsTree.isValid())
        state.removeChild(
            slotsTree,
            nullptr);

    apvts.replaceState(state);
    globalBypassRequested.store(
        apvts.getRawParameterValue("bypassAll")->load()
            >= 0.5f,
        std::memory_order_relaxed);

    // A full state restore is not something a user would expect to "undo"
    // back through - start the undo history fresh from here.
    undoStack.clear();
    redoStack.clear();

    if (!slotsTree.isValid())
        return;

    // Gather every populated slot first (cheap, synchronous ValueTree
    // reads), then restore them one at a time. As with loadFactoryChain,
    // firing off several loadAudioUnitInSlot() calls at once is not safe -
    // different vendors' Audio Units can share non-reentrant licensing/init
    // code, and each restored slot's setLatencySamples() can make the host
    // rebuild its audio callback graph, which raced against a still
    // in-flight restore badly enough to corrupt memory. This path runs on
    // every project/session open with two or more saved slots, so it is not
    // just a factory-chain concern.
    struct PendingSlot
    {
        int index;
        juce::PluginDescription description;
        juce::String encodedState;
    };

    auto pending = std::make_shared<std::vector<PendingSlot>>();

    for (int child = 0;
         child < slotsTree.getNumChildren();
         ++child)
    {
        auto slotState =
            slotsTree.getChild(child);

        const int slotIndex =
            (int) slotState.getProperty(
                "index",
                -1);

        if (!juce::isPositiveAndBelow(
                slotIndex,
                slotCount))
            continue;

        const auto descriptionText =
            slotState.getProperty(
                "descriptionXml")
                .toString();

        if (descriptionText.isEmpty())
            continue;

        auto descriptionXml =
            juce::parseXML(
                descriptionText);

        if (descriptionXml == nullptr)
            continue;

        juce::PluginDescription description;

        if (!description.loadFromXml(
                *descriptionXml))
            continue;

        pending->push_back(
            { slotIndex,
              description,
              slotState.getProperty("pluginState").toString() });
    }

    auto restoreNext =
        std::make_shared<std::function<void(size_t)>>();

    *restoreNext =
        [this, pending, restoreNext](size_t index)
        {
            if (index >= pending->size())
            {
                updateReportedLatency();
                return;
            }

            const auto& item = (*pending)[index];

            loadAudioUnitInSlot(
                item.index,
                item.description,
                [this, item, restoreNext, index]
                (bool success, const juce::String&)
                {
                    if (success && item.encodedState.isNotEmpty())
                    {
                        juce::MemoryBlock pluginState;

                        if (pluginState.fromBase64Encoding(
                                item.encodedState))
                        {
                            applyHostedPluginState(
                                item.index,
                                pluginState);
                        }
                    }

                    (*restoreNext)(index + 1);
                },
                false,
                false);
        };

    (*restoreNext)(0);
}

juce::AudioProcessorEditor*
RuFTraXLogicChainerAudioProcessor::createEditor()
{
    return new
        RuFTraXLogicChainerAudioProcessorEditor(
            *this);
}

juce::AudioProcessor* JUCE_CALLTYPE
createPluginFilter()
{
    return new
        RuFTraXLogicChainerAudioProcessor();
}
