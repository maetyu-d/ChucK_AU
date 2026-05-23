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

The editor has:

- `Apply`: compile/run the current editor text.
- `Load`: load a plain text code example into the editor without applying it.
- `Save`: save the current editor text as a reusable code example.
- `Reset`: restore the built-in starter program.

Validate them with:

```sh
auval -v aufx LChk MtDs
auval -v aumi LcMF MtDs
```

The audio plugin compiles ChucK code through the embedded engine. The MIDI FX plugin is currently a lightweight MIDI pattern processor because the vendored Weld ChucK engine is built with ChucK MIDI disabled. Its editor accepts one command per line:

The audio plugin advances ChucK only while the host transport is playing. It injects these globals into every ChucK program:

```chuck
hostTempo              // current host tempo in BPM
hostTransportPlaying   // 1.0 while the transport is playing, 0.0 while stopped
```

Example:

```chuck
SinOsc osc => dac;

while (true)
{
    hostTempo * 2.0 => osc.freq;
    10::ms => now;
}
```

The MIDI FX plugin also follows the host transport and stops pending notes when playback stops.

Arpeggio examples are included here:

```text
Examples/audio-arpeggio.ck
Examples/midi-fx-arpeggio.txt
```

Paste `audio-arpeggio.ck` into `Live ChucK`. It uses `hostTempo` to calculate sixteenth-note timing.

Paste `midi-fx-arpeggio.txt` into `Live ChucK MIDI FX`. Its current mini-language uses fixed millisecond note periods.

```text
note <pitch> <velocity> <lengthMs> <periodMs>
```

Example:

```text
note 60 96 120 500
note 67 72 90 1000
```
