#include "PluginEditor.h"

namespace
{
    const juce::Colour bg          (0xff090a0d);
    const juce::Colour panel       (0xff121419);
    const juce::Colour scope       (0xff0a0e12);
    const juce::Colour cyan        (0xff00e5ff);
    const juce::Colour green       (0xff00ff87);
    const juce::Colour violet      (0xffb66cff);
    const juce::Colour red         (0xffff4f70);
    const juce::Colour text        (0xfff4f6f8);
    const juce::Colour muted       (0xff7d8591);
    const juce::Colour dim         (0xff2a3038);

    class ReceiverLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            juce::ignoreUnused (minSliderPos, maxSliderPos, style);
            auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
            const float cy = b.getCentreY();
            const float left = b.getX() + 4.0f;
            const float right = b.getRight() - 4.0f;

            g.setColour (dim.withAlpha (0.9f));
            g.fillRoundedRectangle (left, cy - 2.0f, right - left, 4.0f, 2.0f);

            const float p = juce::jlimit (left, right, sliderPos);
            g.setColour (cyan.withAlpha (0.55f));
            g.fillRoundedRectangle (left, cy - 2.0f, juce::jmax (0.0f, p - left), 4.0f, 2.0f);

            g.setColour (cyan.withAlpha (0.16f));
            g.fillEllipse (p - 10.0f, cy - 10.0f, 20.0f, 20.0f);
            g.setColour (text);
            g.fillEllipse (p - 4.5f, cy - 4.5f, 9.0f, 9.0f);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                   bool highlighted, bool down) override
        {
            const auto b = button.getLocalBounds().toFloat();
            if (button.getName() == "BYPASS")
            {
                const bool on = button.getToggleState();
                g.setColour (on ? red.withAlpha (0.18f) : dim.withAlpha (0.5f));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (on ? red : text.withAlpha (0.45f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.1f);
                const float cx = b.getCentreX();
                const float cy = b.getCentreY();
                g.setColour (on ? red : text.withAlpha (0.65f));
                juce::Path p;
                p.addCentredArc (cx, cy + 1.0f, 6.0f, 6.0f, 0.0f, 0.55f, 5.73f, true);
                g.strokePath (p, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.drawLine (cx, cy - 7.0f, cx, cy + 0.5f, 1.7f);
                return;
            }

            if (button.getName().startsWith ("LINK_"))
            {
                const bool on = button.getToggleState();
                g.setColour (on ? cyan.withAlpha (0.11f) : juce::Colours::transparentBlack);
                g.fillRoundedRectangle (b, 6.0f);
                g.setColour (on ? cyan : text.withAlpha (highlighted ? 0.7f : 0.3f));
                g.drawRoundedRectangle (b.reduced (1.0f), 6.0f, on ? 1.2f : 0.8f);
                return;
            }

            if (button.getName() == "TEST")
            {
                g.setColour (highlighted ? cyan.withAlpha (0.14f) : dim.withAlpha (0.32f));
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (cyan.withAlpha (down ? 1.0f : 0.75f));
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
                return;
            }
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            auto b = button.getLocalBounds().toFloat();
            g.setFont (juce::FontOptions (button.getName().startsWith ("LINK_") ? 10.0f : 8.5f)
                           .withName ("Helvetica").withStyle ("Bold"));
            g.setColour (button.getName().startsWith ("LINK_") && button.getToggleState()
                         ? cyan : text.withAlpha (0.82f));
            g.drawText (button.getButtonText(), b, juce::Justification::centred);
        }
    };
}

juce::Point<float> ReceiverEnvelopeView::pointForIndex (int index) const
{
    const auto b = getLocalBounds().toFloat().reduced (28.0f, 18.0f);
    return {
        b.getX() + b.getWidth() * (static_cast<float> (index) / 4.0f),
        b.getBottom() - b.getHeight() * processor.getShapePoint (index)
    };
}

int ReceiverEnvelopeView::nearestPoint (juce::Point<float> p) const
{
    int best = -1;
    float bestDistance = 20.0f;
    for (int i = 0; i < 5; ++i)
    {
        const auto d = pointForIndex (i).getDistanceFrom (p);
        if (d < bestDistance)
        {
            best = i;
            bestDistance = d;
        }
    }
    return best;
}

