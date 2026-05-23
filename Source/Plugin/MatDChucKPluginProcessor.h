#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <WeldChucKEngine.h>

#include <atomic>
#include <vector>

class MatDChucKAudioProcessor final : public juce::AudioProcessor
{
public:
    MatDChucKAudioProcessor();
    ~MatDChucKAudioProcessor() override;

    const juce::String getName() const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; }

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::String getProgramText() const;
    juce::String getStatusText() const;
    bool applyProgramFromEditor (const juce::String& newProgram);
    void resetToDefaultProgram();
    static juce::String getBuiltInProgram();
    static juce::StringArray getBuiltInExampleNames();
    static juce::String getBuiltInExample (const juce::String& name);

private:
    struct HostTransportState
    {
        bool isPlaying = false;
        double bpm = 120.0;
        double ppqPosition = 0.0;
        double ppqLastBarStart = 0.0;
        double beatInBar = 0.0;
        double barCount = 0.0;
        double timeInSeconds = 0.0;
        double timeInSamples = 0.0;
        double sampleRate = 44100.0;
        double timeSigNumerator = 4.0;
        double timeSigDenominator = 4.0;
    };

    HostTransportState readHostTransportState() const;
    void createHostParameters();
    void saveHostParameters (juce::XmlElement& state) const;
    void restoreHostParameters (const juce::XmlElement& state);

#if MATD_CHUCK_MIDI_FX
    struct MidiPattern
    {
        std::vector<int> notes { 60 };
        int currentStep = 0;
        int velocity = 96;
        int lengthSamples = 4800;
        int periodSamples = 24000;
        double lengthBeats = 0.25;
        double periodBeats = 0.25;
        int nextOnSample = 0;
        bool tempoSync = false;
        bool enabled = true;
    };

    bool compileMidiProgram (const juce::String& text, juce::String& error);
    void processMidiProgram (juce::MidiBuffer& midiMessages, int numSamples, double bpm);
    void stopActiveMidiNotes (juce::MidiBuffer& midiMessages);

    std::vector<MidiPattern> midiPatterns;
    std::vector<std::pair<int, int>> pendingNoteOffs;
    double preparedSampleRate = 44100.0;
    bool wasTransportPlaying = false;
#else
    static std::vector<EmbeddedChucKEngine::ParameterBinding> getHostParameterBindings();
    void updateHostGlobals (const HostTransportState& transport);

    EmbeddedChucKEngine engine;
#endif

    mutable juce::CriticalSection stateLock;
    juce::String programText;
    juce::String statusText;
    juce::AudioParameterFloat* gainParameter = nullptr;
    juce::AudioParameterFloat* control1Parameter = nullptr;
    juce::AudioParameterFloat* control2Parameter = nullptr;
    juce::AudioParameterFloat* control3Parameter = nullptr;
    std::atomic<bool> prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatDChucKAudioProcessor)
};
