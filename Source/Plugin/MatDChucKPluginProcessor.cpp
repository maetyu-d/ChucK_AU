#include "MatDChucKPluginProcessor.h"
#include "MatDChucKPluginEditor.h"

#include <cmath>

namespace
{
#if ! MATD_CHUCK_MIDI_FX
juce::String audioDefaultProgram()
{
    return R"chuck(
SinOsc left => Gain master => dac;
TriOsc right => master => dac;

while (true)
{
    Math.max(0.0, Math.min(hostParamGain, 1.0)) => master.gain;
    Math.max(20.0, hostTempo) * 2.0 => float base;
    base => left.freq;
    base * 1.005 => right.freq;
    20::ms => now;
}
)chuck";
}

juce::String audioArpeggioProgram()
{
    return R"chuck(
TriOsc osc => ADSR env => Gain master => dac;

env.set (6::ms, 35::ms, 0.35, 80::ms);

[0, 4, 7, 12, 7, 4] @=> int intervals[];
48 => int root;
0 => int step;

while (true)
{
    Math.max(0.0, Math.min(hostParamGain, 1.0)) => master.gain;
    Std.mtof (root + intervals[step]) => osc.freq;
    env.keyOn();

    (60000.0 / Math.max(20.0, hostTempo) / 4.0)::ms => dur sixteenth;
    sixteenth * 0.55 => now;

    env.keyOff();
    sixteenth * 0.45 => now;

    (step + 1) % intervals.size() => step;
}
)chuck";
}

juce::String audioBeatGateProgram()
{
    return R"chuck(
SawOsc osc => LPF filter => Gain gate => dac;

0.35 => filter.Q;

while (true)
{
    80.0 + hostParam1 * 640.0 => osc.freq;
    300.0 + hostParam2 * 3600.0 => filter.freq;
    if (hostBeat % 1.0 < 0.5) hostParamGain => gate.gain;
    else 0.0 => gate.gain;
    8::ms => now;
}
)chuck";
}

juce::String audioAutomationProgram()
{
    return R"chuck(
SinOsc carrier => Gain out => dac;
SinOsc vibrato => blackhole;

5.0 => vibrato.freq;

while (true)
{
    110.0 + hostParam1 * 880.0 => float base;
    0.0 + hostParam2 * 14.0 => float depth;
    base + vibrato.last() * depth => carrier.freq;
    hostParamGain * (0.4 + hostParam3 * 0.6) => out.gain;
    1::samp => now;
}
)chuck";
}

juce::String audioPulseGardenProgram()
{
    return R"chuck(
SawOsc bass => LPF bassFilter => Gain bassGain => Gain master => dac;
TriOsc chime => ADSR chimeEnv => Gain chimeGain => master;
Noise hat => BPF hatFilter => ADSR hatEnv => Gain hatGain => master;

0.8 => bassFilter.Q;
5200.0 => hatFilter.freq;
7.0 => hatFilter.Q;
chimeEnv.set (5::ms, 60::ms, 0.18, 160::ms);
hatEnv.set (1::ms, 18::ms, 0.0, 12::ms);

[0, 3, 7, 10, 12, 10, 7, 3] @=> int tones[];
0 => int step;

while (true)
{
    36.0 + hostParam1 * 24.0 => float root;
    Std.mtof (root + tones[step]) => bass.freq;
    Std.mtof (root + 24 + tones[(step * 3) % tones.size()]) => chime.freq;
    180.0 + hostParam2 * 4200.0 => bassFilter.freq;
    hostParamGain * 0.75 => master.gain;
    0.35 + hostParam3 * 0.55 => bassGain.gain;
    0.18 + hostParam2 * 0.22 => chimeGain.gain;
    0.11 => hatGain.gain;

    chimeEnv.keyOn();
    if (step % 2 == 0) hatEnv.keyOn();

    (60000.0 / Math.max(20.0, hostTempo) / 4.0)::ms => dur tick;
    tick * 0.45 => now;

    chimeEnv.keyOff();
    hatEnv.keyOff();
    tick * 0.55 => now;

    (step + 1) % tones.size() => step;
}
)chuck";
}

