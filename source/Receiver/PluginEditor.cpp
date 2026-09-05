#include "PluginEditor.h"

namespace
{
    const juce::Colour background  (0xff06080b);
    const juce::Colour chassis     (0xff11151b);
    const juce::Colour scopeBg     (0xff080c11);
    const juce::Colour cyan        (0xff00e5ff);
    const juce::Colour cyanSoft    (0xff39d8f7);
    const juce::Colour violet      (0xffa970ff);
    const juce::Colour green       (0xff00ff87);
    const juce::Colour red         (0xffff5a70);
    const juce::Colour white       (0xfff4f6f8);
    const juce::Colour muted       (0xff77818e);
    const juce::Colour grid        (0xff202631);

    class ReceiverLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            juce::ignoreUnused (style, slider);
            const float left = static_cast<float> (x) + 5.0f;
            const float right = static_cast<float> (x + width) - 5.0f;
            const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
            const float p = juce::jlimit (minSliderPos, maxSliderPos, sliderPos);

            g.setColour (grid);
            g.fillRoundedRectangle (left, cy - 2.0f, right - left, 4.0f, 2.0f);

            const float fillW = juce::jmax (0.0f, p - left);
            g.setColour (cyan.withAlpha (0.28f));
            g.fillRoundedRectangle (left, cy - 2.0f, fillW, 4.0f, 2.0f);

            g.setColour (cyan.withAlpha (0.14f));
            g.fillEllipse (p - 10.0f, cy - 10.0f, 20.0f, 20.0f);
            g.setColour (white);
            g.fillEllipse (p - 4.5f, cy - 4.5f, 9.0f, 9.0f);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                   bool highlighted, bool down) override
        {
            const auto b = button.getLocalBounds().toFloat();

            if (button.getName() == "BYPASS")
            {
                const bool active = button.getToggleState();
                const auto c = active ? red : cyan;
                g.setColour (c.withAlpha (active ? 0.13f : 0.08f));
                g.fillRoundedRectangle (b, 9.0f);
                g.setColour (c.withAlpha (highlighted ? 0.95f : 0.55f));
                g.drawRoundedRectangle (b.reduced (0.5f), 9.0f, 1.0f);

                const float cx = b.getCentreX();
                const float cy = b.getCentreY();
                juce::Path p;
                p.addCentredArc (cx, cy + 1.5f, 6.2f, 6.2f, 0.0f, 0.45f,
                                 juce::MathConstants<float>::twoPi - 0.45f, true);
                g.setColour (c);
                g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
                g.drawLine (cx, cy - 7.0f, cx, cy + 0.2f, 1.8f);
                return;
            }

            if (button.getName() == "TEST")
            {
                const auto c = cyan;
                g.setColour (down ? c.withAlpha (0.22f) : (highlighted ? c.withAlpha (0.12f) : white.withAlpha (0.025f)));
                g.fillRoundedRectangle (b, 8.0f);
                g.setColour (c.withAlpha (highlighted ? 0.95f : 0.48f));
                g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);
                return;
            }

            if (button.getName() == "RESET")
            {
                g.setColour (highlighted ? white.withAlpha (0.08f) : white.withAlpha (0.025f));
                g.fillRoundedRectangle (b, 8.0f);
                g.setColour (white.withAlpha (0.25f));
                g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);
                return;
            }

            if (button.getName().startsWith ("LINK_"))
            {
                const bool active = button.getToggleState();
                if (active)
                {
                    g.setColour (cyan.withAlpha (0.055f));
                    g.fillRoundedRectangle (b, 8.0f);
                    g.setColour (cyan.withAlpha (0.75f));
                    g.drawRoundedRectangle (b.reduced (0.6f), 8.0f, 1.0f);
                    g.setColour (cyan);
                    g.fillRoundedRectangle (b.getX() + 7.0f, b.getBottom() - 3.0f,
                                            b.getWidth() - 14.0f, 2.0f, 1.0f);
                }
                else
                {
                    g.setColour (white.withAlpha (0.06f));
                    g.drawRoundedRectangle (b.reduced (0.6f), 8.0f, 1.0f);
                }
                return;
            }
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            if (button.getName() == "BYPASS")
                return;

            const auto b = button.getLocalBounds().toFloat();
            const bool link = button.getName().startsWith ("LINK_");
            const float size = link ? 10.5f : 8.2f;
            g.setFont (juce::FontOptions (size).withName ("Helvetica").withStyle ("Bold"));
            g.setColour (link && button.getToggleState() ? cyan : white.withAlpha (link ? 0.64f : 0.84f));
            g.drawText (button.getButtonText(), b, juce::Justification::centred);
        }
    };

    static ReceiverLookAndFeel receiverLookAndFeel;
}

