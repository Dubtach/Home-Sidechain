#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
    const juce::Colour bg0    (0xff040609);
    const juce::Colour bg1    (0xff090d12);
    const juce::Colour panel  (0xff0c1319);
    const juce::Colour plotBg (0xff05090d);
    const juce::Colour edge   (0xff263640);
    const juce::Colour grid   (0xff21414e);
    const juce::Colour white  (0xfff1f6fa);
    const juce::Colour muted  (0xff8796a3);
    const juce::Colour cyan   (0xff1ee7ff);
    const juce::Colour violet (0xff9b6cff);
    const juce::Colour green  (0xff36e79a);
    const juce::Colour red    (0xffff5965);
    const juce::Colour black  (0xff010204);

    juce::Font uiFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica")
                                                        .withStyle (bold ? "Bold" : "Plain"));
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float radius)
    {
        g.setColour (black.withAlpha (0.66f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius + 1.0f);

        juce::ColourGradient fill (panel.brighter (0.045f), r.getX(), r.getY(),
                                   bg1, r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);

        juce::ColourGradient glow (accent.withAlpha (0.055f), r.getX(), r.getY(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r.reduced (1.0f), juce::jmax (0.0f, radius - 1.0f));

        g.setColour (edge.withAlpha (0.96f));
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

void ReceiverHomeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                                  float startAngle, float endAngle, juce::Slider& slider)
{
    const float radius = juce::jmin ((float) w, (float) h) * 0.34f;
    const float cx = (float) x + (float) w * 0.5f;
    const float cy = (float) y + (float) h * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto c = slider.getName() == "MIX_KNOB" ? cyan : violet;

    g.setColour (black.withAlpha (0.55f));
    g.fillEllipse (cx - radius + 2.0f, cy - radius + 3.0f, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colour (0xff111820));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    juce::Path arc;
    arc.addCentredArc (cx, cy, radius + 5.0f, radius + 5.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (edge);
    g.strokePath (arc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path fillArc;
    fillArc.addCentredArc (cx, cy, radius + 5.0f, radius + 5.0f, 0.0f, startAngle, angle, true);
    g.setColour (c.withAlpha (0.22f));
    g.strokePath (fillArc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (fillArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (white);
    g.fillEllipse (cx - 2.8f, cy - 2.8f, 5.6f, 5.6f);
    g.setColour (c);
    g.drawLine (cx, cy, cx + std::sin (angle) * (radius - 4.0f),
                cy - std::cos (angle) * (radius - 4.0f), 2.2f);
}

void ReceiverHomeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float p,
                                                  float minP, float maxP, juce::Slider::SliderStyle,
                                                  juce::Slider&)
{
    const float cy = (float) y + (float) h * 0.5f;
    const float left = (float) x + 3.0f;
    const float right = (float) (x + w) - 3.0f;
    const float clamped = juce::jlimit (minP, maxP, p);

    g.setColour (edge.withAlpha (0.8f));
    g.fillRoundedRectangle (left, cy - 2.0f, right - left, 4.0f, 2.0f);
    g.setColour (cyan.withAlpha (0.80f));
    g.fillRoundedRectangle (left, cy - 2.0f, juce::jmax (0.0f, clamped - left), 4.0f, 2.0f);
    g.setColour (cyan.withAlpha (0.12f));
    g.fillEllipse (clamped - 9.0f, cy - 9.0f, 18.0f, 18.0f);
    g.setColour (white);
    g.fillEllipse (clamped - 4.5f, cy - 4.5f, 9.0f, 9.0f);
}

void ReceiverHomeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                      const juce::Colour&, bool hi, bool down)
{
    const auto b = button.getLocalBounds().toFloat();
    const auto name = button.getName();
    const bool active = button.getToggleState();

    if (name == "BYPASS")
    {
        const auto c = active ? red : cyan;
        g.setColour (c.withAlpha (hi ? 0.18f : 0.09f));
        g.fillEllipse (b.reduced (2.0f));
        g.setColour (c.withAlpha (hi ? 1.0f : 0.76f));
        g.drawEllipse (b.reduced (2.0f), 1.3f);
        return;
    }

    if (name.startsWith ("LINK_"))
    {
        g.setColour (active ? cyan.withAlpha (0.08f) : white.withAlpha (0.012f));
        g.fillRoundedRectangle (b, 9.0f);
        g.setColour (active ? cyan.withAlpha (0.82f) : white.withAlpha (0.17f));
        g.drawRoundedRectangle (b.reduced (0.5f), 9.0f, 1.0f);
        if (active)
        {
            g.setColour (cyan);
            g.fillRoundedRectangle (b.getX() + 8.0f, b.getBottom() - 2.5f,
                                    b.getWidth() - 16.0f, 1.7f, 0.8f);
        }
        return;
    }

    if (name.startsWith ("RATE_"))
    {
        g.setColour (active ? cyan.withAlpha (0.10f) : white.withAlpha (0.018f));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (active ? cyan.withAlpha (0.80f) : white.withAlpha (0.14f));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
        return;
    }

    if (name == "SYNC" || name.startsWith ("BAND_") || name == "TEST" || name == "RESET")
    {
        const auto c = name == "SYNC" ? green : (name == "TEST" ? cyan : violet);
        g.setColour (active ? c.withAlpha (0.12f) : (down ? c.withAlpha (0.11f) : white.withAlpha (0.018f)));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (active ? c.withAlpha (0.84f) : c.withAlpha (hi ? 0.62f : 0.24f));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
        return;
    }

    if (name.startsWith ("PRESET_"))
    {
        g.setColour (active ? cyan.withAlpha (0.11f) : white.withAlpha (0.012f));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (active ? cyan.withAlpha (0.75f) : white.withAlpha (0.10f));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
        return;
    }

    g.setColour (down ? white.withAlpha (0.06f) : white.withAlpha (0.018f));
    g.fillRoundedRectangle (b, 5.0f);
}

void ReceiverHomeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const auto name = button.getName();
    if (name == "RESET")
    {
        g.setColour (white.withAlpha (0.70f));
        g.setFont (uiFont (8.0f, true));
        g.drawText ("RESET", button.getLocalBounds().toFloat(), juce::Justification::centred);
        return;
    }

    if (name == "TEST")
    {
        g.setColour (cyan);
        g.setFont (uiFont (7.4f, true));
        g.drawText ("TEST", button.getLocalBounds().toFloat(), juce::Justification::centred);
        return;
    }

    if (name == "BYPASS")
        return;

    const bool active = button.getToggleState();
    const bool link = name.startsWith ("LINK_");
    const bool preset = name.startsWith ("PRESET_");
    const bool rate = name.startsWith ("RATE_");
    const bool band = name.startsWith ("BAND_");
    const bool sync = name == "SYNC";
    const auto c = link || rate ? cyan : (band ? violet : (sync ? green : white));
    g.setColour (active ? c : white.withAlpha (preset ? 0.42f : 0.56f));
    g.setFont (uiFont (preset ? 6.0f : (link ? 8.0f : 7.2f), active));
    if (! preset)
        g.drawText (button.getButtonText(), button.getLocalBounds().toFloat(), juce::Justification::centred);
}

void ReceiverHomeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool hi, bool)
{
    if (button.getName() != "BYPASS")
        return;
    drawButtonBackground (g, button, {}, hi, false);

    const auto b = button.getLocalBounds().toFloat().reduced (7.0f);
    const float cx = b.getCentreX();
    const float cy = b.getCentreY();
    const float r = b.getWidth() * 0.27f;
    const auto c = button.getToggleState() ? red : cyan;

    juce::Path arc;
    arc.addCentredArc (cx, cy, r, r, 0.0f,
                       juce::MathConstants<float>::pi * 0.22f,
                       juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi * 0.22f, true);
    g.setColour (c);
    g.strokePath (arc, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    g.drawLine (cx, cy - r - 2.0f, cx, cy + 1.0f, 1.4f);
}

ReceiverShapeEditor::ReceiverShapeEditor (HomeSidechainReceiverAudioProcessor& p)
    : processor (p)
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> ReceiverShapeEditor::plotBounds() const noexcept
{
    return getLocalBounds().toFloat().reduced (13.0f, 13.0f);
}

float ReceiverShapeEditor::phaseToX (float phase) const noexcept
{
    const auto b = plotBounds();
    return b.getX() + b.getWidth() * juce::jlimit (0.0f, 1.0f, phase);
}

float ReceiverShapeEditor::valueToY (float value) const noexcept
{
    const auto b = plotBounds();
    return b.getBottom() - b.getHeight() * juce::jlimit (0.0f, 1.0f, value);
}

float ReceiverShapeEditor::xToPhase (float x) const noexcept
{
    const auto b = plotBounds();
    return juce::jlimit (0.0f, 1.0f, (x - b.getX()) / juce::jmax (1.0f, b.getWidth()));
}

float ReceiverShapeEditor::yToValue (float y) const noexcept
{
    const auto b = plotBounds();
    return juce::jlimit (0.0f, 1.0f, (b.getBottom() - y) / juce::jmax (1.0f, b.getHeight()));
}

juce::Point<float> ReceiverShapeEditor::nodePoint (int index) const noexcept
{
    return { phaseToX (processor.getNodeX (index)), valueToY (processor.getNodeY (index)) };
}

std::vector<int> sortedNodeIndices (const HomeSidechainReceiverAudioProcessor& processor)
{
    std::vector<int> ids;
    ids.reserve (HomeSidechainReceiverAudioProcessor::maxNodes);
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
        if (processor.isNodeActive (i))
            ids.push_back (i);
    std::sort (ids.begin(), ids.end(), [&processor] (int a, int b) {
        return processor.getNodeX (a) < processor.getNodeX (b);
    });
    return ids;
}

juce::Point<float> ReceiverShapeEditor::handlePoint (int segment) const noexcept
{
    const auto ids = sortedNodeIndices (processor);
    if (segment < 0 || segment >= static_cast<int> (ids.size()) - 1)
        return {};

    const auto a = nodePoint (ids[static_cast<size_t> (segment)]);
    const auto b = nodePoint (ids[static_cast<size_t> (segment + 1)]);
    const auto mid = a + (b - a) * 0.5f;
    const float h = processor.getHandle (segment);
    const float offset = h * plotBounds().getHeight() * 0.17f;
    return { mid.x, mid.y - offset };
}

int ReceiverShapeEditor::nearestNode (juce::Point<float> p) const noexcept
{
    int best = -1;
    float bestDistance = 13.0f;
    for (int i = 0; i < HomeSidechainReceiverAudioProcessor::maxNodes; ++i)
    {
        if (! processor.isNodeActive (i))
            continue;
        const float d = nodePoint (i).getDistanceFrom (p);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

int ReceiverShapeEditor::nearestHandle (juce::Point<float> p) const noexcept
{
    const int segments = juce::jmax (0, processor.activeNodeCount() - 1);
    int best = -1;
    float bestDistance = 10.0f;
    for (int i = 0; i < segments; ++i)
    {
        const float d = handlePoint (i).getDistanceFrom (p);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

void ReceiverShapeEditor::sortNodesByX (int movingIndex)
{
    juce::ignoreUnused (movingIndex);
    // Slots are intentionally not swapped. The processor sorts by X for
    // evaluation, while the handle parameters remain segment-ordered.
}

void ReceiverShapeEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (plotBg);
    g.fillRoundedRectangle (b, 10.0f);

    const auto plot = plotBounds();

    for (int i = 0; i <= 16; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 16.0f;
        g.setColour (grid.withAlpha (i % 4 == 0 ? 0.38f : 0.14f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 6; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 6.0f;
        g.setColour (grid.withAlpha (i % 2 == 0 ? 0.34f : 0.12f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    const auto ids = sortedNodeIndices (processor);
    if (ids.size() < 2)
        return;

    juce::Path curve;
    curve.startNewSubPath (nodePoint (ids[0]));
    for (size_t s = 0; s + 1 < ids.size(); ++s)
    {
        const auto p0 = nodePoint (ids[s]);
        const auto p1 = nodePoint (ids[s + 1]);
        const auto mid = p0 + (p1 - p0) * 0.5f;
        const float h = processor.getHandle (static_cast<int> (s));
        const float off = h * plot.getHeight() * 0.17f;
        const auto c1 = juce::Point<float> (mid.x - (mid.x - p0.x) * 0.35f, mid.y - off);
        const auto c2 = juce::Point<float> (mid.x + (p1.x - mid.x) * 0.35f, mid.y - off);
        curve.cubicTo (c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
    }

    juce::Path fill = curve;
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();
    g.setColour (cyan.withAlpha (0.065f));
    g.fillPath (fill);

    g.setColour (violet.withAlpha (0.11f));
    g.strokePath (curve, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (cyan);
    g.strokePath (curve, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    for (int i : ids)
    {
        const auto p = nodePoint (i);
        const bool hovered = (i == hoveredNode);
        g.setColour (black.withAlpha (0.55f));
        g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        g.setColour (hovered ? white : cyan);
        g.fillEllipse (p.x - (hovered ? 4.2f : 3.4f), p.y - (hovered ? 4.2f : 3.4f),
                       hovered ? 8.4f : 6.8f, hovered ? 8.4f : 6.8f);
    }

    const int segments = static_cast<int> (ids.size()) - 1;
    for (int s = 0; s < segments; ++s)
    {
        const auto p = handlePoint (s);
        const bool hovered = s == hoveredHandle;
        g.setColour (violet.withAlpha (hovered ? 0.90f : 0.55f));
        g.fillEllipse (p.x - 2.7f, p.y - 2.7f, 5.4f, 5.4f);
    }

    if (processor.envelopeActiveForUI.load (std::memory_order_relaxed))
    {
        const float phase = juce::jlimit<float> (0.0f, 1.0f,
            processor.envelopeDisplayPhase.load (std::memory_order_relaxed));
        const float x = phaseToX (phase);
        const float y = valueToY (processor.shapeValue (phase));
        g.setColour (green.withAlpha (0.42f));
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.2f);
        g.setColour (green);
        g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
    }

    g.setFont (uiFont (7.0f, true));
    g.setColour (muted.withAlpha (0.70f));
    g.drawText ("0 dB", 8, plot.getY() - 1, 32, 10, juce::Justification::left);
    g.drawText ("-6", 8, plot.getY() + plot.getHeight() * 0.25f - 5.0f, 26, 10, juce::Justification::left);
    g.drawText ("-12", 8, plot.getY() + plot.getHeight() * 0.50f - 5.0f, 26, 10, juce::Justification::left);
    g.drawText ("-24", 8, plot.getY() + plot.getHeight() * 0.75f - 5.0f, 26, 10, juce::Justification::left);
    g.drawText ("FULL", plot.getRight() - 34, plot.getY() - 1, 34, 10, juce::Justification::right);

    g.setFont (uiFont (6.5f, true));
    g.setColour (muted.withAlpha (0.62f));
    g.drawText ("ATTACK", plot.getX(), plot.getBottom() - 9, 48, 9, juce::Justification::left);
    g.drawText ("RELEASE", plot.getRight() - 48, plot.getBottom() - 9, 48, 9, juce::Justification::right);
}

void ReceiverShapeEditor::mouseMove (const juce::MouseEvent& e)
{
    hoveredNode = nearestNode (e.position);
    hoveredHandle = hoveredNode < 0 ? nearestHandle (e.position) : -1;
    setMouseCursor ((hoveredNode >= 0 || hoveredHandle >= 0)
                        ? juce::MouseCursor::PointingHandCursor
                        : juce::MouseCursor::CrosshairCursor);
    repaint();
}

void ReceiverShapeEditor::mouseExit (const juce::MouseEvent&)
{
    hoveredNode = -1;
    hoveredHandle = -1;
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    repaint();
}

void ReceiverShapeEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;
    draggedNode = nearestNode (e.position);
    draggedHandle = -1;
    if (draggedNode < 0)
    {
        draggedHandle = nearestHandle (e.position);
        draggingHandle = draggedHandle >= 0;
    }
    else
    {
        draggingHandle = false;
    }
}

void ReceiverShapeEditor::mouseDrag (const juce::MouseEvent& e)
{
    const auto plot = plotBounds();
    if (draggedNode >= 0)
    {
        float x = xToPhase (e.position.x);
        const auto ids = sortedNodeIndices (processor);
        auto it = std::find (ids.begin(), ids.end(), draggedNode);
        const int position = it != ids.end() ? static_cast<int> (std::distance (ids.begin(), it)) : -1;
        if (position == 0)
            x = 0.0f;
        else if (position == static_cast<int> (ids.size()) - 1)
            x = 1.0f;
        else
        {
            const float left = processor.getNodeX (ids[static_cast<size_t> (position - 1)]) + 0.015f;
            const float right = processor.getNodeX (ids[static_cast<size_t> (position + 1)]) - 0.015f;
            x = juce::jlimit (left, right, x);
        }
        processor.setNodeX (draggedNode, x);
        processor.setNodeY (draggedNode, yToValue (e.position.y));
    }
    else if (draggingHandle && draggedHandle >= 0)
    {
        const auto ids = sortedNodeIndices (processor);
        if (draggedHandle < static_cast<int> (ids.size()) - 1)
        {
            const auto a = nodePoint (ids[static_cast<size_t> (draggedHandle)]);
            const auto b = nodePoint (ids[static_cast<size_t> (draggedHandle + 1)]);
            const float midY = (a.y + b.y) * 0.5f;
            const float amount = juce::jlimit (-1.0f, 1.0f,
                (midY - e.position.y) / juce::jmax (1.0f, plot.getHeight() * 0.17f));
            processor.setHandle (draggedHandle, amount);
        }
    }
    repaint();
}

void ReceiverShapeEditor::mouseUp (const juce::MouseEvent&)
{
    draggedNode = -1;
    draggedHandle = -1;
    draggingHandle = false;
}

void ReceiverShapeEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int existingNode = nearestNode (e.position);
    const int existingHandle = nearestHandle (e.position);
    if (existingNode >= 0 || existingHandle >= 0)
        return;

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

ReceiverFilterEditor::ReceiverFilterEditor (HomeSidechainReceiverAudioProcessor& p)
    : processor (p)
{
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

juce::Point<float> ReceiverFilterEditor::filterPointForHz (double hz) const noexcept
{
    const auto b = getLocalBounds().toFloat().reduced (7.0f, 9.0f);
    const double lo = std::log (20.0);
    const double hi = std::log (20000.0);
    const float x = b.getX() + b.getWidth() * static_cast<float> ((std::log (juce::jlimit (20.0, 20000.0, hz)) - lo) / (hi - lo));
    return { x, b.getCentreY() };
}

double ReceiverFilterEditor::hzForX (float x) const noexcept
{
    const auto b = getLocalBounds().toFloat().reduced (7.0f, 9.0f);
    const double lo = std::log (20.0);
    const double hi = std::log (20000.0);
    const double p = juce::jlimit<double> (0.0, 1.0, (x - b.getX()) / juce::jmax (1.0f, b.getWidth()));
    return std::exp (lo + (hi - lo) * p);
}

void ReceiverFilterEditor::updateFromX (float x, bool lowCut)
{
    const double hz = hzForX (x);
    if (lowCut)
    {
        const double high = processor.apvts.getRawParameterValue ("HIGH_CUT")->load();
        const double safe = juce::jmin (hz, high * 0.96);
        if (auto* p = processor.apvts.getParameter ("LOW_CUT"))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jmax (20.0, safe)));
    }
    else
    {
        const double low = processor.apvts.getRawParameterValue ("LOW_CUT")->load();
        const double safe = juce::jmax (hz, low * 1.04);
        if (auto* p = processor.apvts.getParameter ("HIGH_CUT"))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jmin (20000.0, safe)));
    }
    repaint();
}

void ReceiverFilterEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (plotBg);
    g.fillRoundedRectangle (b, 7.0f);
    g.setColour (edge.withAlpha (0.85f));
    g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);

    const auto inner = b.reduced (8.0f, 11.0f);
    const float lowHz = processor.apvts.getRawParameterValue ("LOW_CUT")->load();
    const float highHz = processor.apvts.getRawParameterValue ("HIGH_CUT")->load();
    const auto low = filterPointForHz (lowHz);
    const auto high = filterPointForHz (highHz);

    g.setColour (cyan.withAlpha (0.07f));
    g.fillRoundedRectangle (low.x, inner.getY(), juce::jmax (0.0f, high.x - low.x), inner.getHeight(), 3.0f);

    g.setColour (edge.withAlpha (0.55f));
    g.fillRoundedRectangle (inner.getX(), inner.getCentreY() - 2.0f, inner.getWidth(), 4.0f, 2.0f);
    g.setColour (cyan.withAlpha (0.75f));
    g.fillRoundedRectangle (low.x, inner.getCentreY() - 2.0f, juce::jmax (0.0f, high.x - low.x), 4.0f, 2.0f);

    g.setColour (cyan);
    g.fillEllipse (low.x - 5.0f, inner.getCentreY() - 5.0f, 10.0f, 10.0f);
    g.fillEllipse (high.x - 5.0f, inner.getCentreY() - 5.0f, 10.0f, 10.0f);

    g.setColour (muted);
    g.setFont (uiFont (6.4f, true));
    g.drawText ("LOW CUT", 8, 2, 50, 9, juce::Justification::left);
    g.drawText ("HIGH CUT", b.getRight() - 56, 2, 48, 9, juce::Justification::right);
}

void ReceiverFilterEditor::mouseMove (const juce::MouseEvent& e)
{
    const auto low = filterPointForHz (processor.apvts.getRawParameterValue ("LOW_CUT")->load());
    const auto high = filterPointForHz (processor.apvts.getRawParameterValue ("HIGH_CUT")->load());
    draggingLow = false;
    draggingHigh = false;
    const bool nearLow = e.position.getDistanceFrom (low) < 10.0f;
    const bool nearHigh = e.position.getDistanceFrom (high) < 10.0f;
    setMouseCursor ((nearLow || nearHigh) ? juce::MouseCursor::LeftRightResizeCursor
                                           : juce::MouseCursor::NormalCursor);
}

void ReceiverFilterEditor::mouseExit (const juce::MouseEvent&) {}

void ReceiverFilterEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;
    const auto low = filterPointForHz (processor.apvts.getRawParameterValue ("LOW_CUT")->load());
    const auto high = filterPointForHz (processor.apvts.getRawParameterValue ("HIGH_CUT")->load());
    if (e.position.getDistanceFrom (low) < 13.0f)
        draggingLow = true;
    else if (e.position.getDistanceFrom (high) < 13.0f)
        draggingHigh = true;
}

void ReceiverFilterEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingLow)
        updateFromX (e.position.x, true);
    else if (draggingHigh)
        updateFromX (e.position.x, false);
}

void ReceiverFilterEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto b = getLocalBounds().toFloat().reduced (7.0f, 9.0f);
    if (b.contains (e.position))
    {
        if (e.position.x < b.getCentreX())
        {
            if (auto* p = processor.apvts.getParameter ("LOW_CUT"))
                p->setValueNotifyingHost (p->convertTo0to1 (20.0f));
        }
        else
        {
            if (auto* p = processor.apvts.getParameter ("HIGH_CUT"))
                p->setValueNotifyingHost (p->convertTo0to1 (20000.0f));
        }
        repaint();
    }
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), shapeEditor (p), filterEditor (p)
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    setLookAndFeel (&lookAndFeel);
    setSize (720, 410);

    addAndMakeVisible (shapeEditor);
    addAndMakeVisible (filterEditor);

    bypassButton.setName ("BYPASS");
    bypassButton.setButtonText ("");
    bypassButton.setClickingTogglesState (true);
    bypassButton.setLookAndFeel (&lookAndFeel);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", bypassButton);
    addAndMakeVisible (bypassButton);

    for (int i = 0; i < 3; ++i)
    {
        linkButtons[i].setName ("LINK_" + juce::String (i));
        linkButtons[i].setButtonText (homeSidechain::linkName (i));
        linkButtons[i].setClickingTogglesState (false);
        linkButtons[i].setLookAndFeel (&lookAndFeel);
        addAndMakeVisible (linkButtons[i]);
        linkButtons[i].onClick = [this, i] { selectLink (i); };
    }

    testButton.setName ("TEST");
    testButton.setButtonText ("TEST");
    testButton.setLookAndFeel (&lookAndFeel);
    testButton.onClick = [this] { requestTest(); };
    addAndMakeVisible (testButton);

    resetButton.setName ("RESET");
    resetButton.setButtonText ("RESET");
    resetButton.setLookAndFeel (&lookAndFeel);
    resetButton.onClick = [this] { resetShape(); };
    addAndMakeVisible (resetButton);

    mixKnob.setName ("MIX_KNOB");
    mixKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    mixKnob.setLookAndFeel (&lookAndFeel);
    mixKnob.setPopupDisplayEnabled (true, false, this);
    mixKnob.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, "MIX", mixKnob);
    addAndMakeVisible (mixKnob);

    depthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    depthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 19);
    depthSlider.setTextValueSuffix (" dB");
    depthSlider.setLookAndFeel (&lookAndFeel);
    depthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "DEPTH", depthSlider);
    addAndMakeVisible (depthSlider);

    syncButton.setName ("SYNC");
    syncButton.setButtonText ("SYNC");
    syncButton.setClickingTogglesState (true);
    syncButton.setLookAndFeel (&lookAndFeel);
    syncAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "SYNC", syncButton);
    addAndMakeVisible (syncButton);

    for (int i = 0; i < 4; ++i)
    {
        rateButtons[i].setName ("RATE_" + juce::String (i));
        rateButtons[i].setButtonText (juce::StringArray { "1/8", "1/4", "1/2", "1/1" }[i]);
        rateButtons[i].setRadioGroupId (401);
        rateButtons[i].setClickingTogglesState (false);
        rateButtons[i].setLookAndFeel (&lookAndFeel);
        addAndMakeVisible (rateButtons[i]);
        rateButtons[i].onClick = [this, i] { selectRate (i); };
    }

    const char* presetLabels[12] = { "", "", "", "", "", "", "", "", "", "", "", "" };
    for (int i = 0; i < 12; ++i)
    {
        presetButtons[i].setName ("PRESET_" + juce::String (i));
        presetButtons[i].setButtonText (presetLabels[i]);
        presetButtons[i].setLookAndFeel (&lookAndFeel);
        presetButtons[i].setClickingTogglesState (false);
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].onClick = [this, i] { selectPreset (i); };
    }

    selectLink (processor.getLink());
    selectRate (juce::jlimit (0, 3, static_cast<int> (processor.apvts.getRawParameterValue ("RATE")->load())));
    refreshLinkButtons();
    startTimerHz (24);
}

