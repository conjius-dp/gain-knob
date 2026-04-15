#pragma once
#include "PluginProcessor.h"
#include "KnobDesign.h"
#include "BinaryData.h"

// ── Slider subclass that delegates double-click to the editor ──
class AnimatedSlider : public juce::Slider
{
public:
    std::function<void()> onDoubleClick;

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (onDoubleClick)
            onDoubleClick();
    }
};

class BoostorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit BoostorAudioProcessorEditor(BoostorAudioProcessor&);
    ~BoostorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BoostorAudioProcessor& processorRef;
    ConjusKnobLookAndFeel conjusLAF;
    AnimatedSlider gainSlider;
    juce::Label gainLabel;
    juce::Label latencyLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::Image logoImage;

    // ── Snap-to-default animation ──
    struct SliderAnimation
    {
        bool active = false;
        double targetValue = 0.0;
        double currentValue = 0.0;
    };
    SliderAnimation gainAnim;

    void startSnapAnimation(juce::Slider& slider, SliderAnimation& anim);
    void updateSnapAnimation(juce::Slider& slider, SliderAnimation& anim);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoostorAudioProcessorEditor)
};
