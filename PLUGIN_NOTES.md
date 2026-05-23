# Live ChucK AU plugins

This repo now builds two macOS Audio Unit plugins for `matd.space`:

- `Live ChucK`: an audio effect AU (`aufx/LChk/MtDs`) that embeds the Weld ChucK engine and supports mono or stereo host layouts.
- `Live ChucK MIDI FX`: a MIDI FX AU (`aumi/LcMF/MtDs`) with a live code editor and MIDI note pattern output.

Build both plugins:

```sh
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DJUCE_PATH=/Users/user/Documents/Fabric/JUCE
cmake --build build --config Release --target MatDLiveChucK_AU MatDLiveChucKMidiFX_AU -j4
```

The build copies the AU bundles into:

```text
~/Library/Audio/Plug-Ins/Components/Live ChucK.component
~/Library/Audio/Plug-Ins/Components/Live ChucK MIDI FX.component
```

Validate them with:

```sh
auval -v aufx LChk MtDs
auval -v aumi LcMF MtDs
```

The audio plugin compiles ChucK code through the embedded engine. The MIDI FX plugin is currently a lightweight MIDI pattern processor because the vendored Weld ChucK engine is built with ChucK MIDI disabled. Its editor accepts one command per line:

```text
note <pitch> <velocity> <lengthMs> <periodMs>
```

Example:

```text
note 60 96 120 500
note 67 72 90 1000
```
