#include "PluginEditor.h"

namespace
{
    const juce::Colour background    (0xff07080b);
    const juce::Colour frame         (0xff101217);
    const juce::Colour graphBg       (0xff090d12);
    const juce::Colour cyan          (0xff00e5ff);
    const juce::Colour green         (0xff00ff87);
    const juce::Colour violet        (0xffa768ff);
    const juce::Colour red           (0xffff4f70);
    const juce::Colour white         (0xfff4f6f8);
    const juce::Colour muted         (0xff7d8591);
    const juce::Colour grid          (0xff222832);
    const juce::Colour panelStroke   (0xff252b35);

    class ReceiverLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            juce::ignoreUnused (minSliderPos, maxSliderPos, style);
            const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
            const float cy = b.getCentreY();
            const float left = b.getX() + 7.0f;
            const float right = b.getRight() - 7.0f;
            const float p = juce::jlimit (left, right, sliderPos);

            g.setColour (grid);
            g.fillRoundedRectangle (left, cy - 2.0f, right - left, 4.0f, 2.0f);
            g.setColour (cyan.withAlpha (0.18f));
            g.fillRoundedRectangle (left, cy - 2.0f, juce::jmax (0.0f, p - left), 4.0f, 2.0f);
            g.setColour (cyan.withAlpha (0.12f));
            g.fillEllipse (p - 10.0f, cy - 10.0f, 20.0f, 20.0f);
            g.setColour (white);
            g.fillEllipse (p - 4.5f, cy - 4.5f, 9.0f, 9.0f);
            juce::ignoreUnused (slider);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                   bool highlighted, bool down) override
        {
            const auto b = button.getLocalBounds().toFloat();

            if (button.getName() == "BYPASS")
            {
                const bool on = button.getToggleState();
                g.setColour (on ? red.withAlpha (0.16f) : white.withAlpha (0.025f));
                g.fillRoundedRectangle (b, 8.0f);
                g.setColour (on ? red : white.withAlpha (0.32f));
                g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

                const float cx = b.getCentreX();
                const float cy = b.getCentreY() + 0.5f;
                juce::Path icon;
                icon.addCentredArc (cx, cy + 1.5f, 6.0f, 6.0f, 0.0f,
                                   0.42f, juce::MathConstants<float>::twoPi - 0.42f, true);
                g.setColour (on ? red : white.withAlpha (0.75f));
                g.strokePath (icon, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
                g.drawLine (cx, cy - 7.0f, cx, cy + 0.5f, 1.7f);
                return;
            }

            if (button.getName() == "TEST")
            {
                g.setColour (down ? cyan.withAlpha (0.2f) : (highlighted ? cyan.withAlpha (0.11f) : white.withAlpha (0.025f)));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (cyan.withAlpha (highlighted ? 0.95f : 0.55f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                return;
            }

            if (button.getName() == "RESET")
            {
                g.setColour (highlighted ? white.withAlpha (0.08f) : white.withAlpha (0.025f));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (white.withAlpha (0.32f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                return;
            }

            if (button.getName().startsWith ("LINK_"))
            {
                const bool active = button.getToggleState();
                g.setColour (active ? cyan.withAlpha (0.08f) : juce::Colours::transparentBlack);
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (active ? cyan.withAlpha (0.8f) : white.withAlpha (0.13f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                if (active)
                {
                    g.setColour (cyan);
                    g.fillRoundedRectangle (b.getX() + 5.0f, b.getBottom() - 3.0f,
                                            b.getWidth() - 10.0f, 2.0f, 1.0f);
                }
                return;
            }
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            if (button.getName() == "BYPASS")
                return;

            auto b = button.getLocalBounds().toFloat();
            const bool link = button.getName().startsWith ("LINK_");
            const bool active = button.getToggleState();
            const float size = link ? 11.0f : 8.3f;
            g.setFont (juce::FontOptions (size).withName ("Helvetica").withStyle ("Bold"));
            g.setColour (link && active ? cyan : white.withAlpha (link ? 0.72f : 0.84f));
            g.drawText (button.getButtonText(), b, juce::Justification::centred);
        }
    };

    static ReceiverLookAndFeel receiverLookAndFeel;
}

juce::Rectangle<float> ReceiverShaperGraph::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (15.0f, 18.0f).withTrimmedTop (4.0f);
}

juce::Point<float> ReceiverShaperGraph::pointForIndex (int index) const
{
    const auto b = plotBounds();
    const float x = b.getX() + b.getWidth() * (static_cast<float> (index) / 4.0f);
    const float value = processor.getShapePoint (index);
    return { x, b.getBottom() - b.getHeight() * juce::jlimit (0.0f, 1.0f, value) };
}

int ReceiverShaperGraph::nearestPoint (juce::Point<float> p) const
{
    int best = -1;
    float distance = 17.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float d = pointForIndex (i).getDistanceFrom (p);
        if (d < distance)
        {
            distance = d;
            best = i;
        }
    }
    return best;
}

void ReceiverShaperGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (graphBg);
    g.fillRoundedRectangle (bounds, 12.0f);

    const auto plot = plotBounds();

    // Very subtle ShaperBox-style grid.
    g.setColour (grid.withAlpha (0.72f));
    for (int i = 0; i <= 16; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 16.0f;
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 6; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * static_cast<float> (i) / 6.0f;
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    // Ducking scale.
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (muted.withAlpha (0.75f));
    g.drawText ("0 dB", plot.getX() + 3, plot.getY() - 13, 35, 10, juce::Justification::left);
    g.drawText ("-12", plot.getX() + 3, plot.getY() + plot.getHeight() * 0.25f - 5.0f, 35, 10, juce::Justification::left);
    g.drawText ("-24", plot.getX() + 3, plot.getY() + plot.getHeight() * 0.5f - 5.0f, 35, 10, juce::Justification::left);
    g.drawText ("-36", plot.getX() + 3, plot.getY() + plot.getHeight() * 0.75f - 5.0f, 35, 10, juce::Justification::left);

    juce::Path curve;
    curve.startNewSubPath (pointForIndex (0));
    for (int i = 1; i < 5; ++i)
    {
        const auto a = curve.getCurrentPosition();
        const auto p = pointForIndex (i);
        const float midX = (a.x + p.x) * 0.5f;
        curve.cubicTo (midX, a.y, midX, p.y, p.x, p.y);
    }

    // Envelope fill.
    juce::Path fill = curve;
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();
    g.setColour (cyan.withAlpha (0.06f));
    g.fillPath (fill);
    g.setColour (violet.withAlpha (0.10f));
    g.strokePath (curve, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (cyan);
    g.strokePath (curve, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Section labels make the timing relationship explicit without adding controls.
    const double totalMs = juce::jmax<double> (1.0,
        processor.apvts.getRawParameterValue ("ATTACK")->load()
      + processor.apvts.getRawParameterValue ("HOLD")->load()
      + processor.apvts.getRawParameterValue ("RELEASE")->load());
    const double attackMs = processor.apvts.getRawParameterValue ("ATTACK")->load();
    const double holdMs = processor.apvts.getRawParameterValue ("HOLD")->load();

    const float xAttack = plot.getX() + plot.getWidth() * static_cast<float> (attackMs / totalMs);
    const float xHold = plot.getX() + plot.getWidth() * static_cast<float> ((attackMs + holdMs) / totalMs);

    g.setColour (cyan.withAlpha (0.12f));
    g.drawLine (xAttack, plot.getY(), xAttack, plot.getBottom(), 1.0f);
    g.drawLine (xHold, plot.getY(), xHold, plot.getBottom(), 1.0f);
    g.setColour (muted.withAlpha (0.55f));
    g.drawText ("ATTACK", plot.getX() + 8, plot.getBottom() - 16, 50, 10, juce::Justification::left);
    g.drawText ("HOLD", xHold + 5, plot.getBottom() - 16, 34, 10, juce::Justification::left);
    g.drawText ("RELEASE", plot.getRight() - 55, plot.getBottom() - 16, 50, 10, juce::Justification::right);

    // Live envelope playhead.
    const float phase = processor.envelopeDisplayPhase.load (std::memory_order_relaxed);
    if (processor.envelopeActiveForUI.load (std::memory_order_relaxed) || phase > 0.001f)
    {
        const float px = plot.getX() + plot.getWidth() * juce::jlimit (0.0f, 1.0f, phase);
        g.setColour (green.withAlpha (0.65f));
        g.drawLine (px, plot.getY(), px, plot.getBottom(), 1.8f);
        g.setColour (green);
        g.fillEllipse (px - 3.5f, plot.getY() - 1.5f, 7.0f, 7.0f);
    }

    // Editable curve points.
    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        const bool active = (i == hoveredPoint || i == draggedPoint);
        const float r = active ? 7.0f : 5.0f;
        g.setColour (cyan.withAlpha (active ? 0.22f : 0.12f));
        g.fillEllipse (p.x - r - 4.0f, p.y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
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
    repaint();
}

void ReceiverShaperGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedPoint < 0)
        return;

    const auto b = plotBounds();
    const float value = 1.0f - juce::jlimit<float> (0.0f, 1.0f, (e.position.y - b.getY()) / juce::jmax<float> (1.0f, static_cast<float> (b.getHeight())));
    processor.setShapePoint (draggedPoint, value);
    repaint();
}

void ReceiverShaperGraph::mouseUp (const juce::MouseEvent&)
{
    draggedPoint = -1;
    setMouseCursor (hoveredPoint >= 0 ? juce::MouseCursor::PointingHandCursor
                                      : juce::MouseCursor::NormalCursor);
    repaint();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), graph (p)
{
    setSize (620, 390);
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
        linkButtons[i].setClickingTogglesState (false);
        addAndMakeVisible (linkButtons[i]);
        linkButtons[i].onClick = [this, i] { selectLink (i); };
    }

    testButton.setName ("TEST");
    addAndMakeVisible (testButton);
    testButton.onClick = [this] { requestTest(); };

    resetButton.setName ("RESET");
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

    // Curve remains available in state but is not given another UI control.
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
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, white);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, graphBg);
    slider.setColour (juce::Slider::textBoxOutlineColourId, panelStroke);
    slider.setTextValueSuffix (suffix);
    addAndMakeVisible (slider);
}

void HomeSidechainReceiverAudioProcessorEditor::selectLink (int index)
{
    index = juce::jlimit (0, 7, index);
    if (auto* p = processor.apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (index)));

    // This UI intentionally exposes the first three links; the rest remain state-compatible.
    const int visible = juce::jlimit (0, 2, index);
    for (int i = 0; i < 3; ++i)
        linkButtons[i].setToggleState (i == visible, juce::dontSendNotification);
    repaint();
}

