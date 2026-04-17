#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace KnobDesign
{
    // ── Colors (matching conji.us) ──
    inline const juce::Colour bgColour         { 0xff111111 };  // #111
    inline const juce::Colour accentColour     { 0xffd48300 };  // #d48300
    inline const juce::Colour accentHoverColour{ 0xffffb84d };  // much lighter orange for hover/press

    // ── Knob geometry (proportional to diameter) ──
    // All stroke/size values are fractions of the knob diameter
    inline constexpr float knobStrokeFrac    = 0.033f;   // circle stroke (~5px at 150px diameter)
    inline constexpr float indicatorWidthFrac= 0.040f;   // indicator stroke (~6px at 150px)
    inline constexpr float tickStrokeFrac    = 0.033f;   // tick stroke (~5px at 150px)

    // Indicator spans from 33% to 75% of radius (inner to outer — longer on circle side)
    inline constexpr float indicatorStart    = 0.33f;
    inline constexpr float indicatorEnd      = 0.75f;

    // Tick marks: start further out from circle, extend outward
    inline constexpr float tickGap           = 1.15f;   // start outside the circle (3x original gap)
    inline constexpr float tickLength        = 0.18f;   // length as fraction of radius

    // ── Rotation range ──
    // The knob rotates from -135° to +135° (270° total arc)
    inline constexpr float rotationStartAngle = -135.0f;  // degrees from 12 o'clock
    inline constexpr float rotationEndAngle   =  135.0f;

    // ── Label style (proportional to diameter / window) ──
    inline constexpr float labelFontScale    = 0.18f;   // "0"/"11" font size as fraction of diameter
    inline constexpr float gainLabelScale    = 0.06f;   // "Gain" font size as fraction of window width
    inline constexpr float dbTextScale       = 0.06f;   // dB readout as fraction of window width
    inline constexpr float latencyTextScale  = 0.017f;  // latency label as fraction of window width

    // ── Window ──
    inline constexpr int   defaultSize       = 450;   // plugin width
    inline constexpr int   defaultHeight     = 500;   // plugin height (taller than width)
    inline constexpr int   minSize           = 200;
    inline constexpr int   maxSize           = 800;

    // ── Angle helpers ──
    // Convert a normalised 0–1 value to an angle in radians from 12 o'clock
    inline float normToAngleRad(float norm01)
    {
        float degrees = rotationStartAngle + norm01 * (rotationEndAngle - rotationStartAngle);
        return juce::degreesToRadians(degrees);
    }

    // Non-deprecated replacement for juce::Font::getStringWidthFloat (which
    // was marked deprecated in JUCE 8 in favour of GlyphArrangement).
    inline float stringWidth(const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText(font, text, 0.0f, 0.0f);
        return ga.getBoundingBox(0, -1, true).getWidth();
    }
}

