#include "MatDChucKPluginEditor.h"

namespace
{
juce::Colour backgroundColour() { return juce::Colour (0xff16171a); }
juce::Colour panelColour() { return juce::Colour (0xff202329); }
juce::Colour textColour() { return juce::Colour (0xfff0f2f5); }
juce::Colour accentColour() { return juce::Colour (0xff4fb477); }
}

MatDChucKPluginEditor::MatDChucKPluginEditor (MatDChucKAudioProcessor& owner)
    : AudioProcessorEditor (&owner),
      ownerProcessor (owner),
      editor (codeDocument, tokeniser)
{
    setSize (820, 560);
    codeDocument.replaceAllContent (owner.getProgramText());
    lastCodeDirectory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    titleLabel.setText (ownerProcessor.getName(), juce::dontSendNotification);
    titleLabel.setColour (juce::Label::textColourId, textColour());
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    applyButton.addListener (this);
    loadButton.addListener (this);
    saveButton.addListener (this);
    resetButton.addListener (this);
    stopButton.addListener (this);
    restartButton.addListener (this);
    panicButton.addListener (this);
    applyButton.setColour (juce::TextButton::buttonColourId, accentColour());
    applyButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
    loadButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff30343b));
    loadButton.setColour (juce::TextButton::textColourOffId, textColour());
    saveButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff30343b));
    saveButton.setColour (juce::TextButton::textColourOffId, textColour());
    resetButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff30343b));
    resetButton.setColour (juce::TextButton::textColourOffId, textColour());
    stopButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff30343b));
    stopButton.setColour (juce::TextButton::textColourOffId, textColour());
    restartButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff30343b));
    restartButton.setColour (juce::TextButton::textColourOffId, textColour());
    panicButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff5f3535));
    panicButton.setColour (juce::TextButton::textColourOffId, textColour());
    addAndMakeVisible (applyButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (stopButton);
    addAndMakeVisible (restartButton);
    addAndMakeVisible (panicButton);

    exampleBox.addListener (this);
    exampleBox.setTextWhenNothingSelected ("Examples");
    exampleBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff30343b));
    exampleBox.setColour (juce::ComboBox::textColourId, textColour());
    exampleBox.setColour (juce::ComboBox::arrowColourId, accentColour());
    const auto examples = MatDChucKAudioProcessor::getBuiltInExampleNames();
    for (int i = 0; i < examples.size(); ++i)
        exampleBox.addItem (examples[i], i + 1);
    addAndMakeVisible (exampleBox);

    const std::array<juce::String, 4> parameterNames { "Gain", "Ctrl 1", "Ctrl 2", "Ctrl 3" };
    for (size_t i = 0; i < parameterSliders.size(); ++i)
    {
        auto& label = parameterLabels[i];
        auto& slider = parameterSliders[i];

        label.setText (parameterNames[i], juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, juce::Colour (0xffb8bec8));
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (label);

        slider.setRange (0.0, 1.0, 0.001);
        slider.setValue (ownerProcessor.getHostParameterValue (static_cast<int> (i)), juce::dontSendNotification);
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
        slider.setColour (juce::Slider::trackColourId, accentColour());
        slider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff30343b));
        slider.addListener (this);
        addAndMakeVisible (slider);
    }

    editor.setFont (juce::FontOptions (15.0f));
    editor.setTabSize (4, true);
    editor.setColour (juce::CodeEditorComponent::backgroundColourId, panelColour());
    editor.setColour (juce::CodeEditorComponent::defaultTextColourId, textColour());
    editor.setColour (juce::CodeEditorComponent::highlightColourId, juce::Colour (0xff355f49));
    editor.setColour (juce::CaretComponent::caretColourId, accentColour());
    addAndMakeVisible (editor);

    statusLabel.setText (ownerProcessor.getStatusText(), juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb8bec8));
    statusLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (statusLabel);

    statusConsole.setMultiLine (true);
    statusConsole.setReadOnly (true);
    statusConsole.setScrollbarsShown (true);
    statusConsole.setCaretVisible (false);
    statusConsole.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff111317));
    statusConsole.setColour (juce::TextEditor::textColourId, juce::Colour (0xffcfd6df));
    statusConsole.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff2b2f36));
    statusConsole.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (statusConsole);

    startTimerHz (8);
}