juce::String audioShimmerPadProgram()
{
    return R"chuck(
SinOsc low => Gain lowGain => Gain master => dac;
TriOsc mid => Gain midGain => master;
TriOsc high => Gain highGain => master;
SinOsc lfo => blackhole;

0.05 => lfo.freq;

while (true)
{
    42.0 + hostParam1 * 18.0 => float root;
    Std.mtof (root) => float base;
    base => low.freq;
    base * 1.501 => mid.freq;
    base * 2.006 + lfo.last() * (2.0 + hostParam2 * 18.0) => high.freq;

    hostParamGain * 0.42 => master.gain;
    0.55 => lowGain.gain;
    0.25 + hostParam2 * 0.2 => midGain.gain;
    0.12 + hostParam3 * 0.28 => highGain.gain;
    5::ms => now;
}
)chuck";
}

juce::String audioClockworkLeadProgram()
{
    return R"chuck(
SqrOsc lead => ADSR env => LPF filter => Gain master => dac;

env.set (2::ms, 30::ms, 0.28, 65::ms);
0.45 => filter.Q;

[0, 2, 7, 9, 12, 14, 12, 7, 5, 9, 7, 2] @=> int notes[];
0 => int step;

while (true)
{
    48.0 + hostParam1 * 24.0 => float root;
    Std.mtof (root + notes[step]) => lead.freq;
    550.0 + hostParam2 * 5200.0 => filter.freq;
    hostParamGain * (0.35 + hostParam3 * 0.45) => master.gain;

    env.keyOn();
    (60000.0 / Math.max(20.0, hostTempo) / 8.0)::ms => dur thirtySecond;
    thirtySecond * (1.0 + hostParam3 * 3.0) => now;
    env.keyOff();
    thirtySecond => now;

    (step + 1) % notes.size() => step;
}
)chuck";
}
#endif

#if MATD_CHUCK_MIDI_FX
juce::String midiDefaultProgram()
{
    return R"midi(# Live ChucK MIDI FX
# Commands:
# note <pitch> <velocity> <lengthMs> <periodMs>
# arp <comma-separated-pitches> <velocity> <lengthBeats> <stepBeats>

arp 48,51,55,58,60,58,55,51 96 0.25 0.25
)midi";
}

juce::String midiArpeggioProgram()
{
    return R"midi(# Live ChucK MIDI FX arpeggio
# Commands:
# arp <comma-separated-pitches> <velocity> <lengthBeats> <stepBeats>
#
# This follows Logic's tempo and emits one note per sixteenth note.

arp 48,51,55,58,60,58,55,51 96 0.25 0.25
)midi";
}

juce::String midiOctaveArpProgram()
{
    return R"midi(# Higher octave tempo arpeggio
arp 60,64,67,72,76,72,67,64 104 0.125 0.125
)midi";
}

juce::String midiGlassCascadeProgram()
{
    return R"midi(# Glass cascade
arp 60,64,67,71,76,79,83,88 92 0.125 0.125
arp 48,55,60,64,67,64,60,55 74 0.5 0.5
)midi";
}

juce::String midiPulseStackProgram()
{
    return R"midi(# Pulse stack
arp 36,36,43,48,36,43,50,48 112 0.125 0.25
arp 72,75,79,82,84,82,79,75 86 0.0625 0.125
arp 55,58,62,67 68 0.25 0.75
)midi";
}
#endif

juce::String stateTag() { return "MatDLiveChucKState"; }

template <typename OptionalDouble>
double sanitiseBpm (const OptionalDouble& bpm)
{
    if (bpm.hasValue() && std::isfinite (*bpm) && *bpm > 0.0)
        return juce::jlimit (20.0, 999.0, *bpm);

    return 120.0;
}

#if MATD_CHUCK_MIDI_FX
int beatsToSamples (double sampleRate, double bpm, double beats)
{
    const auto seconds = (60.0 / juce::jmax (20.0, bpm)) * juce::jmax (0.001, beats);
    return juce::jmax (1, juce::roundToInt (sampleRate * seconds));
}
#endif

float getParameterValueOrDefault (const juce::AudioParameterFloat* parameter, float fallback) noexcept
{
    return parameter != nullptr ? parameter->get() : fallback;
}
}

