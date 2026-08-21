#include "PluginEditor.h"

namespace
{
    const juce::Colour background    (0xff0a0d10);
    const juce::Colour panel         (0xff11161b);
    const juce::Colour surface       (0xff151b21);
    const juce::Colour surfaceRaised (0xff1b232b);
    const juce::Colour lineColour    (0xff2a333d);
    const juce::Colour gridColour    (0xff222a32);
    const juce::Colour accent        (0xff57e7ff);
    const juce::Colour triggerHot    (0xffff5d5d);
    const juce::Colour text          (0xfff3f6f8);
    const juce::Colour muted         (0xff8e98a4);
    const juce::Colour success       (0xff69e1a4);
}

HomeSidechainTriggerGapSlider::HomeSidechainTriggerGapSlider()
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId, accent);
}

void HomeSidechainTriggerGapSlider::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    const float labelX = 0.0f;
    const float labelY = 2.0f;
    const float trackX = 2.0f;
    const float valueW = 74.0f;
    const float gap = 10.0f;
    const float trackW = juce::jmax (100.0f, bounds.getWidth() - valueW - gap - 4.0f);
    const float trackY = bounds.getBottom() - 13.0f;
    const float trackH = 5.0f;
    const float thumbRadius = 7.0f;

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("COOL DOWN", labelX, labelY, 90.0f, 14.0f, juce::Justification::left);

    // Use the slider's actual NormalisableRange so the painted thumb exactly
    // matches JUCE's parameter mapping, including the skew used by the APVTS.
    const auto range = getRange();
    const double normalised = range.getLength() > 0.0
        ? range.convertTo0to1 (getValue())
        : 0.0;
    const float proportion = static_cast<float> (juce::jlimit (0.0, 1.0, normalised));
    const float thumbX = trackX + thumbRadius + (trackW - thumbRadius * 2.0f) * proportion;

    const auto track = juce::Rectangle<float> (trackX + thumbRadius,
                                                trackY - trackH * 0.5f,
                                                trackW - thumbRadius * 2.0f,
                                                trackH);

    g.setColour (lineColour);
    g.fillRoundedRectangle (track, trackH * 0.5f);

    const auto filled = track.withWidth (track.getWidth() * proportion);
    g.setColour (accent.withAlpha (0.90f));
    if (filled.getWidth() > 0.1f)
        g.fillRoundedRectangle (filled, trackH * 0.5f);

    // Subtle center line gives the control a physical, hardware-like feel.
    g.setColour (accent.withAlpha (0.16f));
    g.fillEllipse (thumbX - 10.0f, trackY - 10.0f, 20.0f, 20.0f);

    g.setColour (accent);
    g.fillEllipse (thumbX - thumbRadius, trackY - thumbRadius,
                   thumbRadius * 2.0f, thumbRadius * 2.0f);

    const auto valueBox = juce::Rectangle<float> (bounds.getRight() - valueW, 2.0f,
                                                   valueW, 28.0f);
    g.setColour (surfaceRaised);
    g.fillRoundedRectangle (valueBox, 8.0f);
    g.setColour (lineColour);
    g.drawRoundedRectangle (valueBox, 8.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms",
                valueBox.reduced (7.0f, 0.0f).toNearestInt(),
                juce::Justification::centred);
}


HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (640, 360);
    setResizable (false, false);

    addAndMakeVisible (retrigger);
    addAndMakeVisible (link);
    addAndMakeVisible (bypass);

    styleComboBox();
    styleBypass();

    link.addItemList (homeSidechain::linkNames(), 1);

    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "RETRIGGER", retrigger);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "LINK", link);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "BYPASS", bypass);

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleComboBox()
{
    link.setJustificationType (juce::Justification::centred);
    link.setColour (juce::ComboBox::backgroundColourId, surfaceRaised);
    link.setColour (juce::ComboBox::textColourId, text);
    link.setColour (juce::ComboBox::outlineColourId, lineColour);
    link.setColour (juce::ComboBox::arrowColourId, accent);
    link.setColour (juce::ComboBox::focusedOutlineColourId, accent);
}

void HomeSidechainTriggerAudioProcessorEditor::styleBypass()
{
    bypass.setClickingTogglesState (true);
    bypass.setColour (juce::ToggleButton::textColourId, muted);
    bypass.setColour (juce::ToggleButton::tickColourId, accent);
    bypass.setColour (juce::ToggleButton::tickDisabledColourId, muted);
}

