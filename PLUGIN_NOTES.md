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
- A status console: shows full ChucK compiler errors and file messages.
- `Examples`: load a built-in starter or arpeggio into the editor without applying it.
- `Load`: load a plain text code example into the editor without applying it.
- `Save`: save the current editor text as a reusable code example.
- `Reset`: restore the built-in starter program.
- `Stop`, `Restart`, `Panic`: stop code, restart the current code, or reset audio/MIDI output.
- `Gain`, `Ctrl 1`, `Ctrl 2`, `Ctrl 3`: automatable sliders that map to ChucK globals.

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
hostPpq                // host song position in quarter notes
hostBeat               // beat offset from the current bar start
hostBar                // host bar count when available
hostSampleRate         // current audio sample rate
hostTimeSeconds        // host timeline position in seconds
hostTimeSamples        // host timeline position in samples
hostTimeSigNumerator
hostTimeSigDenominator
hostParamGain          // Logic-automatable ChucK Gain parameter
hostParam1             // Logic-automatable ChucK Control 1
hostParam2             // Logic-automatable ChucK Control 2
hostParam3             // Logic-automatable ChucK Control 3
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

The MIDI FX plugin also follows the host transport and stops pending notes when playback stops. It accepts one command per line:

```text
note <pitch> <velocity> <lengthMs> <periodMs>
arp <comma-separated-pitches> <velocity> <lengthBeats> <stepBeats>
variation <startBeats> <durationBeats> <pitches> <velocity> <lengthBeats> <stepBeats>
```

`note` is a fixed millisecond note repeater. `arp` steps through pitches and follows the host tempo. `variation` cycles timed arp sections over the transport.

Arpeggio examples are included here:

```text
Examples/audio-arpeggio.ck
Examples/beat-gate.ck
Examples/automation-demo.ck
Examples/pulse-garden.ck
Examples/shimmer-pad.ck
Examples/clockwork-lead.ck
Examples/final-arrangement.ck
Examples/midi-fx-arpeggio.txt
Examples/midi-glass-cascade.txt
Examples/midi-pulse-stack.txt
Examples/midi-final-arrangement.txt
```

Paste `audio-arpeggio.ck` into `Live ChucK`. It uses `hostTempo` to calculate sixteenth-note timing.

Paste `midi-fx-arpeggio.txt` into `Live ChucK MIDI FX`, or choose the built-in `C Minor MIDI Arp` example from the Examples menu.

Example:

```text
arp 48,51,55,58,60,58,55,51 96 0.25 0.25
```