HomeSidechainReceiverAudioProcessorEditor::~HomeSidechainReceiverAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
    mixKnob.setLookAndFeel (nullptr);
    depthSlider.setLookAndFeel (nullptr);
    syncButton.setLookAndFeel (nullptr);
    bypassButton.setLookAndFeel (nullptr);
    testButton.setLookAndFeel (nullptr);
    resetButton.setLookAndFeel (nullptr);
    for (auto& b : linkButtons) b.setLookAndFeel (nullptr);
    for (auto& b : rateButtons) b.setLookAndFeel (nullptr);
    for (auto& b : presetButtons) b.setLookAndFeel (nullptr);
}

void HomeSidechainReceiverAudioProcessorEditor::selectLink (int index)
{
    const int clamped = juce::jlimit (0, 2, index);
    if (auto* p = processor.apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (clamped)));
    refreshLinkButtons();
}

void HomeSidechainReceiverAudioProcessorEditor::selectRate (int index)
{
    const int clamped = juce::jlimit (0, 3, index);
    if (auto* p = processor.apvts.getParameter ("RATE"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (clamped)));
    for (int i = 0; i < 4; ++i)
        rateButtons[i].setToggleState (i == clamped, juce::dontSendNotification);
}

void HomeSidechainReceiverAudioProcessorEditor::selectPreset (int index)
{
    const int i = juce::jlimit (0, 11, index);
    for (int n = 0; n < HomeSidechainReceiverAudioProcessor::maxNodes; ++n)
    {
        const bool active = n < 7;
        processor.setNodeActive (n, active);
        processor.setNodeX (n, defaultNodeX[n]);
        processor.setNodeY (n, presetY[i][n]);
    }
    for (int s = 0; s < HomeSidechainReceiverAudioProcessor::maxNodes - 1; ++s)
        processor.setHandle (s, 0.0f);
    if (i == 1 || i == 3 || i == 5 || i == 8)
    {
        for (int s = 0; s < 6; ++s)
            processor.setHandle (s, (s % 2 == 0 ? 0.35f : -0.15f));
    }
    for (int n = 0; n < 12; ++n)
        presetButtons[n].setToggleState (n == i, juce::dontSendNotification);
    shapeEditor.repaint();
}

