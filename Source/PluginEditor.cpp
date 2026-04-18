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
    gainSlider.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible(gainSlider);

    gainLabel.setText("GAIN", juce::dontSendNotification);
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

    latencyLabel.setText("LATENCY: 0.000ms", juce::dontSendNotification);
    latencyLabel.setJustificationType(juce::Justification::centredLeft);
    latencyLabel.setColour(juce::Label::textColourId, KnobDesign::accentColour.darker(0.3f));
    // Label doesn't intercept — the HitArea above it handles hover and clicks
    latencyLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(latencyLabel);

    latencyHitArea.onClick = [this]() { latencyHidden = !latencyHidden; };
    latencyHitArea.onHover = [this](bool over) { latencyHoverTarget = over; };
    addAndMakeVisible(latencyHitArea);
    latencyHitArea.toFront(false); // keep above latencyLabel

    // Bypass button — power-switch in the top-right corner. Drives the
    // APVTS bool parameter via ButtonAttachment; processBlock early-returns
    // on that flag for a true untouched pass-through.
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "bypass", bypassButton);

    // Resizable window (slightly taller than wide) — restore saved size or use default
    int savedW = processorRef.editorWidth.load();
    int savedH = processorRef.editorHeight.load();
    setResizable(true, false); // we provide our own, larger corner
    // Aspect ratio of width:height, fixed at default proportions (450:500 = 0.9)
    const double aspect = static_cast<double>(KnobDesign::defaultSize)
                        / static_cast<double>(KnobDesign::defaultHeight);
    // Height limits scale from width limits via aspect
    const int minH = static_cast<int>(KnobDesign::minSize / aspect);
    const int maxH = static_cast<int>(KnobDesign::maxSize / aspect);
    setResizeLimits(KnobDesign::minSize, minH, KnobDesign::maxSize, maxH);
    getConstrainer()->setFixedAspectRatio(aspect);
    resizer = std::make_unique<juce::ResizableCornerComponent>(this, getConstrainer());
    resizer->setLookAndFeel(&conjusLAF);
    addAndMakeVisible(resizer.get());
    // setSize() must come AFTER the resizer is created — it triggers resized()
    // which positions the resizer. Before this reorder, resized() ran while
    // `resizer` was still null, so the handle stayed at 0x0 until the first
    // user-driven resize.
    setSize(savedW, savedH);

    logoImage = juce::ImageCache::getFromMemory(
        BinaryData::conjiusavatartransparentbg_png,
        BinaryData::conjiusavatartransparentbg_pngSize);

    titleLogoImage = juce::ImageCache::getFromMemory(
        BinaryData::boostorlogoorange_png,
        BinaryData::boostorlogoorange_pngSize);

    // Receive mouse events from self and children (for conjius logo hover detection)
    addMouseListener(this, true);

    startTimerHz(60);
}

BoostorAudioProcessorEditor::~BoostorAudioProcessorEditor()
{
    if (resizer) resizer->setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    stopTimer();
}

void BoostorAudioProcessorEditor::setChromeVisible(bool visible)
{
    showChrome = visible;
    latencyLabel.setVisible(visible);
    latencyHitArea.setVisible(visible);
    repaint();
}

void BoostorAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    auto pos = e.getEventRelativeTo(this).getPosition();
    logoHoverTarget = logoBounds.contains(pos);
    setMouseCursor(logoHoverTarget
                   ? juce::MouseCursor::PointingHandCursor
                   : juce::MouseCursor::NormalCursor);
}