MatDChucKPluginEditor::~MatDChucKPluginEditor()
{
    stopTimer();
    applyButton.removeListener (this);
    loadButton.removeListener (this);
    saveButton.removeListener (this);
    resetButton.removeListener (this);
    stopButton.removeListener (this);
    restartButton.removeListener (this);
    panicButton.removeListener (this);
    exampleBox.removeListener (this);
    for (auto& slider : parameterSliders)
        slider.removeListener (this);
}

void MatDChucKPluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour());
    g.setColour (juce::Colour (0xff2b2f36));
    g.drawRect (getLocalBounds(), 1);
}

void MatDChucKPluginEditor::resized()
{
    auto bounds = getLocalBounds().reduced (18);
    auto header = bounds.removeFromTop (38);

    titleLabel.setBounds (header.removeFromLeft (220));
    exampleBox.setBounds (header.removeFromLeft (150).reduced (0, 2));
    header.removeFromLeft (8);
    resetButton.setBounds (header.removeFromRight (76).reduced (0, 2));
    header.removeFromRight (8);
    saveButton.setBounds (header.removeFromRight (76).reduced (0, 2));
    header.removeFromRight (8);
    loadButton.setBounds (header.removeFromRight (76).reduced (0, 2));
    header.removeFromRight (8);
    applyButton.setBounds (header.removeFromRight (82).reduced (0, 2));

    bounds.removeFromTop (10);
    auto controls = bounds.removeFromTop (64);
    auto lifecycle = controls.removeFromRight (230);
    panicButton.setBounds (lifecycle.removeFromRight (68).reduced (0, 20));
    lifecycle.removeFromRight (8);
    restartButton.setBounds (lifecycle.removeFromRight (74).reduced (0, 20));
    lifecycle.removeFromRight (8);
    stopButton.setBounds (lifecycle.removeFromRight (58).reduced (0, 20));

    const auto sliderWidth = controls.getWidth() / static_cast<int> (parameterSliders.size());
    for (size_t i = 0; i < parameterSliders.size(); ++i)
    {
        auto slot = controls.removeFromLeft (sliderWidth).reduced (4, 0);
        parameterLabels[i].setBounds (slot.removeFromTop (18));
        parameterSliders[i].setBounds (slot.removeFromTop (34));
    }

    bounds.removeFromTop (8);
    statusConsole.setBounds (bounds.removeFromBottom (76));
    bounds.removeFromBottom (8);
    statusLabel.setBounds (bounds.removeFromBottom (24));
    bounds.removeFromBottom (8);
    editor.setBounds (bounds);
}

void MatDChucKPluginEditor::buttonClicked (juce::Button* button)
{
    if (button == &applyButton)
    {
        ownerProcessor.applyProgramFromEditor (codeDocument.getAllContent());
        refreshStatus();
    }
    else if (button == &resetButton)
    {
        ownerProcessor.resetToDefaultProgram();
        codeDocument.replaceAllContent (ownerProcessor.getProgramText());
        refreshStatus();
    }
    else if (button == &loadButton)
    {
        loadCodeFromFile();
    }
    else if (button == &saveButton)
    {
        saveCodeToFile();
    }
    else if (button == &stopButton)
    {
        ownerProcessor.stopCode();
        refreshStatus();
    }
    else if (button == &restartButton)
    {
        ownerProcessor.restartCode();
        refreshStatus();
    }
    else if (button == &panicButton)
    {
        ownerProcessor.panic();
        refreshStatus();
    }
}

void MatDChucKPluginEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox != &exampleBox || exampleBox.getSelectedId() <= 0)
        return;

    const auto exampleName = exampleBox.getText();
    codeDocument.replaceAllContent (MatDChucKAudioProcessor::getBuiltInExample (exampleName));
    const auto message = "Loaded " + exampleName + " - press Apply to run it";
    statusLabel.setText (message, juce::dontSendNotification);
    statusConsole.setText (message, false);
    exampleBox.setSelectedId (0, juce::dontSendNotification);
}

void MatDChucKPluginEditor::timerCallback()
{
    for (size_t i = 0; i < parameterSliders.size(); ++i)
        parameterSliders[i].setValue (ownerProcessor.getHostParameterValue (static_cast<int> (i)),
                                      juce::dontSendNotification);

    refreshStatus();
}

void MatDChucKPluginEditor::sliderValueChanged (juce::Slider* slider)
{
    for (size_t i = 0; i < parameterSliders.size(); ++i)
        if (slider == &parameterSliders[i])
        {
            ownerProcessor.setHostParameterValue (static_cast<int> (i), static_cast<float> (slider->getValue()));
            return;
        }
}

void MatDChucKPluginEditor::refreshStatus()
{
    const auto status = ownerProcessor.getStatusText();
    statusLabel.setText (status.upToFirstOccurrenceOf ("\n", false, false), juce::dontSendNotification);
    if (statusConsole.getText() != status)
        statusConsole.setText (status, false);
}

void MatDChucKPluginEditor::loadCodeFromFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Load code example",
                                                       lastCodeDirectory,
                                                       getFilePatterns(),
                                                       true,
                                                       false,
                                                       this);

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();

        if (file == juce::File())
            return;

        lastCodeDirectory = file.getParentDirectory();
        const auto text = file.loadFileAsString();

        if (! file.existsAsFile() || text.isEmpty())
        {
            showFileError ("Load failed", "Could not read a code file from the selected path.");
            return;
        }

        codeDocument.replaceAllContent (text);
        const auto message = "Loaded " + file.getFileName() + " - press Apply to run it";
        statusLabel.setText (message, juce::dontSendNotification);
        statusConsole.setText (message, false);
    });
}

void MatDChucKPluginEditor::saveCodeToFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Save code example",
                                                       getDefaultCodeFile(),
                                                       getFilePatterns(),
                                                       true,
                                                       false,
                                                       this);

    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();

        if (file == juce::File())
            return;

        if (! file.hasFileExtension (getFilePatterns()))
            file = file.withFileExtension (MATD_CHUCK_MIDI_FX ? ".txt" : ".ck");

        lastCodeDirectory = file.getParentDirectory();

        if (! file.replaceWithText (codeDocument.getAllContent()))
        {
            showFileError ("Save failed", "Could not write the code file.");
            return;
        }

        const auto message = "Saved " + file.getFileName();
        statusLabel.setText (message, juce::dontSendNotification);
        statusConsole.setText (message, false);
    });
}

void MatDChucKPluginEditor::showFileError (const juce::String& title, const juce::String& message)
{
    const auto options = juce::MessageBoxOptions::makeOptionsOk (juce::AlertWindow::WarningIcon,
                                                                title,
                                                                message);
    messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
}

juce::String MatDChucKPluginEditor::getFilePatterns() const
{
#if MATD_CHUCK_MIDI_FX
    return "*.txt;*.midifx;*";
#else
    return "*.ck;*.chuck;*.txt;*";
#endif
}

juce::File MatDChucKPluginEditor::getDefaultCodeFile() const
{
    const auto directory = lastCodeDirectory.exists() ? lastCodeDirectory
                                                      : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

#if MATD_CHUCK_MIDI_FX
    return directory.getChildFile ("Live ChucK MIDI FX Example.txt");
#else
    return directory.getChildFile ("Live ChucK Example.ck");
#endif
}