void HomeSidechainReceiverAudioProcessorEditor::requestTest()
{
    processor.requestTestTrigger();
}

void HomeSidechainReceiverAudioProcessorEditor::resetShape()
{
    selectPreset (0);
}

void HomeSidechainReceiverAudioProcessorEditor::styleSlider (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 19);
    slider.setTextValueSuffix (suffix);
    slider.setLookAndFeel (&lookAndFeel);
}

void HomeSidechainReceiverAudioProcessorEditor::refreshLinkButtons()
{
    const int active = juce::jlimit (0, 2, processor.getLink());
    for (int i = 0; i < 3; ++i)
        linkButtons[i].setToggleState (i == active, juce::dontSendNotification);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);
    const auto frame = getLocalBounds().toFloat().reduced (5.0f);
    juce::ColourGradient bg (bg1, frame.getX(), frame.getY(), bg0, frame.getRight(), frame.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (frame, 12.0f);

    // Header deliberately mirrors the Trigger plugin: title on the left,
    // routing and bypass as compact utilities on the right.
    g.setFont (uiFont (20.0f, true));
    g.setColour (white);
    g.drawText ("HOME-", 20, 12, 78, 23, juce::Justification::left);
    g.setColour (cyan);
    g.drawText ("SIDECHAIN", 91, 12, 122, 23, juce::Justification::left);
    g.setColour (white);
    g.drawText ("RECEIVER", 211, 12, 98, 23, juce::Justification::left);

    g.setFont (uiFont (8.0f, true));
    g.setColour (muted);
    g.drawText ("D U B T A C H   D S P", 21, 36, 155, 10, juce::Justification::left);

    drawPanel (g, { 18.0f, 55.0f, 684.0f, 294.0f }, cyan, 12.0f);
    drawPanel (g, { 18.0f, 357.0f, 684.0f, 47.0f }, violet, 10.0f);

    // Graph header/status overlay.
    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool receiving = processor.envelopeActiveForUI.load (std::memory_order_relaxed);
    const auto stateColour = bypassed ? red : (receiving ? green : cyan);
    const auto state = bypassed ? "BYPASSED" : (receiving ? "RECEIVING" : "READY");
    auto status = juce::Rectangle<float> (31.0f, 67.0f, 92.0f, 22.0f);
    g.setColour (stateColour.withAlpha (0.10f));
    g.fillRoundedRectangle (status, 11.0f);
    g.setColour (stateColour.withAlpha (0.72f));
    g.drawRoundedRectangle (status, 11.0f, 1.0f);
    g.setColour (stateColour);
    g.fillEllipse (41.0f, 75.0f, 6.0f, 6.0f);
    g.setFont (uiFont (7.1f, true));
    g.drawText (state, 53, 72, 61, 13, juce::Justification::left);

    g.setFont (uiFont (7.0f, true));
    g.setColour (muted);
    g.drawText ("MIX", 55, 190, 100, 10, juce::Justification::centred);
    g.drawText ("DEPTH", 31, 319, 54, 9, juce::Justification::left);
    g.drawText ("FILTER", 31, 225, 54, 9, juce::Justification::left);
    g.drawText ("LFO RATE", 205, 319, 52, 9, juce::Justification::left);
    g.drawText ("CURVE SHAPES", 405, 319, 90, 9, juce::Justification::left);

    const double mix = processor.apvts.getRawParameterValue ("MIX")->load();
    g.setColour (white);
    g.setFont (uiFont (12.0f, true));
    g.drawText (juce::String (juce::roundToInt (mix * 100.0)) + "%", 55, 202, 100, 15,
                juce::Justification::centred);

    g.setColour (muted.withAlpha (0.78f));
    g.setFont (uiFont (6.6f, true));
    const auto filterTextY = 255;
    g.drawText (juce::String (juce::roundToInt (processor.apvts.getRawParameterValue ("LOW_CUT")->load())) + " Hz",
                31, filterTextY, 58, 9, juce::Justification::left);
    g.drawText (juce::String (juce::roundToInt (processor.apvts.getRawParameterValue ("HIGH_CUT")->load())) + " Hz",
                115, filterTextY, 65, 9, juce::Justification::right);

    // Rate row and preset thumbnails are rendered here; the children only
    // provide the hit targets and state.
    for (int i = 0; i < 12; ++i)
    {
        const auto r = presetButtons[i].getBounds().toFloat().reduced (7.0f);
        const auto& shape = presetY[i];
        juce::Path thumb;
        thumb.startNewSubPath (r.getX(), r.getBottom());
        for (int p = 0; p < HomeSidechainReceiverAudioProcessor::maxNodes; ++p)
        {
            const float x = r.getX() + r.getWidth() * defaultNodeX[p];
            const float y = r.getBottom() - r.getHeight() * shape[p];
            if (p == 0) thumb.startNewSubPath (x, y); else thumb.lineTo (x, y);
        }
        g.setColour (i == 0 && presetButtons[i].getToggleState() ? cyan : white.withAlpha (0.46f));
        g.strokePath (thumb, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    shapeEditor.setBounds (198, 88, 484, 222);
    mixKnob.setBounds (32, 72, 148, 116);
    filterEditor.setBounds (29, 232, 151, 47);
    depthSlider.setBounds (30, 325, 150, 21);

    for (int i = 0; i < 3; ++i)
        linkButtons[i].setBounds (528 + i * 30, 15, 29, 25);
    bypassButton.setBounds (620, 11, 37, 33);

    syncButton.setBounds (198, 328, 46, 22);
    for (int i = 0; i < 4; ++i)
        rateButtons[i].setBounds (248 + i * 38, 328, 34, 22);

    testButton.setBounds (548, 66, 48, 22);
    resetButton.setBounds (600, 66, 48, 22);

    const int x0 = 404;
    const int y0 = 328;
    const int w = 44;
    const int h = 22;
    const int gap = 4;
    for (int i = 0; i < 12; ++i)
    {
        const int row = i / 6;
        const int col = i % 6;
        presetButtons[i].setBounds (x0 + col * (w + gap), y0 + row * (h + gap), w, h);
    }
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    shapeEditor.repaint();
    filterEditor.repaint();
    refreshLinkButtons();
    repaint ();
}
