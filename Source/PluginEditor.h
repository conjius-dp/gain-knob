#pragma once
#include "PluginProcessor.h"
#include "KnobDesign.h"
#include "BinaryData.h"

// ── Slider subclass that delegates double-click to the editor ──
class AnimatedSlider : public juce::Slider
{
public:
    std::function<void()> onDoubleClick;

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (onDoubleClick)
            onDoubleClick();
    }

    // Tight hit-test: only the knob circle and the value-pill rectangle intercept mouse events.
    // Everywhere else within the slider's bounds, events pass through to the parent editor.
    bool hitTest(int x, int y) override
    {
        auto* parent = getParentComponent();
        if (parent == nullptr) return true;

        float windowW = static_cast<float>(parent->getWidth());
        float windowH = static_cast<float>(parent->getHeight());
        float windowSize = juce::jmin(windowW, windowH);

        // Knob circle (matches ConjusKnobLookAndFeel)
        float knobCx = static_cast<float>(getWidth()) * 0.5f;
        float knobCy = windowH * 0.56f - static_cast<float>(getY());
        float r = windowSize * 0.35f * 0.5f;
        float dx = static_cast<float>(x) - knobCx;
        float dy = static_cast<float>(y) - knobCy;
        if (dx * dx + dy * dy <= r * r) return true;

        // Pill rect — approx centred above the slider's bottom edge
        int pillHalfW = static_cast<int>(windowSize * 0.14f);
        int pillTop    = getHeight() - static_cast<int>(windowSize * 0.11f);
        int pillBottom = getHeight() - static_cast<int>(windowSize * 0.02f);
        return (x >= static_cast<int>(knobCx) - pillHalfW
             && x <= static_cast<int>(knobCx) + pillHalfW
             && y >= pillTop
             && y <= pillBottom);
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
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Hide the conjius logo + latency label — used by the headless screenshot
    // tool so the README image doesn't include the footer chrome.
    void setChromeVisible(bool visible);

private:
    void timerCallback() override;

    bool showChrome = true;

    BoostorAudioProcessor& processorRef;
    ConjusKnobLookAndFeel conjusLAF;
    AnimatedSlider gainSlider;
    juce::Label gainLabel;
    juce::Label latencyLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::Image logoImage;
    juce::Image titleLogoImage;

    // Custom resize corner — larger than JUCE's default 16x16
    std::unique_ptr<juce::ResizableCornerComponent> resizer;

    // Conjius logo hover animation (dim -> bright + scale up on hover)
    juce::Rectangle<int> logoBounds;
    bool  logoHoverTarget   = false;
    float logoHoverProgress = 0.0f;

    // Latency label:
    //  - default mode: visible, grows 1.5x on hover
    //  - hide mode: click toggles, label slides down out of view; hovering the
    //    original hit area peeks it back in; leaving slides it back out;
    //    clicking again returns to default mode.
    // Use a dedicated child Component for the hit area so clicks always hit.
    struct HitArea : juce::Component
    {
        std::function<void()> onClick;
        std::function<void(bool)> onHover;
        HitArea() {
            setInterceptsMouseClicks(true, false);
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }
        void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseEnter(const juce::MouseEvent&) override { if (onHover) onHover(true); }
        void mouseExit (const juce::MouseEvent&) override { if (onHover) onHover(false); }
    };
    HitArea latencyHitArea;
    juce::Rectangle<int> latencyBaseBounds;     // the label's default on-screen bounds
    float latencyBaseFontSize = 1.0f;
    bool  latencyHoverTarget   = false;
    float latencyHoverProgress = 0.0f;          // 0 = normal, 1 = hovered (1.5x)
    bool  latencyHidden        = false;         // toggled by click
    float latencyHideProgress  = 0.0f;          // 0 = visible, 1 = slid out of window

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