float HomeSidechainTriggerAudioProcessorEditor::yForDb (float db) const noexcept
{
    const float thresholdDb = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, thresholdDb - 30.0f);
    constexpr float maxDb = 0.0f;

    if (graphBounds.getHeight() <= 0.0f)
        return graphBounds.getBottom();

    const float span = juce::jmax (1.0f, maxDb - minDb);
    const float n = juce::jlimit (0.0f, 1.0f, (db - minDb) / span);
    return graphBounds.getBottom() - n * graphBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    const float thresholdDb = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, thresholdDb - 30.0f);
    constexpr float maxDb = 0.0f;

    if (graphBounds.getHeight() <= 0.0f)
        return thresholdDb;

    const float span = juce::jmax (1.0f, maxDb - minDb);
    const float n = juce::jlimit (0.0f, 1.0f,
                                  (graphBounds.getBottom() - y) / graphBounds.getHeight());
    return minDb + n * span;
}

void HomeSidechainTriggerAudioProcessorEditor::setThresholdFromY (float y)
{
    const float clamped = juce::jlimit (graphBounds.getY(), graphBounds.getBottom(), y);
    const float db = thresholdForY (clamped);

    if (auto* param = processor.apvts.getParameter ("THRESHOLD"))
        param->setValueNotifyingHost (param->convertTo0to1 (db));
}

