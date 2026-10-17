# Working on this repository

## Every app has an icon - always, no exceptions

The owner's standing rule (2026-10-17): every RuFTraX app ships with its own icon - every app, every build target that
makes an app (a Standalone counts), every build the owner installs. A build without its icon isn't finished.

- **Where icons come from:** the RuFTraX icon standard, `Assets/Artwork/ruftrax_icons.py` in the RuFTraX-TraX repository
  (ruftrax/RuFTraX-TraX). It draws every app's icon in one family - Apple's icon grid, a brushed-graphite tile, the app's
  own symbol in its own colours (the disk imagers keep their dark-green line-drawn look). A new app gets an entry there - a
  symbol that says what it does, in colours no other app uses - and its icon before its first build.
- **Here:** `Assets/AppIcon.png` (1024 px) and `Assets/AppIconSmall.png` (32 px - the bolder drawing for
  16-32 px; keep it 32 px: JUCE uses each picture at its own size), given to `juce_add_plugin` as `ICON_BIG` / `ICON_SMALL`.
  The SVG sources are in `Assets/Artwork/`.
  (The `RuFTraX_Logic_AU_Chainer_v*` folders are earlier versions kept for reference; if one is ever built and
  installed again, it gets the icon too.)
- **Check it** before calling a build done: on the Mac the built `.app` has `Contents/Resources/AppIcon.icns` (or the
  icon file its Info.plist names), and Finder shows the icon.
