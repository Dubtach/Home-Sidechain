#include "PluginEditor.h"

namespace
{
    const juce::Colour bg (0xff0a0b0d);
    const juce::Colour panel (0xff111317);
    const juce::Colour panel2 (0xff171a20);
    const juce::Colour outline (0xff292e36);
    const juce::Colour grid (0xff20242b);
    const juce::Colour accent (0xff00e5ff);
    const juce::Colour triggerColour (0xffffd166);
    const juce::Colour text (0xfff4f5f7);
    const juce::Colour muted (0xff8a919c);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (620, 380);
    setResizable (false, false);

    addAndMakeVisible (sensitivity);
    addAndMakeVisible (retrigger);
    addAndMakeVisible (link);
    addAndMakeVisible (bypass);

    styleSlider (sensitivity);
    styleSlider (retrigger);

    sensitivity.setNumDecimalPlacesToDisplay (2);
    retrigger.setTextValueSuffix (" ms");
    sensitivity.setTextValueSuffix ("");

    link.addItemList (homeSidechain::linkNames(), 1);

    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SENSITIVITY", sensitivity);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RETRIGGER", retrigger);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LINK", link);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 22);
    slider.setColour (juce::Slider::backgroundColourId, outline);
    slider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.75f));
    slider.setColour (juce::Slider::thumbColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel2);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

float HomeSidechainTriggerAudioProcessorEditor::yForThreshold (float thresholdDb) const noexcept
{
    const float minDb = -60.0f;
    const float maxDb = 0.0f;
    const float n = juce::jlimit (0.0f, 1.0f, (thresholdDb - minDb) / (maxDb - minDb));
    return graphBounds.getBottom() - n * graphBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    const float minDb = -60.0f;
    const float maxDb = 0.0f;
    if (graphBounds.getHeight() <= 0.0f)
        return minDb;

    const float n = juce::jlimit (0.0f, 1.0f,
                                  (graphBounds.getBottom() - y) / graphBounds.getHeight());
    return minDb + n * (maxDb - minDb);
}

void HomeSidechainTriggerAudioProcessorEditor::setThresholdFromY (float y)
{
    if (graphBounds.getHeight() <= 0.0f)
        return;

    const float db = thresholdForY (juce::jlimit (graphBounds.getY(),
                                                  graphBounds.getBottom(), y));
    if (auto* param = processor.apvts.getParameter ("THRESHOLD"))
        param->setValueNotifyingHost (param->convertTo0to1 (db));
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setColour (panel2);
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    auto plot = area.reduced (14.0f, 12.0f);
    plot.removeFromTop (8.0f);

    // Horizontal dB grid.
    g.setFont (juce::FontOptions (9.0f));
    for (int db = 0; db >= -60; db -= 12)
    {
        const float y = yForThreshold (static_cast<float> (db));
        g.setColour (grid);
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (muted);
        g.drawText (juce::String (db) + " dB", plot.getX() + 6.0f,
                    y - 7.0f, 42.0f, 14.0f, juce::Justification::left);
    }

    // Recent input waveform.
    juce::Path waveform;
    const int pointCount = processor.getWaveformPointCount();
    if (pointCount > 1)
    {
        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float x = plot.getX() + plot.getWidth() * (static_cast<float> (i) / static_cast<float> (pointCount - 1));
            const float y = plot.getBottom() - peak * plot.getHeight();
            if (i == 0)
                waveform.startNewSubPath (x, y);
            else
                waveform.lineTo (x, y);
        }
    }

    g.setColour (accent.withAlpha (0.30f));
    g.strokePath (waveform, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
    g.setColour (accent.withAlpha (0.88f));
    g.strokePath (waveform, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved));

    // Trigger markers.
    const auto triggerMask = processor.getTriggerMarkers();
    for (size_t i = 0; i < triggerMask.size(); ++i)
    {
        if (triggerMask[i] == 0)
            continue;

        const float x = plot.getX() + plot.getWidth() * (static_cast<float> (i) / static_cast<float> (triggerMask.size() - 1));
        g.setColour (triggerColour.withAlpha (0.92f));
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.2f);
        g.fillEllipse (x - 3.0f, plot.getBottom() - 9.0f, 6.0f, 6.0f);
    }

    const float thresholdY = yForThreshold (processor.getThresholdDb());
    g.setColour (accent);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 2.0f);
    g.fillRoundedRectangle (plot.getX() + 8.0f, thresholdY - 11.0f, 92.0f, 22.0f, 6.0f);
    g.setColour (bg);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText ("THRESHOLD  " + juce::String (processor.getThresholdDb(), 1) + " dB",
                plot.getX() + 14.0f, thresholdY - 8.0f, 82.0f, 16.0f,
                juce::Justification::left);

    // Sensitivity visualization: the active trigger boundary moves down as sensitivity rises.
    const float sensitivityAmount = processor.getSensitivity();
    const float curveY = plot.getBottom() - (0.15f + 0.55f * sensitivityAmount) * plot.getHeight();
    juce::Path curve;
    curve.startNewSubPath (plot.getX(), curveY + 18.0f);
    curve.quadraticTo (plot.getCentreX(), curveY - 10.0f, plot.getRight(), curveY + 6.0f);
    g.setColour (triggerColour.withAlpha (0.65f));
    g.strokePath (curve, juce::PathStrokeType (1.0f));
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    const auto bounds = getLocalBounds().toFloat().reduced (12.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 14.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 14.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (19.0f).withStyle ("Bold"));
    g.drawText ("HOME-SIDECHAIN", 24, 16, 260, 24, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("TRIGGER", 25, 40, 100, 14, juce::Justification::left);

    const int linkIndex = static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load());
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("AUTO AUDIO + MIDI   •   LINK " + homeSidechain::linkName (linkIndex)
                + "   •   NOTE " + juce::String (homeSidechain::midiNoteForLink (linkIndex)),
                190, 22, 290, 16, juce::Justification::right);

    g.setColour (muted);
    g.drawText ("Drag the threshold line directly on the graph", 24, 55, 320, 16, juce::Justification::left);

    drawGraph (g, graphBounds);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("SENSITIVITY", sensitivity.getX(), sensitivity.getY() - 16, 100, 14, juce::Justification::left);
    g.drawText ("RETRIGGER", retrigger.getX(), retrigger.getY() - 16, 100, 14, juce::Justification::left);

    g.setColour (muted);
    g.drawText ("HOME-LINK ACTIVE", 24, getHeight() - 24, 180, 14, juce::Justification::left);
    g.drawText ("No velocity control — triggers use full MIDI velocity for a consistent hit.",
                210, getHeight() - 24, 370, 14, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    link.setBounds (492, 17, 58, 24);
    bypass.setBounds (556, 17, 48, 24);

    graphBounds = { 24.0f, 78.0f, 572.0f, 224.0f };

    sensitivity.setBounds (24, 330, 272, 22);
    retrigger.setBounds (324, 330, 272, 22);
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (graphBounds.contains (e.position))
    {
        draggingThreshold = true;
        setThresholdFromY (e.position.y);
    }
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingThreshold)
        setThresholdFromY (e.position.y);
}

void HomeSidechainTriggerAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    draggingThreshold = false;
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    repaint (graphBounds.toNearestInt());
}
