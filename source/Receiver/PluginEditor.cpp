#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>

namespace
{
    const auto bg = juce::Colour (0xff080a0d);
    const auto panel = juce::Colour (0xff10151a);
    const auto plotBg = juce::Colour (0xff070b0f);
    const auto cyan = juce::Colour (0xff00e5ff);
    const auto cyanSoft = juce::Colour (0xff69f3ff);
    const auto violet = juce::Colour (0xff9b6cff);
    const auto green = juce::Colour (0xff00ff87);
    const auto yellow = juce::Colour (0xffffe44d);
    const auto red = juce::Colour (0xffff536b);
    const auto white = juce::Colour (0xfff6f8fa);
    const auto muted = juce::Colour (0xff93a0aa);
    const auto grid = juce::Colour (0xff2a3540);

    const std::array<std::array<float, 5>, 16> presetShapes = {{
        {{1.0f, 0.18f, 0.08f, 0.26f, 1.0f}},
        {{1.0f, 0.05f, 0.02f, 0.08f, 1.0f}},
        {{1.0f, 0.42f, 0.12f, 0.18f, 1.0f}},
        {{1.0f, 0.64f, 0.20f, 0.08f, 1.0f}},
        {{1.0f, 0.24f, 0.18f, 0.48f, 1.0f}},
        {{1.0f, 0.72f, 0.02f, 0.06f, 1.0f}},
        {{1.0f, 0.52f, 0.48f, 0.06f, 1.0f}},
        {{1.0f, 0.14f, 0.34f, 0.12f, 1.0f}},
        {{1.0f, 0.86f, 0.18f, 0.06f, 1.0f}},
        {{1.0f, 0.10f, 0.78f, 0.12f, 1.0f}},
        {{1.0f, 0.34f, 0.78f, 0.22f, 1.0f}},
        {{1.0f, 0.08f, 0.08f, 0.76f, 1.0f}},
        {{1.0f, 0.36f, 0.06f, 0.82f, 1.0f}},
        {{1.0f, 0.58f, 0.38f, 0.88f, 1.0f}},
        {{1.0f, 0.06f, 0.48f, 0.52f, 1.0f}},
        {{1.0f, 0.50f, 0.04f, 0.50f, 1.0f}}
    }};

    const char* presetNames[numPresets] = {
        "1", "2", "3", "4", "5", "6", "7", "8",
        "9", "10", "11", "12", "13", "14", "15", "16"
    };

    const char* rateNames[4] = { "1/8", "1/4", "1/2", "1/1" };

