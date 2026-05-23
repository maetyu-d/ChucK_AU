#include "MatDChucKPluginProcessor.h"
#include "MatDChucKPluginEditor.h"

namespace
{
#if ! MATD_CHUCK_MIDI_FX
juce::String audioDefaultProgram()
{
    return R"chuck(
SinOsc left => Gain master => dac;
TriOsc right => master => dac;

0.12 => master.gain;
220.0 => float base;

while (true)
{
    base => left.freq;
    base * 1.005 => right.freq;
    20::ms => now;
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

note 60 96 120 500
note 67 72 90 1000
)midi";
}
#endif

juce::String stateTag() { return "MatDLiveChucKState"; }
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
    auto ok = engine.prepare (sampleRate, blockSize, inputChannels, outputChannels);

    if (ok)
        ok = engine.loadProgram (getProgramText());

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

#if MATD_CHUCK_MIDI_FX
    buffer.clear();
    if (prepared.load (std::memory_order_acquire))
        processMidiProgram (midiMessages, buffer.getNumSamples());
#else
    midiMessages.clear();

    if (! prepared.load (std::memory_order_acquire))
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

    const auto queued = engine.loadProgramAsync (newProgram);
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

        const auto tokens = juce::StringArray::fromTokens (line, " \t,", {});
        if (tokens.size() != 5 || tokens[0] != "note")
        {
            error = "Expected: note <pitch> <velocity> <lengthMs> <periodMs>";
            return false;
        }

        MidiPattern pattern;
        pattern.note = juce::jlimit (0, 127, tokens[1].getIntValue());
        pattern.velocity = juce::jlimit (1, 127, tokens[2].getIntValue());
        const auto lengthMs = juce::jlimit (1, 60000, tokens[3].getIntValue());
        const auto periodMs = juce::jlimit (1, 60000, tokens[4].getIntValue());
        pattern.lengthSamples = juce::jmax (1, juce::roundToInt (preparedSampleRate * static_cast<double> (lengthMs) / 1000.0));
        pattern.periodSamples = juce::jmax (1, juce::roundToInt (preparedSampleRate * static_cast<double> (periodMs) / 1000.0));
        parsed.push_back (pattern);
    }

    const juce::ScopedLock lock (stateLock);
    midiPatterns = std::move (parsed);
    pendingNoteOffs.clear();
    return true;
}

void MatDChucKAudioProcessor::processMidiProgram (juce::MidiBuffer& midiMessages, int numSamples)
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

        pattern.nextOnSample -= numSamples;
        while (pattern.nextOnSample <= 0)
        {
            const auto samplePosition = juce::jlimit (0, juce::jmax (0, numSamples - 1), numSamples + pattern.nextOnSample);
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, pattern.note, static_cast<juce::uint8> (pattern.velocity)),
                                   samplePosition);
            pendingNoteOffs.push_back ({ pattern.note, pattern.lengthSamples });
            pattern.nextOnSample += pattern.periodSamples;
        }
    }
}
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MatDChucKAudioProcessor();
}