MatDChucKAudioProcessor::MatDChucKAudioProcessor()
    : AudioProcessor (
#if MATD_CHUCK_MIDI_FX
          BusesProperties()
#else
          BusesProperties()
              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
      programText (getBuiltInProgram()),
      statusText ("Ready")
{
    createHostParameters();
}

MatDChucKAudioProcessor::~MatDChucKAudioProcessor() = default;

const juce::String MatDChucKAudioProcessor::getName() const
{
#if MATD_CHUCK_MIDI_FX
    return "Live ChucK MIDI FX";
#else
    return "Live ChucK";
#endif
}

bool MatDChucKAudioProcessor::acceptsMidi() const
{
#if MATD_CHUCK_MIDI_FX
    return true;
#else
    return false;
#endif
}

bool MatDChucKAudioProcessor::producesMidi() const
{
#if MATD_CHUCK_MIDI_FX
    return true;
#else
    return false;
#endif
}

bool MatDChucKAudioProcessor::isMidiEffect() const
{
#if MATD_CHUCK_MIDI_FX
    return true;
#else
    return false;
#endif
}

MatDChucKAudioProcessor::HostTransportState MatDChucKAudioProcessor::readHostTransportState() const
{
    HostTransportState state;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            state.isPlaying = position->getIsPlaying();
            state.bpm = sanitiseBpm (position->getBpm());

            if (auto ppq = position->getPpqPosition())
                state.ppqPosition = *ppq;

            if (auto lastBar = position->getPpqPositionOfLastBarStart())
                state.ppqLastBarStart = *lastBar;

            if (auto bar = position->getBarCount())
                state.barCount = static_cast<double> (*bar);

            if (auto seconds = position->getTimeInSeconds())
                state.timeInSeconds = *seconds;

            if (auto samples = position->getTimeInSamples())
                state.timeInSamples = static_cast<double> (*samples);

            if (auto sig = position->getTimeSignature())
            {
                state.timeSigNumerator = static_cast<double> (sig->numerator);
                state.timeSigDenominator = static_cast<double> (sig->denominator);
            }

            state.beatInBar = state.ppqPosition - state.ppqLastBarStart;
        }
    }

    state.sampleRate = getSampleRate() > 0.0 ? getSampleRate() : state.sampleRate;
    return state;
}

void MatDChucKAudioProcessor::createHostParameters()
{
    const auto addFloatParameter = [this] (juce::AudioParameterFloat*& target,
                                           const juce::String& id,
                                           const juce::String& name,
                                           float minimum,
                                           float maximum,
                                           float defaultValue)
    {
        auto parameter = std::make_unique<juce::AudioParameterFloat> (id, name, minimum, maximum, defaultValue);
        target = parameter.get();
        addParameter (parameter.release());
    };

    addFloatParameter (gainParameter, "hostParamGain", "ChucK Gain", 0.0f, 1.0f, 0.14f);
    addFloatParameter (control1Parameter, "hostParam1", "ChucK Control 1", 0.0f, 1.0f, 0.0f);
    addFloatParameter (control2Parameter, "hostParam2", "ChucK Control 2", 0.0f, 1.0f, 0.0f);
    addFloatParameter (control3Parameter, "hostParam3", "ChucK Control 3", 0.0f, 1.0f, 0.0f);
}

void MatDChucKAudioProcessor::saveHostParameters (juce::XmlElement& state) const
{
    state.setAttribute ("hostParamGain", static_cast<double> (getParameterValueOrDefault (gainParameter, 0.14f)));
    state.setAttribute ("hostParam1", static_cast<double> (getParameterValueOrDefault (control1Parameter, 0.0f)));
    state.setAttribute ("hostParam2", static_cast<double> (getParameterValueOrDefault (control2Parameter, 0.0f)));
    state.setAttribute ("hostParam3", static_cast<double> (getParameterValueOrDefault (control3Parameter, 0.0f)));
}

void MatDChucKAudioProcessor::restoreHostParameters (const juce::XmlElement& state)
{
    const auto restore = [&state] (juce::AudioParameterFloat* parameter, const juce::String& attribute)
    {
        if (parameter == nullptr || ! state.hasAttribute (attribute))
            return;

        const auto value = static_cast<float> (state.getDoubleAttribute (attribute, parameter->get()));
        const auto normalised = parameter->getNormalisableRange().convertTo0to1 (value);
        parameter->setValueNotifyingHost (normalised);
    };

    restore (gainParameter, "hostParamGain");
    restore (control1Parameter, "hostParam1");
    restore (control2Parameter, "hostParam2");
    restore (control3Parameter, "hostParam3");
}

void MatDChucKAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ScopedNoDenormals noDenormals;
    prepared.store (false, std::memory_order_release);

