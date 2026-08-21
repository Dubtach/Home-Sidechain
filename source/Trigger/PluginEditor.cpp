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
    const juce::Colour trigger       (0xffffc85a);
    const juce::Colour triggerHot    (0xffff8350);
    const juce::Colour text          (0xfff3f6f8);
    const juce::Colour muted         (0xff8e98a4);
    const juce::Colour success       (0xff69e1a4);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (640, 360);
    setResizable (false, false);

    addAndMakeVisible (sensitivity);
    addAndMakeVisible (retrigger);
    addAndMakeVisible (link);
    addAndMakeVisible (bypass);

    styleSlider (sensitivity);
    styleSlider (retrigger);
    styleComboBox();
    styleBypass();

    sensitivity.setNumDecimalPlacesToDisplay (0);
    sensitivity.setTextValueSuffix ("%");
    retrigger.setNumDecimalPlacesToDisplay (0);
    retrigger.setTextValueSuffix (" ms");

    link.addItemList (homeSidechain::linkNames(), 1);

    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "SENSITIVITY", sensitivity);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "RETRIGGER", retrigger);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "LINK", link);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "BYPASS", bypass);

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 24);
    slider.setColour (juce::Slider::backgroundColourId, lineColour);
    slider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.82f));
    slider.setColour (juce::Slider::thumbColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, surfaceRaised);
    slider.setColour (juce::Slider::textBoxOutlineColourId, lineColour);
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
    constexpr float minDb = -48.0f;
    constexpr float maxDb = 0.0f;
    if (graphBounds.getHeight() <= 0.0f)
        return graphBounds.getBottom();

    const float n = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
    return graphBounds.getBottom() - n * graphBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    constexpr float minDb = -48.0f;
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

    drawPill (g, { area.getRight() - 176.0f, area.getY() + 1.0f, 76.0f, 24.0f },
              firing ? triggerHot : success,
              firing ? "TRIGGER" : "READY", firing);
    drawPill (g, { area.getRight() - 92.0f, area.getY() + 1.0f, 92.0f, 24.0f },
              accent, "AUDIO + MIDI", false);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("LINK " + homeSidechain::linkName (linkIndex)
                    + "  •  " + juce::String (processor.getTriggerCount()) + " triggers",
                area.getRight() - 286.0f, area.getY() + 26.0f, 286.0f, 14.0f,
                juce::Justification::right);
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

    // Quiet, readable grid: only useful reference lines.
    for (int db : { 0, -12, -24, -36, -48 })
    {
        const float y = yForDb (static_cast<float> (db));
        g.setColour (gridColour.withAlpha (db == 0 ? 0.85f : 0.55f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());

        g.setColour (muted.withAlpha (0.82f));
        g.setFont (juce::FontOptions (8.0f));
        g.drawText (juce::String (db), area.getX() + 8.0f, y - 6.0f, 28.0f, 12.0f,
                    juce::Justification::left);
    }

    const int pointCount = processor.getWaveformPointCount();
    if (pointCount > 1)
    {
        juce::Path line;
        juce::Path fill;
        const float baseline = plot.getBottom();
        fill.startNewSubPath (plot.getX(), baseline);

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float visual = std::pow (peak, 0.42f);
            const float x = plot.getX() + plot.getWidth()
                * (static_cast<float> (i) / static_cast<float> (pointCount - 1));
            const float y = baseline - visual * plot.getHeight() * 0.92f;

            if (i == 0)
                line.startNewSubPath (x, y);
            else
                line.lineTo (x, y);
            fill.lineTo (x, y);
        }

        fill.lineTo (plot.getRight(), baseline);
        fill.closeSubPath();

        g.setColour (accent.withAlpha (0.055f));
        g.fillPath (fill);
        g.setColour (accent.withAlpha (0.20f));
        g.strokePath (line, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
        g.setColour (accent.withAlpha (0.90f));
        g.strokePath (line, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
    }

    const float rawThreshold = processor.getThresholdDb();
    const float activeThreshold = rawThreshold - processor.getSensitivity() * 12.0f;
    const float thresholdY = yForDb (activeThreshold);

    // Threshold is the one direct-manipulation control of the plugin.
    g.setColour (accent.withAlpha (0.09f));
    g.fillRoundedRectangle (plot.getX(), thresholdY - 3.0f, plot.getWidth(), 6.0f, 3.0f);
    g.setColour (accent);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 2.0f);

    const float handleX = plot.getX() + 10.0f;
    juce::Path handle;
    handle.startNewSubPath (handleX, thresholdY);
    handle.lineTo (handleX + 10.0f, thresholdY - 7.0f);
    handle.lineTo (handleX + 10.0f, thresholdY + 7.0f);
    handle.closeSubPath();
    g.setColour (accent);
    g.fillPath (handle);

    g.setColour (surfaceRaised.withAlpha (0.96f));
    g.fillRoundedRectangle (plot.getRight() - 112.0f, thresholdY - 12.0f, 106.0f, 24.0f, 8.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
    g.drawText (juce::String (activeThreshold, 1) + " dB",
                plot.getRight() - 104.0f, thresholdY - 7.0f, 90.0f, 14.0f,
                juce::Justification::centredRight);

    // Recent events: a small, deliberately bounded visual trail.
    const auto markerMask = processor.getTriggerMarkers();
    constexpr int maxVisibleMarkers = 6;
    int visibleMarkers = 0;
    for (int i = static_cast<int> (markerMask.size()) - 1;
         i >= 0 && visibleMarkers < maxVisibleMarkers; --i)
    {
        if (markerMask[static_cast<size_t> (i)] == 0)
            continue;

        const float x = plot.getX() + plot.getWidth()
            * (static_cast<float> (i) / static_cast<float> (markerMask.size() - 1));
        const float age = static_cast<float> (visibleMarkers);
        const float alpha = juce::jlimit (0.18f, 0.82f, 0.72f - age * 0.09f);

        g.setColour (trigger.withAlpha (alpha));
        g.drawLine (x, plot.getY() + 12.0f, x, plot.getBottom(),
                    visibleMarkers == 0 ? 2.0f : 1.0f);
        g.setColour (trigger.withAlpha (alpha + 0.08f));
        g.fillEllipse (x - 3.0f, plot.getY() + 8.0f, 6.0f, 6.0f);
        ++visibleMarkers;
    }

    // Current-event flash is deliberately separate from the history markers.
    const float meter = processor.getTriggerMeter();
    const float rightX = plot.getRight() - 2.0f;
    const float flash = juce::jlimit (0.0f, 1.0f, meter);
    g.setColour (triggerHot.withAlpha (0.20f + 0.70f * flash));
    g.fillRoundedRectangle (rightX - 2.0f, plot.getY(), 4.0f, plot.getHeight(), 2.0f);

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

    auto meterBox = area.removeFromLeft (118.0f).reduced (12.0f, 8.0f);
    const auto meter = processor.getTriggerMeter();
    const bool firing = meter > 0.10f;
    g.setColour (firing ? triggerHot : success);
    g.fillEllipse (meterBox.getX(), meterBox.getCentreY() - 5.0f, 10.0f, 10.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (firing ? "TRIGGERING" : "READY", meterBox.getX() + 17.0f,
                meterBox.getY(), 80.0f, 16.0f, juce::Justification::left);

    g.setColour (lineColour);
    g.drawVerticalLine (juce::roundToInt (area.getX()), area.getY() + 8.0f, area.getBottom() - 8.0f);

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
    g.drawText ("SENSITIVITY", area.getX() + 16.0f, area.getY() + 7.0f, 80.0f, 12.0f,
                juce::Justification::left);
    g.drawText ("RETRIGGER", area.getCentreX() + 8.0f, area.getY() + 7.0f, 80.0f, 12.0f,
                juce::Justification::left);
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

    link.setBounds (486, 18, 54, 24);
    bypass.setBounds (548, 18, 72, 24);

    sensitivity.setBounds (156, 316, 168, 24);
    retrigger.setBounds (416, 316, 166, 24);
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
    repaint();
}
