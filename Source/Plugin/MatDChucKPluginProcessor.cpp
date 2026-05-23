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

0.12 => master.gain;

while (true)
{
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

0.16 => master.gain;
env.set (6::ms, 35::ms, 0.35, 80::ms);

[0, 4, 7, 12, 7, 4] @=> int intervals[];
48 => int root;
0 => int step;

while (true)
{
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
        }
    }

    return state;
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
    copyXmlToBinary (*state, destData);
}

void MatDChucKAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto state = getXmlFromBinary (data, sizeInBytes);
    if (state == nullptr || ! state->hasTagName (stateTag()))
        return;

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

    const auto queued = engine.loadProgramAsync (newProgram, getHostParameterBindings());
    const juce::ScopedLock lock (stateLock);
    statusText = queued ? "ChucK program queued" : ("ChucK error: " + engine.getLastError());
    return queued;
#endif
}

void MatDChucKAudioProcessor::resetToDefaultProgram()
{
    applyProgramFromEditor (getBuiltInProgram());
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
    return { "Starter MIDI Arp", "C Minor MIDI Arp" };
#else
    return { "Starter Tone", "Tempo Audio Arp" };
#endif
}

juce::String MatDChucKAudioProcessor::getBuiltInExample (const juce::String& name)
{
#if MATD_CHUCK_MIDI_FX
    if (name == "C Minor MIDI Arp")
        return midiArpeggioProgram();

    return midiDefaultProgram();
#else
    if (name == "Tempo Audio Arp")
        return audioArpeggioProgram();

    return audioDefaultProgram();
#endif
}

#if ! MATD_CHUCK_MIDI_FX
std::vector<EmbeddedChucKEngine::ParameterBinding> MatDChucKAudioProcessor::getHostParameterBindings()
{
    return
    {
        { "hostTempo", 120.0f, 20.0f, 999.0f },
        { "hostTransportPlaying", 0.0f, 0.0f, 1.0f }
    };
}

void MatDChucKAudioProcessor::updateHostGlobals (const HostTransportState& transport)
{
    static_cast<void> (engine.setParameterValue ("hostTempo", static_cast<float> (transport.bpm)));
    static_cast<void> (engine.setParameterValue ("hostTransportPlaying", transport.isPlaying ? 1.0f : 0.0f));
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
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MatDChucKAudioProcessor();
}