#if MATD_CHUCK_MIDI_FX
    juce::ignoreUnused (samplesPerBlock);
    preparedSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    juce::String error;
    const auto ok = compileMidiProgram (getProgramText(), error);
    const juce::ScopedLock lock (stateLock);
    statusText = ok ? "MIDI program ready" : ("MIDI program error: " + error);
    prepared.store (ok, std::memory_order_release);
#else
    engine.release();

    const auto inputChannels = juce::jmax (1, getTotalNumInputChannels());
    const auto outputChannels = juce::jlimit (1, 2, getTotalNumOutputChannels());
    const auto blockSize = juce::jmax (1, samplesPerBlock);
    lastPreparedSampleRate = sampleRate;
    lastPreparedBlockSize = blockSize;
    lastPreparedInputChannels = inputChannels;
    lastPreparedOutputChannels = outputChannels;
    const auto ok = engine.prepare (sampleRate,
                                    blockSize,
                                    inputChannels,
                                    outputChannels,
                                    getProgramText(),
                                    getHostParameterBindings());

    const juce::ScopedLock lock (stateLock);
    statusText = ok ? "Audio engine ready" : ("Audio engine error: " + engine.getLastError());
    prepared.store (ok, std::memory_order_release);
#endif
}

void MatDChucKAudioProcessor::releaseResources()
{
    prepared.store (false, std::memory_order_release);

#if ! MATD_CHUCK_MIDI_FX
    engine.release();
#endif
}

bool MatDChucKAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if MATD_CHUCK_MIDI_FX
    juce::ignoreUnused (layouts);
    return true;
#else
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
#endif
}

void MatDChucKAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto transport = readHostTransportState();

#if MATD_CHUCK_MIDI_FX
    buffer.clear();
    if (midiPanicRequested.exchange (false, std::memory_order_acq_rel))
        addAllNotesOff (midiMessages);

    if (prepared.load (std::memory_order_acquire))
    {
        if (transport.isPlaying)
            processMidiProgram (midiMessages, buffer.getNumSamples(), transport.bpm);
        else
            stopActiveMidiNotes (midiMessages);

        wasTransportPlaying = transport.isPlaying;
    }
#else
    midiMessages.clear();

    if (! prepared.load (std::memory_order_acquire))
    {
        buffer.clear();
        return;
    }

    updateHostGlobals (transport);

    if (! transport.isPlaying)
    {
        buffer.clear();
        return;
    }

    engine.process (buffer, buffer);
#endif
}

juce::AudioProcessorEditor* MatDChucKAudioProcessor::createEditor()
{
    return new MatDChucKPluginEditor (*this);
}

void MatDChucKAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = std::make_unique<juce::XmlElement> (stateTag());
    state->setAttribute ("program", getProgramText());
    saveHostParameters (*state);
    copyXmlToBinary (*state, destData);
}

void MatDChucKAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto state = getXmlFromBinary (data, sizeInBytes);
    if (state == nullptr || ! state->hasTagName (stateTag()))
        return;

    restoreHostParameters (*state);
    applyProgramFromEditor (state->getStringAttribute ("program", getBuiltInProgram()));
}

juce::String MatDChucKAudioProcessor::getProgramText() const
{
    const juce::ScopedLock lock (stateLock);
    return programText;
}

juce::String MatDChucKAudioProcessor::getStatusText() const
{
    const juce::ScopedLock lock (stateLock);
    return statusText;
}

bool MatDChucKAudioProcessor::applyProgramFromEditor (const juce::String& newProgram)
{
    {
        const juce::ScopedLock lock (stateLock);
        programText = newProgram;
        statusText = "Applying...";
    }

#if MATD_CHUCK_MIDI_FX
    juce::String error;
    const auto ok = compileMidiProgram (newProgram, error);
    const juce::ScopedLock lock (stateLock);
    statusText = ok ? "MIDI program applied" : ("MIDI program error: " + error);
    return ok;
#else
    if (! prepared.load (std::memory_order_acquire))
    {
        const juce::ScopedLock lock (stateLock);
        statusText = "Program saved; audio engine is not prepared";
        return true;
    }

    const auto loaded = engine.loadProgram (newProgram, getHostParameterBindings());
    const juce::ScopedLock lock (stateLock);
    statusText = loaded ? "ChucK program applied" : engine.getLastError();
    return loaded;
#endif
}

