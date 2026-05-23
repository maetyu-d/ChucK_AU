#pragma once

#include "MatDChucKPluginProcessor.h"

class MatDChucKPluginEditor final : public juce::AudioProcessorEditor,
                                    private juce::Button::Listener,
                                    private juce::Timer
{
public:
    explicit MatDChucKPluginEditor (MatDChucKAudioProcessor&);
    ~MatDChucKPluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void buttonClicked (juce::Button*) override;
    void timerCallback() override;
    void refreshStatus();

    MatDChucKAudioProcessor& ownerProcessor;
    juce::CodeDocument codeDocument;
    juce::CodeTokeniser* tokeniser = nullptr;
    juce::CodeEditorComponent editor;
    juce::TextButton applyButton { "Apply" };
    juce::TextButton resetButton { "Reset" };
    juce::Label titleLabel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatDChucKPluginEditor)
};
