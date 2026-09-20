#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

using namespace homeUI;

namespace
{
    juce::String percentText (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + "%";
    }

    juce::String hertzText (double value)
    {
        return value >= 1000.0 ? juce::String (value / 1000.0, 1) + "k"
                               : juce::String (juce::roundToInt (value)) + "Hz";
    }

    juce::String msText (double value)
    {
        return juce::String (value < 10.0 ? juce::String (value, 1) : juce::String (juce::roundToInt (value))) + "ms";
    }

    juce::String gainText (float gain)
    {
        if (gain <= 0.0009f)
            return "-inf";

        return juce::String (juce::Decibels::gainToDecibels (gain), 1) + "dB";
    }
}

// =============================================================================
// ReceiverCurveEditor
// =============================================================================

ReceiverCurveEditor::ReceiverCurveEditor (HomeSidechainReceiverAudioProcessor& p)
    : processor (p)
{
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
    // Extra top margin reserved for the gain readout drawn in paint() -- a
    // node at full value (y=1.0) sits exactly at the plot's top edge, and a
    // symmetric inset put that right where the readout text was, so the two
    // could touch. Trimming the top further keeps the readout in its own
    // strip above the curve, clear of every node position.
    return getLocalBounds().toFloat().reduced (10.0f, 12.0f).withTrimmedTop (10.0f);
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

float ReceiverCurveEditor::snapPhase (float phase, bool shiftHeld) const noexcept
{
    // Shift forces a snap for that drag even when Always Snap (in Advanced)
    // is off -- it's an on-demand grid-lock, not a fine-control override.
    if (! (snapEnabled || shiftHeld))
        return juce::jlimit (0.0f, 1.0f, phase);

    const auto steps = static_cast<float> (gridDivisions * 2);
    return juce::jlimit (0.0f, 1.0f, std::round (phase * steps) / steps);
}

float ReceiverCurveEditor::snapValue (float value, bool shiftHeld) const noexcept
{
    if (! (snapEnabled || shiftHeld))
        return juce::jlimit (0.0f, 1.0f, value);

    return juce::jlimit (0.0f, 1.0f, std::round (value * 20.0f) / 20.0f);
}

int ReceiverCurveEditor::buildSorted (std::array<SortedNode, maxNodes>& out) const
{
    int count = 0;

    for (int slot = 0; slot < maxNodes; ++slot)
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
    const float t = receiverCurve::applyTension (0.5f, a.tension);
    return { phaseToX ((a.x + b.x) * 0.5f), valueToY (a.y + (b.y - a.y) * t) };
}

int ReceiverCurveEditor::nodeAt (juce::Point<float> position) const
{
    std::array<SortedNode, maxNodes> nodes;
    const int count = buildSorted (nodes);

    for (int i = 0; i < count; ++i)
    {
        const auto& node = nodes[static_cast<size_t> (i)];
        const juce::Point<float> point (phaseToX (node.x), valueToY (node.y));

        if (point.getDistanceFrom (position) <= 10.0f)
            return node.slot;
    }

    return -1;
}

int ReceiverCurveEditor::segmentHandleAt (juce::Point<float> position) const
{
    std::array<SortedNode, maxNodes> nodes;
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
    std::array<SortedNode, maxNodes> nodes;
    const int count = buildSorted (nodes);

    if (count <= 3)
        return false;

    // The two outer points are the loop seam. Deleting one would leave the
    // cycle ending somewhere other than where it starts, which clicks.
    return nodes[0].slot != slot && nodes[static_cast<size_t> (count - 1)].slot != slot;
}

void ReceiverCurveEditor::drawGrid (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    for (int i = 1; i < 4; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * (static_cast<float> (i) / 4.0f);
        g.setColour (juce::Colours::white.withAlpha (i == 2 ? 0.13f : 0.06f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    for (int i = 0; i <= gridDivisions; ++i)
    {
        const float x = plot.getX() + plot.getWidth()
                        * (static_cast<float> (i) / static_cast<float> (gridDivisions));
        const bool strong = (i % 4) == 0;

        g.setColour (juce::Colours::white.withAlpha (strong ? 0.16f : 0.06f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
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

    juce::Path body (curve);
    body.lineTo (plot.getRight(), plot.getBottom());
    body.lineTo (plot.getX(), plot.getBottom());
    body.closeSubPath();

    juce::ColourGradient fill (cyan.withAlpha (0.42f), plot.getCentreX(), plot.getY(),
                               cyan.withAlpha (0.04f), plot.getCentreX(), plot.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (body);

    g.setColour (cyan.withAlpha (0.20f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
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

    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillRect (juce::Rectangle<float> (plot.getX(), plot.getY(), x - plot.getX(), plot.getHeight()));

    g.setColour (green.withAlpha (0.85f));
    g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.2f);

    g.setColour (green.withAlpha (0.30f));
    g.fillEllipse (x - 7.0f, y - 7.0f, 14.0f, 14.0f);
    g.setColour (green);
    g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
}

void ReceiverCurveEditor::drawNodes (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    std::array<SortedNode, maxNodes> nodes;
    const int count = buildSorted (nodes);

    // The seam line: both outer points always share a height, so showing that
    // height across the whole cycle makes the rule visible rather than magic.
    if (count >= 2)
    {
        const float seamY = valueToY (nodes[0].y);
        g.setColour (purple.withAlpha (0.45f));

        for (float x = plot.getX(); x < plot.getRight(); x += 7.0f)
            g.drawLine (x, seamY, juce::jmin (x + 3.5f, plot.getRight()), seamY, 1.0f);
    }

    for (int i = 0; i < count - 1; ++i)
    {
        const auto& a = nodes[static_cast<size_t> (i)];
        const auto& b = nodes[static_cast<size_t> (i + 1)];

        if (std::abs (b.y - a.y) < 0.004f)
            continue;

        const auto point = handlePosition (a, b);
        const bool active = (a.slot == hoveredSegment) || (a.slot == draggedSegment);
        const float size = active ? 5.0f : 3.4f;

        juce::Path diamond;
        diamond.addQuadrilateral (point.x, point.y - size, point.x + size, point.y,
                                  point.x, point.y + size, point.x - size, point.y);

        g.setColour (active ? purple : purple.withAlpha (0.75f));
        g.fillPath (diamond);
        g.setColour (juce::Colours::white.withAlpha (active ? 0.9f : 0.45f));
        g.strokePath (diamond, juce::PathStrokeType (1.0f));
    }

    for (int i = 0; i < count; ++i)
    {
        const auto& node = nodes[static_cast<size_t> (i)];
        const bool seam = (i == 0) || (i == count - 1);
        const float x = phaseToX (node.x);
        const float y = valueToY (node.y);
        const bool active = (node.slot == hoveredSlot) || (node.slot == draggedSlot);
        const float radius = active ? 6.5f : 4.6f;
        const auto colour = seam ? purple : cyan;

        if (active)
        {
            g.setColour (colour.withAlpha (0.30f));
            g.fillEllipse (x - radius - 5.0f, y - radius - 5.0f,
                           (radius + 5.0f) * 2.0f, (radius + 5.0f) * 2.0f);
        }

        g.setColour (chassis);
        g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (seam ? purple : juce::Colours::white);
        g.drawEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    }
}

void ReceiverCurveEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot = plotBounds();

    drawWell (g, bounds, 5.0f);
    drawGrid (g, plot);
    drawCurve (g, plot);
    drawPlayhead (g, plot);
    drawNodes (g, plot);

    g.setFont (font (8.5f));
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawText (gainText (processor.currentGainForUI.load (std::memory_order_relaxed)),
                juce::Rectangle<float> (bounds.getRight() - 62.0f, bounds.getY() + 2.0f, 56.0f, 12.0f),
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
            processor.setTension (segment, 0.5f);
        else
            draggedSegment = segment;

        repaint();
    }
}

void ReceiverCurveEditor::mouseDrag (const juce::MouseEvent& e)
{
    // Shift forces a snap for this drag regardless of the Always Snap
    // setting -- see snapPhase()/snapValue().
    const bool shiftHeld = e.mods.isShiftDown();

    std::array<SortedNode, maxNodes> nodes;
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

        const float value = snapValue (yToValue (e.position.y), shiftHeld);

        if (index == 0 || index == count - 1)
        {
            // Dragging either end of the loop seam moves both, so the cycle
            // always finishes at the height it starts from.
            processor.setEndpointY (value);
        }
        else
        {
            processor.setNodeY (draggedSlot, value);

            const float low = nodes[static_cast<size_t> (index - 1)].x + 0.004f;
            const float high = nodes[static_cast<size_t> (index + 1)].x - 0.004f;
            const float phase = snapPhase (xToPhase (e.position.x), shiftHeld);

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
    processor.addNode (phase, snapValue (yToValue (e.position.y), e.mods.isShiftDown()));
    repaint();
}

// =============================================================================
// ReceiverShapeStrip
// =============================================================================

juce::Rectangle<float> ReceiverShapeStrip::cellBounds (int index) const noexcept
{
    const float width = static_cast<float> (getWidth())
                        / static_cast<float> (HomeSidechainReceiverAudioProcessor::numPresets);

    return juce::Rectangle<float> (width * static_cast<float> (index), 0.0f,
                                   width, static_cast<float> (getHeight())).reduced (3.0f, 0.0f);
}

int ReceiverShapeStrip::cellAt (juce::Point<float> position) const noexcept
{
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numPresets; ++i)
        if (cellBounds (i).contains (position))
            return i;

    return -1;
}

void ReceiverShapeStrip::setSelected (int index)
{
    const int clamped = juce::jlimit (0, HomeSidechainReceiverAudioProcessor::numPresets - 1, index);

    if (clamped != selected)
    {
        selected = clamped;
        repaint();
    }
}

void ReceiverShapeStrip::paint (juce::Graphics& g)
{
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numPresets; ++i)
    {
        const auto cell = cellBounds (i);
        const bool isSelected = i == selected;

        // Selected reads as a darker, richer chip with a bright white curve
        // and border -- the "spotlighted" look, rather than matching the
        // white-fill Pill convention used for buttons elsewhere. This is a
        // thumbnail preview, not a button, so it gets its own visual
        // language on purpose.
        g.setColour (juce::Colours::black.withAlpha (isSelected ? 0.55f : 0.34f));
        g.fillRoundedRectangle (cell, 5.0f);

        if (i == hovered && ! isSelected)
        {
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRoundedRectangle (cell, 5.0f);
        }

        g.setColour (isSelected ? juce::Colours::white : juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (cell, 5.0f, isSelected ? 1.6f : 1.0f);

        auto plot = cell.reduced (7.0f, 6.0f);
        auto nameRow = plot.removeFromBottom (11.0f);

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

        g.setColour (isSelected ? juce::Colours::white : juce::Colours::white.withAlpha (0.55f));
        g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setFont (font (8.0f, isSelected));
        g.setColour (isSelected ? juce::Colours::white : juce::Colours::white.withAlpha (0.55f));
        g.drawText (HomeSidechainReceiverAudioProcessor::presetName (i), nameRow,
                    juce::Justification::centred, false);
    }
}

void ReceiverShapeStrip::mouseMove (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.position);

    if (cell != hovered)
    {
        hovered = cell;
        repaint();
    }
}

void ReceiverShapeStrip::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void ReceiverShapeStrip::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.position);

    if (cell < 0)
        return;

    setSelected (cell);

    if (onSelect != nullptr)
        onSelect (cell);
}

// =============================================================================
// ReceiverRateSelector
// =============================================================================

juce::String ReceiverRateSelector::categoryName (int rateIndex)
{
    using P = HomeSidechainReceiverAudioProcessor;

    if (rateIndex < P::numStraightRates)
        return "STRAIGHT";

    rateIndex -= P::numStraightRates;

    if (rateIndex < P::numTripletRates)
        return "TRIPLET";

    rateIndex -= P::numTripletRates;

    if (rateIndex < P::numDottedRates)
        return "DOTTED";

    return "GROOVE";
}

ReceiverRateSelector::ReceiverRateSelector (HomeSidechainReceiverAudioProcessor& p)
    : processor (p)
{
    prevPill.onClick = [this] { step (-1); };
    addAndMakeVisible (prevPill);

    nextPill.onClick = [this] { step (1); };
    addAndMakeVisible (nextPill);
}

void ReceiverRateSelector::step (int delta)
{
    const int count = HomeSidechainReceiverAudioProcessor::numRates;
    const int next = (processor.getRate() + delta + count) % count;
    processor.setRate (next);
    repaint();
}

void ReceiverRateSelector::openMenu()
{
    using P = HomeSidechainReceiverAudioProcessor;

    const auto names = P::rateNames();
    const int current = processor.getRate();

    juce::PopupMenu menu;
    juce::PopupMenu straight, triplet, dotted, groove;

    int index = 0;

    for (int i = 0; i < P::numStraightRates; ++i, ++index)
        straight.addItem (index + 1, names[index], true, index == current);

    for (int i = 0; i < P::numTripletRates; ++i, ++index)
        triplet.addItem (index + 1, names[index], true, index == current);

    for (int i = 0; i < P::numDottedRates; ++i, ++index)
        dotted.addItem (index + 1, names[index], true, index == current);

    for (int i = 0; i < P::numGrooveRates; ++i, ++index)
        groove.addItem (index + 1, names[index], true, index == current);

    menu.addSubMenu ("Straight", straight);
    menu.addSubMenu ("Triplet", triplet);
    menu.addSubMenu ("Dotted", dotted);
    menu.addSubMenu ("Groove / poly", groove);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (*this),
                        [this] (int result)
                        {
                            if (result > 0)
                            {
                                processor.setRate (result - 1);
                                repaint();
                            }
                        });
}

void ReceiverRateSelector::mouseDown (const juce::MouseEvent& e)
{
    if (usable && labelArea.contains (e.getPosition()))
        openMenu();
}

void ReceiverRateSelector::setUsable (bool shouldBeUsable)
{
    usable = shouldBeUsable;

    // A firmly reduced alpha with the border removed entirely in paint()
    // reads as "not applicable right now" rather than just a fainter version
    // of the normal look -- that similarity was part of what made this card
    // hard to read at a glance.
    setAlpha (usable ? 1.0f : 0.30f);
    prevPill.setEnabled (usable);
    nextPill.setEnabled (usable);
    repaint();
}

void ReceiverRateSelector::resized()
{
    auto area = getLocalBounds();
    prevPill.setBounds (area.removeFromLeft (30));
    nextPill.setBounds (area.removeFromRight (30));
    area.removeFromLeft (4);
    area.removeFromRight (4);
    labelArea = area;
}

void ReceiverRateSelector::paint (juce::Graphics& g)
{
    const auto rate = processor.getRate();
    const auto names = HomeSidechainReceiverAudioProcessor::rateNames();

    auto area = labelArea.toFloat();

    // One accent throughout -- green, matching the Timing card itself --
    // instead of a different colour per rate category. The category system
    // stayed useful for grouping the menu, but colour-coding it here meant
    // this readout could show cyan, purple or pink inside a green card,
    // which read as unrelated to everything else in Timing.
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (area, 6.0f);
    g.setColour (green.withAlpha (usable ? 0.55f : 0.0f));
    g.drawRoundedRectangle (area, 6.0f, 1.2f);

    auto textArea = area.reduced (2.0f, 4.0f);
    auto categoryRow = textArea.removeFromBottom (11.0f);

    g.setFont (font (20.0f, true));
    g.setColour (juce::Colours::white);
    g.drawText (rate >= 0 && rate < names.size() ? names[rate] : juce::String(),
                textArea, juce::Justification::centred, false);

    g.setFont (font (7.5f));
    g.setColour (green.withAlpha (0.75f));
    g.drawText (categoryName (rate), categoryRow, juce::Justification::centred, false);
}

// =============================================================================
// ReceiverSettingsPanel
// =============================================================================

ReceiverSettingsPanel::ReceiverSettingsPanel (HomeSidechainReceiverAudioProcessor& p,
                                              ReceiverCurveEditor& editor)
    : processor (p), curveEditor (editor)
{
    const char* names[] = { "LINK", "MIDI", "BOTH" };

    for (int i = 0; i < 3; ++i)
    {
        auto pill = std::make_unique<Pill> (names[i], green);
        pill->setFontSize (9.5f);
        pill->onClick = [this, i] { processor.setSource (i); refresh(); };
        addAndMakeVisible (*pill);
        sourcePills[static_cast<size_t> (i)] = std::move (pill);
    }

    alwaysSnapPill.setFontSize (9.5f);
    alwaysSnapPill.setToggleState (curveEditor.isSnapEnabled(), juce::dontSendNotification);
    alwaysSnapPill.onClick = [this]
    {
        const bool snap = ! curveEditor.isSnapEnabled();
        curveEditor.setSnapEnabled (snap);
        alwaysSnapPill.setToggleState (snap, juce::dontSendNotification);
    };
    addAndMakeVisible (alwaysSnapPill);

    closePill.setFontSize (9.5f);
    closePill.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closePill);
}

namespace
{
    // Shared between resized() and paint() so the two can't drift apart.
    constexpr int settingsTopPad = 16;
    constexpr int settingsTitleGap = 22;
    constexpr int settingsGap = 12;
    constexpr int settingsRowHeight = 26;
}

void ReceiverSettingsPanel::resized()
{
    auto card = cardBounds().toNearestInt();
    auto inner = card.reduced (18, settingsTopPad);
    inner.removeFromTop (settingsTitleGap);

    auto sourceRow = inner.removeFromTop (settingsRowHeight);
    sourceRow.removeFromLeft (96);

    for (auto& pill : sourcePills)
    {
        pill->setBounds (sourceRow.removeFromLeft (64));
        sourceRow.removeFromLeft (6);
    }

    inner.removeFromTop (settingsGap);
    auto snapRow = inner.removeFromTop (settingsRowHeight);
    alwaysSnapPill.setBounds (snapRow.removeFromLeft (160));

    closePill.setBounds (card.getRight() - 82, card.getBottom() - 38, 64, 24);
}

void ReceiverSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (chassis.withAlpha (0.82f));

    const auto card = cardBounds();
    drawCard (g, card, cyan);
    drawCardTitle (g, "ADVANCED", card);

    // Mirrors resized()'s row math exactly, so labels always land on the
    // controls they describe.
    float y = card.getY() + settingsTopPad + settingsTitleGap;
    const float sourceRowY = y;
    y += settingsRowHeight + settingsGap;
    y += settingsRowHeight;

    drawCardText (g, "TRIGGER FROM",
                  juce::Rectangle<float> (card.getX() + 18.0f, sourceRowY, 96.0f, settingsRowHeight),
                  9.5f, juce::Justification::centredLeft);

    // The checkbox carries its own "Always snap to grid" label now, so
    // there's no separate caption competing with it for the same line.
    drawCardText (g, "Off by default -- drag freely, or hold Shift to snap any drag to the grid.",
                  juce::Rectangle<float> (card.getX() + 18.0f, y + 6.0f, card.getWidth() - 36.0f, 16.0f),
                  8.0f, juce::Justification::topLeft, 0.6f);
}

void ReceiverSettingsPanel::mouseDown (const juce::MouseEvent& e)
{
    if (! cardBounds().contains (e.position))
        setVisible (false);
}

void ReceiverSettingsPanel::refresh()
{
    const int source = processor.getSource();

    for (int i = 0; i < 3; ++i)
        sourcePills[static_cast<size_t> (i)]->setToggleState (i == source, juce::dontSendNotification);

    alwaysSnapPill.setToggleState (curveEditor.isSnapEnabled(), juce::dontSendNotification);
}

// =============================================================================
// Editor
// =============================================================================

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (
    HomeSidechainReceiverAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), curveEditor (p), rateSelector (p),
      settingsPanel (p, curveEditor)
{
    addAndMakeVisible (curveEditor);
    addAndMakeVisible (shapeStrip);
    addAndMakeVisible (rateSelector);

    shapeStrip.onSelect = [this] (int index)
    {
        processor.applyPreset (index);
        curveEditor.repaint();
    };

    linkSelector.setFontSize (9.5f);
    linkSelector.onChange = [this] (int index) { processor.setLink (index); refreshFromParameters(); };
    addAndMakeVisible (linkSelector);

    modeSwitch.setFontSize (9.5f);
    modeSwitch.onChange = [this] (int index) { processor.setRunMode (index); refreshFromParameters(); };
    addAndMakeVisible (modeSwitch);

    shapesFiltersTab.setFontSize (9.5f);
    shapesFiltersTab.onChange = [this] (int) { refreshFromParameters(); };
    addAndMakeVisible (shapesFiltersTab);

    syncPill.setClickingTogglesState (true);
    syncPill.setFontSize (9.0f);
    addAndMakeVisible (syncPill);

    testPill.setFontSize (9.5f);
    testPill.onClick = [this] { processor.requestTestTrigger(); };
    addAndMakeVisible (testPill);

    advPill.setFontSize (9.5f);
    advPill.onClick = [this]
    {
        settingsPanel.refresh();
        settingsPanel.setVisible (! settingsPanel.isVisible());
    };
    addAndMakeVisible (advPill);

    resetIcon.setAccent (purple);
    resetIcon.onClick = [this]
    {
        processor.resetCurve();
        shapeStrip.setSelected (0);
        curveEditor.repaint();
    };
    addAndMakeVisible (resetIcon);

    addAndMakeVisible (power);

    depthKnob.valueText = percentText;
    mixKnob.valueText = percentText;
    addAndMakeVisible (depthKnob);
    addAndMakeVisible (mixKnob);

    lengthKnob.valueText = msText;
    addAndMakeVisible (lengthKnob);

    // Moved here from the Advanced panel; shown behind Shapes card's Filters
    // tab (see refreshFromParameters()), not always visible.
    smoothKnob.valueText = msText;
    lowCutKnob.valueText = hertzText;
    highCutKnob.valueText = hertzText;
    addChildComponent (smoothKnob);
    addChildComponent (lowCutKnob);
    addChildComponent (highCutKnob);

    depthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "DEPTH", depthKnob);
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, "MIX", mixKnob);
    lengthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "LENGTH", lengthKnob);
    smoothAttachment = std::make_unique<SliderAttachment> (processor.apvts, "SMOOTH", smoothKnob);
    lowCutAttachment = std::make_unique<SliderAttachment> (processor.apvts, "LOW_CUT", lowCutKnob);
    highCutAttachment = std::make_unique<SliderAttachment> (processor.apvts, "HIGH_CUT", highCutKnob);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", power);
    syncAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "SYNC", syncPill);

    addChildComponent (settingsPanel);

    setSize (designWidth, designHeight);
    refreshFromParameters();
    startTimerHz (30);
}