void MatDChucKAudioProcessor::resetToDefaultProgram()
{
    applyProgramFromEditor (getBuiltInProgram());
}

void MatDChucKAudioProcessor::stopCode()
{
#if MATD_CHUCK_MIDI_FX
    const juce::ScopedLock lock (stateLock);
    midiPatterns.clear();
    pendingNoteOffs.clear();
    prepared.store (false, std::memory_order_release);
    midiPanicRequested.store (true, std::memory_order_release);
    statusText = "Stopped";
#else
    prepared.store (false, std::memory_order_release);
    engine.release();
    const juce::ScopedLock lock (stateLock);
    statusText = "Stopped";
#endif
}

bool MatDChucKAudioProcessor::restartCode()
{
#if MATD_CHUCK_MIDI_FX
    juce::String error;
    const auto ok = compileMidiProgram (getProgramText(), error);
    const juce::ScopedLock lock (stateLock);
    prepared.store (ok, std::memory_order_release);
    statusText = ok ? "Restarted" : ("MIDI program error: " + error);
    return ok;
#else
    prepared.store (false, std::memory_order_release);
    engine.release();

    const auto ok = engine.prepare (lastPreparedSampleRate,
                                    lastPreparedBlockSize,
                                    lastPreparedInputChannels,
                                    lastPreparedOutputChannels,
                                    getProgramText(),
                                    getHostParameterBindings());

    const juce::ScopedLock lock (stateLock);
    prepared.store (ok, std::memory_order_release);
    statusText = ok ? "Restarted" : engine.getLastError();
    return ok;
#endif
}

void MatDChucKAudioProcessor::panic()
{
#if MATD_CHUCK_MIDI_FX
    const juce::ScopedLock lock (stateLock);
    pendingNoteOffs.clear();
    for (auto& pattern : midiPatterns)
    {
        pattern.nextOnSample = 0;
        pattern.currentStep = 0;
    }
    midiPanicRequested.store (true, std::memory_order_release);
    statusText = "Panic sent";
#else
    static_cast<void> (restartCode());
#endif
}

float MatDChucKAudioProcessor::getHostParameterValue (int index) const noexcept
{
    switch (index)
    {
        case 0: return getParameterValueOrDefault (gainParameter, 0.14f);
        case 1: return getParameterValueOrDefault (control1Parameter, 0.0f);
        case 2: return getParameterValueOrDefault (control2Parameter, 0.0f);
        case 3: return getParameterValueOrDefault (control3Parameter, 0.0f);
        default: return 0.0f;
    }
}

void MatDChucKAudioProcessor::setHostParameterValue (int index, float value)
{
    juce::AudioParameterFloat* parameter = nullptr;

    switch (index)
    {
        case 0: parameter = gainParameter; break;
        case 1: parameter = control1Parameter; break;
        case 2: parameter = control2Parameter; break;
        case 3: parameter = control3Parameter; break;
        default: break;
    }

    if (parameter == nullptr)
        return;

    const auto normalised = parameter->getNormalisableRange().convertTo0to1 (juce::jlimit (0.0f, 1.0f, value));
    parameter->setValueNotifyingHost (normalised);
}

juce::String MatDChucKAudioProcessor::getBuiltInProgram()
{
#if MATD_CHUCK_MIDI_FX
    return midiDefaultProgram();
#else
    return audioDefaultProgram();
#endif
}

juce::StringArray MatDChucKAudioProcessor::getBuiltInExampleNames()
{
#if MATD_CHUCK_MIDI_FX
    return { "Starter MIDI Arp", "C Minor MIDI Arp", "Fast Octave MIDI Arp", "Glass Cascade", "Pulse Stack" };
#else
    return { "Starter Tone", "Tempo Audio Arp", "Beat Gate", "Automation Demo", "Pulse Garden", "Shimmer Pad", "Clockwork Lead" };
#endif
}

juce::String MatDChucKAudioProcessor::getBuiltInExample (const juce::String& name)
{
#if MATD_CHUCK_MIDI_FX
    if (name == "C Minor MIDI Arp")
        return midiArpeggioProgram();
    if (name == "Fast Octave MIDI Arp")
        return midiOctaveArpProgram();
    if (name == "Glass Cascade")
        return midiGlassCascadeProgram();
    if (name == "Pulse Stack")
        return midiPulseStackProgram();

    return midiDefaultProgram();
#else
    if (name == "Tempo Audio Arp")
        return audioArpeggioProgram();
    if (name == "Beat Gate")
        return audioBeatGateProgram();
    if (name == "Automation Demo")
        return audioAutomationProgram();
    if (name == "Pulse Garden")
        return audioPulseGardenProgram();
    if (name == "Shimmer Pad")
        return audioShimmerPadProgram();
    if (name == "Clockwork Lead")
        return audioClockworkLeadProgram();

    return audioDefaultProgram();
#endif
}

