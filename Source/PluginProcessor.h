#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

struct ChainSlot
{
    bool enabled = false;
    juce::String displayName { "<Empty Slot>" };
    juce::PluginDescription description;
    bool hasDescription = false;
};

// A complete, self-contained record of one slot: identity, mixer settings and
// the hosted plug-in's own serialized parameter state. This is everything
// needed to recreate a slot from nothing, so it is the unit both chain
// presets and undo/redo are built from.
struct SlotSnapshot
{
    bool enabled = false;
    bool bypass = false;
    float mix = 1.0f;
    float gain = 0.0f;
    float pan = 0.0f;
    bool hasDescription = false;
    juce::PluginDescription description;
    juce::MemoryBlock pluginState;
};

class RuFTraXLogicChainerAudioProcessor final
    : public juce::AudioProcessor,
      private juce::Thread,
      private juce::AudioProcessorValueTreeState::Listener
{
public:
    using LoadCallback =
        std::function<void(bool success, const juce::String& message)>;
    using HostedPluginPtr =
        std::shared_ptr<juce::AudioPluginInstance>;

    static constexpr int slotCount = 8;

    RuFTraXLogicChainerAudioProcessor();
    ~RuFTraXLogicChainerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return "RuFTraX Plug-In Chainer";
    }

    // MIDI is relayed through to hosted Music Effect Audio Units (filters,
    // harmonizers and similar effects that track incoming note data). The
    // chainer itself neither generates nor consumes MIDI.
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void startAudioUnitScan();
    bool isAudioUnitScanFinished() const noexcept;
    bool didAudioUnitScanFail() const noexcept;
    juce::String getAudioUnitScanStatus() const;
    juce::Array<juce::PluginDescription> getAvailableAudioUnits() const;

    void loadAudioUnitInSlot(int slotIndex,
                             const juce::PluginDescription& description,
                             LoadCallback callback,
                             bool recordUndo = true,
                             bool notifyLatency = true);

    void clearSlot(int slotIndex,
                   bool recordUndo = true,
                   bool notifyLatency = true);

    bool isSlotEnabled(int slotIndex) const;
    bool isSlotBypassed(int slotIndex) const;
    bool isSlotFaulted(int slotIndex) const;
    float getSlotCpuUsage(int slotIndex) const;
    int getSlotLatencySamples(int slotIndex) const;
    void retrySlot(int slotIndex);

    bool isGlobalBypassed() const;
    void setGlobalBypassed(bool shouldBypass);
    juce::String getSlotDisplayName(int slotIndex) const;
    HostedPluginPtr getHostedPluginHandle(int slotIndex) const;
    bool applyHostedPluginState(int slotIndex,
                                const juce::MemoryBlock& state);

    // Drag-and-drop reordering. Moves everything about a slot - the live
    // plug-in instance, its buffers and its mixer parameter values - to a
    // new position without touching the plug-in itself, so reordering never
    // triggers a reload.
    void moveSlot(int fromIndex, int toIndex, bool recordUndo = true);

    // Chain-edit undo/redo. Scoped to structural edits (insert, remove,
    // reorder) rather than individual parameter automation, which the host
    // already manages its own way.
    bool canUndoChainEdit() const;
    bool canRedoChainEdit() const;
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    void undoChainEdit();
    void redoChainEdit();

    // Chain presets: the whole 8-slot rack, saved to and loaded from a
    // single named file so a rack can be recalled or shared.
    juce::File getChainPresetsDirectory() const;
    bool saveChainPreset(const juce::String& name);
    void loadChainPreset(const juce::File& file,
                         std::function<void()> onComplete = nullptr);
    bool deleteChainPreset(const juce::String& name);
    juce::StringArray getSavedChainPresetNames() const;

    // Built-in factory chains: pristine known-good starting racks for common
    // sources, assembled from the real Audio Units installed on this Mac.
    // Baked into the binary rather than a saved file, so they are always
    // available on a fresh install.
    juce::StringArray getFactoryChainNames() const;
    void loadFactoryChain(int chainIndex, std::function<void()> onComplete = nullptr);

    bool isSidechainConnected() const;

    void setTestToneEnabled(bool shouldPlay);
    bool isTestToneEnabled() const noexcept;

    // Linear-gain peak level of the signal entering and leaving the chain,
    // updated every block. The editor polls these for the front-panel
    // Input/Output meters.
    float getInputLevel() const noexcept;
    float getOutputLevel() const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    struct AsyncLifetime
    {
        juce::CriticalSection lock;
        bool alive = true;
    };

    enum ScanState
    {
        scanNotStarted = 0,
        scanRunning,
        scanReady,
        scanFailed
    };

    enum class ChainEditKind
    {
        insert,
        remove,
        reorder
    };

    struct ChainEditRecord
    {
        ChainEditKind kind;
        juce::String description;
        int slotIndex = -1;
        SlotSnapshot before, after;
        int fromIndex = -1, toIndex = -1;
    };

    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    void run() override;
    void parameterChanged(const juce::String& parameterID,
                          float newValue) override;
    void setScanStatus(ScanState state, const juce::String& message);

    void setBoolParameter(const juce::String& parameterID, bool value);
    void setFloatParameter(const juce::String& parameterID, float value);
    juce::String slotParameterID(int slotIndex,
                                 const juce::String& suffix) const;

    bool prepareHostedPlugin(juce::AudioPluginInstance&,
                             juce::AudioBuffer<float>& scratchBuffer,
                             juce::String& errorMessage);
    void processHostedChain(juce::AudioBuffer<float>& mainBuffer,
                            const juce::AudioBuffer<float>* sidechainBuffer,
                            juce::MidiBuffer&);

    void updateReportedLatency();
    bool isSlotActiveForRouting(int slotIndex) const;
    void decaySlotCpuUsage(int slotIndex) noexcept;

    SlotSnapshot captureSlotSnapshot(int slotIndex) const;
    void applySlotParameterValues(int slotIndex, const SlotSnapshot& snapshot);
    void restoreSlotFromSnapshot(int slotIndex,
                                 const SlotSnapshot& snapshot,
                                 bool recordUndo,
                                 std::function<void()> onComplete);
    void pushChainEdit(ChainEditRecord record);

    void injectTestTone(juce::AudioBuffer<float>& mainBuffer);

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;

    mutable juce::CriticalSection pluginListLock;
    mutable juce::CriticalSection slotInfoLock;
    mutable juce::SpinLock hostedPluginLock;

    std::array<ChainSlot, slotCount> slots;
    std::array<HostedPluginPtr, slotCount> hostedPlugins;
    std::array<juce::AudioBuffer<float>, slotCount> pluginBuffers;
    std::array<std::atomic<std::uint64_t>, slotCount>
        slotLoadGenerations {};
    std::array<std::atomic<bool>, slotCount>
        slotProcessingFaults {};
    std::array<std::atomic<float>, slotCount>
        slotCpuUsage {};

    juce::AudioBuffer<float> dryBuffer;
    std::atomic<bool> globalBypassRequested { false };

    std::atomic<int> scanState { scanNotStarted };
    juce::String scanStatus { "Audio Unit scan has not started." };

    std::atomic<bool> shuttingDown { false };
    std::shared_ptr<AsyncLifetime> asyncLifetime =
        std::make_shared<AsyncLifetime>();

    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<int> currentBlockSize { 512 };
    std::atomic<bool> isPrepared { false };
    mutable std::atomic<double> lastKnownTailSeconds { 0.0 };

    std::atomic<bool> testToneEnabled { false };
    double testTonePhase = 0.0;

    std::atomic<float> inputPeakLevel { 0.0f };
    std::atomic<float> outputPeakLevel { 0.0f };

    std::vector<ChainEditRecord> undoStack;
    std::vector<ChainEditRecord> redoStack;
    static constexpr size_t maxUndoDepth = 50;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        RuFTraXLogicChainerAudioProcessor)
};