HomeSidechainReceiverAudioProcessorEditor::~HomeSidechainReceiverAudioProcessorEditor()
{
    stopTimer();
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    settingsPanel.setBounds (getLocalBounds());

    // ---- header ----
    linkSelector.setBounds (320, 29, 190, 22);

    testPill.setBounds (582, 29, 44, 22);
    advPill.setBounds (630, 29, 40, 22);
    power.setBounds (678, 27, 26, 26);

    // ---- graph card ----
    const auto graph = graphCard();
    curveEditor.setBounds (graph.reduced (12.0f, 0.0f)
                                .withTrimmedTop (26.0f)
                                .withTrimmedBottom (10.0f).toNearestInt());

    // ---- timing card ----
    auto timing = timingCard().toNearestInt().reduced (14, 12);
    timing.removeFromTop (18);

    // TRIG/HOST are a single connected switch (one mutually-exclusive pair);
    // Sync is an independent on/off modifier (Checkbox) and gets a visual
    // gap plus a divider line (drawn in paint()) so it doesn't read as a
    // third option belonging to that switch.
    auto runRow = timing.removeFromTop (24);
    modeSwitch.setBounds (runRow.removeFromLeft (100));
    runRow.removeFromLeft (11);
    syncDividerX = runRow.getX();
    runRow.removeFromLeft (9);
    syncPill.setBounds (runRow);

    timing.removeFromTop (12);

    // Rate selector and Length knob share the same slot -- only one is
    // visible at a time, depending on whether Sync is on (see
    // refreshFromParameters()).
    rateSelector.setBounds (timing);

    const int lengthKnobWidth = 100;
    lengthKnob.setBounds (timing.getCentreX() - lengthKnobWidth / 2, timing.getY(),
                          lengthKnobWidth, timing.getHeight());

    // ---- shape card: Shapes/Filters tab sits where the static title used
    //      to be, and its two pages share the strip's exact footprint ----
    const auto shapes = shapeCard().toNearestInt();
    shapesFiltersTab.setBounds (shapes.getCentreX() - 90, shapes.getY() + 6, 180, 20);

    const auto contentArea = juce::Rectangle<int> (shapes.getX() + 10, shapes.getY() + 28,
                                                    shapes.getWidth() - 20, shapes.getHeight() - 38);
    shapeStrip.setBounds (contentArea);
    resetIcon.setBounds (shapes.getRight() - 30, shapes.getY() + 4, 18, 18);

    const int filterKnobWidth = (contentArea.getWidth() - 40) / 3;
    auto filterRow = contentArea.reduced (10, 0);
    smoothKnob.setBounds (filterRow.removeFromLeft (filterKnobWidth));
    filterRow.removeFromLeft (20);
    lowCutKnob.setBounds (filterRow.removeFromLeft (filterKnobWidth));
    filterRow.removeFromLeft (20);
    highCutKnob.setBounds (filterRow);

    // ---- output card (restored to its previous size; sits directly under
    //      the now-shorter Timing card instead of matching Shape's row.
    //      Knobs stay side by side, per the earlier request -- only the
    //      squished size is being undone here) ----
    const auto output = outputCard().toNearestInt();
    depthKnob.setBounds (output.getX() + 20, output.getY() + 40, 78, 74);
    mixKnob.setBounds (output.getRight() - 98, output.getY() + 40, 78, 74);
}