#if ! MATD_CHUCK_MIDI_FX
std::vector<EmbeddedChucKEngine::ParameterBinding> MatDChucKAudioProcessor::getHostParameterBindings()
{
    return
    {
        { "hostTempo", 120.0f, 20.0f, 999.0f },
        { "hostTransportPlaying", 0.0f, 0.0f, 1.0f },
        { "hostPpq", 0.0f, -1000000.0f, 1000000.0f },
        { "hostBeat", 0.0f, -1000000.0f, 1000000.0f },
        { "hostBar", 0.0f, -1000000.0f, 1000000.0f },
        { "hostSampleRate", 44100.0f, 1.0f, 384000.0f },
        { "hostTimeSeconds", 0.0f, -1000000.0f, 1000000.0f },
        { "hostTimeSamples", 0.0f, -1000000000.0f, 1000000000.0f },
        { "hostTimeSigNumerator", 4.0f, 1.0f, 64.0f },
        { "hostTimeSigDenominator", 4.0f, 1.0f, 64.0f },
        { "hostParamGain", 0.14f, 0.0f, 1.0f },
        { "hostParam1", 0.0f, 0.0f, 1.0f },
        { "hostParam2", 0.0f, 0.0f, 1.0f },
        { "hostParam3", 0.0f, 0.0f, 1.0f }
    };
}

void MatDChucKAudioProcessor::updateHostGlobals (const HostTransportState& transport)
{
    static_cast<void> (engine.setParameterValue ("hostTempo", static_cast<float> (transport.bpm)));
    static_cast<void> (engine.setParameterValue ("hostTransportPlaying", transport.isPlaying ? 1.0f : 0.0f));
    static_cast<void> (engine.setParameterValue ("hostPpq", static_cast<float> (transport.ppqPosition)));
    static_cast<void> (engine.setParameterValue ("hostBeat", static_cast<float> (transport.beatInBar)));
    static_cast<void> (engine.setParameterValue ("hostBar", static_cast<float> (transport.barCount)));
    static_cast<void> (engine.setParameterValue ("hostSampleRate", static_cast<float> (transport.sampleRate)));
    static_cast<void> (engine.setParameterValue ("hostTimeSeconds", static_cast<float> (transport.timeInSeconds)));
    static_cast<void> (engine.setParameterValue ("hostTimeSamples", static_cast<float> (transport.timeInSamples)));
    static_cast<void> (engine.setParameterValue ("hostTimeSigNumerator", static_cast<float> (transport.timeSigNumerator)));
    static_cast<void> (engine.setParameterValue ("hostTimeSigDenominator", static_cast<float> (transport.timeSigDenominator)));
    static_cast<void> (engine.setParameterValue ("hostParamGain", getParameterValueOrDefault (gainParameter, 0.14f)));
    static_cast<void> (engine.setParameterValue ("hostParam1", getParameterValueOrDefault (control1Parameter, 0.0f)));
    static_cast<void> (engine.setParameterValue ("hostParam2", getParameterValueOrDefault (control2Parameter, 0.0f)));
    static_cast<void> (engine.setParameterValue ("hostParam3", getParameterValueOrDefault (control3Parameter, 0.0f)));
}
#endif

