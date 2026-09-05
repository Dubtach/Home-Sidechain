#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
    const juce::Colour bg0   (0xff05070a);
    const juce::Colour bg1   (0xff0b1016);
    const juce::Colour panel (0xff0b1117);
    const juce::Colour plotBg(0xff060a0f);
    const juce::Colour edge  (0xff25333d);
    const juce::Colour grid  (0xff21404d);
    const juce::Colour white (0xfff3f7fa);
    const juce::Colour muted (0xff82919e);
    const juce::Colour cyan  (0xff27e7ff);
    const juce::Colour violet(0xffa26bff);
    const juce::Colour green (0xff39e79b);
    const juce::Colour red   (0xffff5f68);
    const juce::Colour black (0xff010204);

    juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica")
                                                       .withStyle (bold ? "Bold" : "Plain"));
    }

    constexpr std::array<float, HomeSidechainReceiverAudioProcessor::maxNodes> presetX =
        { 0.00f, 0.10f, 0.22f, 0.38f, 0.56f, 0.72f, 0.88f, 1.00f };

    constexpr std::array<std::array<float, HomeSidechainReceiverAudioProcessor::maxNodes>, 12> presetY = {{
        { 1.00f, 1.00f, 1.00f, 0.96f, 0.62f, 0.18f, 0.04f, 0.04f },
        { 1.00f, 0.92f, 0.28f, 0.08f, 0.06f, 0.06f, 0.05f, 0.05f },
        { 1.00f, 0.66f, 0.12f, 0.06f, 0.08f, 0.12f, 0.30f, 1.00f },
        { 1.00f, 0.92f, 0.56f, 0.28f, 0.14f, 0.08f, 0.06f, 0.06f },
        { 1.00f, 0.98f, 0.80f, 0.48f, 0.18f, 0.08f, 0.06f, 0.06f },
        { 1.00f, 0.88f, 0.34f, 0.18f, 0.16f, 0.14f, 0.12f, 0.10f },
        { 1.00f, 0.96f, 0.86f, 0.72f, 0.48f, 0.22f, 0.08f, 0.04f },
        { 1.00f, 0.72f, 0.50f, 0.28f, 0.12f, 0.08f, 0.18f, 0.60f },
        { 1.00f, 0.24f, 0.10f, 0.22f, 0.58f, 0.86f, 0.94f, 0.96f },
        { 1.00f, 0.46f, 0.34f, 0.22f, 0.16f, 0.14f, 0.12f, 0.10f },
        { 1.00f, 0.82f, 0.66f, 0.44f, 0.22f, 0.18f, 0.36f, 0.72f },
        { 1.00f, 0.76f, 0.18f, 0.06f, 0.06f, 0.06f, 0.72f, 1.00f }
    }};

    void drawCard (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float radius)
    {
        g.setColour (black.withAlpha (0.60f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius + 1.0f);
        juce::ColourGradient fill (panel.brighter (0.05f), r.getX(), r.getY(), bg1,
                                   r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);
        juce::ColourGradient glow (accent.withAlpha (0.05f), r.getX(), r.getY(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r.reduced (1.0f), juce::jmax (0.0f, radius - 1.0f));
        g.setColour (edge.withAlpha (0.9f));
        g.drawRoundedRectangle (r, radius, 1.0f);
    }
}

ReceiverHomeLookAndFeel::ReceiverHomeLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, cyan);
    setColour (juce::Slider::textBoxTextColourId, white);
    setColour (juce::Slider::textBoxBackgroundColourId, plotBg);
    setColour (juce::Slider::textBoxOutlineColourId, edge);
}

void ReceiverHomeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                  float pos, float startAngle, float endAngle,
                                                  juce::Slider& slider)
{
    const float radius = juce::jmin ((float) w, (float) h) * 0.32f;
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto c = slider.getName() == "MIX" ? cyan : violet;

    g.setColour (black.withAlpha (0.55f));
    g.fillEllipse (cx - radius + 2.0f, cy - radius + 2.0f, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colour (0xff10171d));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    juce::Path bgArc;
    bgArc.addCentredArc (cx, cy, radius + 5.0f, radius + 5.0f, 0.0f,
                          startAngle, endAngle, true);
    g.setColour (edge);
    g.strokePath (bgArc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path fgArc;
    fgArc.addCentredArc (cx, cy, radius + 5.0f, radius + 5.0f, 0.0f,
                          startAngle, angle, true);
    g.setColour (c.withAlpha (0.20f));
    g.strokePath (fgArc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (fgArc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    g.setColour (white);
    g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    g.setColour (c);
    g.drawLine (cx, cy, cx + std::sin (angle) * (radius - 5.0f),
                cy - std::cos (angle) * (radius - 5.0f), 2.0f);
}

void ReceiverHomeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                                  float pos, float minPos, float maxPos,
                                                  juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = y + h * 0.5f;
    const float left = x + 2.0f;
    const float right = x + w - 2.0f;
    const float p = juce::jlimit (minPos, maxPos, pos);
    g.setColour (edge);
    g.fillRoundedRectangle (left, cy - 2.0f, right - left, 4.0f, 2.0f);
    g.setColour (cyan.withAlpha (0.85f));
    g.fillRoundedRectangle (left, cy - 2.0f, juce::jmax (0.0f, p - left), 4.0f, 2.0f);
    g.setColour (cyan.withAlpha (0.10f));
    g.fillEllipse (p - 9.0f, cy - 9.0f, 18.0f, 18.0f);
    g.setColour (white);
    g.fillEllipse (p - 4.0f, cy - 4.0f, 8.0f, 8.0f);
}

void ReceiverHomeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                      const juce::Colour&, bool hi, bool down)
{
    const auto b = button.getLocalBounds().toFloat();
    const auto name = button.getName();
    const bool active = button.getToggleState();

    if (name == "BYPASS")
        return;

    juce::Colour c = white;
    if (name.startsWith ("LINK_")) c = cyan;
    else if (name.startsWith ("RATE_")) c = green;
    else if (name.startsWith ("PRESET_")) c = violet;
    else if (name == "SYNC") c = green;
    else if (name == "TEST") c = cyan;
    else if (name == "RESET") c = white;

    g.setColour (active ? c.withAlpha (0.10f) : white.withAlpha (down ? 0.05f : 0.015f));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (active ? c.withAlpha (0.75f) : c.withAlpha (hi ? 0.42f : 0.18f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
    if (name.startsWith ("LINK_") && active)
    {
        g.setColour (cyan);
        g.fillRoundedRectangle (b.getX() + 7.0f, b.getBottom() - 2.0f, b.getWidth() - 14.0f, 1.5f, 1.0f);
    }
}

void ReceiverHomeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const auto name = button.getName();
    if (name == "BYPASS") return;
    const bool active = button.getToggleState();
    juce::Colour c = white;
    if (name.startsWith ("LINK_")) c = cyan;
    else if (name.startsWith ("RATE_")) c = green;
    else if (name.startsWith ("PRESET_")) c = violet;
    else if (name == "SYNC") c = green;
    else if (name == "TEST") c = cyan;
    g.setColour (active ? c : white.withAlpha (name.startsWith ("PRESET_") ? 0.40f : 0.60f));
    g.setFont (font (name.startsWith ("PRESET_") ? 6.2f : 7.4f, active));
    g.drawText (button.getButtonText(), button.getLocalBounds().toFloat(), juce::Justification::centred);
}

void ReceiverHomeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool hi, bool)
{
    if (button.getName() != "BYPASS" && button.getName() != "SYNC")
        return;
    const auto b = button.getLocalBounds().toFloat();
    if (button.getName() == "SYNC")
    {
        const auto c = green;
        g.setColour (button.getToggleState() ? c.withAlpha (0.14f) : white.withAlpha (0.015f));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (button.getToggleState() ? c.withAlpha (0.84f) : c.withAlpha (hi ? 0.58f : 0.24f));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (button.getToggleState() ? c : white.withAlpha (0.60f));
        g.setFont (font (7.2f, button.getToggleState()));
        g.drawText ("SYNC", b, juce::Justification::centred);
        return;
    }

    const auto c = button.getToggleState() ? red : cyan;
    g.setColour (black.withAlpha (0.35f));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (c.withAlpha (0.35f));
    g.fillRoundedRectangle (b.reduced (1.0f), 7.0f);
    const float cy = b.getCentreY();
    const float knobR = b.getHeight() * 0.28f;
    const float left = b.getX() + 9.0f;
    const float right = b.getRight() - 9.0f;
    const float knobX = button.getToggleState() ? right : left;
    g.setColour (white);
    g.fillEllipse (knobX - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);
}

ReceiverShaperGraph::ReceiverShaperGraph (HomeSidechainReceiverAudioProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> ReceiverShaperGraph::plotBounds() const noexcept
{
    return getLocalBounds().toFloat().reduced (20.0f, 18.0f);
}

std::array<int, HomeSidechainReceiverAudioProcessor::maxNodes> ReceiverShaperGraph::sortedNodeIndices (int& count) const noexcept
{
    std::array<int, HomeSidechainReceiverAudioProcessor::maxNodes> ids {};
    count = 0;
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
        if (processor.isNodeActive (i)) ids[static_cast<size_t> (count++)] = i;
    std::sort (ids.begin(), ids.begin() + count, [this] (int a, int b)
    {
        return processor.getNodeX (a) < processor.getNodeX (b);
    });
    return ids;
}

juce::Point<float> ReceiverShaperGraph::nodePoint (int index) const noexcept
{
    const auto b = plotBounds();
    return { b.getX() + b.getWidth() * processor.getNodeX (index),
             b.getBottom() - b.getHeight() * processor.getNodeY (index) };
}

juce::Point<float> ReceiverShaperGraph::handlePoint (int segment) const noexcept
{
    int count = 0;
    const auto ids = sortedNodeIndices (count);
    if (segment < 0 || segment >= count - 1) return {};
    const auto a = nodePoint (ids[static_cast<size_t> (segment)]);
    const auto b = nodePoint (ids[static_cast<size_t> (segment + 1)]);
    const auto mid = a + (b - a) * 0.5f;
    const float offset = processor.getHandle (segment) * plotBounds().getHeight() * 0.16f;
    return { mid.x, mid.y - offset };
}

int ReceiverShaperGraph::nearestNode (juce::Point<float> p) const noexcept
{
    int best = -1; float dBest = 12.0f;
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
        if (processor.isNodeActive (i))
        {
            const float d = nodePoint (i).getDistanceFrom (p);
            if (d < dBest) { dBest = d; best = i; }
        }
    return best;
}

int ReceiverShaperGraph::nearestHandle (juce::Point<float> p) const noexcept
{
    int count = 0; sortedNodeIndices (count);
    int best = -1; float dBest = 10.0f;
    for (int i = 0; i < count - 1; ++i)
    {
        const float d = handlePoint (i).getDistanceFrom (p);
        if (d < dBest) { dBest = d; best = i; }
    }
    return best;
}

float ReceiverShaperGraph::xToPhase (float x) const noexcept
{
    const auto b = plotBounds();
    return juce::jlimit (0.0f, 1.0f, (x - b.getX()) / juce::jmax (1.0f, b.getWidth()));
}

float ReceiverShaperGraph::yToValue (float y) const noexcept
{
    const auto b = plotBounds();
    return juce::jlimit (0.0f, 1.0f, (b.getBottom() - y) / juce::jmax (1.0f, b.getHeight()));
}

float ReceiverShaperGraph::phaseToX (float phase) const noexcept
{
    const auto b = plotBounds(); return b.getX() + b.getWidth() * juce::jlimit (0.0f, 1.0f, phase);
}

float ReceiverShaperGraph::valueToY (float value) const noexcept
{
    const auto b = plotBounds(); return b.getBottom() - b.getHeight() * juce::jlimit (0.0f, 1.0f, value);
}

bool ReceiverShaperGraph::canDeleteNode (int index) const noexcept
{
    int count = 0; const auto ids = sortedNodeIndices (count);
    if (count <= 2) return false;
    return ids[0] != index && ids[static_cast<size_t> (count - 1)] != index;
}

void ReceiverShaperGraph::deleteNode (int index)
{
    if (! canDeleteNode (index)) return;
    processor.setNodeActive (index, false);
    repaint();
}

void ReceiverShaperGraph::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (plotBg);
    g.fillRoundedRectangle (b, 10.0f);

    const auto plot = plotBounds();
    for (int i = 0; i <= 16; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * i / 16.0f;
        g.setColour (grid.withAlpha ((i % 4 == 0) ? 0.30f : 0.11f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 6; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * i / 6.0f;
        g.setColour (grid.withAlpha ((i % 2 == 0) ? 0.24f : 0.09f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    int count = 0;
    const auto ids = sortedNodeIndices (count);
    if (count >= 2)
    {
        juce::Path curve;
        curve.startNewSubPath (nodePoint (ids[0]));
        for (int s = 0; s < count - 1; ++s)
        {
            const auto p0 = nodePoint (ids[static_cast<size_t> (s)]);
            const auto p1 = nodePoint (ids[static_cast<size_t> (s + 1)]);
            const auto mid = p0 + (p1 - p0) * 0.5f;
            const float off = processor.getHandle (s) * plot.getHeight() * 0.16f;
            const auto c1 = juce::Point<float> (mid.x - (mid.x - p0.x) * 0.40f, mid.y - off);
            const auto c2 = juce::Point<float> (mid.x + (p1.x - mid.x) * 0.40f, mid.y - off);
            curve.cubicTo (c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        }

        juce::Path fill = curve;
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        g.setColour (cyan.withAlpha (0.055f));
        g.fillPath (fill);

        g.setColour (violet.withAlpha (0.11f));
        g.strokePath (curve, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        g.setColour (cyan);
        g.strokePath (curve, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        for (int s = 0; s < count - 1; ++s)
        {
            const auto hp = handlePoint (s);
            g.setColour (violet.withAlpha (s == hoveredHandle ? 1.0f : 0.52f));
            g.fillEllipse (hp.x - 2.8f, hp.y - 2.8f, 5.6f, 5.6f);
        }

        for (int i : ids)
        {
            const auto np = nodePoint (i);
            const bool hot = i == hoveredNode;
            g.setColour (black.withAlpha (0.6f));
            g.fillRoundedRectangle (np.x - 5.0f, np.y - 5.0f, 10.0f, 10.0f, 2.0f);
            g.setColour (hot ? white : cyan);
            g.fillRoundedRectangle (np.x - (hot ? 4.2f : 3.5f), np.y - (hot ? 4.2f : 3.5f),
                                    hot ? 8.4f : 7.0f, hot ? 8.4f : 7.0f, 1.8f);
        }
    }

    const bool active = processor.envelopeActiveForUI.load (std::memory_order_relaxed);
    if (active)
    {
        const float phase = juce::jlimit (0.0f, 1.0f, processor.envelopeDisplayPhase.load (std::memory_order_relaxed));
        const float x = phaseToX (phase);
        const float y = valueToY (processor.shapeValue (phase));
        g.setColour (green.withAlpha (0.24f));
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 2.0f);
        g.setColour (green);
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
    }

    g.setFont (font (7.0f, true));
    g.setColour (muted.withAlpha (0.70f));
    g.drawText ("FULL", plot.getX(), plot.getY() - 1, 36, 10, juce::Justification::left);
    g.drawText ("DUCK", plot.getX(), plot.getBottom() - 10, 40, 10, juce::Justification::left);
    g.drawText ("0", plot.getRight() - 12, plot.getBottom() - 10, 12, 10, juce::Justification::right);
}

void ReceiverShaperGraph::mouseMove (const juce::MouseEvent& e)
{
    hoveredNode = nearestNode (e.position);
    hoveredHandle = hoveredNode < 0 ? nearestHandle (e.position) : -1;
    setMouseCursor ((hoveredNode >= 0 || hoveredHandle >= 0)
                        ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::CrosshairCursor);
    repaint();
}

void ReceiverShaperGraph::mouseExit (const juce::MouseEvent&)
{
    hoveredNode = hoveredHandle = -1;
    repaint();
}

void ReceiverShaperGraph::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;
    draggedNode = nearestNode (e.position);
    draggedHandle = -1;
    draggingHandle = false;
    if (draggedNode < 0)
    {
        draggedHandle = nearestHandle (e.position);
        draggingHandle = draggedHandle >= 0;
    }
}

void ReceiverShaperGraph::mouseDrag (const juce::MouseEvent& e)
{
    const auto plot = plotBounds();
    if (draggedNode >= 0)
    {
        int count = 0; const auto ids = sortedNodeIndices (count);
        auto it = std::find (ids.begin(), ids.begin() + count, draggedNode);
        const int pos = it == ids.begin() + count ? -1 : static_cast<int> (std::distance (ids.begin(), it));
        float x = xToPhase (e.position.x);
        if (pos == 0) x = 0.0f;
        else if (pos == count - 1) x = 1.0f;
        else
        {
            const float lo = processor.getNodeX (ids[static_cast<size_t> (pos - 1)]) + 0.01f;
            const float hi = processor.getNodeX (ids[static_cast<size_t> (pos + 1)]) - 0.01f;
            x = juce::jlimit (lo, hi, x);
        }
        if (e.mods.isShiftDown())
        {
            const float oldY = processor.getNodeY (draggedNode);
            const float wanted = juce::jlimit (0.0f, 1.0f, oldY - (e.getDistanceFromDragStartY() * 0.0015f));
            processor.setNodeY (draggedNode, wanted);
        }
        else
        {
            processor.setNodeX (draggedNode, x);
            processor.setNodeY (draggedNode, yToValue (e.position.y));
        }
    }
    else if (draggingHandle && draggedHandle >= 0)
    {
        int count = 0; sortedNodeIndices (count);
        if (draggedHandle < count - 1)
        {
            const auto a = nodePoint (sortedNodeIndices (count)[static_cast<size_t> (draggedHandle)]);
            const auto b = nodePoint (sortedNodeIndices (count)[static_cast<size_t> (draggedHandle + 1)]);
            const float midY = (a.y + b.y) * 0.5f;
            const float amount = juce::jlimit (-1.0f, 1.0f,
                                               (midY - e.position.y) / juce::jmax (1.0f, plot.getHeight() * 0.16f));
            processor.setHandle (draggedHandle, amount);
        }
    }
    repaint();
}

void ReceiverShaperGraph::mouseUp (const juce::MouseEvent&)
{
    draggedNode = draggedHandle = -1;
    draggingHandle = false;
}

void ReceiverShaperGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int node = nearestNode (e.position);
    if (e.mods.isRightButtonDown() && node >= 0) { deleteNode (node); return; }
    if (node >= 0)
    {
        processor.setNodeX (node, node == 0 ? 0.0f : node == HomeSidechainReceiverAudioProcessor::maxNodes - 1 ? 1.0f : processor.getNodeX (node));
        processor.setNodeY (node, 0.85f);
        repaint();
        return;
    }
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
    {
        if (! processor.isNodeActive (i))
        {
            processor.setNodeX (i, xToPhase (e.position.x));
            processor.setNodeY (i, yToValue (e.position.y));
            processor.setNodeActive (i, true);
            repaint();
            return;
        }
    }
}

ReceiverFilterEditor::ReceiverFilterEditor (HomeSidechainReceiverAudioProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

juce::Point<float> ReceiverFilterEditor::filterPointForHz (double hz) const noexcept
{
    const auto b = getLocalBounds().toFloat().reduced (8.0f, 8.0f);
    const double lo = std::log (20.0), hi = std::log (20000.0);
    const double safe = juce::jlimit (20.0, 20000.0, hz);
    return { b.getX() + b.getWidth() * static_cast<float> ((std::log (safe) - lo) / (hi - lo)), b.getCentreY() };
}

double ReceiverFilterEditor::hzForX (float x) const noexcept
{
    const auto b = getLocalBounds().toFloat().reduced (8.0f, 8.0f);
    const double lo = std::log (20.0), hi = std::log (20000.0);
    const double p = juce::jlimit<double> (0.0, 1.0, (x - b.getX()) / juce::jmax (1.0f, b.getWidth()));
    return std::exp (lo + (hi - lo) * p);
}

void ReceiverFilterEditor::updateFromX (float x, bool lowCut)
{
    const double hz = hzForX (x);
    if (lowCut)
    {
        const double high = processor.apvts.getRawParameterValue ("HIGH_CUT")->load();
        const float safe = static_cast<float> (juce::jmin (hz, high * 0.96));
        if (auto* p = processor.apvts.getParameter ("LOW_CUT")) p->setValueNotifyingHost (p->convertTo0to1 (safe));
    }
    else
    {
        const double low = processor.apvts.getRawParameterValue ("LOW_CUT")->load();
        const float safe = static_cast<float> (juce::jmax (hz, low * 1.04));
        if (auto* p = processor.apvts.getParameter ("HIGH_CUT")) p->setValueNotifyingHost (p->convertTo0to1 (juce::jmin (20000.0, static_cast<double> (safe))));
    }
    repaint();
}

void ReceiverFilterEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto inner = b.reduced (8.0f, 8.0f);
    const auto low = filterPointForHz (processor.apvts.getRawParameterValue ("LOW_CUT")->load());
    const auto high = filterPointForHz (processor.apvts.getRawParameterValue ("HIGH_CUT")->load());

    g.setColour (edge.withAlpha (0.35f));
    g.fillRoundedRectangle (inner.getX(), inner.getCentreY() - 2.0f, inner.getWidth(), 4.0f, 2.0f);
    g.setColour (cyan.withAlpha (0.18f));
    g.fillRoundedRectangle (low.x, inner.getCentreY() - 4.0f, juce::jmax (0.0f, high.x - low.x), 8.0f, 4.0f);
    g.setColour (cyan.withAlpha (0.75f));
    g.fillRoundedRectangle (low.x, inner.getCentreY() - 2.0f, juce::jmax (0.0f, high.x - low.x), 4.0f, 2.0f);

    const float r1 = hoveredLow ? 7.0f : 5.0f;
    const float r2 = hoveredHigh ? 7.0f : 5.0f;
    g.setColour (hoveredLow ? white : cyan);  g.fillEllipse (low.x - r1, low.y - r1, r1 * 2.0f, r1 * 2.0f);
    g.setColour (hoveredHigh ? white : cyan); g.fillEllipse (high.x - r2, high.y - r2, r2 * 2.0f, r2 * 2.0f);

    g.setColour (muted);
    g.setFont (font (6.6f, true));
    g.drawText ("LOW CUT", b.getX(), b.getY() - 1, 52, 9, juce::Justification::left);
    g.drawText ("HIGH CUT", b.getRight() - 56, b.getY() - 1, 56, 9, juce::Justification::right);
}

void ReceiverFilterEditor::mouseMove (const juce::MouseEvent& e)
{
    const auto low = filterPointForHz (processor.apvts.getRawParameterValue ("LOW_CUT")->load());
    const auto high = filterPointForHz (processor.apvts.getRawParameterValue ("HIGH_CUT")->load());
    hoveredLow = e.position.getDistanceFrom (low) < 11.0f;
    hoveredHigh = e.position.getDistanceFrom (high) < 11.0f;
    setMouseCursor ((hoveredLow || hoveredHigh) ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    repaint();
}
void ReceiverFilterEditor::mouseExit (const juce::MouseEvent&) { hoveredLow = hoveredHigh = false; repaint(); }
void ReceiverFilterEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown()) return;
    const auto low = filterPointForHz (processor.apvts.getRawParameterValue ("LOW_CUT")->load());
    const auto high = filterPointForHz (processor.apvts.getRawParameterValue ("HIGH_CUT")->load());
    draggingLow = e.position.getDistanceFrom (low) < 13.0f;
    draggingHigh = !draggingLow && e.position.getDistanceFrom (high) < 13.0f;
}
void ReceiverFilterEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingLow) updateFromX (e.position.x, true);
    else if (draggingHigh) updateFromX (e.position.x, false);
}
void ReceiverFilterEditor::mouseUp (const juce::MouseEvent&) { draggingLow = draggingHigh = false; }
void ReceiverFilterEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto b = getLocalBounds().toFloat().reduced (8.0f);
    if (! b.contains (e.position)) return;
    if (std::abs (e.position.x - filterPointForHz (processor.apvts.getRawParameterValue ("LOW_CUT")->load()).x)
        < std::abs (e.position.x - filterPointForHz (processor.apvts.getRawParameterValue ("HIGH_CUT")->load()).x))
    {
        if (auto* p = processor.apvts.getParameter ("LOW_CUT")) p->setValueNotifyingHost (p->convertTo0to1 (20.0f));
    }
    else
    {
        if (auto* p = processor.apvts.getParameter ("HIGH_CUT")) p->setValueNotifyingHost (p->convertTo0to1 (20000.0f));
    }
    repaint();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), shaperGraph (p), filterEditor (p)
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    setLookAndFeel (&lookAndFeel);
    setSize (640, 390);

    addAndMakeVisible (shaperGraph);
    addAndMakeVisible (filterEditor);

    bypassButton.setName ("BYPASS");
    bypassButton.setClickingTogglesState (true);
    bypassButton.setLookAndFeel (&lookAndFeel);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", bypassButton);
    addAndMakeVisible (bypassButton);

    for (int i = 0; i < 3; ++i)
    {
        linkButtons[i].setName ("LINK_" + juce::String (i));
        linkButtons[i].setButtonText (homeSidechain::linkName (i));
        linkButtons[i].setLookAndFeel (&lookAndFeel);
        linkButtons[i].onClick = [this, i] { selectLink (i); };
        addAndMakeVisible (linkButtons[i]);
    }

    testButton.setName ("TEST"); testButton.setButtonText ("TEST"); testButton.setLookAndFeel (&lookAndFeel);
    testButton.onClick = [this] { requestTest(); }; addAndMakeVisible (testButton);
    resetButton.setName ("RESET"); resetButton.setButtonText ("RESET"); resetButton.setLookAndFeel (&lookAndFeel);
    resetButton.onClick = [this] { resetShape(); }; addAndMakeVisible (resetButton);

    mixKnob.setName ("MIX");
    mixKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    mixKnob.setLookAndFeel (&lookAndFeel);
    mixKnob.setPopupDisplayEnabled (true, false, this);
    mixKnob.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, "MIX", mixKnob);
    addAndMakeVisible (mixKnob);

    depthKnob.setName ("DEPTH");
    depthKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    depthKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    depthKnob.setLookAndFeel (&lookAndFeel);
    depthKnob.setPopupDisplayEnabled (true, false, this);
    depthKnob.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; };
    depthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "DEPTH", depthKnob);
    addAndMakeVisible (depthKnob);

    syncButton.setName ("SYNC"); syncButton.setButtonText ("SYNC"); syncButton.setClickingTogglesState (true);
    syncButton.setLookAndFeel (&lookAndFeel); syncAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "SYNC", syncButton);
    addAndMakeVisible (syncButton);

    for (int i = 0; i < 4; ++i)
    {
        rateButtons[i].setName ("RATE_" + juce::String (i));
        rateButtons[i].setButtonText (juce::StringArray { "1/8", "1/4", "1/2", "1/1" }[i]);
        rateButtons[i].setLookAndFeel (&lookAndFeel);
        rateButtons[i].onClick = [this, i] { selectRate (i); };
        addAndMakeVisible (rateButtons[i]);
    }

    for (int i = 0; i < 12; ++i)
    {
        presetButtons[i].setName ("PRESET_" + juce::String (i));
        presetButtons[i].setLookAndFeel (&lookAndFeel);
        presetButtons[i].onClick = [this, i] { selectPreset (i); };
        addAndMakeVisible (presetButtons[i]);
    }

    selectLink (processor.getLink());
    selectRate (juce::jlimit (0, 3, static_cast<int> (processor.apvts.getRawParameterValue ("RATE")->load())));
    for (auto& b : presetButtons) b.setToggleState (false, juce::dontSendNotification);
    startTimerHz (24);
}

