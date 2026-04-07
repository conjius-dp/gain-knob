#include "PluginEditor.h"

GainKnobAudioProcessorEditor::GainKnobAudioProcessorEditor(GainKnobAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 25);
    gainSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(gainSlider);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.attachToComponent(&gainSlider, false);
    addAndMakeVisible(gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "gain", gainSlider);

    latencyLabel.setText("Latency: 0ms", juce::dontSendNotification);
    latencyLabel.setJustificationType(juce::Justification::centredLeft);
    latencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    latencyLabel.setFont(juce::Font(13.0f));
    addAndMakeVisible(latencyLabel);

    setSize(300, 300);
    startTimerHz(30);
}

GainKnobAudioProcessorEditor::~GainKnobAudioProcessorEditor()
{
    stopTimer();
}

void GainKnobAudioProcessorEditor::timerCallback()
{
    float latencyMs = processorRef.getLastProcessLatencyMs();
    latencyLabel.setText("Latency: " + juce::String(latencyMs, 3) + "ms",
                         juce::dontSendNotification);
}

void GainKnobAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void GainKnobAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(30);
    bounds.removeFromTop(30); // space for label

    auto latencyArea = bounds.removeFromBottom(20);
    latencyLabel.setBounds(latencyArea);

    gainSlider.setBounds(bounds);
}