#if MATD_CHUCK_MIDI_FX
bool MatDChucKAudioProcessor::compileMidiProgram (const juce::String& text, juce::String& error)
{
    std::vector<MidiPattern> parsed;
    const juce::StringArray lines = juce::StringArray::fromLines (text);

    for (auto line : lines)
    {
        line = line.upToFirstOccurrenceOf ("#", false, false).trim();
        if (line.isEmpty())
            continue;

        const auto tokens = juce::StringArray::fromTokens (line, " \t", {});
        if (tokens.size() == 5 && tokens[0] == "note")
        {
            MidiPattern pattern;
            pattern.notes = { juce::jlimit (0, 127, tokens[1].getIntValue()) };
            pattern.velocity = juce::jlimit (1, 127, tokens[2].getIntValue());
            const auto lengthMs = juce::jlimit (1, 60000, tokens[3].getIntValue());
            const auto periodMs = juce::jlimit (1, 60000, tokens[4].getIntValue());
            pattern.lengthSamples = juce::jmax (1, juce::roundToInt (preparedSampleRate * static_cast<double> (lengthMs) / 1000.0));
            pattern.periodSamples = juce::jmax (1, juce::roundToInt (preparedSampleRate * static_cast<double> (periodMs) / 1000.0));
            parsed.push_back (std::move (pattern));
            continue;
        }

        if (tokens.size() == 5 && tokens[0] == "arp")
        {
            MidiPattern pattern;
            pattern.notes.clear();
            const auto pitchTokens = juce::StringArray::fromTokens (tokens[1], ",", {});

            for (const auto& pitch : pitchTokens)
                pattern.notes.push_back (juce::jlimit (0, 127, pitch.getIntValue()));

            if (pattern.notes.empty())
            {
                error = "Arp needs at least one pitch";
                return false;
            }

            pattern.velocity = juce::jlimit (1, 127, tokens[2].getIntValue());
            pattern.lengthBeats = juce::jlimit (0.01, 16.0, tokens[3].getDoubleValue());
            pattern.periodBeats = juce::jlimit (0.01, 16.0, tokens[4].getDoubleValue());
            pattern.tempoSync = true;
            parsed.push_back (std::move (pattern));
            continue;
        }

        error = "Expected: note <pitch> <velocity> <lengthMs> <periodMs> or arp <pitches> <velocity> <lengthBeats> <stepBeats>";
        return false;
    }

    const juce::ScopedLock lock (stateLock);
    midiPatterns = std::move (parsed);
    pendingNoteOffs.clear();
    wasTransportPlaying = false;
    return true;
}

void MatDChucKAudioProcessor::processMidiProgram (juce::MidiBuffer& midiMessages, int numSamples, double bpm)
{
    const juce::ScopedLock lock (stateLock);

    for (auto it = pendingNoteOffs.begin(); it != pendingNoteOffs.end();)
    {
        it->second -= numSamples;
        if (it->second <= 0)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, it->first), 0);
            it = pendingNoteOffs.erase (it);
        }
        else
            ++it;
    }

    for (auto& pattern : midiPatterns)
    {
        if (! pattern.enabled)
            continue;

        const auto lengthSamples = pattern.tempoSync ? beatsToSamples (preparedSampleRate, bpm, pattern.lengthBeats)
                                                     : pattern.lengthSamples;
        const auto periodSamples = pattern.tempoSync ? beatsToSamples (preparedSampleRate, bpm, pattern.periodBeats)
                                                     : pattern.periodSamples;

        pattern.nextOnSample -= numSamples;
        while (pattern.nextOnSample <= 0)
        {
            const auto note = pattern.notes[static_cast<size_t> (pattern.currentStep % static_cast<int> (pattern.notes.size()))];
            const auto samplePosition = juce::jlimit (0, juce::jmax (0, numSamples - 1), numSamples + pattern.nextOnSample);
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (pattern.velocity)),
                                   samplePosition);
            pendingNoteOffs.push_back ({ note, lengthSamples });
            pattern.nextOnSample += periodSamples;
            pattern.currentStep = (pattern.currentStep + 1) % static_cast<int> (pattern.notes.size());
        }
    }
}

void MatDChucKAudioProcessor::stopActiveMidiNotes (juce::MidiBuffer& midiMessages)
{
    const juce::ScopedLock lock (stateLock);

    if (! wasTransportPlaying && pendingNoteOffs.empty())
        return;

    for (const auto& noteOff : pendingNoteOffs)
        midiMessages.addEvent (juce::MidiMessage::noteOff (1, noteOff.first), 0);

    pendingNoteOffs.clear();

    for (auto& pattern : midiPatterns)
    {
        pattern.nextOnSample = 0;
        pattern.currentStep = 0;
    }
}

void MatDChucKAudioProcessor::addAllNotesOff (juce::MidiBuffer& midiMessages)
{
    for (int channel = 1; channel <= 16; ++channel)
        midiMessages.addEvent (juce::MidiMessage::allNotesOff (channel), 0);
}
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MatDChucKAudioProcessor();
}
