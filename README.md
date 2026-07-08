# RuFTraX Logic Chainer — AU Starter

This is a Mac-first JUCE starter for a Logic Pro version of RuFTraX Chainer.

## What this version is
- Audio Unit / AU target for Logic Pro
- VST3 and Standalone targets for testing
- SSL-console-style interface scaffold
- 8 chain slots UI
- Input/output gain and bypass-all controls
- State-save scaffolding

## Current limitation
This starter is a clean, buildable pass-through AU scaffold. The next engineering step is adding child plug-in hosting using JUCE `AudioPluginFormatManager`, `KnownPluginList`, and `AudioPluginInstance`.

For Logic Pro, the most natural host format is AU. Hosting AU inside an AU is possible technically, but has sandboxing, validation, UI, scanning, and reliability issues. A safer first production version is:

1. RuFTraX Chainer AU loads built-in RuFTraX DSP modules, or
2. RuFTraX Chainer Standalone/VST3 hosts AU/VST3, or
3. RuFTraX Chainer AU uses a curated internal module chain first, then external AU hosting later.

## Build
```bash
cmake -B build -S . -DJUCE_DIR=/Users/mike/JUCE
cmake --build build --config Release
```

The AU component should be copied after build when JUCE is configured correctly. If not, copy it to:

`~/Library/Audio/Plug-Ins/Components/`

Then run:

```bash
auval -v aufx RfLC RfTx
```

Open Logic Pro and rescan Audio Units if needed.

## Suggested next features
- Actual AU plug-in scanner
- Load AU plug-in per slot
- Open child plug-in editor window
- Latency compensation
- Chain preset files
- Parallel routing
- Wet/dry per slot
- SSL channel-strip module mode