void BoostorAudioProcessorEditor::mouseExit(const juce::MouseEvent& e)
{
    if (e.eventComponent == this)
    {
        logoHoverTarget = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void BoostorAudioProcessorEditor::mouseDown(const juce::MouseEvent& /*e*/) {}

void BoostorAudioProcessorEditor::timerCallback()
{
    updateSnapAnimation(gainSlider, gainAnim);

    // Update latency text only every 12th frame (~5 Hz) so the number is readable
    static int latencyTick = 0;
    if (++latencyTick >= 12)
    {
        latencyTick = 0;
        // Algorithmic latency — what the host compensates for and what users
        // hear vs. bypass. Boostor's is zero (no buffering / no FFT).
        float latencyMs = processorRef.getAlgorithmicLatencyMs();
        latencyLabel.setText("LATENCY: " + juce::String(latencyMs, 3) + "ms",
                             juce::dontSendNotification);
    }

    // Animate conjius logo hover state
    float target = logoHoverTarget ? 1.0f : 0.0f;
    if (std::abs(target - logoHoverProgress) > 0.002f)
    {
        logoHoverProgress += (target - logoHoverProgress) * 0.18f;
        repaint(logoBounds.expanded(static_cast<int>(logoBounds.getWidth() * 0.2f)));
    }

    // Animate knob hover colour — restrict hover to the knob circle itself + the pill,
    // not the slider's full bounding rect (which spans the whole window).
    {
        auto& props = gainSlider.getProperties();
        float current = static_cast<float>(props.getWithDefault("hoverProgress", 0.0));

        auto mouse = getMouseXYRelative();
        float windowSize = static_cast<float>(juce::jmin(getWidth(), getHeight()));
        float knobCx = static_cast<float>(getWidth()) * 0.5f;
        float knobCy = static_cast<float>(getHeight()) * 0.56f;
        float knobR  = windowSize * 0.35f * 0.5f;
        float dx = static_cast<float>(mouse.x) - knobCx;
        float dy = static_cast<float>(mouse.y) - knobCy;
        bool overKnob = (dx * dx + dy * dy) <= (knobR * knobR);

        // Pill hit area: approximate rectangle just below the knob within the slider's text box
        auto sbounds = gainSlider.getBounds();
        int pillW = static_cast<int>(windowSize * 0.28f);
        int pillH = static_cast<int>(windowSize * 0.07f);
        juce::Rectangle<int> pillHit(sbounds.getCentreX() - pillW / 2,
                                     sbounds.getBottom() - pillH - 4,
                                     pillW, pillH);
        bool overPill = pillHit.contains(mouse);
        bool dragging = gainSlider.isMouseButtonDown(false);

        float dest = (overKnob || overPill || dragging) ? 1.0f : 0.0f;
        if (std::abs(dest - current) > 0.002f)
        {
            current += (dest - current) * 0.22f;
            props.set("hoverProgress", current);
            gainSlider.repaint();
        }
    }

    // Animate latency label: hover growth (when visible) + slide out/in (when hidden)
    {
        // Hover growth: scales 3x on hover in both modes (visible and peek)
        float hoverDest = latencyHoverTarget ? 1.0f : 0.0f;
        if (std::abs(hoverDest - latencyHoverProgress) > 0.002f)
            latencyHoverProgress += (hoverDest - latencyHoverProgress) * 0.22f;

        // Hide: target 1 if hidden+not hovered (fully slid out); 0 if visible or peeking
        float hideDest;
        if (!latencyHidden)
            hideDest = 0.0f;
        else
            hideDest = latencyHoverTarget ? 0.0f : 1.0f;
        if (std::abs(hideDest - latencyHideProgress) > 0.002f)
            latencyHideProgress += (hideDest - latencyHideProgress) * 0.09f;

        // Apply font size and position each frame
        if (!latencyBaseBounds.isEmpty())
        {
            float scale = 1.0f + 1.4f * latencyHoverProgress; // 1.0 → 2.4x
            latencyLabel.setFont(conjusLAF.getRegularFont(latencyBaseFontSize * scale));

            // Slide down: fully offscreen when hideProgress = 1
            float slidePx = static_cast<float>(latencyBaseBounds.getHeight()) * 2.0f * latencyHideProgress;
            // Expand label bounds to fit the scaled text without clipping, anchoring
            // the expansion to the bottom of the label (so growth stays above the baseline)
            int scaledH = static_cast<int>(latencyBaseBounds.getHeight() * scale);
            int extra = scaledH - latencyBaseBounds.getHeight();
            auto bounds = latencyBaseBounds.withY(latencyBaseBounds.getY() - extra)
                                           .withHeight(scaledH)
                                           .translated(0, static_cast<int>(slidePx));
            latencyLabel.setBounds(bounds);
        }
    }
}

void BoostorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(KnobDesign::bgColour);

    // Draw conjius logo in bottom-left — dim by default, brighten + scale up on hover
    if (logoImage.isValid() && showChrome)
    {
        float scale = static_cast<float>(getWidth()) / static_cast<float>(KnobDesign::defaultSize);
        int baseSize = static_cast<int>(37.5f * scale);
        int padLeft = baseSize / 3;
        int baseX = padLeft;
        int baseY = getHeight() - baseSize;
        logoBounds = { baseX, baseY, baseSize, baseSize };

        float hoverScale = 1.0f + 0.2f * logoHoverProgress;
        int drawSize = static_cast<int>(baseSize * hoverScale);
        int drawX = baseX + (baseSize - drawSize) / 2;
        int drawY = baseY + (baseSize - drawSize) / 2;
        float brightness = 0.35f + 0.65f * logoHoverProgress;
        g.setOpacity(brightness);
        g.drawImage(logoImage,
                    drawX, drawY, drawSize, drawSize,
                    0, 0, logoImage.getWidth(), logoImage.getHeight());
        g.setOpacity(1.0f);
    }

    // Title logo at top-centre, small
    if (titleLogoImage.isValid())
    {
        float w = static_cast<float>(getWidth());
        float h = static_cast<float>(getHeight());
        float titleH = h * 0.09f;
        float aspect = static_cast<float>(titleLogoImage.getWidth())
                     / static_cast<float>(titleLogoImage.getHeight());
        float titleW = titleH * aspect;
        float titleX = (w - titleW) * 0.5f;
        // Logo sits ~half-logo-height lower than before so it clears the new
        // bypass button in the top-right corner and feels less cramped
        // against the border arc.
        float titleY = h * 0.085f + titleH * 0.5f;
        g.drawImage(titleLogoImage,
                    juce::Rectangle<float>(titleX, titleY, titleW, titleH),
                    juce::RectanglePlacement::centred);
    }

    // Rounded orange border inside ~20px of bg padding. Radius matches the
    // gain knob (windowSize * 0.35 / 2 → ~79px at default size).
    {
        const float scaleF  = static_cast<float>(getWidth())
                            / static_cast<float>(KnobDesign::defaultSize);
        const float pad     = 20.0f * scaleF;
        const float borderW = 4.0f  * scaleF;
        const float radius  = 79.0f * scaleF;
        juce::Rectangle<float> borderRect{ pad, pad,
                                           static_cast<float>(getWidth())  - 2.0f * pad,
                                           static_cast<float>(getHeight()) - 2.0f * pad };
        juce::Path border;
        border.addRoundedRectangle(borderRect, radius);
        g.setColour(KnobDesign::accentColour);
        g.strokePath(border, juce::PathStrokeType(borderW));
    }
}

void BoostorAudioProcessorEditor::resized()
{
    // Persist editor size for DAW state save/restore
    processorRef.editorWidth.store(getWidth());
    processorRef.editorHeight.store(getHeight());

    if (resizer != nullptr)
    {
        const int handleSize = 28;
        resizer->setBounds(getWidth() - handleSize, getHeight() - handleSize,
                           handleSize, handleSize);
        resizer->toFront(false);
        resizer->repaint(); // force initial paint so handle is visible at rest
    }

    float w = static_cast<float>(getWidth());

    // Bypass button — top-right corner. Padding on top + right are equal
    // and larger than the first pass so the button sits comfortably inside
    // the orange border, a bit lower and further left than the original
    // tight-corner position.
    {
        const float scaleF  = w / static_cast<float>(KnobDesign::defaultSize);
        const float btnPad  = 56.0f * scaleF;   // was 36 — nudged inward
        const float btnSize = 34.0f * scaleF;
        const float btnX    = static_cast<float>(getWidth()) - btnPad - btnSize;
        const float btnY    = btnPad;
        bypassButton.setBounds(static_cast<int>(btnX),
                               static_cast<int>(btnY),
                               static_cast<int>(btnSize),
                               static_cast<int>(btnSize));
        bypassButton.toFront(false);
    }

    // Dynamic "Gain" label — placed below the title logo
    float gainFontSize = w * KnobDesign::gainLabelScale;
    gainLabel.setFont(conjusLAF.getBoldFont(gainFontSize));
    int gainLabelH = static_cast<int>(gainFontSize * 1.2f);
    // Title logo spans h * [0.04, 0.125], so start "Gain" just below it
    int gainLabelY = static_cast<int>(getHeight() * 0.27f);  // small portion of extra height added here, below the logo
    gainLabel.setBounds(0, gainLabelY, getWidth(), gainLabelH);

    // Dynamic latency label — centred at bottom edge, light weight
    float latencyFontSize = w * KnobDesign::latencyTextScale;
    latencyLabel.setFont(conjusLAF.getRegularFont(latencyFontSize));
    latencyLabel.setJustificationType(juce::Justification::centredBottom);
    int latencyH = static_cast<int>(latencyFontSize * 2.0f);
    latencyBaseBounds = { 0, getHeight() - latencyH, getWidth(), latencyH };
    latencyBaseFontSize = latencyFontSize;
    // Hit area: narrow — matches the actual text width with a small horizontal pad
    auto latencyFont = conjusLAF.getRegularFont(latencyFontSize);
    int textW = static_cast<int>(KnobDesign::stringWidth(latencyFont, "LATENCY: 0.000ms"));
    int hitPadX = static_cast<int>(latencyFontSize * 0.8f);
    int hitPadY = latencyH;
    int hitW = textW + 2 * hitPadX;
    int hitX = (getWidth() - hitW) / 2;
    latencyHitArea.setBounds(hitX, getHeight() - latencyH - hitPadY, hitW, latencyH + hitPadY);
    latencyHitArea.toFront(false);
    // Initial label bounds: apply current animation state in case resize happens mid-anim
    int slideOffset = static_cast<int>(latencyH * 2.0f * latencyHideProgress);
    latencyLabel.setBounds(latencyBaseBounds.translated(0, slideOffset));

    // Slider fills most of the window — knob draws at window centre independently
    // Text box at bottom of slider area positions the dB readout
    float dbFontSize = w * KnobDesign::dbTextScale;
    int sliderTop = gainLabelY + gainLabelH;
    // End slider area above the latency label, closer to centre
    int sliderBottom = static_cast<int>(getHeight() * 0.90f);
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
    auto range = juce::NormalisableRange<float>(-100.0f, 24.0f, 0.1f, 3.25f);
    anim.targetValue = static_cast<double>(range.convertFrom0to1(param->getDefaultValue()));
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
