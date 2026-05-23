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

    titleLabel.setText (ownerProcessor.getName(), juce::dontSendNotification);
    titleLabel.setColour (juce::Label::textColourId, textColour());
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    applyButton.addListener (this);
    resetButton.addListener (this);
    applyButton.setColour (juce::TextButton::buttonColourId, accentColour());
    applyButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
    resetButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff30343b));
    resetButton.setColour (juce::TextButton::textColourOffId, textColour());
    addAndMakeVisible (applyButton);
    addAndMakeVisible (resetButton);

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

    startTimerHz (8);
}

MatDChucKPluginEditor::~MatDChucKPluginEditor()
{
    stopTimer();
    applyButton.removeListener (this);
    resetButton.removeListener (this);
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

    titleLabel.setBounds (header.removeFromLeft (360));
    resetButton.setBounds (header.removeFromRight (92).reduced (0, 2));
    header.removeFromRight (8);
    applyButton.setBounds (header.removeFromRight (100).reduced (0, 2));

    bounds.removeFromTop (10);
    statusLabel.setBounds (bounds.removeFromBottom (28));
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
}

void MatDChucKPluginEditor::timerCallback()
{
    refreshStatus();
}

void MatDChucKPluginEditor::refreshStatus()
{
    statusLabel.setText (ownerProcessor.getStatusText(), juce::dontSendNotification);
}