void ReceiverEnvelopeView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (scope);
    g.fillRoundedRectangle (b, 10.0f);

    const auto plot = b.reduced (28.0f, 18.0f);
    g.setColour (dim.withAlpha (0.42f));
    for (int i = 0; i <= 8; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (static_cast<float> (i) / 8.0f);
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 4; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * (static_cast<float> (i) / 4.0f);
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    g.setFont (juce::FontOptions (7.5f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (muted.withAlpha (0.8f));
    g.drawText ("FULL", plot.getX(), plot.getY() - 13, 40, 10, juce::Justification::left);
    g.drawText ("DUCK", plot.getX(), plot.getBottom() + 5, 40, 10, juce::Justification::left);
    g.drawText ("TRIGGER", plot.getRight() - 52, plot.getBottom() + 5, 52, 10, juce::Justification::right);

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

    g.setColour (cyan.withAlpha (0.07f));
    g.fillPath (fill);
    g.setColour (cyan.withAlpha (0.22f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (cyan);
    g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        g.setColour (draggedPoint == i ? text : cyan);
        g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
    }

    const float activity = juce::jlimit (0.0f, 1.0f,
        processor.triggerActivity.load (std::memory_order_relaxed));
    if (activity > 0.05f)
    {
        const float x = plot.getX() + plot.getWidth() * (1.0f - juce::jmin (1.0f, activity));
        g.setColour (green.withAlpha (0.55f * activity));
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 2.0f);
    }
}

void ReceiverEnvelopeView::mouseDown (const juce::MouseEvent& e)
{
    draggedPoint = nearestPoint (e.position);
    if (draggedPoint >= 0)
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    repaint();
}

void ReceiverEnvelopeView::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedPoint < 0)
        return;

    const auto b = getLocalBounds().toFloat().reduced (28.0f, 18.0f);
    const float normalized = 1.0f - juce::jlimit (0.0f, 1.0f,
        (e.position.y - b.getY()) / juce::jmax (1.0f, b.getHeight()));
    processor.setShapePoint (draggedPoint, normalized);
    repaint();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), envelopeView (p)
{
    setSize (600, 360);
    setResizable (false, false);
    setLookAndFeel (&lookAndFeel);

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

    addAndMakeVisible (envelopeView);

    auto wire = [this] (juce::Slider& slider, const juce::String& id,
                        std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix)
    {
        styleSlider (slider, suffix);
        attachment = std::make_unique<SliderAttachment> (processor.apvts, id, slider);
    };

    wire (depthSlider, "DEPTH", depthAttachment, " dB");
    wire (attackSlider, "ATTACK", attackAttachment, " ms");
    wire (holdSlider, "HOLD", holdAttachment, " ms");
    wire (releaseSlider, "RELEASE", releaseAttachment, " ms");
    wire (curveSlider, "CURVE", curveAttachment, "");
    wire (mixSlider, "MIX", mixAttachment, " %");

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
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, scope);
    slider.setColour (juce::Slider::textBoxOutlineColourId, dim);
    slider.setTextValueSuffix (suffix);
    addAndMakeVisible (slider);
}

void HomeSidechainReceiverAudioProcessorEditor::selectLink (int index)
{
    index = juce::jlimit (0, 2, index);
    if (auto* p = processor.apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (index)));

    for (int i = 0; i < 3; ++i)
        linkButtons[i].setToggleState (i == index, juce::dontSendNotification);
    repaint();
}

void HomeSidechainReceiverAudioProcessorEditor::requestTest()
{
    processor.requestTestTrigger();
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    const auto body = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (body, 13.0f);

    // Subtle Home-series colour atmosphere.
    g.setColour (cyan.withAlpha (0.03f));
    g.fillEllipse (body.getRight() - 210.0f, body.getY() - 50.0f, 270.0f, 180.0f);
    g.setColour (violet.withAlpha (0.024f));
    g.fillEllipse (body.getX() - 80.0f, body.getBottom() - 125.0f, 230.0f, 180.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (18.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("Home-Sidechain Receiver", 20, 14, 320, 23, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("DUBTACH DSP", 21, 38, 110, 11, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (7.5f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("LINK", 390, 17, 26, 10, juce::Justification::left);

    g.setColour (muted);
    const bool connected = processor.homeLinkConnected.load (std::memory_order_relaxed);
    const float activity = juce::jmax (processor.triggerActivity.load (std::memory_order_relaxed),
                                       processor.midiActivity.load (std::memory_order_relaxed));
    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool receiving = activity > 0.1f && ! bypassed;
    g.setColour (bypassed ? muted : (receiving ? green : (connected ? cyan : dim)));
    g.fillEllipse (300, 21, 6, 6);
    g.setColour (text.withAlpha (0.82f));
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (bypassed ? "BYPASS" : (receiving ? "RECEIVING" : (connected ? "READY" : "WAITING")),
                310, 15, 72, 16, juce::Justification::left);

    g.setColour (muted.withAlpha (0.65f));
    g.setFont (juce::FontOptions (7.0f).withName ("Helvetica"));
    g.drawText ("RECEIVES HOME-LINK + MIDI", 20, 57, 180, 10, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    linkButtons[0].setBounds (408, 12, 27, 26);
    linkButtons[1].setBounds (435, 12, 27, 26);
    linkButtons[2].setBounds (462, 12, 27, 26);
    bypassButton.setBounds (497, 12, 31, 26);
    testButton.setBounds (530, 12, 46, 26);

    envelopeView.setBounds (18, 68, 558, 178);

    const int y = 260;
    const int h = 24;
    const int w = 78;
    const int gap = 12;
    depthSlider.setBounds   (18, y, w + 5, h);
    attackSlider.setBounds  (18 + (w + gap), y, w + 5, h);
    holdSlider.setBounds    (18 + 2 * (w + gap), y, w + 5, h);
    releaseSlider.setBounds (18 + 3 * (w + gap), y, w + 5, h);
    curveSlider.setBounds   (18 + 4 * (w + gap), y, w + 5, h);
    mixSlider.setBounds     (18 + 5 * (w + gap), y, w + 5, h);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    envelopeView.repaint();
    repaint();
}
