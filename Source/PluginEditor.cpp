#include "PluginEditor.h"

BoostorAudioProcessorEditor::BoostorAudioProcessorEditor(BoostorAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    conjusLAF.loadFonts(BinaryData::InconsolataBold_ttf,
                        BinaryData::InconsolataBold_ttfSize,
                        BinaryData::InconsolataRegular_ttf,
                        BinaryData::InconsolataRegular_ttfSize);
    setLookAndFeel(&conjusLAF);

    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 120, 30);
    addAndMakeVisible(gainSlider);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "gain", gainSlider);

    // Set text functions AFTER attachment so they aren't overridden
    gainSlider.textFromValueFunction = [](double value) -> juce::String {
        if (value <= -100.0)
            return juce::String(juce::CharPointer_UTF8("\xe2\x88\x92\xe2\x88\x9e")) + " dB";
        return juce::String(value, 1) + " dB";
    };
    gainSlider.valueFromTextFunction = [](const juce::String& text) -> double {
        if (text.containsChar(0x221e)) // ∞
            return -100.0;
        return text.getDoubleValue();
    };
    // Force text update with new function
    gainSlider.updateText();

    // ── Animated snap to default on double-click ──
    gainSlider.onDoubleClick = [this]() {
        startSnapAnimation(gainSlider, gainAnim);
    };

    latencyLabel.setText("Latency: 0ms", juce::dontSendNotification);
    latencyLabel.setJustificationType(juce::Justification::centredLeft);
    latencyLabel.setColour(juce::Label::textColourId, KnobDesign::accentColour.darker(0.3f));
    addAndMakeVisible(latencyLabel);

    // Resizable square window — restore saved size or use default
    int savedW = processorRef.editorWidth.load();
    int savedH = processorRef.editorHeight.load();
    setSize(savedW, savedH);
    setResizable(true, true);
    setResizeLimits(KnobDesign::minSize, KnobDesign::minSize,
                    KnobDesign::maxSize, KnobDesign::maxSize);
    getConstrainer()->setFixedAspectRatio(1.0);

    logoImage = juce::ImageCache::getFromMemory(
        BinaryData::conjiusavatartransparentbg_png,
        BinaryData::conjiusavatartransparentbg_pngSize);

    startTimerHz(60);
}

BoostorAudioProcessorEditor::~BoostorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void BoostorAudioProcessorEditor::timerCallback()
{
    updateSnapAnimation(gainSlider, gainAnim);

    float latencyMs = processorRef.getLastProcessLatencyMs();
    latencyLabel.setText("Latency: " + juce::String(latencyMs, 3) + "ms",
                         juce::dontSendNotification);
}

void BoostorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(KnobDesign::bgColour);

    // Draw logo in bottom-left corner
    if (logoImage.isValid())
    {
        float scale = static_cast<float>(getWidth()) / static_cast<float>(KnobDesign::defaultSize);
        int logoSize = static_cast<int>(37.5f * scale);  // 150/4 = 37.5
        int padLeft = logoSize / 3;
        g.drawImage(logoImage,
                    padLeft, getHeight() - logoSize, logoSize, logoSize,
                    0, 0, logoImage.getWidth(), logoImage.getHeight());
    }
}

void BoostorAudioProcessorEditor::resized()
{
    // Persist editor size for DAW state save/restore
    processorRef.editorWidth.store(getWidth());
    processorRef.editorHeight.store(getHeight());

    float w = static_cast<float>(getWidth());
    float margin = w * 0.1f;

    // Dynamic "Gain" label — near top
    float gainFontSize = w * KnobDesign::gainLabelScale;
    gainLabel.setFont(conjusLAF.getBoldFont(gainFontSize));
    int gainLabelH = static_cast<int>(gainFontSize * 1.2f);
    int gainLabelY = static_cast<int>(margin * 0.5f);
    gainLabel.setBounds(0, gainLabelY, getWidth(), gainLabelH);

    // Dynamic latency label — centred at bottom edge, light weight
    float latencyFontSize = w * KnobDesign::latencyTextScale;
    latencyLabel.setFont(conjusLAF.getRegularFont(latencyFontSize));
    latencyLabel.setJustificationType(juce::Justification::centredBottom);
    int latencyH = static_cast<int>(latencyFontSize * 2.0f);
    latencyLabel.setBounds(0, getHeight() - latencyH, getWidth(), latencyH);

    // Slider fills most of the window — knob draws at window centre independently
    // Text box at bottom of slider area positions the dB readout
    float dbFontSize = w * KnobDesign::dbTextScale;
    int sliderTop = gainLabelY + gainLabelH;
    // End slider area above the latency label, closer to centre
    int sliderBottom = static_cast<int>(getHeight() * 0.82f);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                               static_cast<int>(w * 0.8f),
                               static_cast<int>(dbFontSize * 2.0f));

    // Set the text box font via the slider's text editor
    gainSlider.setColour(juce::Slider::textBoxTextColourId, KnobDesign::accentColour);
    gainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    gainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    // Scale drag sensitivity with window size so larger windows don't feel too touchy
    gainSlider.setMouseDragSensitivity(static_cast<int>(w * 0.8f));

    gainSlider.setBounds(0, sliderTop, getWidth(), sliderBottom - sliderTop);

    // Update text box font size dynamically
    if (auto* textBox = gainSlider.getChildComponent(0))
    {
        if (auto* label = dynamic_cast<juce::Label*>(textBox))
        {
            label->setFont(conjusLAF.getRegularFont(dbFontSize));
            label->setMinimumHorizontalScale(1.0f);  // never stretch/compress text
            label->setColour(juce::Label::textColourId, KnobDesign::accentColour);
            label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
            label->setInterceptsMouseClicks(false, false);  // pass mouse events to slider
        }
    }
}

void BoostorAudioProcessorEditor::startSnapAnimation(juce::Slider& slider, SliderAnimation& anim)
{
    auto* param = processorRef.getAPVTS().getParameter("gain");
    if (param == nullptr) return;

    anim.currentValue = slider.getValue();
    anim.targetValue = static_cast<double>(param->getDefaultValue())
                       * (slider.getMaximum() - slider.getMinimum()) + slider.getMinimum();
    anim.active = true;
}

void BoostorAudioProcessorEditor::updateSnapAnimation(juce::Slider& slider, SliderAnimation& anim)
{
    if (!anim.active) return;

    constexpr double smoothing = 0.15;
    anim.currentValue += (anim.targetValue - anim.currentValue) * smoothing;

    if (std::abs(anim.targetValue - anim.currentValue) < 0.01)
    {
        anim.currentValue = anim.targetValue;
        anim.active = false;
    }

    slider.setValue(anim.currentValue, juce::sendNotificationAsync);
}
