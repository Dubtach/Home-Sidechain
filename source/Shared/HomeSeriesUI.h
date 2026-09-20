#pragma once

#include <JuceHeader.h>
#include <functional>
#include <cmath>

// =============================================================================
// Home series look.
//
// The palette and the card treatment come straight from Home-Disto so the
// whole family reads as one product: near-black chassis, a slightly lighter
// face plate, and saturated gradient cards with dark text on top of them.
//
// Everything here is header-only and defined inline, because the Trigger and
// the Receiver are separate binaries that both include it.
// =============================================================================

namespace homeUI
{
    inline const juce::Colour chassis  (0xff09090b);
    inline const juce::Colour face     (0xff111114);
    inline const juce::Colour faceEdge (0xff222228);
    inline const juce::Colour rule     (0xff1e1e24);
    inline const juce::Colour slot     (0xff161618);
    inline const juce::Colour slotAlt  (0xff1a1a1e);
    inline const juce::Colour slotEdge (0xff2a2a30);
    inline const juce::Colour ink      (0xff09090b);   // text sitting on a card
    inline const juce::Colour cyan     (0xff00e5ff);
    inline const juce::Colour green    (0xff00ff87);
    inline const juce::Colour purple   (0xffb900ff);
    inline const juce::Colour pink     (0xffff007f);
    inline const juce::Colour warn     (0xffff6b7a);