HomeSidechainReceiverAudioProcessorEditor::~HomeSidechainReceiverAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
    mixKnob.setLookAndFeel (nullptr); depthKnob.setLookAndFeel (nullptr);
    bypassButton.setLookAndFeel (nullptr); syncButton.setLookAndFeel (nullptr);
    for (auto& b : linkButtons) b.setLookAndFeel (nullptr);
    for (auto& b : rateButtons) b.setLookAndFeel (nullptr);
    for (auto& b : presetButtons) b.setLookAndFeel (nullptr);
    testButton.setLookAndFeel (nullptr); resetButton.setLookAndFeel (nullptr);
}

void HomeSidechainReceiverAudioProcessorEditor::selectLink (int index)
{
    const int c = juce::jlimit (0, 2, index);
    if (auto* p = processor.apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (c)));
    refreshButtons();
}

void HomeSidechainReceiverAudioProcessorEditor::selectRate (int index)
{
    const int c = juce::jlimit (0, 3, index);
    if (auto* p = processor.apvts.getParameter ("RATE"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (c)));
    for (int i = 0; i < 4; ++i) rateButtons[i].setToggleState (i == c, juce::dontSendNotification);
}

void HomeSidechainReceiverAudioProcessorEditor::selectPreset (int index)
{
    const int p = juce::jlimit (0, 11, index);
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
    {
        processor.setNodeActive (i, true);
        processor.setNodeX (i, presetX[static_cast<size_t> (i)]);
        processor.setNodeY (i, presetY[static_cast<size_t> (p)][static_cast<size_t> (i)]);
    }
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes - 1; ++i)
        processor.setHandle (i, (p % 3 == 0) ? 0.0f : ((i % 2 == 0) ? 0.25f : -0.15f));
    for (int i = 0; i < 12; ++i) presetButtons[i].setToggleState (i == p, juce::dontSendNotification);
    shaperGraph.repaint();
}

