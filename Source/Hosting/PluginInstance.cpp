#include "PluginInstance.h"

namespace
{
class HostedEditorWindow final : public juce::DocumentWindow
{
public:
    HostedEditorWindow(const juce::String& title,
                       juce::AudioProcessorEditor* editor)
        : juce::DocumentWindow(title,
                               juce::Colour(28, 28, 28),
                               juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setContentOwned(editor, true);

        const int width  = juce::jmax(320, editor->getWidth());
        const int height = juce::jmax(200, editor->getHeight());

        centreWithSize(width, height);
        setVisible(true);
        toFront(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};
}

PluginInstance::~PluginInstance()
{
    clear();
}

void PluginInstance::setDisplayName(const juce::String& name)
{
    displayName = name;
}

juce::String PluginInstance::getDisplayName() const
{
    return displayName;
}

void PluginInstance::setEnabled(bool shouldBeEnabled)
{
    enabled = shouldBeEnabled;
}

bool PluginInstance::isEnabled() const
{
    return enabled;
}

void PluginInstance::setHostedPlugin(
    std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    clear();

    hostedPlugin = std::move(plugin);
    enabled = hostedPlugin != nullptr;

    if (hostedPlugin != nullptr)
        displayName = hostedPlugin->getName();
}

bool PluginInstance::hasHostedPlugin() const
{
    return hostedPlugin != nullptr;
}

bool PluginInstance::isProcessingHostedPlugin() const
{
    return enabled && hostedPlugin != nullptr;
}

void PluginInstance::prepare(double sampleRate, int blockSize)
{
    if (hostedPlugin != nullptr)
        hostedPlugin->prepareToPlay(sampleRate, blockSize);
}

void PluginInstance::process(juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midi)
{
    if (enabled && hostedPlugin != nullptr)
        hostedPlugin->processBlock(buffer, midi);
}

bool PluginInstance::openEditorWindow(juce::String& errorMessage)
{
    if (hostedPlugin == nullptr)
    {
        errorMessage = "No hosted plug-in is loaded.";
        return false;
    }

    if (editorWindow != nullptr)
    {
        editorWindow->setVisible(true);
        editorWindow->toFront(true);
        return true;
    }

    auto* editor = hostedPlugin->createEditorAndMakeActive();

    if (editor == nullptr)
    {
        errorMessage = "This plug-in did not provide an editor.";
        return false;
    }

    editorWindow =
        std::make_unique<HostedEditorWindow>(displayName, editor);

    return true;
}

void PluginInstance::clear()
{
    editorWindow.reset();

    if (hostedPlugin != nullptr)
        hostedPlugin->releaseResources();

    hostedPlugin.reset();
    displayName = "<Empty Slot>";
    enabled = false;
}
