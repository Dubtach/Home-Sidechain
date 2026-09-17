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
    return getLocalBounds().toFloat().reduced (10.0f, 12.0f);
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

float ReceiverCurveEditor::snapValue (float value, bool fine) const noexcept
{
    if (! snapEnabled || fine)
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
                juce::Rectangle<float> (bounds.getRight() - 62.0f, bounds.getY() + 3.0f, 56.0f, 11.0f),
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
    const bool fine = e.mods.isShiftDown();

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

        const float value = snapValue (yToValue (e.position.y), fine);

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
// ReceiverSettingsPanel
// =============================================================================

ReceiverSettingsPanel::ReceiverSettingsPanel (HomeSidechainReceiverAudioProcessor& p)
    : processor (p)
{
    smoothKnob.valueText = msText;
    lengthKnob.valueText = msText;
    lowCutKnob.valueText = hertzText;
    highCutKnob.valueText = hertzText;

    for (auto* knob : { &smoothKnob, &lowCutKnob, &highCutKnob, &lengthKnob })
        addAndMakeVisible (*knob);

    const char* names[] = { "LINK", "MIDI", "BOTH" };

    for (int i = 0; i < 3; ++i)
    {
        auto pill = std::make_unique<Pill> (names[i], green);
        pill->setFontSize (9.5f);
        pill->onClick = [this, i] { processor.setSource (i); refresh(); };
        addAndMakeVisible (*pill);
        sourcePills[static_cast<size_t> (i)] = std::move (pill);
    }

    closePill.setFontSize (9.5f);
    closePill.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closePill);

    smoothAttachment = std::make_unique<SliderAttachment> (processor.apvts, "SMOOTH", smoothKnob);
    lowCutAttachment = std::make_unique<SliderAttachment> (processor.apvts, "LOW_CUT", lowCutKnob);
    highCutAttachment = std::make_unique<SliderAttachment> (processor.apvts, "HIGH_CUT", highCutKnob);
    lengthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "LENGTH", lengthKnob);
}

void ReceiverSettingsPanel::resized()
{
    auto card = cardBounds().toNearestInt();
    auto inner = card.reduced (18, 16);
    inner.removeFromTop (22);

    auto knobRow = inner.removeFromTop (96);
    const int knobWidth = (knobRow.getWidth() - 24) / 4;

    for (auto* knob : { &smoothKnob, &lengthKnob, &lowCutKnob, &highCutKnob })
    {
        knob->setBounds (knobRow.removeFromLeft (knobWidth));
        knobRow.removeFromLeft (8);
    }

    inner.removeFromTop (14);
    auto row = inner.removeFromTop (26);
    row.removeFromLeft (76);

    for (auto& pill : sourcePills)
    {
        pill->setBounds (row.removeFromLeft (64));
        row.removeFromLeft (6);
    }

    closePill.setBounds (card.getRight() - 82, card.getBottom() - 38, 64, 24);
}

void ReceiverSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (chassis.withAlpha (0.82f));

    const auto card = cardBounds();
    drawCard (g, card, cyan);
    drawCardTitle (g, "ADVANCED", card);

    drawCardText (g, "TRIGGER FROM",
                  juce::Rectangle<float> (card.getX() + 18.0f, card.getY() + 148.0f, 76.0f, 26.0f),
                  9.5f, juce::Justification::centredLeft);

    drawCardText (g, "Low cut and high cut set the band that ducks. Everything outside it passes through.",
                  juce::Rectangle<float> (card.getX() + 18.0f, card.getBottom() - 40.0f,
                                          card.getWidth() - 110.0f, 28.0f),
                  9.0f, juce::Justification::centredLeft, 0.65f);
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

    const bool freeLength = processor.getRunMode() == 0 && ! processor.isSynced();
    lengthKnob.setEnabled (freeLength);
    lengthKnob.setAlpha (freeLength ? 1.0f : 0.4f);
}