juce::Rectangle<float> ReceiverShaperGraph::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 10.0f);
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
    float bestDistance = 18.0f;
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
    const float values[5] = { 1.0f, 0.18f, 0.10f, 0.34f, 1.0f };
    for (int i = 0; i < 5; ++i)
        processor.setShapePoint (i, values[i]);
    repaint();
}

void ReceiverShaperGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (scopeBg);
    g.fillRoundedRectangle (bounds, 12.0f);

    const auto plot = plotBounds();

    // Shaper-style grid: dense vertically, lighter horizontally.
    for (int i = 0; i <= 16; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 16.0f;
        g.setColour (grid.withAlpha (i % 4 == 0 ? 0.80f : 0.36f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 8; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 8.0f;
        g.setColour (grid.withAlpha (i % 2 == 0 ? 0.65f : 0.30f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (muted.withAlpha (0.72f));
    g.drawText ("0 dB", 7, plot.getY() - 1, 34, 10, juce::Justification::left);
    g.drawText ("-6", 7, plot.getY() + plot.getHeight() * 0.25f - 5, 28, 10, juce::Justification::left);
    g.drawText ("-12", 7, plot.getY() + plot.getHeight() * 0.50f - 5, 28, 10, juce::Justification::left);
    g.drawText ("-24", 7, plot.getY() + plot.getHeight() * 0.75f - 5, 28, 10, juce::Justification::left);

    const double attackMs = processor.apvts.getRawParameterValue ("ATTACK")->load();
    const double holdMs = processor.apvts.getRawParameterValue ("HOLD")->load();
    const double releaseMs = processor.apvts.getRawParameterValue ("RELEASE")->load();
    const double totalMs = juce::jmax<double> (1.0, attackMs + holdMs + releaseMs);

    const float xAttack = plot.getX() + plot.getWidth() * static_cast<float> (attackMs / totalMs);
    const float xHold = plot.getX() + plot.getWidth() * static_cast<float> ((attackMs + holdMs) / totalMs);

    g.setColour (violet.withAlpha (0.22f));
    g.drawLine (xAttack, plot.getY(), xAttack, plot.getBottom(), 1.0f);
    g.drawLine (xHold, plot.getY(), xHold, plot.getBottom(), 1.0f);

    g.setFont (juce::FontOptions (6.8f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (muted.withAlpha (0.60f));
    g.drawText ("ATTACK", plot.getX() + 6, plot.getBottom() - 12, 44, 9, juce::Justification::left);
    g.drawText ("HOLD", xHold + 4, plot.getBottom() - 12, 28, 9, juce::Justification::left);
    g.drawText ("RELEASE", plot.getRight() - 48, plot.getBottom() - 12, 44, 9, juce::Justification::right);

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

    g.setColour (cyan.withAlpha (0.045f));
    g.fillPath (fill);
    g.setColour (violet.withAlpha (0.11f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (cyanSoft);
    g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Running-envelope playhead.
    const float phase = juce::jlimit<float> (0.0f, 1.0f,
        processor.envelopeDisplayPhase.load (std::memory_order_relaxed));
    if (processor.envelopeActiveForUI.load (std::memory_order_relaxed))
    {
        const float px = plot.getX() + plot.getWidth() * phase;
        const float y = plot.getY() + plot.getHeight() *
            (1.0f - juce::jlimit (0.0f, 1.0f, processor.shapeValue (phase)));
        g.setColour (green.withAlpha (0.68f));
        g.drawLine (px, plot.getY(), px, plot.getBottom(), 1.3f);
        g.setColour (green);
        g.fillEllipse (px - 3.5f, y - 3.5f, 7.0f, 7.0f);
    }

    // Editable points.
    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        const bool active = (i == hoveredPoint || i == draggedPoint);
        const float r = active ? 6.5f : 5.0f;
        g.setColour (cyan.withAlpha (active ? 0.20f : 0.10f));
        g.fillEllipse (p.x - r - 3.5f, p.y - r - 3.5f, (r + 3.5f) * 2.0f, (r + 3.5f) * 2.0f);
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
    const float yNorm = (e.position.y - b.getY()) / juce::jmax<float> (1.0f, b.getHeight());
    const float value = 1.0f - juce::jlimit<float> (0.0f, 1.0f, yNorm);
    processor.setShapePoint (draggedPoint, value);
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
    if (nearestPoint (e.position) >= 0)
        resetShape();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), graph (p)
{
    setSize (620, 360);
    setResizable (false, false);
    setLookAndFeel (&receiverLookAndFeel);

    bypassButton.setName ("BYPASS");
    bypassButton.setClickingTogglesState (true);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", bypassButton);

    for (int i = 0; i < 3; ++i)
    {
        linkButtons[i].setButtonText (juce::String::charToString (static_cast<juce::juce_wchar> ('A' + i)));
        linkButtons[i].setName ("LINK_" + juce::String (i));
        addAndMakeVisible (linkButtons[i]);
        linkButtons[i].onClick = [this, i] { selectLink (i); };
    }

    testButton.setName ("TEST");
    testButton.setButtonText ("TEST");
    addAndMakeVisible (testButton);
    testButton.onClick = [this] { requestTest(); };

    resetButton.setName ("RESET");
    resetButton.setButtonText ("RESET");
    addAndMakeVisible (resetButton);
    resetButton.onClick = [this] { resetShape(); };

    addAndMakeVisible (graph);

    auto wire = [this] (juce::Slider& slider, const juce::String& id,
                        std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix)
    {
        styleSlider (slider, suffix);
        attachment = std::make_unique<SliderAttachment> (processor.apvts, id, slider);
    };

    wire (depthSlider,   "DEPTH",   depthAttachment,   " dB");
    wire (attackSlider,  "ATTACK",  attackAttachment,  " ms");
    wire (holdSlider,    "HOLD",    holdAttachment,    " ms");
    wire (releaseSlider, "RELEASE", releaseAttachment, " ms");
    wire (mixSlider,     "MIX",     mixAttachment,     " %");

    selectLink (processor.getLink());
    startTimerHz (30);
}

HomeSidechainReceiverAudioProcessorEditor::~HomeSidechainReceiverAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HomeSidechainReceiverAudioProcessorEditor::styleSlider (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 18);
    slider.setColour (juce::Slider::textBoxTextColourId, white);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, scopeBg);
    slider.setColour (juce::Slider::textBoxOutlineColourId, grid);
    slider.setTextValueSuffix (suffix);
    addAndMakeVisible (slider);
}

void HomeSidechainReceiverAudioProcessorEditor::selectLink (int index)
{
    const int clamped = juce::jlimit (0, 2, index);
    if (auto* p = processor.apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (clamped)));

    for (int i = 0; i < 3; ++i)
        linkButtons[i].setToggleState (i == clamped, juce::dontSendNotification);
}

void HomeSidechainReceiverAudioProcessorEditor::requestTest()
{
    processor.requestTestTrigger();
}

void HomeSidechainReceiverAudioProcessorEditor::resetShape()
{
    graph.resetShape();
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    const auto body = getLocalBounds().toFloat().reduced (7.0f);
    g.setColour (chassis);
    g.fillRoundedRectangle (body, 13.0f);

    // Header title.
    g.setColour (white);
    g.setFont (juce::FontOptions (16.5f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("Home-Sidechain Receiver", 16, 10, 285, 20, juce::Justification::left);

    g.setColour (cyan);
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("DUBTACH DSP", 17, 31, 110, 10, juce::Justification::left);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const float activity = juce::jmax (processor.triggerActivity.load (std::memory_order_relaxed),
                                       processor.midiActivity.load (std::memory_order_relaxed));
    const bool receiving = activity > 0.08f && !bypassed;
    const bool connected = processor.homeLinkConnected.load (std::memory_order_relaxed);
    const juce::String state = bypassed ? "BYPASSED" : (receiving ? "RECEIVING" : (connected ? "READY" : "WAITING"));
    const juce::Colour stateColour = bypassed ? red : (receiving ? green : (connected ? cyan : muted));

    g.setColour (stateColour.withAlpha (0.12f));
    g.fillRoundedRectangle (306.0f, 11.0f, 82.0f, 21.0f, 10.5f);
    g.setColour (stateColour);
    g.fillEllipse (313.0f, 18.0f, 6.0f, 6.0f);
    g.setFont (juce::FontOptions (7.6f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (state, 326, 14, 56, 14, juce::Justification::left);

    g.setColour (muted.withAlpha (0.55f));
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("LINK", 420, 14, 23, 10, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    const int topY = 9;
    linkButtons[0].setBounds (445, topY, 28, 26);
    linkButtons[1].setBounds (474, topY, 28, 26);
    linkButtons[2].setBounds (503, topY, 28, 26);
    bypassButton.setBounds (536, topY, 40, 26);

    graph.setBounds (16, 43, 588, 232);

    testButton.setBounds (506, 286, 46, 22);
    resetButton.setBounds (558, 286, 46, 22);

    const int y = 315;
    const int h = 31;
    const int w = 111;
    const int gap = 6;
    depthSlider.setBounds   (16 + 0 * (w + gap), y, w, h);
    attackSlider.setBounds  (16 + 1 * (w + gap), y, w, h);
    holdSlider.setBounds    (16 + 2 * (w + gap), y, w, h);
    releaseSlider.setBounds (16 + 3 * (w + gap), y, w, h);
    mixSlider.setBounds     (16 + 4 * (w + gap), y, w, h);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
