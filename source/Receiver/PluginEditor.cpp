#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
    const juce::Colour bg0       (0xff05070a);
    const juce::Colour bg1       (0xff0a0f15);
    const juce::Colour panelCol  (0xff0d141b);
    const juce::Colour plotBg    (0xff05090d);
    const juce::Colour edgeCol   (0xff223039);
    const juce::Colour gridCol   (0xff1b3542);
    const juce::Colour textHi    (0xfff1f6fa);
    const juce::Colour textLo    (0xff7d8c99);
    const juce::Colour cyan      (0xff1ee7ff);
    const juce::Colour violet    (0xffb08cff);
    const juce::Colour green     (0xff36e79a);
    const juce::Colour red       (0xffff5965);
    const juce::Colour shadow    (0xff000000);

    juce::Font uiFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica")
                                                   .withStyle (bold ? "Bold" : "Plain"));
    }

    void fillCard (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float radius)
    {
        g.setColour (shadow.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius + 1.0f);

        juce::ColourGradient fill (panelCol.brighter (0.05f), r.getX(), r.getY(),
                                   bg1, r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);

        juce::ColourGradient glow (accent.withAlpha (0.055f), r.getCentreX(), r.getY(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r.reduced (1.0f), radius - 1.0f);

        g.setColour (edgeCol);
        g.drawRoundedRectangle (r, radius, 1.0f);
    }

    juce::String gainToText (float gain)
    {
        if (gain <= 0.0009f)
            return "-inf";

        return juce::String (juce::Decibels::gainToDecibels (gain), 1) + " dB";
    }
}

// =============================================================================
// ReceiverPill
// =============================================================================

ReceiverPill::ReceiverPill (const juce::String& text, juce::Colour accentColour)
    : juce::Button (text), accent (accentColour)
{
    setButtonText (text);
    setClickingTogglesState (false);
}

void ReceiverPill::setAccent (juce::Colour newAccent)
{
    accent = newAccent;
    repaint();
}

void ReceiverPill::paintButton (juce::Graphics& g, bool isMouseOver, bool isMouseDown)
{
    const auto r = getLocalBounds().toFloat().reduced (0.6f);
    const float radius = juce::jmin (r.getHeight() * 0.34f, 9.0f);
    const bool on = getToggleState();

    g.setColour (on ? (filled ? accent : accent.withAlpha (0.14f))
                    : juce::Colour (0xff0f161d));
    g.fillRoundedRectangle (r, radius);

    if (isMouseOver || isMouseDown)
    {
        g.setColour (juce::Colours::white.withAlpha (isMouseDown ? 0.10f : 0.05f));
        g.fillRoundedRectangle (r, radius);
    }

    g.setColour (on ? accent.withAlpha (0.90f) : edgeCol);
    g.drawRoundedRectangle (r, radius, on ? 1.2f : 1.0f);

    g.setFont (uiFont (fontSize, on));
    g.setColour (on ? (filled ? bg0 : accent) : textLo);
    g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
}

// =============================================================================
// ReceiverKnob
// =============================================================================

ReceiverKnob::ReceiverKnob (const juce::String& captionText, juce::Colour accentColour)
    : caption (captionText), accent (accentColour)
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f, true);
    setVelocityBasedMode (false);
    setDoubleClickReturnValue (false, 0.0);
    setName (captionText);
}

