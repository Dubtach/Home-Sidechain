#include "PluginEditor.h"

namespace
{
    const juce::Colour bg         (0xff090b0e);
    const juce::Colour panel      (0xff101318);
    const juce::Colour surface    (0xff151a21);
    const juce::Colour surface2   (0xff1b2129);
    const juce::Colour outline    (0xff2b333e);
    const juce::Colour grid       (0xff20262e);
    const juce::Colour accent     (0xff4de8ff);
    const juce::Colour trigger    (0xffffc857);
    const juce::Colour triggerHot (0xffff8f3d);
    const juce::Colour text       (0xfff4f6f8);
    const juce::Colour muted      (0xff89929e);
    const juce::Colour green      (0xff6be7a7);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (700, 450);
    setResizable (false, false);

    addAndMakeVisible (sensitivity);
    addAndMakeVisible (retrigger);
    addAndMakeVisible (link);
    addAndMakeVisible (bypass);

    styleSlider (sensitivity);
    styleSlider (retrigger);

    sensitivity.setNumDecimalPlacesToDisplay (2);
    retrigger.setNumDecimalPlacesToDisplay (0);
    retrigger.setTextValueSuffix (" ms");

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
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    slider.setColour (juce::Slider::backgroundColourId, outline);
    slider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.78f));
    slider.setColour (juce::Slider::thumbColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, surface2);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

float HomeSidechainTriggerAudioProcessorEditor::yForDb (float db) const noexcept
{
    constexpr float minDb = -60.0f;
    constexpr float maxDb = 0.0f;
    if (graphBounds.getHeight() <= 0.0f)
        return graphBounds.getBottom();

    const float n = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
    return graphBounds.getBottom() - n * graphBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    constexpr float minDb = -60.0f;
    constexpr float maxDb = 0.0f;
    if (graphBounds.getHeight() <= 0.0f)
        return minDb;

    const float n = juce::jlimit (0.0f, 1.0f,
                                  (graphBounds.getBottom() - y) / graphBounds.getHeight());
    return minDb + n * (maxDb - minDb);
}

