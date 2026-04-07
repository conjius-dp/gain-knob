#pragma once
#include "PluginProcessor.h"

class GainKnobAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit GainKnobAudioProcessorEditor(GainKnobAudioProcessor&);
    ~GainKnobAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    GainKnobAudioProcessor& processorRef;
    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::Label latencyLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainKnobAudioProcessorEditor)
};