    class ReceiverValueLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                   bool highlighted, bool down) override
        {
            const auto b = button.getLocalBounds().toFloat();
            const auto name = button.getName();

            if (name.startsWith ("LINK_"))
            {
                const bool active = button.getToggleState();
                g.setColour (active ? cyan.withAlpha (0.08f) : white.withAlpha (0.015f));
                g.fillRoundedRectangle (b, 8.0f);
                g.setColour (active ? cyan.withAlpha (0.65f) : white.withAlpha (0.14f));
                g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);
                if (active)
                    g.fillRoundedRectangle (b.getX() + 7.0f, b.getBottom() - 3.0f, b.getWidth() - 14.0f, 2.0f, 1.0f);
                return;
            }

            if (name.startsWith ("RATE_"))
            {
                const bool active = button.getToggleState();
                g.setColour (active ? cyan.withAlpha (0.10f) : white.withAlpha (0.025f));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (active ? cyan.withAlpha (0.8f) : white.withAlpha (0.16f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                return;
            }

            if (name.startsWith ("PRESET_"))
            {
                const bool active = button.getToggleState();
                g.setColour (active ? cyan.withAlpha (0.15f) : white.withAlpha (0.025f));
                g.fillRoundedRectangle (b, 5.0f);
                g.setColour (active ? cyan.withAlpha (0.80f) : white.withAlpha (0.12f));
                g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
                if (active)
                    g.fillRoundedRectangle (b.getX() + 5.0f, b.getBottom() - 2.0f, b.getWidth() - 10.0f, 1.8f, 1.0f);
                return;
            }

            if (name == "BAND_FULL" || name == "BAND_LOW" || name == "BAND_HIGH")
            {
                const bool active = button.getToggleState();
                g.setColour (active ? violet.withAlpha (0.12f) : white.withAlpha (0.018f));
                g.fillRoundedRectangle (b, 6.0f);
                g.setColour (active ? violet.withAlpha (0.8f) : white.withAlpha (0.13f));
                g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
                return;
            }

            if (name == "SYNC")
            {
                const bool active = button.getToggleState();
                g.setColour (active ? green.withAlpha (0.10f) : white.withAlpha (0.025f));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (active ? green.withAlpha (0.75f) : white.withAlpha (0.16f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                return;
            }

            if (name == "BYPASS")
            {
                const bool active = button.getToggleState();
                const auto c = active ? red : cyan;
                g.setColour (c.withAlpha (active ? 0.14f : 0.07f));
                g.fillRoundedRectangle (b, 9.0f);
                g.setColour (c.withAlpha (highlighted ? 0.95f : 0.62f));
                g.drawRoundedRectangle (b.reduced (0.5f), 9.0f, 1.2f);
                return;
            }

            if (name == "TEST" || name == "RESET")
            {
                const auto c = name == "TEST" ? cyan : white;
                g.setColour (down ? c.withAlpha (0.20f) : (highlighted ? c.withAlpha (0.09f) : white.withAlpha (0.02f)));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (c.withAlpha (highlighted ? 0.9f : 0.30f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                return;
            }

            g.setColour (white.withAlpha (0.025f));
            g.fillRoundedRectangle (b, 6.0f);
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            const auto name = button.getName();
            if (name == "BYPASS") return;

            const bool active = button.getToggleState();
            const bool link = name.startsWith ("LINK_");
            const bool rate = name.startsWith ("RATE_");
            const bool preset = name.startsWith ("PRESET_");
            const bool band = name.startsWith ("BAND_");
            const bool sync = name == "SYNC";
            const float size = preset ? 7.3f : (link ? 10.0f : 8.0f);
            g.setFont (juce::FontOptions (size).withName ("Helvetica").withStyle (active ? "Bold" : "Plain"));
            g.setColour (active ? (link || rate ? cyan : band ? violet : sync ? green : white)
                                : white.withAlpha ((link || rate) ? 0.58f : 0.60f));
            g.drawText (button.getButtonText(), button.getLocalBounds().toFloat(), juce::Justification::centred);
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle, juce::Slider&) override
        {
            const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
            const float left = static_cast<float> (x) + 5.0f;
            const float right = static_cast<float> (x + width) - 5.0f;
            const float p = juce::jlimit (minSliderPos, maxSliderPos, sliderPos);
            g.setColour (grid.withAlpha (0.65f));
            g.fillRoundedRectangle (left, cy - 2.0f, right - left, 4.0f, 2.0f);
            g.setColour (cyan.withAlpha (0.48f));
            g.fillRoundedRectangle (left, cy - 2.0f, juce::jmax (0.0f, p - left), 4.0f, 2.0f);
            g.setColour (cyan.withAlpha (0.14f));
            g.fillEllipse (p - 10.0f, cy - 10.0f, 20.0f, 20.0f);
            g.setColour (white);
            g.fillEllipse (p - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        }
    };

    static ReceiverValueLookAndFeel buttonLnf;
}

HomeSidechainReceiverAudioProcessorEditor::ReceiverLookAndFeel::ReceiverLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, cyan);
    setColour (juce::Slider::textBoxTextColourId, white);
    setColour (juce::Slider::textBoxBackgroundColourId, plotBg);
    setColour (juce::Slider::textBoxOutlineColourId, grid);
}

void HomeSidechainReceiverAudioProcessorEditor::ReceiverLookAndFeel::drawRotarySlider
    (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
     float startAngle, float endAngle, juce::Slider& slider)
{
    const float radius = static_cast<float> (juce::jmin (w, h)) * 0.38f;
    const float cx = static_cast<float> (x) + static_cast<float> (w) * 0.5f;
    const float cy = static_cast<float> (y) + static_cast<float> (h) * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour (juce::Colour (0xff0d1116));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (white.withAlpha (0.08f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    juce::Path bgArc;
    bgArc.addCentredArc (cx, cy, radius + 3.0f, radius + 3.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (grid);
    g.strokePath (bgArc, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    juce::Path fillArc;
    fillArc.addCentredArc (cx, cy, radius + 3.0f, radius + 3.0f, 0.0f, startAngle, angle, true);
    const auto c = slider.getName() == "MIX_KNOB" ? cyan : violet;
    g.setColour (c.withAlpha (0.28f));
    g.strokePath (fillArc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (fillArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    const float px = cx + std::sin (angle) * (radius - 5.0f);
    const float py = cy - std::cos (angle) * (radius - 5.0f);
    g.setColour (white);
    g.fillEllipse (px - 3.0f, py - 3.0f, 6.0f, 6.0f);
}

void HomeSidechainReceiverAudioProcessorEditor::ReceiverLookAndFeel::drawLinearSlider
    (juce::Graphics& g, int x, int y, int w, int h, float p, float minP, float maxP,
     juce::Slider::SliderStyle style, juce::Slider& slider)
{
    ReceiverValueLookAndFeel lnf;
    lnf.drawLinearSlider (g, x, y, w, h, p, minP, maxP, style, slider);
}

void HomeSidechainReceiverAudioProcessorEditor::ReceiverLookAndFeel::drawButtonBackground
    (juce::Graphics& g, juce::Button& b, const juce::Colour& c, bool hi, bool down)
{
    buttonLnf.drawButtonBackground (g, b, c, hi, down);
}

void HomeSidechainReceiverAudioProcessorEditor::ReceiverLookAndFeel::drawButtonText
    (juce::Graphics& g, juce::TextButton& b, bool hi, bool down)
{
    buttonLnf.drawButtonText (g, b, hi, down);
}

juce::Rectangle<float> ReceiverShaperGraph::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (22.0f, 18.0f);
}

juce::Point<float> ReceiverShaperGraph::pointForIndex (int index) const
{
    const auto b = plotBounds();
    const float x = b.getX() + b.getWidth() * static_cast<float> (index) / 4.0f;
    const float v = juce::jlimit (0.0f, 1.0f, processor.getShapePoint (index));
    return { x, b.getBottom() - b.getHeight() * v };
}

int ReceiverShaperGraph::nearestPoint (juce::Point<float> p) const
{
    int best = -1;
    float bestDistance = 15.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float d = pointForIndex (i).getDistanceFrom (p);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

void ReceiverShaperGraph::resetShape()
{
    const auto& values = presetShapes[0];
    for (int i = 0; i < 5; ++i)
        processor.setShapePoint (i, values[static_cast<size_t> (i)]);
    repaint();
}

void ReceiverShaperGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (plotBg);
    g.fillRoundedRectangle (bounds, 11.0f);

    const auto plot = plotBounds();

    for (int i = 0; i <= 16; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 16.0f;
        g.setColour (grid.withAlpha (i % 4 == 0 ? 0.52f : 0.20f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 8; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 8.0f;
        g.setColour (grid.withAlpha (i % 2 == 0 ? 0.42f : 0.16f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (muted.withAlpha (0.75f));
    g.drawText ("0 dB", 7, plot.getY() - 2, 34, 10, juce::Justification::left);
    g.drawText ("-6", 7, plot.getY() + plot.getHeight() * 0.25f - 5, 28, 10, juce::Justification::left);
    g.drawText ("-12", 7, plot.getY() + plot.getHeight() * 0.50f - 5, 28, 10, juce::Justification::left);
    g.drawText ("-24", 7, plot.getY() + plot.getHeight() * 0.75f - 5, 28, 10, juce::Justification::left);
    g.drawText ("DUCK", plot.getRight() - 34, plot.getY() - 2, 34, 10, juce::Justification::right);

    const double totalMs = processor.cycleSamples() * 1000.0 / juce::jmax (1.0, processor.getSampleRate());
    const double attackMs = processor.apvts.getRawParameterValue ("ATTACK")->load();
    const double holdMs = processor.apvts.getRawParameterValue ("HOLD")->load();
    const double shownTotal = juce::jmax (1.0, totalMs);
    const float xAttack = plot.getX() + plot.getWidth() * static_cast<float> (juce::jlimit (0.0, 1.0, attackMs / shownTotal));
    const float xHold = plot.getX() + plot.getWidth() * static_cast<float> (juce::jlimit (0.0, 1.0, (attackMs + holdMs) / shownTotal));

    g.setColour (violet.withAlpha (0.18f));
    g.drawLine (xAttack, plot.getY(), xAttack, plot.getBottom(), 1.0f);
    g.drawLine (xHold, plot.getY(), xHold, plot.getBottom(), 1.0f);

    juce::Path curve;
    curve.startNewSubPath (pointForIndex (0));
    for (int i = 1; i < 5; ++i)
    {
        const auto a = curve.getCurrentPosition();
        const auto p = pointForIndex (i);
        const float midX = (a.x + p.x) * 0.5f;
        curve.cubicTo (midX, a.y, midX, p.y, p.x, p.y);
    }

    juce::Path fill = curve;
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();

    g.setColour (cyan.withAlpha (0.08f));
    g.fillPath (fill);
    g.setColour (violet.withAlpha (0.12f));
    g.strokePath (curve, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (cyanSoft);
    g.strokePath (curve, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (processor.envelopeActiveForUI.load (std::memory_order_relaxed))
    {
        const float phase = juce::jlimit<float> (0.0f, 1.0f,
            processor.envelopeDisplayPhase.load (std::memory_order_relaxed));
        const float px = plot.getX() + plot.getWidth() * phase;
        const float py = plot.getY() + plot.getHeight() *
            (1.0f - juce::jlimit (0.0f, 1.0f, processor.shapeValue (phase)));
        g.setColour (green.withAlpha (0.65f));
        g.drawLine (px, plot.getY(), px, plot.getBottom(), 1.5f);
        g.setColour (green);
        g.fillEllipse (px - 4.0f, py - 4.0f, 8.0f, 8.0f);
    }

    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        const bool active = i == hoveredPoint || i == draggedPoint;
        const float r = active ? 6.0f : 4.5f;
        g.setColour (cyan.withAlpha (active ? 0.24f : 0.10f));
        g.fillEllipse (p.x - r - 3.0f, p.y - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f);
        g.setColour (active ? white : cyan);
        g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
    }
}

void ReceiverShaperGraph::mouseMove (const juce::MouseEvent& e)
{
    hoveredPoint = nearestPoint (e.position);
    setMouseCursor (hoveredPoint >= 0 ? juce::MouseCursor::PointingHandCursor
                                      : juce::MouseCursor::NormalCursor);
    repaint();
}

void ReceiverShaperGraph::mouseExit (const juce::MouseEvent&)
{
    hoveredPoint = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
}

void ReceiverShaperGraph::mouseDown (const juce::MouseEvent& e)
{
    draggedPoint = nearestPoint (e.position);
    if (draggedPoint >= 0)
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void ReceiverShaperGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedPoint < 0)
        return;
    const auto b = plotBounds();
    const float yNorm = (e.position.y - b.getY()) / juce::jmax (1.0f, b.getHeight());
    processor.setShapePoint (draggedPoint, 1.0f - juce::jlimit (0.0f, 1.0f, yNorm));
    repaint();
}

void ReceiverShaperGraph::mouseUp (const juce::MouseEvent&)
{
    draggedPoint = -1;
    setMouseCursor (hoveredPoint >= 0 ? juce::MouseCursor::PointingHandCursor
                                      : juce::MouseCursor::NormalCursor);
}

void ReceiverShaperGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int p = nearestPoint (e.position);
    if (p >= 0)
        processor.setShapePoint (p, presetShapes[0][static_cast<size_t> (p)]);
    repaint();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor
    (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), graph (p)
{
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    setSize (720, 420);
    setResizable (false, false);
    setLookAndFeel (&lookAndFeel);

    bypassButton.setName ("BYPASS");
    bypassButton.setClickingTogglesState (true);
    bypassButton.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", bypassButton);

    for (int i = 0; i < 3; ++i)
    {
        linkButtons[i].setButtonText (juce::String::charToString (static_cast<juce::juce_wchar> ('A' + i)));
        linkButtons[i].setName ("LINK_" + juce::String (i));
        linkButtons[i].setLookAndFeel (&lookAndFeel);
        addAndMakeVisible (linkButtons[i]);
        linkButtons[i].onClick = [this, i] { selectLink (i); };
    }

    testButton.setName ("TEST");
    testButton.setButtonText ("TEST");
    testButton.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (testButton);
    testButton.onClick = [this] { requestTest(); };

    resetButton.setName ("RESET");
    resetButton.setButtonText ("RESET");
    resetButton.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (resetButton);
    resetButton.onClick = [this] { resetShape(); };

    for (int i = 0; i < 3; ++i)
    {
        auto* button = i == 0 ? &fullBandButton : (i == 1 ? &lowBandButton : &highBandButton);
        const auto name = i == 0 ? "BAND_FULL" : (i == 1 ? "BAND_LOW" : "BAND_HIGH");
        button->setName (name);
        button->setButtonText (i == 0 ? "FULL" : (i == 1 ? "LOW" : "HIGH"));
        button->setRadioGroupId (220);
        button->setClickingTogglesState (false);
        button->setLookAndFeel (&lookAndFeel);
        addAndMakeVisible (*button);
        button->onClick = [this, i] { selectBand (i); };
    }

    syncButton.setName ("SYNC");
    syncButton.setButtonText ("SYNC");
    syncButton.setClickingTogglesState (true);
    syncButton.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "SYNC", syncButton);

    for (int i = 0; i < 4; ++i)
    {
        rateButtons[i].setName ("RATE_" + juce::String (i));
        rateButtons[i].setButtonText (rateNames[i]);
        rateButtons[i].setRadioGroupId (221);
        rateButtons[i].setLookAndFeel (&lookAndFeel);
        addAndMakeVisible (rateButtons[i]);
        rateButtons[i].onClick = [this, i] { selectRate (i); };
    }

    for (int i = 0; i < numPresets; ++i)
    {
        presetButtons[i].setName ("PRESET_" + juce::String (i));
        presetButtons[i].setButtonText (presetNames[i]);
        presetButtons[i].setLookAndFeel (&lookAndFeel);
        presetButtons[i].setClickingTogglesState (false);
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].onClick = [this, i] { selectPreset (i); };
    }

    addAndMakeVisible (graph);

    mixKnob.setName ("MIX_KNOB");
    mixKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    mixKnob.setPopupDisplayEnabled (true, false, this);
    addAndMakeVisible (mixKnob);
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, "MIX", mixKnob);

    styleSlider (bandSlider, " Hz");
    bandSlider.setRange (50.0, 800.0, 1.0);
    bandSlider.setValue (150.0);
    bandAttachment = std::make_unique<SliderAttachment> (processor.apvts, "CROSSOVER", bandSlider);

    styleSlider (depthSlider, " dB");
    depthAttachment = std::make_unique<SliderAttachment> (processor.apvts, "DEPTH", depthSlider);

    selectLink (processor.getLink());
    selectBand (juce::roundToInt (processor.apvts.getRawParameterValue ("BAND")->load()));
    selectRate (juce::roundToInt (processor.apvts.getRawParameterValue ("RATE")->load()));
    selectPreset (0);
    startTimerHz (24);
}

HomeSidechainReceiverAudioProcessorEditor::~HomeSidechainReceiverAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
    mixKnob.setLookAndFeel (nullptr);
    bandSlider.setLookAndFeel (nullptr);
    depthSlider.setLookAndFeel (nullptr);
    syncButton.setLookAndFeel (nullptr);
    bypassButton.setLookAndFeel (nullptr);
    testButton.setLookAndFeel (nullptr);
    resetButton.setLookAndFeel (nullptr);
    for (auto& b : linkButtons) b.setLookAndFeel (nullptr);
    for (auto& b : rateButtons) b.setLookAndFeel (nullptr);
    for (auto& b : presetButtons) b.setLookAndFeel (nullptr);
    fullBandButton.setLookAndFeel (nullptr);
    lowBandButton.setLookAndFeel (nullptr);
    highBandButton.setLookAndFeel (nullptr);
}

void HomeSidechainReceiverAudioProcessorEditor::styleSlider (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 19);
    slider.setColour (juce::Slider::textBoxTextColourId, white);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, plotBg);
    slider.setColour (juce::Slider::textBoxOutlineColourId, grid);
    slider.setTextValueSuffix (suffix);
    slider.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (slider);
}

void HomeSidechainReceiverAudioProcessorEditor::selectLink (int index)
{
    const int clamped = juce::jlimit (0, 2, index);
    if (auto* p = processor.apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (clamped)));
    for (int i = 0; i < 3; ++i)
        linkButtons[i].setToggleState (i == clamped, juce::dontSendNotification);
    refreshButtonStates();
}

void HomeSidechainReceiverAudioProcessorEditor::selectBand (int index)
{
    const int clamped = juce::jlimit (0, 2, index);
    if (auto* p = processor.apvts.getParameter ("BAND"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (clamped)));
    fullBandButton.setToggleState (clamped == 0, juce::dontSendNotification);
    lowBandButton.setToggleState (clamped == 1, juce::dontSendNotification);
    highBandButton.setToggleState (clamped == 2, juce::dontSendNotification);
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
    const int clamped = juce::jlimit (0, numPresets - 1, index);
    for (int i = 0; i < 5; ++i)
        processor.setShapePoint (i, presetShapes[static_cast<size_t> (clamped)][static_cast<size_t> (i)]);
    for (int i = 0; i < numPresets; ++i)
        presetButtons[i].setToggleState (i == clamped, juce::dontSendNotification);
    graph.repaint();
}

void HomeSidechainReceiverAudioProcessorEditor::requestTest()
{
    processor.requestTestTrigger();
}

void HomeSidechainReceiverAudioProcessorEditor::resetShape()
{
    selectPreset (0);
}

void HomeSidechainReceiverAudioProcessorEditor::refreshButtonStates()
{
    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    bypassButton.setTooltip (bypassed ? "Bypass receiver" : "Enable receiver");
}

void HomeSidechainReceiverAudioProcessorEditor::drawRotaryLabel (juce::Graphics& g, const juce::String& text,
                                                                  juce::Rectangle<float> area, float size) const
{
    g.setColour (muted);
    g.setFont (juce::FontOptions (size).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (text, area, juce::Justification::centred);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    const auto body = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (juce::Colour (0xff0d1115));
    g.fillRoundedRectangle (body, 14.0f);
    g.setColour (white.withAlpha (0.08f));
    g.drawRoundedRectangle (body, 14.0f, 1.0f);

    g.setColour (white);
    g.setFont (juce::FontOptions (20.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("Home-", 22, 14, 80, 24, juce::Justification::left);
    g.setColour (cyan);
    g.drawText ("SIDECHAIN", 93, 14, 122, 24, juce::Justification::left);
    g.setColour (white);
    g.drawText ("Receiver", 213, 14, 85, 24, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("D U B T A C H   D S P", 23, 37, 150, 11, juce::Justification::left);

    g.setColour (muted.withAlpha (0.55f));
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("SHAPER", 24, 58, 50, 9, juce::Justification::left);
    g.drawText ("16 CURVES", 518, 58, 60, 9, juce::Justification::right);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool receiving = processor.envelopeActiveForUI.load (std::memory_order_relaxed);
    const auto stateColour = bypassed ? red : (receiving ? green : cyan);
    const auto state = bypassed ? "BYPASSED" : (receiving ? "RECEIVING" : "READY");
    g.setColour (stateColour.withAlpha (0.10f));
    g.fillRoundedRectangle (318.0f, 14.0f, 78.0f, 21.0f, 10.0f);
    g.setColour (stateColour);
    g.fillEllipse (326.0f, 21.0f, 6.0f, 6.0f);
    g.setFont (juce::FontOptions (7.3f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (state, 338, 17, 51, 14, juce::Justification::left);

    g.setColour (muted.withAlpha (0.55f));
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("LINK", 480, 18, 28, 9, juce::Justification::left);

    g.setColour (muted.withAlpha (0.55f));
    g.drawText ("MIX", 82, 188, 100, 10, juce::Justification::centred);
    g.drawText ("BAND", 26, 270, 44, 10, juce::Justification::left);
    g.drawText ("CROSSOVER", 26, 326, 68, 9, juce::Justification::left);
    g.drawText ("DEPTH", 26, 365, 46, 9, juce::Justification::left);

    const double mix = processor.apvts.getRawParameterValue ("MIX")->load();
    g.setColour (white);
    g.setFont (juce::FontOptions (13.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (juce::String (juce::roundToInt (mix * 100.0)) + "%", 76, 202, 112, 18, juce::Justification::centred);

    g.setColour (muted.withAlpha (0.48f));
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("SYNC", 430, 322, 38, 9, juce::Justification::left);

    g.setColour (white.withAlpha (0.60f));
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("DUCKING CURVE", 290, 60, 130, 9, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    linkButtons[0].setBounds (511, 14, 34, 26);
    linkButtons[1].setBounds (547, 14, 34, 26);
    linkButtons[2].setBounds (583, 14, 34, 26);
    bypassButton.setBounds (624, 14, 40, 26);

    graph.setBounds (194, 72, 470, 230);

    mixKnob.setBounds (26, 82, 170, 120);

    fullBandButton.setBounds (24, 282, 50, 24);
    lowBandButton.setBounds (77, 282, 50, 24);
    highBandButton.setBounds (130, 282, 50, 24);

    bandSlider.setBounds (24, 340, 172, 22);
    depthSlider.setBounds (24, 381, 172, 22);

    testButton.setBounds (504, 40, 52, 22);
    resetButton.setBounds (560, 40, 52, 22);

    syncButton.setBounds (466, 316, 50, 24);
    for (int i = 0; i < 4; ++i)
        rateButtons[i].setBounds (520 + i * 36, 316, 32, 24);

    const int left = 194;
    const int top = 344;
    const int gap = 4;
    const int w = 54;
    const int h = 25;
    for (int i = 0; i < numPresets; ++i)
    {
        const int row = i / 8;
        const int col = i % 8;
        presetButtons[i].setBounds (left + col * (w + gap), top + row * (h + gap), w, h);
    }
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