void ReceiverKnob::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    auto captionRow = area.removeFromTop (12.0f);
    auto valueRow = area.removeFromBottom (13.0f);

    g.setFont (uiFont (8.6f, false));
    g.setColour (textLo);
    g.drawText (caption.toUpperCase(), captionRow, juce::Justification::centred, false);

    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.44f;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float a0 = juce::MathConstants<float>::pi * 1.25f;
    const float a1 = juce::MathConstants<float>::pi * 2.75f;
    const auto proportion = static_cast<float> (valueToProportionOfLength (getValue()));
    const float angle = a0 + juce::jlimit (0.0f, 1.0f, proportion) * (a1 - a0);

    g.setColour (juce::Colour (0xff0b1117));
    g.fillEllipse (cx - radius - 3.0f, cy - radius - 3.0f, (radius + 3.0f) * 2.0f, (radius + 3.0f) * 2.0f);

    juce::Path background;
    background.addCentredArc (cx, cy, radius, radius, 0.0f, a0, a1, true);
    g.setColour (gridCol);
    g.strokePath (background, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    if (angle > a0 + 0.001f)
    {
        juce::Path filled;
        filled.addCentredArc (cx, cy, radius, radius, 0.0f, a0, angle, true);

        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (filled, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (filled, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    const float sinA = std::sin (angle);
    const float cosA = std::cos (angle);
    g.setColour (textHi);
    g.drawLine (cx + sinA * radius * 0.30f, cy - cosA * radius * 0.30f,
                cx + sinA * radius * 0.86f, cy - cosA * radius * 0.86f, 2.0f);

    g.setFont (uiFont (9.4f, true));
    g.setColour (textHi);
    g.drawText (valueText != nullptr ? valueText (getValue()) : juce::String (getValue(), 2),
                valueRow, juce::Justification::centred, false);
}

// =============================================================================
// ReceiverCurveEditor
// =============================================================================

ReceiverCurveEditor::ReceiverCurveEditor (HomeSidechainReceiverAudioProcessor& p)
    : processor (p)
{
    setWantsKeyboardFocus (false);
}

void ReceiverCurveEditor::setGridDivisions (int divisions) noexcept
{
    const int clamped = juce::jlimit (2, 64, divisions);

    if (clamped != gridDivisions)
    {
        gridDivisions = clamped;
        repaint();
    }
}

juce::Rectangle<float> ReceiverCurveEditor::plotBounds() const noexcept
{
    return getLocalBounds().toFloat().reduced (14.0f, 16.0f);
}

float ReceiverCurveEditor::phaseToX (float phase) const noexcept
{
    const auto plot = plotBounds();
    return plot.getX() + juce::jlimit (0.0f, 1.0f, phase) * plot.getWidth();
}

float ReceiverCurveEditor::valueToY (float value) const noexcept
{
    const auto plot = plotBounds();
    return plot.getBottom() - juce::jlimit (0.0f, 1.0f, value) * plot.getHeight();
}

float ReceiverCurveEditor::xToPhase (float x) const noexcept
{
    const auto plot = plotBounds();
    return juce::jlimit (0.0f, 1.0f, (x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
}

float ReceiverCurveEditor::yToValue (float y) const noexcept
{
    const auto plot = plotBounds();
    return juce::jlimit (0.0f, 1.0f, (plot.getBottom() - y) / juce::jmax (1.0f, plot.getHeight()));
}

float ReceiverCurveEditor::snapPhase (float phase, bool fine) const noexcept
{
    if (! snapEnabled || fine)
        return juce::jlimit (0.0f, 1.0f, phase);

    const auto steps = static_cast<float> (gridDivisions * 2);
    return juce::jlimit (0.0f, 1.0f, std::round (phase * steps) / steps);
}

int ReceiverCurveEditor::buildSorted (std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes>& out) const
{
    int count = 0;

    for (int slot = 0; slot < HomeSidechainReceiverAudioProcessor::maxNodes; ++slot)
    {
        if (! processor.isNodeActive (slot))
            continue;

        out[static_cast<size_t> (count)] = { slot,
                                             processor.getNodeX (slot),
                                             processor.getNodeY (slot),
                                             processor.getTension (slot) };
        ++count;
    }

    std::sort (out.begin(), out.begin() + count,
               [] (const SortedNode& a, const SortedNode& b) { return a.x < b.x; });

    return count;
}

juce::Point<float> ReceiverCurveEditor::handlePosition (const SortedNode& a, const SortedNode& b) const noexcept
{
    const float midPhase = (a.x + b.x) * 0.5f;
    const float t = receiverCurve::applyTension (0.5f, a.tension);
    const float value = a.y + (b.y - a.y) * t;

    return { phaseToX (midPhase), valueToY (value) };
}

int ReceiverCurveEditor::nodeAt (juce::Point<float> position) const
{
    std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes> nodes;
    const int count = buildSorted (nodes);

    for (int i = 0; i < count; ++i)
    {
        const juce::Point<float> point (phaseToX (nodes[static_cast<size_t> (i)].x),
                                        valueToY (nodes[static_cast<size_t> (i)].y));

        if (point.getDistanceFrom (position) <= 10.0f)
            return nodes[static_cast<size_t> (i)].slot;
    }

    return -1;
}

int ReceiverCurveEditor::segmentHandleAt (juce::Point<float> position) const
{
    std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes> nodes;
    const int count = buildSorted (nodes);

    for (int i = 0; i < count - 1; ++i)
    {
        const auto& a = nodes[static_cast<size_t> (i)];
        const auto& b = nodes[static_cast<size_t> (i + 1)];

        if (std::abs (b.y - a.y) < 0.004f)
            continue;

        if (handlePosition (a, b).getDistanceFrom (position) <= 9.0f)
            return a.slot;
    }

    return -1;
}

bool ReceiverCurveEditor::canDeleteNode (int slot) const
{
    std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes> nodes;
    const int count = buildSorted (nodes);

    if (count <= 2)
        return false;

    // The points at each end anchor the cycle; removing one would leave the
    // shape starting or finishing part way through the bar.
    return nodes[0].slot != slot && nodes[static_cast<size_t> (count - 1)].slot != slot;
}

void ReceiverCurveEditor::drawGrid (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    for (int i = 1; i < 4; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * (static_cast<float> (i) / 4.0f);
        g.setColour (gridCol.withAlpha (i == 2 ? 0.55f : 0.30f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    for (int i = 0; i <= gridDivisions; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (static_cast<float> (i) / static_cast<float> (gridDivisions));
        const bool strong = (i % 4) == 0;

        g.setColour (gridCol.withAlpha (strong ? 0.70f : 0.26f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }

    g.setFont (uiFont (7.6f));
    g.setColour (gridCol.brighter (0.35f));

    static const char* labels[] = { "0", "-6", "-12", "-24" };
    static const float levels[] = { 1.0f, 0.5f, 0.25f, 0.06f };

    for (int i = 0; i < 4; ++i)
    {
        const float y = valueToY (levels[i]);
        g.drawText (labels[i], juce::Rectangle<float> (plot.getX() + 3.0f, y + 1.0f, 26.0f, 10.0f),
                    juce::Justification::centredLeft, false);
    }
}

void ReceiverCurveEditor::drawCurve (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    const auto snapshot = processor.buildSnapshot();

    juce::Path curve;
    const int steps = juce::jmax (2, juce::roundToInt (plot.getWidth()));

    for (int i = 0; i <= steps; ++i)
    {
        const float phase = static_cast<float> (i) / static_cast<float> (steps);
        const float x = plot.getX() + phase * plot.getWidth();
        const float y = valueToY (snapshot.valueAt (phase));

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    juce::Path filled (curve);
    filled.lineTo (plot.getRight(), plot.getBottom());
    filled.lineTo (plot.getX(), plot.getBottom());
    filled.closeSubPath();

    juce::ColourGradient fill (cyan.withAlpha (0.26f), plot.getCentreX(), plot.getY(),
                               cyan.withAlpha (0.02f), plot.getCentreX(), plot.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (filled);

    g.setColour (cyan.withAlpha (0.16f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (cyan);
    g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void ReceiverCurveEditor::drawPlayhead (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    if (! processor.envelopeActiveForUI.load (std::memory_order_relaxed))
        return;

    const float phase = juce::jlimit (0.0f, 1.0f,
                                      processor.envelopeDisplayPhase.load (std::memory_order_relaxed));
    const auto snapshot = processor.buildSnapshot();
    const float x = phaseToX (phase);
    const float y = valueToY (snapshot.valueAt (phase));

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillRect (juce::Rectangle<float> (plot.getX(), plot.getY(), x - plot.getX(), plot.getHeight()));

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.fillEllipse (x - 7.0f, y - 7.0f, 14.0f, 14.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
}

void ReceiverCurveEditor::drawNodes (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    juce::ignoreUnused (plot);

    std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes> nodes;
    const int count = buildSorted (nodes);

    for (int i = 0; i < count - 1; ++i)
    {
        const auto& a = nodes[static_cast<size_t> (i)];
        const auto& b = nodes[static_cast<size_t> (i + 1)];

        if (std::abs (b.y - a.y) < 0.004f)
            continue;

        const auto point = handlePosition (a, b);
        const bool active = (a.slot == hoveredSegment) || (a.slot == draggedSegment);
        const float size = active ? 5.0f : 3.6f;

        juce::Path diamond;
        diamond.addQuadrilateral (point.x, point.y - size, point.x + size, point.y,
                                  point.x, point.y + size, point.x - size, point.y);

        g.setColour (active ? violet : violet.withAlpha (0.62f));
        g.fillPath (diamond);
    }

    for (int i = 0; i < count; ++i)
    {
        const auto& node = nodes[static_cast<size_t> (i)];
        const float x = phaseToX (node.x);
        const float y = valueToY (node.y);
        const bool active = (node.slot == hoveredSlot) || (node.slot == draggedSlot);
        const float radius = active ? 6.5f : 4.6f;

        if (active)
        {
            g.setColour (cyan.withAlpha (0.22f));
            g.fillEllipse (x - radius - 5.0f, y - radius - 5.0f,
                           (radius + 5.0f) * 2.0f, (radius + 5.0f) * 2.0f);
        }

        g.setColour (plotBg);
        g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (cyan);
        g.drawEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f, 1.8f);
    }
}

void ReceiverCurveEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot = plotBounds();

    g.setColour (plotBg);
    g.fillRoundedRectangle (bounds, 8.0f);

    drawGrid (g, plot);
    drawCurve (g, plot);
    drawPlayhead (g, plot);
    drawNodes (g, plot);

    g.setColour (edgeCol);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    g.setFont (uiFont (8.2f));
    g.setColour (textLo.withAlpha (0.75f));
    g.drawText (gainToText (processor.currentGainForUI.load (std::memory_order_relaxed)),
                juce::Rectangle<float> (plot.getRight() - 70.0f, plot.getY() - 14.0f, 70.0f, 12.0f),
                juce::Justification::centredRight, false);
}

void ReceiverCurveEditor::mouseMove (const juce::MouseEvent& e)
{
    const int node = nodeAt (e.position);
    const int segment = node >= 0 ? -1 : segmentHandleAt (e.position);

    if (node != hoveredSlot || segment != hoveredSegment)
    {
        hoveredSlot = node;
        hoveredSegment = segment;
        setMouseCursor (node >= 0 || segment >= 0 ? juce::MouseCursor::DraggingHandCursor
                                                  : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void ReceiverCurveEditor::mouseExit (const juce::MouseEvent&)
{
    hoveredSlot = -1;
    hoveredSegment = -1;
    repaint();
}

void ReceiverCurveEditor::mouseDown (const juce::MouseEvent& e)
{
    const int node = nodeAt (e.position);

    if (node >= 0)
    {
        if (e.mods.isPopupMenu() || e.mods.isAltDown())
        {
            if (canDeleteNode (node))
            {
                processor.removeNode (node);
                hoveredSlot = -1;
            }

            repaint();
            return;
        }

        draggedSlot = node;
        repaint();
        return;
    }

    const int segment = segmentHandleAt (e.position);

    if (segment >= 0)
    {
        if (e.mods.isPopupMenu() || e.mods.isAltDown())
        {
            processor.setTension (segment, 0.5f);
            repaint();
            return;
        }

        draggedSegment = segment;
        repaint();
    }
}

void ReceiverCurveEditor::mouseDrag (const juce::MouseEvent& e)
{
    const bool fine = e.mods.isShiftDown();

    std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes> nodes;
    const int count = buildSorted (nodes);

    if (draggedSlot >= 0)
    {
        int index = -1;

        for (int i = 0; i < count; ++i)
        {
            if (nodes[static_cast<size_t> (i)].slot == draggedSlot)
            {
                index = i;
                break;
            }
        }

        if (index < 0)
            return;

        float value = yToValue (e.position.y);

        if (snapEnabled && ! fine)
            value = std::round (value * 20.0f) / 20.0f;

        processor.setNodeY (draggedSlot, value);

        // The first and last nodes anchor the cycle, so only their height moves.
        if (index > 0 && index < count - 1)
        {
            const float low = nodes[static_cast<size_t> (index - 1)].x + 0.004f;
            const float high = nodes[static_cast<size_t> (index + 1)].x - 0.004f;
            const float phase = snapPhase (xToPhase (e.position.x), fine);

            processor.setNodeX (draggedSlot, juce::jlimit (juce::jmin (low, high),
                                                           juce::jmax (low, high), phase));
        }

        repaint();
        return;
    }

    if (draggedSegment >= 0)
    {
        for (int i = 0; i < count - 1; ++i)
        {
            const auto& a = nodes[static_cast<size_t> (i)];

            if (a.slot != draggedSegment)
                continue;

            const auto& b = nodes[static_cast<size_t> (i + 1)];
            const float span = b.y - a.y;

            if (std::abs (span) < 0.004f)
                return;

            const float u = (yToValue (e.position.y) - a.y) / span;
            processor.setTension (draggedSegment, receiverCurve::tensionForPoint (0.5f, u));
            repaint();
            return;
        }
    }
}

void ReceiverCurveEditor::mouseUp (const juce::MouseEvent&)
{
    draggedSlot = -1;
    draggedSegment = -1;
    repaint();
}

void ReceiverCurveEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int node = nodeAt (e.position);

    if (node >= 0)
    {
        if (canDeleteNode (node))
        {
            processor.removeNode (node);
            hoveredSlot = -1;
        }

        repaint();
        return;
    }

    const int segment = segmentHandleAt (e.position);

    if (segment >= 0)
    {
        processor.setTension (segment, 0.5f);
        repaint();
        return;
    }

    const float phase = juce::jlimit (0.01f, 0.99f, snapPhase (xToPhase (e.position.x), e.mods.isShiftDown()));
    float value = yToValue (e.position.y);

    if (snapEnabled && ! e.mods.isShiftDown())
        value = std::round (value * 20.0f) / 20.0f;

    processor.addNode (phase, value);
    repaint();
}

// =============================================================================
// ReceiverPresetStrip
// =============================================================================

ReceiverPresetStrip::ReceiverPresetStrip() = default;

juce::Rectangle<float> ReceiverPresetStrip::cellBounds (int index) const noexcept
{
    const float width = static_cast<float> (getWidth()) / static_cast<float> (HomeSidechainReceiverAudioProcessor::numPresets);
    return juce::Rectangle<float> (width * static_cast<float> (index), 0.0f,
                                   width, static_cast<float> (getHeight())).reduced (3.0f, 1.0f);
}

int ReceiverPresetStrip::cellAt (juce::Point<float> position) const noexcept
{
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numPresets; ++i)
        if (cellBounds (i).contains (position))
            return i;

    return -1;
}

void ReceiverPresetStrip::setSelected (int index)
{
    const int clamped = juce::jlimit (0, HomeSidechainReceiverAudioProcessor::numPresets - 1, index);

    if (clamped != selected)
    {
        selected = clamped;
        repaint();
    }
}

void ReceiverPresetStrip::paint (juce::Graphics& g)
{
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numPresets; ++i)
    {
        const auto cell = cellBounds (i);
        const bool isSelected = i == selected;
        const bool isHovered = i == hovered;

        g.setColour (isSelected ? cyan.withAlpha (0.10f) : juce::Colour (0xff0b1117));
        g.fillRoundedRectangle (cell, 5.0f);

        if (isHovered && ! isSelected)
        {
            g.setColour (juce::Colours::white.withAlpha (0.045f));
            g.fillRoundedRectangle (cell, 5.0f);
        }

        g.setColour (isSelected ? cyan.withAlpha (0.80f) : edgeCol);
        g.drawRoundedRectangle (cell, 5.0f, 1.0f);

        auto plot = cell.reduced (6.0f, 5.0f);
        plot.removeFromBottom (10.0f);

        const auto snapshot = HomeSidechainReceiverAudioProcessor::presetSnapshot (i);
        juce::Path curve;
        const int steps = juce::jmax (8, juce::roundToInt (plot.getWidth()));

        for (int s = 0; s <= steps; ++s)
        {
            const float phase = static_cast<float> (s) / static_cast<float> (steps);
            const float x = plot.getX() + phase * plot.getWidth();
            const float y = plot.getBottom() - snapshot.valueAt (phase) * plot.getHeight();

            if (s == 0)
                curve.startNewSubPath (x, y);
            else
                curve.lineTo (x, y);
        }

        g.setColour (isSelected ? cyan : textLo.withAlpha (0.75f));
        g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setFont (uiFont (7.6f, isSelected));
        g.setColour (isSelected ? cyan : textLo);
        g.drawText (HomeSidechainReceiverAudioProcessor::presetName (i),
                    cell.withTop (cell.getBottom() - 12.0f),
                    juce::Justification::centred, false);
    }
}

void ReceiverPresetStrip::mouseMove (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.position);

    if (cell != hovered)
    {
        hovered = cell;
        repaint();
    }
}

void ReceiverPresetStrip::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void ReceiverPresetStrip::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.position);

    if (cell < 0)
        return;

    setSelected (cell);

    if (onSelect != nullptr)
        onSelect (cell);
}

// =============================================================================
// ReceiverPanel
// =============================================================================

ReceiverPanel::ReceiverPanel (HomeSidechainReceiverAudioProcessor& p)
    : processor (p), curveEditor (p)
{
    addAndMakeVisible (curveEditor);
    addAndMakeVisible (presetStrip);

    presetStrip.onSelect = [this] (int index)
    {
        processor.applyPreset (index);
        curveEditor.repaint();
    };

    for (int i = 0; i < homeSidechain::numberOfLinks; ++i)
    {
        auto pill = std::make_unique<ReceiverPill> (homeSidechain::linkName (i), cyan);
        pill->setFontSize (10.0f);
        pill->onClick = [this, i] { processor.setLink (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        linkPills[static_cast<size_t> (i)] = std::move (pill);
    }

    const auto rates = HomeSidechainReceiverAudioProcessor::rateNames();

    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numRates; ++i)
    {
        auto pill = std::make_unique<ReceiverPill> (rates[i], cyan);
        pill->setFontSize (9.4f);
        pill->onClick = [this, i] { processor.setRate (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        ratePills[static_cast<size_t> (i)] = std::move (pill);
    }

    const char* runNames[] = { "Trigger", "Host sync" };

    for (int i = 0; i < 2; ++i)
    {
        auto pill = std::make_unique<ReceiverPill> (runNames[i], violet);
        pill->setFontSize (9.6f);
        pill->onClick = [this, i] { processor.setRunMode (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        runPills[static_cast<size_t> (i)] = std::move (pill);
    }

    const char* sourceNames[] = { "Link", "MIDI", "Both" };

    for (int i = 0; i < 3; ++i)
    {
        auto pill = std::make_unique<ReceiverPill> (sourceNames[i], green);
        pill->setFontSize (9.0f);
        pill->onClick = [this, i] { processor.setSource (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        sourcePills[static_cast<size_t> (i)] = std::move (pill);
    }

    bypassPill.setClickingTogglesState (true);
    bypassPill.setFontSize (9.6f);
    addAndMakeVisible (bypassPill);

    syncPill.setClickingTogglesState (true);
    syncPill.setFontSize (9.2f);
    addAndMakeVisible (syncPill);

    snapPill.setFontSize (9.2f);
    snapPill.setToggleState (curveEditor.isSnapEnabled(), juce::dontSendNotification);
    snapPill.onClick = [this]
    {
        const bool snap = ! curveEditor.isSnapEnabled();
        curveEditor.setSnapEnabled (snap);
        snapPill.setToggleState (snap, juce::dontSendNotification);
    };
    addAndMakeVisible (snapPill);

    testPill.setFontSize (9.6f);
    testPill.onClick = [this] { processor.requestTestTrigger(); };
    addAndMakeVisible (testPill);

    resetPill.setFontSize (9.2f);
    resetPill.onClick = [this]
    {
        processor.resetCurve();
        presetStrip.setSelected (0);
        curveEditor.repaint();
    };
    addAndMakeVisible (resetPill);

    const auto percent = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + "%";
    };

    const auto hertz = [] (double value)
    {
        return value >= 1000.0 ? juce::String (value / 1000.0, 1) + "k"
                               : juce::String (juce::roundToInt (value)) + " Hz";
    };

    depthKnob.valueText = percent;
    mixKnob.valueText = percent;
    smoothKnob.valueText = [] (double value) { return juce::String (value, 1) + " ms"; };
    lengthKnob.valueText = [] (double value) { return juce::String (juce::roundToInt (value)) + " ms"; };
    lowCutKnob.valueText = hertz;
    highCutKnob.valueText = hertz;

    addKnob (depthKnob);
    addKnob (mixKnob);
    addKnob (smoothKnob);
    addKnob (lengthKnob);
    addKnob (lowCutKnob);
    addKnob (highCutKnob);

    depthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "DEPTH", depthKnob);
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, "MIX", mixKnob);
    smoothAttachment = std::make_unique<SliderAttachment> (processor.apvts, "SMOOTH", smoothKnob);
    lengthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "LENGTH", lengthKnob);
    lowCutAttachment = std::make_unique<SliderAttachment> (processor.apvts, "LOW_CUT", lowCutKnob);
    highCutAttachment = std::make_unique<SliderAttachment> (processor.apvts, "HIGH_CUT", highCutKnob);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", bypassPill);
    syncAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "SYNC", syncPill);

    setSize (designWidth, designHeight);
    refreshFromParameters();
    startTimerHz (30);
}

ReceiverPanel::~ReceiverPanel()
{
    stopTimer();
}

void ReceiverPanel::addKnob (ReceiverKnob& knob)
{
    addAndMakeVisible (knob);
}

void ReceiverPanel::resized()
{
    auto area = getLocalBounds();

    headerArea = area.removeFromTop (64);
    footerArea = area.removeFromBottom (156);
    presetArea = area.removeFromTop (58);
    graphArea = area.reduced (18, 4).withTrimmedBottom (12);

    curveEditor.setBounds (graphArea);

    // ---- header ----
    const int headerCentre = headerArea.getCentreY();
    int x = 344;

    for (auto& pill : linkPills)
    {
        pill->setBounds (x, headerCentre - 11, 28, 22);
        x += 32;
    }

    testPill.setBounds (786, headerCentre - 13, 64, 26);
    bypassPill.setBounds (860, headerCentre - 13, 62, 26);

    // ---- preset strip ----
    presetStrip.setBounds (18, presetArea.getY() + 5, 838, presetArea.getHeight() - 10);
    resetPill.setBounds (868, presetArea.getCentreY() - 12, 54, 24);

    // ---- footer cards ----
    const int cardY = footerArea.getY() + 8;
    const int cardHeight = footerArea.getHeight() - 16;

    auto timingCard = juce::Rectangle<int> (18, cardY, 388, cardHeight);
    auto levelCard = juce::Rectangle<int> (418, cardY, 264, cardHeight);
    auto filterCard = juce::Rectangle<int> (694, cardY, 228, cardHeight);

    {
        auto inner = timingCard.reduced (14, 10);
        inner.removeFromTop (16);

        auto lengthColumn = inner.removeFromRight (74);
        lengthKnob.setBounds (lengthColumn.removeFromTop (94));
        inner.removeFromRight (12);

        auto row = inner.removeFromTop (26);
        runPills[0]->setBounds (row.removeFromLeft (76));
        row.removeFromLeft (6);
        runPills[1]->setBounds (row.removeFromLeft (76));
        row.removeFromLeft (10);
        syncPill.setBounds (row.removeFromLeft (48));
        row.removeFromLeft (6);
        snapPill.setBounds (row.removeFromLeft (48));

        inner.removeFromTop (12);
        auto rateRow = inner.removeFromTop (26);
        const int rateWidth = (rateRow.getWidth() - 25) / 6;

        for (auto& pill : ratePills)
        {
            pill->setBounds (rateRow.removeFromLeft (rateWidth));
            rateRow.removeFromLeft (5);
        }
    }

    {
        auto inner = levelCard.reduced (14, 10);
        inner.removeFromTop (16);

        const int knobWidth = (inner.getWidth() - 16) / 3;
        depthKnob.setBounds (inner.removeFromLeft (knobWidth));
        inner.removeFromLeft (8);
        mixKnob.setBounds (inner.removeFromLeft (knobWidth));
        inner.removeFromLeft (8);
        smoothKnob.setBounds (inner.removeFromLeft (knobWidth));
    }

    {
        auto inner = filterCard.reduced (14, 10);
        inner.removeFromTop (16);

        auto sourceRow = inner.removeFromBottom (24);
        const int sourceWidth = (sourceRow.getWidth() - 12) / 3;

        for (auto& pill : sourcePills)
        {
            pill->setBounds (sourceRow.removeFromLeft (sourceWidth));
            sourceRow.removeFromLeft (6);
        }

        inner.removeFromBottom (8);

        const int knobWidth = (inner.getWidth() - 12) / 2;
        lowCutKnob.setBounds (inner.removeFromLeft (knobWidth));
        inner.removeFromLeft (12);
        highCutKnob.setBounds (inner.removeFromLeft (knobWidth));
    }
}

void ReceiverPanel::drawCard (juce::Graphics& g, juce::Rectangle<float> r,
                              const juce::String& title, juce::Colour accent) const
{
    fillCard (g, r, accent, 10.0f);

    g.setFont (uiFont (8.4f, true));
    g.setColour (accent.withAlpha (0.85f));
    g.drawText (title.toUpperCase(),
                r.withTrimmedLeft (14.0f).withTrimmedTop (9.0f).withHeight (12.0f),
                juce::Justification::topLeft, false);
}

void ReceiverPanel::drawStatusLamp (juce::Graphics& g, juce::Rectangle<float> r,
                                    const juce::String& label, juce::Colour colour,
                                    float activity, bool connected) const
{
    const float alpha = juce::jlimit (0.16f, 1.0f, connected ? 0.5f + activity * 0.5f : 0.16f + activity * 0.6f);
    const auto lamp = juce::Rectangle<float> (r.getX(), r.getCentreY() - 3.5f, 7.0f, 7.0f);

    if (alpha > 0.5f)
    {
        g.setColour (colour.withAlpha ((alpha - 0.5f) * 0.6f));
        g.fillEllipse (lamp.expanded (4.0f));
    }

    g.setColour (colour.withAlpha (alpha));
    g.fillEllipse (lamp);

    g.setFont (uiFont (8.4f));
    g.setColour (textLo);
    g.drawText (label, r.withTrimmedLeft (13.0f), juce::Justification::centredLeft, false);
}

void ReceiverPanel::drawHeader (juce::Graphics& g, juce::Rectangle<float> r) const
{
    g.setFont (uiFont (17.0f, true));
    g.setColour (textHi);
    g.drawText ("Home-Sidechain", r.withTrimmedLeft (20.0f).withWidth (180.0f).withTrimmedBottom (14.0f),
                juce::Justification::bottomLeft, false);

    g.setFont (uiFont (9.0f, false));
    g.setColour (cyan.withAlpha (0.85f));
    g.drawText ("Receiver", r.withTrimmedLeft (20.0f).withWidth (180.0f).withTrimmedTop (34.0f),
                juce::Justification::topLeft, false);

    g.setFont (uiFont (8.4f));
    g.setColour (textLo.withAlpha (0.8f));
    g.drawText ("Link", juce::Rectangle<float> (306.0f, r.getCentreY() - 6.0f, 34.0f, 12.0f),
                juce::Justification::centredLeft, false);

    const bool connected = processor.homeLinkConnected.load (std::memory_order_relaxed);
    const float linkActivity = processor.homeLinkActivity.load (std::memory_order_relaxed);
    const float midiActivity = processor.midiActivity.load (std::memory_order_relaxed);

    drawStatusLamp (g, juce::Rectangle<float> (614.0f, r.getCentreY() - 8.0f, 78.0f, 16.0f),
                    connected ? "Linked" : "No link", connected ? green : red, linkActivity, connected);
    drawStatusLamp (g, juce::Rectangle<float> (700.0f, r.getCentreY() - 8.0f, 70.0f, 16.0f),
                    "MIDI", violet, midiActivity, false);

    g.setFont (uiFont (8.2f));
    g.setColour (textLo.withAlpha (0.55f));
    g.drawText (juce::String (processor.hostBpmForUI.load (std::memory_order_relaxed), 1) + " BPM",
                juce::Rectangle<float> (208.0f, r.getCentreY() - 6.0f, 90.0f, 12.0f),
                juce::Justification::centredRight, false);

    g.setColour (edgeCol.withAlpha (0.8f));
    g.drawHorizontalLine (juce::roundToInt (r.getBottom()) - 1, r.getX() + 18.0f, r.getRight() - 18.0f);
}

void ReceiverPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient chassis (bg1, bounds.getCentreX(), bounds.getY(),
                                  bg0, bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (chassis);
    g.fillRect (bounds);

    drawHeader (g, headerArea.toFloat());

    const int cardY = footerArea.getY() + 8;
    const int cardHeight = footerArea.getHeight() - 16;

    drawCard (g, juce::Rectangle<int> (18, cardY, 388, cardHeight).toFloat(), "Timing", cyan);
    drawCard (g, juce::Rectangle<int> (418, cardY, 264, cardHeight).toFloat(), "Amount", cyan);
    drawCard (g, juce::Rectangle<int> (694, cardY, 228, cardHeight).toFloat(), "Sidechain band", green);

    g.setFont (uiFont (8.2f));
    g.setColour (textLo.withAlpha (0.6f));
    g.drawText ("Drag points  ·  double-click to add or remove  ·  drag a diamond to bend",
                juce::Rectangle<int> (18, footerArea.getY() - 14, 838, 12).toFloat(),
                juce::Justification::centredLeft, false);
}

void ReceiverPanel::refreshFromParameters()
{
    const int link = processor.getLink();

    for (int i = 0; i < homeSidechain::numberOfLinks; ++i)
        linkPills[static_cast<size_t> (i)]->setToggleState (i == link, juce::dontSendNotification);

    const int rate = processor.getRate();

    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numRates; ++i)
        ratePills[static_cast<size_t> (i)]->setToggleState (i == rate, juce::dontSendNotification);

    const int run = processor.getRunMode();

    for (int i = 0; i < 2; ++i)
        runPills[static_cast<size_t> (i)]->setToggleState (i == run, juce::dontSendNotification);

    const int source = processor.getSource();

    for (int i = 0; i < 3; ++i)
        sourcePills[static_cast<size_t> (i)]->setToggleState (i == source, juce::dontSendNotification);

    // Host sync owns the cycle length, so the free-length control steps aside.
    const bool freeLength = run == 0 && ! processor.isSynced();
    lengthKnob.setEnabled (freeLength);
    lengthKnob.setAlpha (freeLength ? 1.0f : 0.35f);
    syncPill.setEnabled (run == 0);
    syncPill.setAlpha (run == 0 ? 1.0f : 0.35f);

    for (auto& pill : ratePills)
    {
        const bool usable = run == 1 || processor.isSynced();
        pill->setEnabled (usable);
        pill->setAlpha (usable ? 1.0f : 0.35f);
    }

    curveEditor.setGridDivisions (processor.gridDivisions());
}

void ReceiverPanel::timerCallback()
{
    refreshFromParameters();
    curveEditor.repaint();
    repaint (headerArea);
}

// =============================================================================
// HomeSidechainReceiverAudioProcessorEditor
// =============================================================================

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (
    HomeSidechainReceiverAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);

    setResizable (true, true);
    setResizeLimits (ReceiverPanel::designWidth * 3 / 4, ReceiverPanel::designHeight * 3 / 4,
                     ReceiverPanel::designWidth * 3 / 2, ReceiverPanel::designHeight * 3 / 2);

    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (static_cast<double> (ReceiverPanel::designWidth)
                                          / static_cast<double> (ReceiverPanel::designHeight));

    setSize (ReceiverPanel::designWidth, ReceiverPanel::designHeight);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    panel.setBounds (0, 0, ReceiverPanel::designWidth, ReceiverPanel::designHeight);

    const auto scale = static_cast<float> (getWidth()) / static_cast<float> (ReceiverPanel::designWidth);
    panel.setTransform (juce::AffineTransform::scale (scale));
}