void HomeSidechainReceiverAudioProcessorEditor::requestTest() { processor.requestTestTrigger(); }
void HomeSidechainReceiverAudioProcessorEditor::resetShape() { selectPreset (0); }
void HomeSidechainReceiverAudioProcessorEditor::refreshButtons()
{
    const int a = juce::jlimit (0, 2, processor.getLink());
    for (int i = 0; i < 3; ++i) linkButtons[i].setToggleState (i == a, juce::dontSendNotification);
}

void HomeSidechainReceiverAudioProcessorEditor::drawStatus (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool receiving = processor.envelopeActiveForUI.load (std::memory_order_relaxed);
    const auto c = bypassed ? red : (receiving ? green : cyan);
    const auto text = bypassed ? "BYPASSED" : (receiving ? "RECEIVING" : "READY");
    auto r = area.withWidth (92.0f).withHeight (22.0f);
    g.setColour (c.withAlpha (0.10f)); g.fillRoundedRectangle (r, 11.0f);
    g.setColour (c.withAlpha (0.72f)); g.drawRoundedRectangle (r, 11.0f, 1.0f);
    g.setColour (c); g.fillEllipse (r.getX() + 9.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setFont (font (7.1f, true)); g.drawText (text, r.getX() + 21.0f, r.getY() + 5.0f, 64, 12, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> b) const
{
    g.setFont (font (19.0f, true));
    g.setColour (white); g.drawText ("HOME-SIDECHAIN", b.getX(), b.getY(), 202, 22, juce::Justification::left);
    g.setColour (cyan); g.drawText ("RECEIVER", b.getX() + 204, b.getY(), 102, 22, juce::Justification::left);
    g.setFont (font (8.0f, true)); g.setColour (muted);
    g.drawText ("D U B T A C H   D S P", b.getX(), b.getY() + 22, 160, 10, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::drawGraphFrame (juce::Graphics& g, juce::Rectangle<float> r) const
{
    drawCard (g, r, cyan, 11.0f);
    g.setFont (font (7.5f, true)); g.setColour (muted);
    g.drawText ("DUCKING SHAPE", r.getX() + 13, r.getY() + 10, 120, 10, juce::Justification::left);
    drawStatus (g, { r.getRight() - 111, r.getY() + 8, 92, 22 });
    g.setFont (font (7.0f, true)); g.setColour (muted.withAlpha (0.65f));
    g.drawText ("DOUBLE-CLICK TO ADD", r.getX() + 13, r.getBottom() - 13, 140, 9, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::drawBottomControls (juce::Graphics& g, juce::Rectangle<float> r) const
{
    drawCard (g, r, violet, 10.0f);
    g.setFont (font (7.2f, true)); g.setColour (muted);
    g.drawText ("RATE", r.getX() + 15, r.getY() + 8, 50, 10, juce::Justification::left);
    g.drawText ("SHAPES", r.getX() + 178, r.getY() + 8, 60, 10, juce::Justification::left);
    g.drawText ("MIX", r.getX() + 424, r.getY() + 8, 40, 10, juce::Justification::centred);
    g.drawText ("DEPTH", r.getX() + 512, r.getY() + 8, 50, 10, juce::Justification::centred);
}

void HomeSidechainReceiverAudioProcessorEditor::drawCurvePreset (juce::Graphics& g, juce::Rectangle<float> r, int index, bool active) const
{
    g.setColour (active ? cyan.withAlpha (0.12f) : white.withAlpha (0.014f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (active ? cyan.withAlpha (0.68f) : white.withAlpha (0.12f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    auto p = r.reduced (5.0f);
    juce::Path curve;
    curve.startNewSubPath (p.getX(), p.getBottom());
    for (int i = 1; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
    {
        const float x = p.getX() + p.getWidth() * presetX[static_cast<size_t> (i)];
        const float y = p.getBottom() - p.getHeight() * presetY[static_cast<size_t> (index)][static_cast<size_t> (i)];
        curve.lineTo (x, y);
    }
    g.setColour (active ? cyan : white.withAlpha (0.44f));
    g.strokePath (curve, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);
    auto frame = getLocalBounds().toFloat().reduced (5.0f);
    juce::ColourGradient bg (bg1, frame.getX(), frame.getY(), bg0, frame.getRight(), frame.getBottom(), false);
    g.setGradientFill (bg); g.fillRoundedRectangle (frame, 12.0f);

    drawHeader (g, { 19.0f, 12.0f, 360.0f, 38.0f });
    const auto graph = juce::Rectangle<float> (18.0f, 55.0f, 604.0f, 247.0f);
    drawGraphFrame (g, graph);

    g.setFont (font (8.0f, true)); g.setColour (muted);
    g.drawText ("FILTER", 30, 264, 50, 10, juce::Justification::left);
    g.setFont (font (6.7f, true)); g.setColour (muted.withAlpha (0.75f));
    g.drawText (juce::String (juce::roundToInt (processor.apvts.getRawParameterValue ("LOW_CUT")->load())) + " Hz", 30, 277, 60, 9, juce::Justification::left);
    g.drawText (juce::String (juce::roundToInt (processor.apvts.getRawParameterValue ("HIGH_CUT")->load())) + " Hz", 132, 277, 65, 9, juce::Justification::right);

    const auto bottom = juce::Rectangle<float> (18.0f, 309.0f, 604.0f, 52.0f);
    drawBottomControls (g, bottom);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    shaperGraph.setBounds (28, 84, 584, 164);
    filterEditor.setBounds (29, 251, 175, 29);

    mixKnob.setBounds (410, 309, 56, 50);
    depthKnob.setBounds (498, 309, 56, 50);

    for (int i = 0; i < 3; ++i) linkButtons[i].setBounds (510 + i * 26, 12, 25, 22);
    bypassButton.setBounds (592, 10, 28, 26);

    syncButton.setBounds (74, 326, 46, 22);
    for (int i = 0; i < 4; ++i) rateButtons[i].setBounds (124 + i * 34, 326, 31, 22);

    testButton.setBounds (516, 63, 42, 19);
    resetButton.setBounds (564, 63, 42, 19);

    const int x0 = 236, y0 = 326, w = 27, h = 21, gap = 4;
    for (int i = 0; i < 12; ++i)
        presetButtons[i].setBounds (x0 + (i % 6) * (w + gap), y0 + (i / 6) * (h + gap), w, h);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    shaperGraph.repaint(); filterEditor.repaint(); refreshButtons(); repaint();
}