void HomeSidechainReceiverAudioProcessorEditor::requestTest()
{
    processor.requestTestTrigger();
}

void HomeSidechainReceiverAudioProcessorEditor::resetShape()
{
    const float values[5] = { 1.0f, 0.14f, 0.10f, 0.48f, 1.0f };
    for (int i = 0; i < 5; ++i)
        processor.setShapePoint (i, values[i]);
    repaint();
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    const auto body = getLocalBounds().toFloat().reduced (7.0f);
    g.setColour (frame);
    g.fillRoundedRectangle (body, 13.0f);

    // Header
    g.setColour (white);
    g.setFont (juce::FontOptions (18.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("Home-Sidechain Receiver", 18, 12, 300, 22, juce::Justification::left);

    g.setColour (cyan);
    g.setFont (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("DUBTACH DSP", 19, 36, 120, 11, juce::Justification::left);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const float activity = juce::jmax (processor.triggerActivity.load (std::memory_order_relaxed),
                                       processor.midiActivity.load (std::memory_order_relaxed));
    const bool receiving = activity > 0.08f && ! bypassed;
    const bool connected = processor.homeLinkConnected.load (std::memory_order_relaxed);

    const juce::String state = bypassed ? "BYPASSED" : (receiving ? "RECEIVING" : (connected ? "READY" : "WAITING"));
    const juce::Colour stateColour = bypassed ? red : (receiving ? green : (connected ? cyan : muted));
    g.setColour (stateColour.withAlpha (0.18f));
    g.fillRoundedRectangle (341.0f, 12.0f, 96.0f, 23.0f, 11.5f);
    g.setColour (stateColour);
    g.fillEllipse (349.0f, 20.0f, 6.0f, 6.0f);
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (state, 362, 15, 65, 16, juce::Justification::left);

    // Header utility group.
    g.setColour (muted);
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("LINK", 446, 15, 22, 10, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    linkButtons[0].setBounds (470, 10, 28, 28);
    linkButtons[1].setBounds (499, 10, 28, 28);
    linkButtons[2].setBounds (528, 10, 28, 28);
    bypassButton.setBounds (560, 10, 40, 28);

    testButton.setBounds (443, 44, 54, 24);
    resetButton.setBounds (502, 44, 54, 24);

    graph.setBounds (17, 48, 586, 236);

    constexpr int y = 300;
    constexpr int h = 34;
    constexpr int w = 94;
    constexpr int gap = 8;

    depthSlider.setBounds   (17 + 0 * (w + gap), y, w, h);
    attackSlider.setBounds  (17 + 1 * (w + gap), y, w, h);
    holdSlider.setBounds    (17 + 2 * (w + gap), y, w, h);
    releaseSlider.setBounds (17 + 3 * (w + gap), y, w, h);
    mixSlider.setBounds     (17 + 4 * (w + gap), y, w, h);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