    inline juce::Font font (float size, bool bold = true)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica")
                                                   .withStyle (bold ? "Bold" : "Plain"));
    }

    // A saturated card: drop shadow, vertical gradient, glass sheen along the
    // top, a fine cross-hatch, and a hard dark edge.
    inline void drawCard (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour base)
    {
        for (int i = 1; i <= 6; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.11f * static_cast<float> (7 - i)));
            g.fillRoundedRectangle (bounds.expanded (static_cast<float> (i) * 0.4f)
                                          .translated (0.0f, 1.6f + static_cast<float> (i) * 0.45f), 8.0f);
        }

        juce::ColourGradient body (base, bounds.getX(), bounds.getY(),
                                   base.darker (0.20f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (bounds, 6.0f);

        {
            const auto sheenArea = bounds.withHeight (bounds.getHeight() * 0.42f);
            juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.10f), sheenArea.getX(), sheenArea.getY(),
                                        juce::Colours::white.withAlpha (0.0f), sheenArea.getX(), sheenArea.getBottom(), false);
            juce::Path clip;
            clip.addRoundedRectangle (bounds, 6.0f);

            g.saveState();
            g.reduceClipRegion (clip);
            g.setGradientFill (sheen);
            g.fillRect (sheenArea);
            g.restoreState();
        }

        g.setColour (juce::Colours::black.withAlpha (0.12f));

        for (float y = bounds.getY() + 4.0f; y < bounds.getBottom() - 2.0f; y += 4.0f)
            g.drawLine (bounds.getX() + 2.0f, y, bounds.getRight() - 2.0f, y, 1.2f);

        for (float x = bounds.getX() + 4.0f; x < bounds.getRight() - 2.0f; x += 4.0f)
            g.drawLine (x, bounds.getY() + 2.0f, x, bounds.getBottom() - 2.0f, 1.2f);

        g.setColour (juce::Colours::black.withAlpha (0.50f));
        g.drawRoundedRectangle (bounds, 6.0f, 2.0f);
    }

    // Dark text on a card, with the same faint emboss Home-Disto uses.
    inline void drawCardText (juce::Graphics& g, const juce::String& text,
                              juce::Rectangle<float> area, float size,
                              juce::Justification justification = juce::Justification::centred,
                              float alpha = 1.0f)
    {
        g.setFont (font (size));
        g.setColour (juce::Colours::black.withAlpha (0.30f * alpha));
        g.drawText (text, area.translated (0.0f, 1.0f), justification, false);
        g.setColour (ink.withAlpha (alpha));
        g.drawText (text, area, justification, false);
    }

    inline void drawCardTitle (juce::Graphics& g, const juce::String& title, juce::Rectangle<float> card)
    {
        const auto row = juce::Rectangle<float> (card.getX(), card.getY() + 7.0f, card.getWidth(), 15.0f);
        drawCardText (g, title, row, 12.0f);

        const float width = 20.0f;
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawLine (card.getCentreX() - width, row.getBottom() + 1.0f,
                    card.getCentreX() + width, row.getBottom() + 1.0f, 1.2f);
    }

    // A recessed dark well, for anything that needs to read as a screen.
    inline void drawWell (juce::Graphics& g, juce::Rectangle<float> bounds, float radius = 5.0f)
    {
        g.setColour (chassis.withAlpha (0.92f));
        g.fillRoundedRectangle (bounds, radius);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawRoundedRectangle (bounds, radius, 1.4f);
    }

    inline void drawTrackedText (juce::Graphics& g, const juce::String& text,
                                 float x, float y, float height, float totalWidth, float size)
    {
        if (text.isEmpty())
            return;

        const auto f = font (size);
        g.setFont (f);

        float natural = 0.0f;

        for (int i = 0; i < text.length(); ++i)
            natural += static_cast<float> (juce::GlyphArrangement::getStringWidthInt (f, text.substring (i, i + 1)));

        const float extra = text.length() > 1
            ? (totalWidth - natural) / static_cast<float> (text.length() - 1)
            : 0.0f;

        float cursor = x;

        for (int i = 0; i < text.length(); ++i)
        {
            const auto glyph = text.substring (i, i + 1);
            const auto width = static_cast<float> (juce::GlyphArrangement::getStringWidthInt (f, glyph));
            g.drawText (glyph, juce::Rectangle<float> (cursor, y, width + 2.0f, height),
                        juce::Justification::centredLeft, false);
            cursor += width + extra;
        }
    }

    // The Home-Disto header: name, accent half, company line, hairline rule.
    inline void drawBrand (juce::Graphics& g, const juce::String& tail, juce::Colour accent,
                           float x, float y, float ruleRight)
    {
        const auto titleFont = font (22.0f);
        g.setFont (titleFont);

        const juce::String head ("Home-");
        const auto headWidth = static_cast<float> (juce::GlyphArrangement::getStringWidthInt (titleFont, head));
        const auto tailWidth = static_cast<float> (juce::GlyphArrangement::getStringWidthInt (titleFont, tail));

        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawText (head, juce::Rectangle<float> (x + 1.0f, y + 1.0f, headWidth, 28.0f),
                    juce::Justification::centredLeft, false);
        g.drawText (tail, juce::Rectangle<float> (x + headWidth + 1.0f, y + 1.0f, tailWidth + 8.0f, 28.0f),
                    juce::Justification::centredLeft, false);

        g.setColour (juce::Colours::white);
        g.drawText (head, juce::Rectangle<float> (x, y, headWidth, 28.0f),
                    juce::Justification::centredLeft, false);
        g.setColour (accent);
        g.drawText (tail, juce::Rectangle<float> (x + headWidth, y, tailWidth + 8.0f, 28.0f),
                    juce::Justification::centredLeft, false);

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        drawTrackedText (g, "DUBTACH DSP", x + 1.0f, y + 28.0f, 11.0f, headWidth + tailWidth, 8.0f);

        g.setColour (rule);
        g.drawLine (x, y + 46.0f, ruleRight, y + 46.0f, 2.0f);
    }

    // =========================================================================
    // Controls
    // =========================================================================

    // Dark chip, white label; goes solid white with dark text when selected.
    // Same read as Home-Disto's MODE buttons.
    class Pill : public juce::Button
    {
    public:
        Pill (const juce::String& text, juce::Colour accentColour = cyan)
            : juce::Button (text), accent (accentColour)
        {
            setButtonText (text);
            setClickingTogglesState (false);
        }

        void setAccent (juce::Colour c) { accent = c; repaint(); }
        void setFontSize (float s) noexcept { fontSize = s; }
        void setCornerRadius (float r) noexcept { corner = r; }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            const auto r = getLocalBounds().toFloat().reduced (0.5f);
            const bool on = getToggleState();

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (r.translated (0.0f, 1.2f), corner);

            if (on)
            {
                g.setColour (juce::Colours::white);
                g.fillRoundedRectangle (r, corner);
            }
            else
            {
                g.setColour (slot.withAlpha (0.94f));
                g.fillRoundedRectangle (r, corner);
                g.setColour (juce::Colours::white.withAlpha (0.14f));
                g.drawRoundedRectangle (r, corner, 1.0f);
            }

            if (over || down)
            {
                g.setColour (juce::Colours::white.withAlpha (down ? 0.16f : 0.08f));
                g.fillRoundedRectangle (r, corner);
            }

            g.setFont (font (fontSize));
            g.setColour (on ? ink : juce::Colours::white.withAlpha (isEnabled() ? 0.86f : 0.35f));
            g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);

            if (on)
            {
                g.setColour (accent.withAlpha (0.55f));
                g.drawRoundedRectangle (r, corner, 1.4f);
            }
        }

    private:
        juce::Colour accent;
        float fontSize = 10.0f;
        float corner = 5.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pill)
    };

    // A checkbox-style toggle, for a single on/off modifier -- matches
    // Home-Disto's own "OUTPUT [x] AUTO" convention. Deliberately a
    // different shape from Pill: Pill is for picking one of several
    // mutually-exclusive options (TRIG vs HOST, A vs B vs C...), Checkbox is
    // for a single independent on/off setting (Sync, Always Snap...).
    // Putting both concepts in identically-shaped pills was what made it
    // hard to tell, at a glance, which control did which kind of thing.
    class Checkbox : public juce::Button
    {
    public:
        explicit Checkbox (const juce::String& label, juce::Colour accentColour = green)
            : juce::Button (label), accent (accentColour)
        {
            setButtonText (label);
            setClickingTogglesState (false);
        }

        void setAccent (juce::Colour c) { accent = c; repaint(); }
        void setFontSize (float s) noexcept { fontSize = s; }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            const auto bounds = getLocalBounds().toFloat();
            const float boxSize = juce::jmin (15.0f, bounds.getHeight());
            const auto box = juce::Rectangle<float> (bounds.getX(), bounds.getCentreY() - boxSize * 0.5f,
                                                      boxSize, boxSize);
            const bool on = getToggleState();
            const bool enabled = isEnabled();

            g.setColour (juce::Colours::black.withAlpha (enabled ? 0.45f : 0.25f));
            g.fillRoundedRectangle (box, 3.0f);

            if (on)
            {
                g.setColour (accent.withAlpha (enabled ? 1.0f : 0.45f));
                g.fillRoundedRectangle (box.reduced (2.2f), 2.0f);

                juce::Path check;
                check.startNewSubPath (box.getX() + 3.4f, box.getCentreY() + 0.5f);
                check.lineTo (box.getX() + box.getWidth() * 0.42f, box.getBottom() - 3.2f);
                check.lineTo (box.getRight() - 3.0f, box.getY() + 3.4f);
                g.setColour (enabled ? ink : ink.withAlpha (0.6f));
                g.strokePath (check, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
            }

            g.setColour (juce::Colours::white.withAlpha (enabled ? (on ? 0.35f : 0.30f) : 0.14f));
            g.drawRoundedRectangle (box, 3.0f, 1.1f);

            if ((over || down) && enabled)
            {
                g.setColour (juce::Colours::white.withAlpha (down ? 0.14f : 0.07f));
                g.fillRoundedRectangle (box.expanded (2.0f), 4.0f);
            }

            g.setFont (font (fontSize, on));
            g.setColour (! enabled ? juce::Colours::black.withAlpha (0.30f) : ink.withAlpha (on ? 1.0f : 0.72f));
            g.drawText (getButtonText(), bounds.withTrimmedLeft (boxSize + 7.0f),
                        juce::Justification::centredLeft, false);
        }

    private:
        juce::Colour accent;
        float fontSize = 9.5f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Checkbox)
    };

    // Round power switch, top right, exactly like Home-Disto's.
    class PowerButton : public juce::Button
    {
    public:
        PowerButton() : juce::Button ("Power") { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool over, bool) override
        {
            const auto r = getLocalBounds().toFloat().reduced (1.0f);
            // Toggled on means bypassed, so the lamp is lit when NOT engaged.
            const bool bypassed = getToggleState();
            const auto colour = bypassed ? juce::Colours::white.withAlpha (0.30f) : green;

            g.setColour (juce::Colours::black.withAlpha (over ? 0.55f : 0.40f));
            g.fillEllipse (r);
            g.setColour (colour.withAlpha (0.55f));
            g.drawEllipse (r, 1.2f);

            const auto cx = r.getCentreX();
            const auto cy = r.getCentreY() + 1.0f;

            juce::Path arc;
            arc.addCentredArc (cx, cy, 5.0f, 5.0f, 0.0f,
                               juce::MathConstants<float>::pi * 0.28f,
                               juce::MathConstants<float>::pi * 1.72f, true);

            g.setColour (colour);
            g.strokePath (arc, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.drawLine (cx, cy - 7.0f, cx, cy - 1.5f, 1.7f);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerButton)
    };

    // Small circular icon button: a refresh/reset glyph, no text. Same read
    // as the reset icon on Home-Disto's EQ card -- a button you recognise by
    // shape rather than by reading a label.
    class ResetButton : public juce::Button
    {
    public:
        ResetButton() : juce::Button ("Reset") {}

        void setAccent (juce::Colour c) { accent = c; repaint(); }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            const auto r = getLocalBounds().toFloat().reduced (1.0f);

            g.setColour (juce::Colours::black.withAlpha (down ? 0.60f : (over ? 0.50f : 0.38f)));
            g.fillEllipse (r);
            g.setColour (accent.withAlpha (over ? 0.85f : 0.55f));
            g.drawEllipse (r, 1.2f);

            const auto cx = r.getCentreX();
            const auto cy = r.getCentreY();
            const float radius = r.getWidth() * 0.26f;

            // A circular arrow, ~300 degrees of a ring with an arrowhead at
            // the open end -- the universal "reset / start over" glyph.
            const float startAngle = juce::MathConstants<float>::pi * 0.15f;
            const float endAngle = juce::MathConstants<float>::pi * 1.65f;

            juce::Path arc;
            arc.addCentredArc (cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
            g.setColour (accent);
            g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            const auto tip = juce::Point<float> (cx + std::sin (endAngle) * radius,
                                                 cy - std::cos (endAngle) * radius);
            const float headAngle = endAngle + juce::MathConstants<float>::pi * 0.5f;
            const auto back = juce::Point<float> (tip.x - std::sin (endAngle) * 4.5f,
                                                  tip.y + std::cos (endAngle) * 4.5f);

            juce::Path head;
            head.addTriangle (tip.x + std::sin (headAngle) * 3.4f, tip.y - std::cos (headAngle) * 3.4f,
                              tip.x - std::sin (headAngle) * 3.4f, tip.y + std::cos (headAngle) * 3.4f,
                              back.x, back.y);
            g.fillPath (head);
        }

    private:
        juce::Colour accent = purple;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResetButton)
    };

    // A single connected track divided into N segments, with the active one
    // shown as a sliding white pill -- one control, not N separate buttons
    // sitting next to each other. Used anywhere the choice is "exactly one
    // of these": Link, Trigger/Host, Shapes/Filters.
    class SegmentedSwitch : public juce::Component
    {
    public:
        explicit SegmentedSwitch (juce::StringArray labels, juce::Colour accentColour = cyan)
            : segmentLabels (std::move (labels)), accent (accentColour)
        {
            setInterceptsMouseClicks (true, false);
        }

        void setSegmentLabels (juce::StringArray labels)
        {
            segmentLabels = std::move (labels);
            selected = juce::jlimit (0, juce::jmax (0, segmentLabels.size() - 1), selected);
            repaint();
        }

        void setSelectedIndex (int index, juce::NotificationType notify = juce::sendNotification)
        {
            index = juce::jlimit (0, juce::jmax (0, segmentLabels.size() - 1), index);

            if (index != selected)
            {
                selected = index;
                repaint();

                if (notify == juce::sendNotification && onChange != nullptr)
                    onChange (selected);
            }
        }

        int getSelectedIndex() const noexcept { return selected; }
        void setFontSize (float size) noexcept { fontSize = size; }
        void setAccent (juce::Colour c) { accent = c; repaint(); }

        std::function<void (int)> onChange;

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            const int n = juce::jmax (1, segmentLabels.size());
            const float segW = bounds.getWidth() / static_cast<float> (n);
            const bool enabled = isEnabled();

            g.setColour (juce::Colours::black.withAlpha (enabled ? 0.42f : 0.24f));
            g.fillRoundedRectangle (bounds, bounds.getHeight() * 0.5f);

            const auto activeRect = juce::Rectangle<float> (bounds.getX() + segW * static_cast<float> (selected),
                                                             bounds.getY(), segW, bounds.getHeight()).reduced (2.2f);
            g.setColour (enabled ? juce::Colours::white : juce::Colours::white.withAlpha (0.35f));
            g.fillRoundedRectangle (activeRect, activeRect.getHeight() * 0.5f);
            g.setColour (accent.withAlpha (enabled ? 0.55f : 0.2f));
            g.drawRoundedRectangle (activeRect, activeRect.getHeight() * 0.5f, 1.2f);

            g.setFont (font (fontSize, true));

            for (int i = 0; i < n; ++i)
            {
                const auto r = juce::Rectangle<float> (bounds.getX() + segW * static_cast<float> (i),
                                                       bounds.getY(), segW, bounds.getHeight());
                const bool isSelected = i == selected;

                if (! isSelected && i > 0 && i - 1 != selected)
                {
                    g.setColour (juce::Colours::white.withAlpha (enabled ? 0.12f : 0.06f));
                    g.drawLine (r.getX(), bounds.getY() + 4.0f, r.getX(), bounds.getBottom() - 4.0f, 1.0f);
                }

                g.setColour (! enabled ? juce::Colours::white.withAlpha (0.28f)
                                       : (isSelected ? ink : juce::Colours::white.withAlpha (0.78f)));
                g.drawText (segmentLabels[i], r, juce::Justification::centred, false);
            }

            g.setColour (juce::Colours::white.withAlpha (enabled ? 0.14f : 0.06f));
            g.drawRoundedRectangle (bounds, bounds.getHeight() * 0.5f, 1.0f);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (! isEnabled())
                return;

            const int n = juce::jmax (1, segmentLabels.size());
            const float segW = static_cast<float> (getWidth()) / static_cast<float> (n);
            const int index = juce::jlimit (0, n - 1, static_cast<int> (e.position.x / segW));
            setSelectedIndex (index);
        }

    private:
        juce::StringArray segmentLabels;
        juce::Colour accent;
        int selected = 0;
        float fontSize = 9.5f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedSwitch)
    };

    // A stepper arrow -- a triangle on a small dark tile, for "previous /
    // next" through a list. Reads as navigation, not as a text button that
    // happens to contain "<" or ">".
    class ChevronButton : public juce::Button
    {
    public:
        enum Direction { left, right };

        ChevronButton (Direction directionIn, juce::Colour accentColour = cyan)
            : juce::Button ("Chevron"), direction (directionIn), accent (accentColour)
        {
        }

        void setAccent (juce::Colour c) { accent = c; repaint(); }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            const auto r = getLocalBounds().toFloat().reduced (1.0f);
            const bool enabled = isEnabled();

            g.setColour (juce::Colours::black.withAlpha (! enabled ? 0.20f : (down ? 0.55f : (over ? 0.42f : 0.30f))));
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (accent.withAlpha (! enabled ? 0.15f : (over ? 0.75f : 0.45f)));
            g.drawRoundedRectangle (r, 6.0f, 1.1f);

            const float cx = r.getCentreX();
            const float cy = r.getCentreY();
            const float size = juce::jmin (r.getWidth(), r.getHeight()) * 0.26f;

            juce::Path tri;

            if (direction == left)
                tri.addTriangle (cx + size * 0.55f, cy - size, cx + size * 0.55f, cy + size, cx - size * 0.65f, cy);
            else
                tri.addTriangle (cx - size * 0.55f, cy - size, cx - size * 0.55f, cy + size, cx + size * 0.65f, cy);

            g.setColour (enabled ? juce::Colours::white : juce::Colours::white.withAlpha (0.3f));
            g.fillPath (tri);
        }

    private:
        Direction direction;
        juce::Colour accent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChevronButton)
    };

    // Black knob body, tick ring, white arc and pointer. Caption and value are
    // drawn in card ink, since knobs always sit on a saturated card here.
    class Knob : public juce::Slider
    {
    public:
        Knob (const juce::String& captionText)
            : caption (captionText)
        {
            setSliderStyle (juce::Slider::RotaryVerticalDrag);
            setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            setRotaryParameters (startAngle, endAngle, true);
            setName (captionText);
        }

        std::function<juce::String (double)> valueText;

        void setCaption (const juce::String& c) { caption = c; repaint(); }
        void setCaptionSize (float s) noexcept { captionSize = s; }

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            auto captionRow = area.removeFromTop (captionSize + 4.0f);
            auto valueRow = area.removeFromBottom (13.0f);

            drawCardText (g, caption, captionRow, captionSize);

            const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 5.0f;
            const float cx = area.getCentreX();
            const float cy = area.getCentreY();
            const auto proportion = juce::jlimit (0.0f, 1.0f,
                                                  static_cast<float> (valueToProportionOfLength (getValue())));
            const float angle = startAngle + proportion * (endAngle - startAngle);

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (cx - radius, cy - radius + 2.0f, radius * 2.0f, radius * 2.0f);

            {
                const int ticks = 11;
                g.setColour (juce::Colours::black.withAlpha (0.28f));

                for (int t = 0; t < ticks; ++t)
                {
                    const float a = startAngle + (endAngle - startAngle)
                                                 * (static_cast<float> (t) / static_cast<float> (ticks - 1));
                    const float inner = radius + 2.0f;
                    const float outer = radius + 5.0f;
                    g.drawLine (cx + std::sin (a) * inner, cy - std::cos (a) * inner,
                                cx + std::sin (a) * outer, cy - std::cos (a) * outer,
                                (t == 0 || t == ticks - 1 || t == ticks / 2) ? 1.5f : 1.0f);
                }
            }

            juce::Path track;
            track.addCentredArc (cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
            g.setColour (juce::Colours::black.withAlpha (0.42f));
            g.strokePath (track, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            if (angle > startAngle + 0.001f)
            {
                juce::Path value;
                value.addCentredArc (cx, cy, radius, radius, 0.0f, startAngle, angle, true);
                g.setColour (juce::Colours::white.withAlpha (0.55f));
                g.strokePath (value, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
                g.setColour (juce::Colours::white);
                g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
            }

            g.setColour (juce::Colour (0xff0a0a0c));
            g.fillEllipse (cx - radius + 4.0f, cy - radius + 4.0f, (radius - 4.0f) * 2.0f, (radius - 4.0f) * 2.0f);

            g.setColour (juce::Colours::white);
            g.drawLine (cx + std::sin (angle) * radius * 0.16f, cy - std::cos (angle) * radius * 0.16f,
                        cx + std::sin (angle) * (radius - 6.0f), cy - std::cos (angle) * (radius - 6.0f), 2.2f);

            drawCardText (g, valueText != nullptr ? valueText (getValue()) : juce::String (getValue(), 2),
                          valueRow, 9.5f);
        }

    private:
        static constexpr float startAngle = juce::MathConstants<float>::pi * 1.22f;
        static constexpr float endAngle = juce::MathConstants<float>::pi * 2.78f;

        juce::String caption;
        float captionSize = 10.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
    };

    // A small square lamp with a caption, used for link/MIDI activity.
    inline void drawLamp (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& label,
                          juce::Colour colour, float activity, bool steady)
    {
        const float level = juce::jlimit (0.0f, 1.0f, (steady ? 0.55f : 0.0f) + activity * 0.75f);
        const auto dot = juce::Rectangle<float> (area.getX(), area.getCentreY() - 3.0f, 6.0f, 6.0f);

        if (level > 0.45f)
        {
            g.setColour (colour.withAlpha ((level - 0.45f) * 0.55f));
            g.fillEllipse (dot.expanded (4.0f));
        }

        g.setColour (colour.withAlpha (juce::jmax (0.16f, level)));
        g.fillEllipse (dot);

        g.setFont (font (8.5f));
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawText (label, area.withTrimmedLeft (11.0f), juce::Justification::centredLeft, false);
    }
}