// =============================================================================
// Editor
// =============================================================================

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (
    HomeSidechainReceiverAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), curveEditor (p), settingsPanel (p)
{
    addAndMakeVisible (curveEditor);
    addAndMakeVisible (shapeStrip);

    shapeStrip.onSelect = [this] (int index)
    {
        processor.applyPreset (index);
        curveEditor.repaint();
    };

    for (int i = 0; i < homeSidechain::numberOfLinks; ++i)
    {
        auto pill = std::make_unique<Pill> (homeSidechain::linkName (i), cyan);
        pill->setFontSize (9.5f);
        pill->setCornerRadius (4.0f);
        pill->onClick = [this, i] { processor.setLink (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        linkPills[static_cast<size_t> (i)] = std::move (pill);
    }

    const auto rates = HomeSidechainReceiverAudioProcessor::rateNames();

    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::numRates; ++i)
    {
        auto pill = std::make_unique<Pill> (rates[i], green);
        pill->setFontSize (9.0f);
        pill->onClick = [this, i] { processor.setRate (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        ratePills[static_cast<size_t> (i)] = std::move (pill);
    }

    const char* runNames[] = { "TRIG", "HOST" };

    for (int i = 0; i < 2; ++i)
    {
        auto pill = std::make_unique<Pill> (runNames[i], green);
        pill->setFontSize (9.5f);
        pill->onClick = [this, i] { processor.setRunMode (i); refreshFromParameters(); };
        addAndMakeVisible (*pill);
        runPills[static_cast<size_t> (i)] = std::move (pill);
    }

    syncPill.setClickingTogglesState (true);
    syncPill.setFontSize (9.0f);
    addAndMakeVisible (syncPill);

    snapPill.setFontSize (9.0f);
    snapPill.setToggleState (curveEditor.isSnapEnabled(), juce::dontSendNotification);
    snapPill.onClick = [this]
    {
        const bool snap = ! curveEditor.isSnapEnabled();
        curveEditor.setSnapEnabled (snap);
        snapPill.setToggleState (snap, juce::dontSendNotification);
    };
    addAndMakeVisible (snapPill);

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

    resetPill.setFontSize (8.5f);
    resetPill.onClick = [this]
    {
        processor.resetCurve();
        shapeStrip.setSelected (0);
        curveEditor.repaint();
    };
    addAndMakeVisible (resetPill);

    addAndMakeVisible (power);

    depthKnob.valueText = percentText;
    mixKnob.valueText = percentText;
    addAndMakeVisible (depthKnob);
    addAndMakeVisible (mixKnob);

    depthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "DEPTH", depthKnob);
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, "MIX", mixKnob);
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
    int x = 320;

    for (auto& pill : linkPills)
    {
        pill->setBounds (x, 30, 20, 20);
        x += 23;
    }

    testPill.setBounds (582, 29, 44, 22);
    advPill.setBounds (630, 29, 40, 22);
    power.setBounds (678, 27, 26, 26);

    // ---- graph card ----
    const auto graph = graphCard();
    curveEditor.setBounds (graph.reduced (12.0f, 0.0f)
                                .withTrimmedTop (26.0f)
                                .withTrimmedBottom (10.0f).toNearestInt());

    // ---- output card ----
    const auto output = outputCard().toNearestInt();
    depthKnob.setBounds (output.getX() + 32, output.getY() + 28, 136, 82);
    mixKnob.setBounds (output.getX() + 32, output.getY() + 114, 136, 80);

    // ---- shape card ----
    const auto shapes = shapeCard().toNearestInt();
    shapeStrip.setBounds (shapes.getX() + 10, shapes.getY() + 28, shapes.getWidth() - 74, 84);
    resetPill.setBounds (shapes.getRight() - 58, shapes.getCentreY() + 4, 48, 22);

    // ---- timing card ----
    auto timing = timingCard().toNearestInt().reduced (12, 10);
    timing.removeFromTop (16);

    auto runRow = timing.removeFromTop (22);
    runPills[0]->setBounds (runRow.removeFromLeft (40));
    runRow.removeFromLeft (5);
    runPills[1]->setBounds (runRow.removeFromLeft (40));
    runRow.removeFromLeft (5);
    syncPill.setBounds (runRow.removeFromLeft (40));
    runRow.removeFromLeft (5);
    snapPill.setBounds (runRow.removeFromLeft (40));

    timing.removeFromTop (8);
    auto rateRowTop = timing.removeFromTop (22);
    timing.removeFromTop (5);
    auto rateRowBottom = timing.removeFromTop (22);

    for (int i = 0; i < 6; ++i)
    {
        auto& row = i < 3 ? rateRowTop : rateRowBottom;
        ratePills[static_cast<size_t> (i)]->setBounds (row.removeFromLeft (54));
        row.removeFromLeft (5);
    }
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
    g.fillRoundedRectangle (10.0f, 10.0f, 700.0f, 410.0f, 8.0f);
    g.setColour (faceEdge);
    g.drawRoundedRectangle (10.0f, 10.0f, 700.0f, 410.0f, 8.0f, 1.5f);

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
    drawCardTitle (g, "SHAPES", shapes);
    drawCardTitle (g, "TIMING", timing);

    // The seam rule, stated where it matters rather than buried in a manual.
    drawCardText (g, "ENDS LOCKED", juce::Rectangle<float> (graph.getRight() - 96.0f, graph.getY() + 7.0f,
                                                            84.0f, 15.0f),
                  8.5f, juce::Justification::centredRight, 0.55f);
}

void HomeSidechainReceiverAudioProcessorEditor::refreshFromParameters()
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

    syncPill.setEnabled (run == 0);
    syncPill.setAlpha (run == 0 ? 1.0f : 0.4f);

    const bool ratesUsable = run == 1 || processor.isSynced();

    for (auto& pill : ratePills)
    {
        pill->setEnabled (ratesUsable);
        pill->setAlpha (ratesUsable ? 1.0f : 0.4f);
    }

    curveEditor.setGridDivisions (processor.gridDivisions());
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    refreshFromParameters();

    if (settingsPanel.isVisible())
        settingsPanel.refresh();

    curveEditor.repaint();
    repaint (juce::Rectangle<int> (10, 10, 700, 62));
}