void HomeSidechainTriggerAudioProcessorEditor::setThresholdFromY (float y)
{
    const float clamped = juce::jlimit (graphBounds.getY(), graphBounds.getBottom(), y);
    const float db = thresholdForY (clamped);

    if (auto* param = processor.apvts.getParameter ("THRESHOLD"))
        param->setValueNotifyingHost (param->convertTo0to1 (db));
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setColour (surface);
    g.fillRoundedRectangle (area, 14.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (area, 14.0f, 1.0f);

    auto plot = area.reduced (18.0f, 16.0f);
    plot.removeFromTop (18.0f);

    // Grid.
    for (int db = 0; db >= -60; db -= 12)
    {
        const float y = yForDb (static_cast<float> (db));
        g.setColour (grid.withAlpha (db == 0 ? 0.9f : 0.65f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());

        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (juce::String (db), plot.getX() + 5.0f, y - 7.0f,
                    28.0f, 14.0f, juce::Justification::left);
    }

    // Time ruler labels: approximately 0.75 seconds of recent history.
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("~0.75s history", plot.getRight() - 90.0f, plot.getY() - 17.0f,
                90.0f, 14.0f, juce::Justification::right);

    const int pointCount = processor.getWaveformPointCount();
    if (pointCount > 1)
    {
        juce::Path fill;
        juce::Path line;
        const float baseline = plot.getBottom();

        fill.startNewSubPath (plot.getX(), baseline);
        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float visual = std::pow (peak, 0.55f);
            const float x = plot.getX() + plot.getWidth() * (static_cast<float> (i) / static_cast<float> (pointCount - 1));
            const float y = baseline - visual * plot.getHeight();
            if (i == 0)
                line.startNewSubPath (x, y);
            else
                line.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (plot.getRight(), baseline);
        fill.closeSubPath();

        g.setColour (accent.withAlpha (0.075f));
        g.fillPath (fill);
        g.setColour (accent.withAlpha (0.16f));
        g.strokePath (line, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
        g.setColour (accent.withAlpha (0.92f));
        g.strokePath (line, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
    }

    // Active threshold is the real threshold after sensitivity is applied.
    const float rawThreshold = processor.getThresholdDb();
    const float activeThreshold = rawThreshold - processor.getSensitivity() * 12.0f;
    const float thresholdY = yForDb (activeThreshold);

    g.setColour (accent.withAlpha (0.10f));
    g.fillRect (juce::Rectangle<float> (plot.getX(), thresholdY - 1.0f, plot.getWidth(), 2.0f));
    g.setColour (accent);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 2.0f);

    g.setColour (surface2);
    g.fillRoundedRectangle (plot.getX() + 10.0f, thresholdY - 14.0f, 170.0f, 26.0f, 8.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText ("THRESHOLD  " + juce::String (activeThreshold, 1) + " dB",
                plot.getX() + 18.0f, thresholdY - 7.0f, 154.0f, 16.0f,
                juce::Justification::left);

    // Trigger markers are intentionally large and bright.
    const auto markerMask = processor.getTriggerMarkers();
    const auto meter = processor.getTriggerMeter();
    for (size_t i = 0; i < markerMask.size(); ++i)
    {
        if (markerMask[i] == 0)
            continue;

        const float x = plot.getX() + plot.getWidth()
            * (static_cast<float> (i) / static_cast<float> (markerMask.size() - 1));

        g.setColour (trigger.withAlpha (0.12f));
        g.fillEllipse (x - 8.0f, plot.getY() + 5.0f, 16.0f, 16.0f);
        g.setColour (trigger);
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 2.2f);
        g.fillEllipse (x - 3.5f, plot.getY() + 4.0f, 7.0f, 7.0f);
    }

    // Live trigger playhead at the newest sample/bin.
    const float playheadX = plot.getRight() - 2.0f;
    g.setColour (meter > 0.08f ? triggerHot.withAlpha (0.92f) : muted.withAlpha (0.35f));
    g.fillRoundedRectangle (playheadX - 2.0f, plot.getY(), 4.0f, plot.getHeight(), 2.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawStatus (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto meter = processor.getTriggerMeter();
    const bool firing = meter > 0.08f;
    const auto linkIndex = static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load());

    g.setColour (surface);
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    const float dotSize = 14.0f + meter * 10.0f;
    const float dotX = area.getX() + 16.0f;
    const float dotY = area.getCentreY() - dotSize * 0.5f;
    g.setColour ((firing ? triggerHot : green).withAlpha (0.15f + meter * 0.30f));
    g.fillEllipse (dotX - 5.0f, dotY - 5.0f, dotSize + 10.0f, dotSize + 10.0f);
    g.setColour (firing ? trigger : green);
    g.fillEllipse (dotX, dotY, dotSize, dotSize);

    g.setColour (text);
    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.drawText (firing ? "TRIGGERING" : "READY",
                area.getX() + 42.0f, area.getY() + 9.0f, 110.0f, 18.0f,
                juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("Triggers  " + juce::String (processor.getTriggerCount()),
                area.getX() + 42.0f, area.getY() + 27.0f, 120.0f, 14.0f,
                juce::Justification::left);

    g.setColour (outline);
    g.drawVerticalLine (juce::roundToInt (area.getRight() - 140.0f), area.getY() + 8.0f, area.getBottom() - 8.0f);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("AUTO AUDIO + MIDI", area.getRight() - 128.0f, area.getY() + 9.0f,
                112.0f, 14.0f, juce::Justification::right);
    g.drawText ("LINK " + homeSidechain::linkName (linkIndex)
                + "   •   NOTE " + juce::String (homeSidechain::midiNoteForLink (linkIndex)),
                area.getRight() - 128.0f, area.getY() + 26.0f,
                112.0f, 14.0f, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    const auto outer = getLocalBounds().toFloat().reduced (12.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (outer, 16.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (outer, 16.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (19.0f).withStyle ("Bold"));
    g.drawText ("HOME TRIGGER", 26, 18, 190, 24, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("SMART TRANSIENT → MIDI / HOME-LINK", 27, 43, 260, 14, juce::Justification::left);

    drawGraph (g, graphBounds);
    drawStatus (g, { 26.0f, 312.0f, 648.0f, 54.0f });

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("SENSITIVITY", 26, 385, 120, 14, juce::Justification::left);
    g.drawText ("RETRIGGER", 382, 385, 120, 14, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    link.setBounds (548, 24, 58, 24);
    bypass.setBounds (610, 24, 62, 24);

    graphBounds = { 26.0f, 68.0f, 648.0f, 230.0f };

    sensitivity.setBounds (26, 398, 300, 26);
    retrigger.setBounds (382, 398, 292, 26);
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