void HomeSidechainTriggerAudioProcessorEditor::drawPill (juce::Graphics& g,
                                                          juce::Rectangle<float> area,
                                                          juce::Colour colour,
                                                          const juce::String& label,
                                                          bool bright) const
{
    g.setColour (colour.withAlpha (bright ? 0.18f : 0.07f));
    g.fillRoundedRectangle (area, area.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (bright ? 0.8f : 0.28f));
    g.drawRoundedRectangle (area, area.getHeight() * 0.5f, 1.0f);
    g.setColour (bright ? text : muted);
    g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
    g.drawText (label, area.toNearestInt(), juce::Justification::centred);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g,
                                                            juce::Rectangle<float> area) const
{
    g.setColour (text);
    g.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.drawText ("HOME TRIGGER", area.getX(), area.getY(), 180.0f, 22.0f,
                juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("TRANSIENT → NOTE", area.getX(), area.getY() + 21.0f, 150.0f, 15.0f,
                juce::Justification::left);

    const auto meter = processor.getTriggerMeter();
    const bool firing = meter > 0.10f;
    const auto linkIndex = processor.getLink();

    drawPill (g, { area.getRight() - 318.0f, area.getY() + 1.0f, 76.0f, 24.0f },
              firing ? triggerHot : success,
              firing ? "TRIGGER" : "READY", firing);
    drawPill (g, { area.getRight() - 234.0f, area.getY() + 1.0f, 96.0f, 24.0f },
              accent, "AUDIO + MIDI", false);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("LINK " + homeSidechain::linkName (linkIndex)
                    + "  •  " + juce::String (processor.getTriggerCount()) + " triggers",
                area.getRight() - 326.0f, area.getY() + 26.0f, 206.0f, 14.0f,
                juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g,
                                                           juce::Rectangle<float> area) const
{
    g.setColour (surface);
    g.fillRoundedRectangle (area, 14.0f);
    g.setColour (lineColour);
    g.drawRoundedRectangle (area, 14.0f, 1.0f);

    auto plot = area.reduced (42.0f, 24.0f);
    plot.removeFromLeft (18.0f);

    const float thresholdDb = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, thresholdDb - 30.0f);
    constexpr float maxDb = 0.0f;
    const float gridStep = (maxDb - minDb) > 42.0f ? 12.0f : 6.0f;

    for (float db = maxDb; db >= minDb - 0.1f; db -= gridStep)
    {
        const float y = yForDb (db);
        g.setColour (gridColour.withAlpha (db == 0.0f ? 0.85f : 0.48f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());

        g.setColour (muted.withAlpha (0.82f));
        g.setFont (juce::FontOptions (8.0f));
        g.drawText (juce::String (juce::roundToInt (db)), area.getX() + 8.0f, y - 6.0f, 30.0f, 12.0f,
                    juce::Justification::left);
    }

    const int pointCount = processor.getWaveformPointCount();
    const int latestTriggerPoint = processor.getLatestTriggerPointIndex();

    if (pointCount > 1)
    {
        juce::Path line;
        juce::Path fill;
        const float baseline = plot.getBottom();
        fill.startNewSubPath (plot.getX(), baseline);

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float peakDb = homeSidechain::linearToDb (peak);
            const float y = yForDb (peakDb);
            const float x = plot.getX() + plot.getWidth()
                * (static_cast<float> (i) / static_cast<float> (pointCount - 1));

            if (i == 0)
                line.startNewSubPath (x, y);
            else
                line.lineTo (x, y);
            fill.lineTo (x, y);
        }

        fill.lineTo (plot.getRight(), baseline);
        fill.closeSubPath();

        g.setColour (accent.withAlpha (0.050f));
        g.fillPath (fill);
        g.setColour (accent.withAlpha (0.16f));
        g.strokePath (line, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
        g.setColour (accent.withAlpha (0.92f));
        g.strokePath (line, juce::PathStrokeType (1.35f, juce::PathStrokeType::curved));

        // The latest audio-triggering section is highlighted in red instead of
        // leaving a trail of marker lines. Only one event is highlighted.
        if (latestTriggerPoint >= 0 && latestTriggerPoint < pointCount)
        {
            const int startIndex = juce::jmax (0, latestTriggerPoint - 3);
            const int endIndex = juce::jmin (pointCount - 1, latestTriggerPoint + 4);

            juce::Path highlight;
            for (int i = startIndex; i <= endIndex; ++i)
            {
                const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
                const float peakDb = homeSidechain::linearToDb (peak);
                const float y = yForDb (peakDb);
                const float x = plot.getX() + plot.getWidth()
                    * (static_cast<float> (i) / static_cast<float> (pointCount - 1));
                if (i == startIndex)
                    highlight.startNewSubPath (x, y);
                else
                    highlight.lineTo (x, y);
            }

            g.setColour (triggerHot.withAlpha (0.12f));
            g.strokePath (highlight, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved));
            g.setColour (triggerHot.withAlpha (0.92f));
            g.strokePath (highlight, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved));
        }

    }

    // Threshold is directly mapped to the same dB scale as the waveform.
    const float thresholdDb = processor.getThresholdDb();
    const float thresholdY = yForDb (thresholdDb);

    g.setColour (accent.withAlpha (0.07f));
    g.fillRoundedRectangle (plot.getX(), thresholdY - 3.0f, plot.getWidth(), 6.0f, 3.0f);
    g.setColour (accent);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 1.7f);

    const float handleX = plot.getX() + 10.0f;
    juce::Path handle;
    handle.startNewSubPath (handleX, thresholdY);
    handle.lineTo (handleX + 10.0f, thresholdY - 7.0f);
    handle.lineTo (handleX + 10.0f, thresholdY + 7.0f);
    handle.closeSubPath();
    g.setColour (accent);
    g.fillPath (handle);

    const float labelY = juce::jlimit (plot.getY(), plot.getBottom() - 24.0f, thresholdY - 10.0f);
    g.setColour (surfaceRaised.withAlpha (0.96f));
    g.fillRoundedRectangle (plot.getRight() - 112.0f, labelY - 2.0f, 106.0f, 24.0f, 8.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
    g.drawText ("THRESHOLD  " + juce::String (thresholdDb, 1) + " dB",
                plot.getRight() - 106.0f, labelY + 4.0f, 94.0f, 14.0f,
                juce::Justification::centredRight);

    g.setColour (muted.withAlpha (0.72f));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("PAST", plot.getX(), area.getBottom() - 15.0f, 34.0f, 12.0f,
                juce::Justification::left);
    g.drawText ("NOW", plot.getRight() - 34.0f, area.getBottom() - 15.0f, 34.0f, 12.0f,
                juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::drawControlStrip (juce::Graphics& g,
                                                                  juce::Rectangle<float> area) const
{
    g.setColour (panel);
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (lineColour);
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    const auto meter = processor.getTriggerMeter();
    const bool firing = meter > 0.10f;

    auto statusArea = area.removeFromLeft (148.0f).reduced (12.0f, 8.0f);
    g.setColour (firing ? triggerHot : success);
    g.fillEllipse (statusArea.getX(), statusArea.getCentreY() - 5.0f, 10.0f, 10.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (firing ? "TRIGGERING" : "READY", statusArea.getX() + 17.0f,
                statusArea.getY(), 100.0f, 16.0f, juce::Justification::left);

    g.setColour (lineColour);
    g.drawVerticalLine (juce::roundToInt (area.getX()), area.getY() + 8.0f, area.getBottom() - 8.0f);

    g.setColour (muted.withAlpha (0.60f));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("minimum time between events", area.getX() + 148.0f, area.getY() + 9.0f,
                138.0f, 12.0f, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    const auto outer = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (outer, 16.0f);
    g.setColour (lineColour);
    g.drawRoundedRectangle (outer, 16.0f, 1.0f);

    drawHeader (g, { 22.0f, 16.0f, getWidth() - 44.0f, 44.0f });
    drawGraph (g, graphBounds);
    drawControlStrip (g, { 20.0f, 292.0f, 600.0f, 52.0f });
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    graphBounds = { 20.0f, 64.0f, 600.0f, 216.0f };

    link.setBounds (478, 18, 54, 24);
    bypass.setBounds (538, 18, 82, 24);

    retrigger.setBounds (292, 295, 312, 44);
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // Never steal clicks intended for actual child controls.
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
    repaint();
}
