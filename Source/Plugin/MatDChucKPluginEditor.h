#pragma once

#include "MatDChucKPluginProcessor.h"

class MatDChucKPluginEditor final : public juce::AudioProcessorEditor,
                                    private juce::Button::Listener,
                                    private juce::ComboBox::Listener,
                                    private juce::Timer
{
public:
    explicit MatDChucKPluginEditor (MatDChucKAudioProcessor&);
    ~MatDChucKPluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void buttonClicked (juce::Button*) override;
    void comboBoxChanged (juce::ComboBox*) override;
    void timerCallback() override;
    void refreshStatus();
    void loadCodeFromFile();
    void saveCodeToFile();
    void showFileError (const juce::String& title, const juce::String& message);
    juce::String getFilePatterns() const;
    juce::File getDefaultCodeFile() const;

    MatDChucKAudioProcessor& ownerProcessor;
    juce::CodeDocument codeDocument;
    juce::CodeTokeniser* tokeniser = nullptr;
    juce::CodeEditorComponent editor;
    juce::TextButton applyButton { "Apply" };
    juce::TextButton loadButton { "Load" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton resetButton { "Reset" };
    juce::ComboBox exampleBox;
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextEditor statusConsole;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::ScopedMessageBox messageBox;
    juce::File lastCodeDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatDChucKPluginEditor)
};