void HomeSidechainReceiverAudioProcessorEditor::drawHeader (juce::Graphics& g) const
{
    drawBrand (g, "Sidechain", cyan, 25.0f, 16.0f, 695.0f);

    g.setFont (font (8.5f));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setColour (cyan.withAlpha (0.75f));
    g.drawText ("RECEIVER", juce::Rectangle<float> (202.0f, 22.0f, 80.0f, 13.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText (juce::String (processor.hostBpmForUI.load (std::memory_order_relaxed), 1) + " BPM",
                juce::Rectangle<float> (202.0f, 37.0f, 80.0f, 12.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawText ("LINK", juce::Rectangle<float> (286.0f, 33.0f, 34.0f, 14.0f),
                juce::Justification::centredLeft, false);

    const bool connected = processor.homeLinkConnected.load (std::memory_order_relaxed);

    drawLamp (g, juce::Rectangle<float> (512.0f, 33.0f, 64.0f, 14.0f),
              connected ? "LINKED" : "NO LINK", connected ? green : warn,
              processor.homeLinkActivity.load (std::memory_order_relaxed), connected);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (chassis);

    g.setColour (face);
    g.fillRoundedRectangle (10.0f, 10.0f, 700.0f, 396.0f, 8.0f);
    g.setColour (faceEdge);
    g.drawRoundedRectangle (10.0f, 10.0f, 700.0f, 396.0f, 8.0f, 1.5f);

    drawHeader (g);

    const auto graph = graphCard();
    const auto output = outputCard();
    const auto shapes = shapeCard();
    const auto timing = timingCard();

    drawCard (g, graph, cyan);
    drawCard (g, output, pink);
    drawCard (g, shapes, purple);
    drawCard (g, timing, green);

    drawCardTitle (g, "SHAPE", graph);
    drawCardTitle (g, "OUTPUT", output);
    drawCardTitle (g, "TIMING", timing);
    // Shapes card has no static title -- the Shapes/Filters tab switch
    // occupies that row instead and serves the same purpose.

    // Marks Sync as a different kind of control from TRIG/HOST -- a
    // modifier, not a third mode -- rather than relying on the checkbox
    // shape alone to carry that distinction.
    {
        const float dividerX = static_cast<float> (syncDividerX);
        const float top = timing.getY() + 30.0f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (dividerX, top, dividerX, top + 20.0f, 1.0f);
    }

    // The seam rule, stated where it matters rather than buried in a manual.
    drawCardText (g, "ENDS LOCKED", juce::Rectangle<float> (graph.getRight() - 96.0f, graph.getY() + 7.0f,
                                                            84.0f, 15.0f),
                  8.5f, juce::Justification::centredRight, 0.55f);
}

void HomeSidechainReceiverAudioProcessorEditor::refreshFromParameters()
{
    linkSelector.setSelectedIndex (processor.getLink(), juce::dontSendNotification);

    const int run = processor.getRunMode();
    modeSwitch.setSelectedIndex (run, juce::dontSendNotification);

    syncPill.setEnabled (run == 0);
    syncPill.setAlpha (run == 0 ? 1.0f : 0.4f);

    // Length only means anything with Sync off in Trigger mode -- otherwise
    // the rate drives the cycle. Rather than bury Length in Advanced, it
    // swaps in for the rate selector right in the timing card.
    const bool freeLength = run == 0 && ! processor.isSynced();
    rateSelector.setVisible (! freeLength);
    rateSelector.setUsable (! freeLength);
    lengthKnob.setVisible (freeLength);

    // Shapes/Filters tab: the two pages share the same footprint, so only
    // one set is ever visible.
    const bool showFilters = shapesFiltersTab.getSelectedIndex() == 1;
    shapeStrip.setVisible (! showFilters);
    resetIcon.setVisible (! showFilters);
    smoothKnob.setVisible (showFilters);
    lowCutKnob.setVisible (showFilters);
    highCutKnob.setVisible (showFilters);

    curveEditor.setGridDivisions (processor.gridDivisions());
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    refreshFromParameters();
    rateSelector.refresh();

    if (settingsPanel.isVisible())
        settingsPanel.refresh();

    curveEditor.repaint();
    repaint (juce::Rectangle<int> (10, 10, 700, 62));
}