// ── Custom LookAndFeel ──
class ConjusKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ConjusKnobLookAndFeel()
    {
        // Slider text box colours
        setColour(juce::Slider::textBoxTextColourId, KnobDesign::accentColour);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

        // Label colours
        setColour(juce::Label::textColourId, KnobDesign::accentColour);
    }

    // Call this after BinaryData is available to load Inconsolata
    void loadFonts(const void* boldData, int boldSize, const void* regularData, int regularSize)
    {
        boldTypeface = juce::Typeface::createSystemTypefaceFor(boldData, static_cast<size_t>(boldSize));
        regularTypeface = juce::Typeface::createSystemTypefaceFor(regularData, static_cast<size_t>(regularSize));
    }

    juce::Font getBoldFont(float height) const
    {
        if (boldTypeface != nullptr)
            return juce::Font(juce::FontOptions(boldTypeface).withHeight(height));
        return juce::Font(juce::FontOptions().withHeight(height).withStyle("Bold"));
    }

    juce::Font getRegularFont(float height) const
    {
        if (regularTypeface != nullptr)
            return juce::Font(juce::FontOptions(regularTypeface).withHeight(height));
        return juce::Font(juce::FontOptions().withHeight(height));
    }

    // Orange corner resizer — three diagonal lines in the accent colour.
    // Default state uses a brighter tint + thicker strokes so the affordance
    // is clearly visible at rest (previously very thin 1.5px strokes in the
    // dim accent colour made it look invisible until the user grabbed it).
    void drawCornerResizer(juce::Graphics& g, int w, int h,
                           bool isMouseOver, bool isMouseDragging) override
    {
        const auto colour = (isMouseOver || isMouseDragging)
                            ? KnobDesign::accentHoverColour
                            : KnobDesign::accentColour.brighter(0.2f);
        g.setColour(colour);

        const float minDim = juce::jmin(static_cast<float>(w), static_cast<float>(h));
        const float inset = minDim * 0.20f;
        const float right = static_cast<float>(w);
        const float bottom = static_cast<float>(h);
        const float strokeW = juce::jmax(2.5f, minDim * 0.11f);

        for (int i = 1; i <= 3; ++i)
        {
            const float t = static_cast<float>(i) * (bottom - inset) / 4.0f;
            juce::Line<float> line{ right - 2.0f, bottom - t - 2.0f,
                                    right - t - 2.0f, bottom - 2.0f };
            g.drawLine(line, strokeW);
        }
    }

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float /*rotaryStartAngle*/, float /*rotaryEndAngle*/,
                          juce::Slider& slider) override
    {
        using namespace KnobDesign;

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        // Size knob based on window, not slider bounds (which shrink with text box)
        auto* parent = slider.getParentComponent();
        float windowW = parent ? static_cast<float>(parent->getWidth()) : bounds.getWidth();
        float windowH = parent ? static_cast<float>(parent->getHeight()) : bounds.getHeight();
        float windowSize = juce::jmin(windowW, windowH);
        float diameter = windowSize * 0.35f;
        float radius = diameter * 0.5f;
        // Centre the knob a little below the middle so the title has room at top
        float cx = bounds.getCentreX();
        float sliderY = static_cast<float>(slider.getY());
        float cy = windowH * 0.56f - sliderY;

        // Scale strokes to diameter
        float strokeW = diameter * knobStrokeFrac;
        float indW = diameter * indicatorWidthFrac;
        float tickW = diameter * tickStrokeFrac;

        // Interactive colour: smoothly interpolate toward accentHoverColour on hover/drag
        float hoverProgress = static_cast<float>(
            slider.getProperties().getWithDefault("hoverProgress", 0.0));
        auto interactiveColour = accentColour.interpolatedWith(accentHoverColour, hoverProgress);

        // ── Draw knob circle ──
        g.setColour(interactiveColour);
        g.drawEllipse(cx - radius + strokeW * 0.5f,
                      cy - radius + strokeW * 0.5f,
                      diameter - strokeW,
                      diameter - strokeW,
                      strokeW);

        // ── Draw indicator line ──
        float angle = normToAngleRad(sliderPosProportional);
        float innerR = radius * indicatorStart;
        float outerR = radius * indicatorEnd;

        juce::Path indicator;
        indicator.startNewSubPath(cx + std::sin(angle) * innerR,
                                 cy - std::cos(angle) * innerR);
        indicator.lineTo(cx + std::sin(angle) * outerR,
                         cy - std::cos(angle) * outerR);
        g.setColour(interactiveColour);
        g.strokePath(indicator,
                     juce::PathStrokeType(indW,
                                          juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        // ── Draw tick marks at -∞, 0 dB (top), and +24 dB positions ──
        float tickStartR = radius * tickGap;
        float tickEndR = radius * (tickGap + tickLength);

        float tickAngles[3] = {
            juce::degreesToRadians(rotationStartAngle),  // -∞ (left)
            0.0f,                                         // 0 dB (top, 12 o'clock)
            juce::degreesToRadians(rotationEndAngle)      // +24 dB (right)
        };

        // Ticks and tick labels always use the base accent colour (no hover highlight)
        g.setColour(accentColour);
        for (int i = 0; i < 3; ++i)
        {
            juce::Path tick;
            tick.startNewSubPath(cx + std::sin(tickAngles[i]) * tickStartR,
                                cy - std::cos(tickAngles[i]) * tickStartR);
            tick.lineTo(cx + std::sin(tickAngles[i]) * tickEndR,
                        cy - std::cos(tickAngles[i]) * tickEndR);
            g.strokePath(tick,
                         juce::PathStrokeType(tickW,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
        }

        // ── Draw labels ──
        float fontSize = diameter * labelFontScale;
        float markerFontSize = fontSize * 0.85f;   // size for 0, +24, and the minus sign
        float infFontSize = fontSize * 1.8f;        // large infinity symbol (slightly thinner)
        g.setColour(accentColour);

        float labelR = tickEndR + fontSize * 0.8f;
        float labelYOffset = fontSize * 0.05f;

        // "−∞" label — below-left tick: minus at markerFontSize, ∞ at infFontSize
        float angle0 = juce::degreesToRadians(rotationStartAngle);
        float label0X = cx + std::sin(angle0) * labelR;
        float label0Y = cy - std::cos(angle0) * labelR + labelYOffset;
        {
            // Measure the minus sign width at marker size
            g.setFont(getBoldFont(markerFontSize));
            auto minusStr = juce::String(juce::CharPointer_UTF8("\xe2\x88\x92"));
            float minusW = KnobDesign::stringWidth(g.getCurrentFont(), minusStr);

            // Measure the infinity symbol width at inf size (thinner weight)
            g.setFont(getRegularFont(infFontSize));
            auto infStr = juce::String(juce::CharPointer_UTF8("\xe2\x88\x9e"));
            float infW = KnobDesign::stringWidth(g.getCurrentFont(), infStr);

            float totalW = minusW + infW;
            float startX = label0X - totalW * 0.5f;

            // Draw minus sign
            g.setFont(getBoldFont(markerFontSize));
            g.drawText(minusStr,
                       juce::Rectangle<float>(startX, label0Y - infFontSize * 0.5f,
                                              minusW, infFontSize),
                       juce::Justification::centred, false);

            // Draw infinity symbol (thinner, shifted slightly up and right)
            float infNudgeX = diameter * 0.01f;
            float infNudgeY = -diameter * 0.01f;
            g.setFont(getRegularFont(infFontSize));
            g.drawText(infStr,
                       juce::Rectangle<float>(startX + minusW + infNudgeX,
                                              label0Y - infFontSize * 0.5f + infNudgeY,
                                              infW, infFontSize),
                       juce::Justification::centred, false);
        }

        // "+24" label — below-right tick
        g.setFont(getBoldFont(markerFontSize));
        float angle24 = juce::degreesToRadians(rotationEndAngle);
        float label24X = cx + std::sin(angle24) * labelR - diameter * 0.012f;
        float label24Y = cy - std::cos(angle24) * labelR + labelYOffset - diameter * 0.012f;
        g.drawText("+24",
                   juce::Rectangle<float>(label24X - fontSize * 2.0f, label24Y - markerFontSize * 0.5f,
                                          fontSize * 4.0f, markerFontSize * 1.2f),
                   juce::Justification::centred, false);

        // "0" label — above top tick (12 o'clock), close to tick
        float topLabelR = tickEndR + markerFontSize * 0.3f;
        float topLabelX = cx;
        float topLabelY = cy - topLabelR - markerFontSize * 0.5f;
        g.drawText("0",
                   juce::Rectangle<float>(topLabelX - fontSize * 2.0f, topLabelY - markerFontSize * 0.5f,
                                          fontSize * 4.0f, markerFontSize * 1.2f),
                   juce::Justification::centred, false);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        // Check if this label belongs to a slider (the dB text box)
        bool isSliderTextBox = (dynamic_cast<juce::Slider*>(label.getParentComponent()) != nullptr);

        if (isSliderTextBox)
        {
            auto text = label.getText();
            auto font = label.getFont();
            float baseHeight = font.getHeight();
            bool hasInfinity = text.containsChar(0x221e);

            // ── Fixed pill dimensions ──
            // Pill layout: [padLeft | minus zone | gap | value zone | " dB" | padRight]
            // Minus is pinned to left edge; " dB" is pinned to right edge; value sits between.

            // Use exact same font sizes as knob marker labels
            // Knob: diameter = windowSize * 0.42, fontSize = diameter * 0.18
            //        markerFontSize = fontSize * 0.85, infFontSize = fontSize * 1.8
            auto* slider = dynamic_cast<juce::Slider*>(label.getParentComponent());
            auto* editor = slider ? slider->getParentComponent() : nullptr;
            float windowSize = editor
                ? juce::jmin(static_cast<float>(editor->getWidth()),
                             static_cast<float>(editor->getHeight()))
                : 450.0f;
            float knobDiameter = windowSize * 0.42f;
            float knobFontSize = knobDiameter * KnobDesign::labelFontScale;
            float pillMinusFontSize = knobFontSize * 0.85f;
            float pillInfFontSize = knobFontSize * 1.8f;
            auto pillMinusFont = getBoldFont(pillMinusFontSize);
            auto pillInfFont = getRegularFont(pillInfFontSize);

            // dB suffix uses bold (same weight as +24 marker)
            auto dbFont = getBoldFont(baseHeight);
            juce::String dbSuffix = " dB";
            float dbW = KnobDesign::stringWidth(dbFont, dbSuffix);
            auto baseFont = getRegularFont(baseHeight);

            // Measure minus sign width (static left zone)
            auto minusStr = juce::String(juce::CharPointer_UTF8("\xe2\x88\x92"));
            float minusW = KnobDesign::stringWidth(pillMinusFont, minusStr);
            float minusGap = baseHeight * 0.15f;  // gap between minus and value

            // Measure widest possible value area
            // Widest numeric: "99.9" (digits only, sign is separate)
            // Widest symbol: ∞ at knob marker size
            auto infStr = juce::String(juce::CharPointer_UTF8("\xe2\x88\x9e"));
            float infW = KnobDesign::stringWidth(pillInfFont, infStr);
            float numW = KnobDesign::stringWidth(baseFont, "99.9");
            float valueZoneW = juce::jmax(infW, numW);

            float pillH = baseHeight * 1.4f;
            float padLeft = pillH * 0.45f;
            float padRight = pillH * 0.25f;
            float pillW = padLeft + minusW + minusGap + valueZoneW + dbW + padRight;

            // Centre the pill horizontally in the label
            float labelW = static_cast<float>(label.getWidth());
            float pillX = (labelW - pillW) * 0.5f;
            float pillY = (static_cast<float>(label.getHeight()) - pillH) * 0.5f - pillH * 0.2f;

            auto pillBounds = juce::Rectangle<float>(pillX, pillY, pillW, pillH);

            // Interactive pill colour: smoothly animate toward accentHoverColour when parent slider is hovered/dragged
            float pillHoverProgress = 0.0f;
            if (auto* parentSlider = label.findParentComponentOfClass<juce::Slider>())
                pillHoverProgress = static_cast<float>(
                    parentSlider->getProperties().getWithDefault("hoverProgress", 0.0));
            auto pillFillColour = KnobDesign::accentColour
                .interpolatedWith(KnobDesign::accentHoverColour, pillHoverProgress);

            // Draw orange pill background with fully circular edges
            g.setColour(pillFillColour);
            g.fillRoundedRectangle(pillBounds, pillH * 0.5f);

            g.setColour(KnobDesign::bgColour);
            float centreY = pillBounds.getCentreY();

            // ── Fixed right zone: " dB" always at same position, bold weight ──
            float dbX = pillBounds.getRight() - padRight - dbW;
            g.setFont(dbFont);
            g.drawText(dbSuffix,
                       juce::Rectangle<float>(dbX, centreY - baseHeight * 0.5f,
                                              dbW, baseHeight),
                       juce::Justification::centred, false);

            // ── Fixed left zone: minus sign pinned to left edge (when negative) ──
            float minusX = pillBounds.getX() + padLeft;
            // Value zone sits right of the minus+gap area, right-aligned against " dB"
            float valueRight = dbX;
            float valueLeft = minusX + minusW + minusGap;

            if (hasInfinity)
            {
                // Draw minus pinned to left — bold, same as knob marker
                g.setFont(pillMinusFont);
                g.drawText(minusStr,
                           juce::Rectangle<float>(minusX, centreY - pillMinusFontSize * 0.5f,
                                                  minusW, pillMinusFontSize),
                           juce::Justification::centred, false);

                // Draw ∞ — regular weight, same as knob marker, nudged up to centre in pill
                float infNudgeY = windowSize * -0.004f;
                g.setFont(pillInfFont);
                g.drawText(infStr,
                           juce::Rectangle<float>(valueLeft, centreY - pillInfFontSize * 0.5f + infNudgeY,
                                                  valueRight - valueLeft, pillInfFontSize),
                           juce::Justification::centredLeft, false);
            }
            else
            {
                // Parse the value text: strip " dB" suffix, split sign from digits
                juce::String valueStr = text.replace(" dB", "").trim();
                bool isNegative = valueStr.startsWith("-");
                bool isPositive = valueStr.getDoubleValue() > 0.0;
                juce::String digits = isNegative ? valueStr.substring(1) : valueStr;
                // Strip leading '+' if present in the digits
                if (digits.startsWith("+"))
                    digits = digits.substring(1);

                if (isNegative)
                {
                    // Draw minus pinned to left — bold, same as knob marker
                    g.setFont(pillMinusFont);
                    g.drawText(minusStr,
                               juce::Rectangle<float>(minusX, centreY - pillMinusFontSize * 0.5f,
                                                      minusW, pillMinusFontSize),
                               juce::Justification::centred, false);
                }
                else if (isPositive)
                {
                    // Draw plus pinned to left — bold, same weight as minus
                    g.setFont(pillMinusFont);
                    g.drawText("+",
                               juce::Rectangle<float>(minusX, centreY - pillMinusFontSize * 0.5f,
                                                      minusW, pillMinusFontSize),
                               juce::Justification::centred, false);
                }

                // Draw value digits left-aligned in the value zone — bold, same as dB
                g.setFont(dbFont);
                g.drawText(digits,
                           juce::Rectangle<float>(valueLeft, centreY - baseHeight * 0.5f,
                                                  valueRight - valueLeft, baseHeight),
                           juce::Justification::centredLeft, false);
            }
        }
        else
        {
            // Default rendering for other labels (Gain, Latency)
            LookAndFeel_V4::drawLabel(g, label);
        }
    }

private:
    juce::Typeface::Ptr boldTypeface;
    juce::Typeface::Ptr regularTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConjusKnobLookAndFeel)
};
