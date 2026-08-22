#include "PluginEditor.h"

namespace
{
    // Home-series visual language: near-black surfaces, crisp white type,
    // cyan utility accents and neon green as the positive/active accent.
    const juce::Colour background    (0xff09090b);
    const juce::Colour panel         (0xff161618);
    const juce::Colour surface       (0xff121215);
    const juce::Colour surfaceRaised (0xff1a1a1e);
    const juce::Colour lineColour    (0xff2a2a30);
    const juce::Colour gridColour    (0xff1e1e24);
    const juce::Colour accent        (0xff00e5ff);
    const juce::Colour activeAccent  (0xff00ff87);
    const juce::Colour triggerHot    (0xffff5d5d);
    const juce::Colour text          (0xffffffff);
    const juce::Colour muted         (0xffffffff);
}

HomeSidechainTriggerGapSlider::HomeSidechainTriggerGapSlider()
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId, activeAccent);
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

    g.setColour (text.withAlpha (0.55f));
    g.setFont (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("COOL DOWN", labelX, labelY, 90.0f, 14.0f, juce::Justification::left);

    // Use the slider's actual NormalisableRange so the painted thumb exactly
    // matches JUCE's parameter mapping, including the skew used by the APVTS.
    // Use JUCE's own slider mapping so the visual thumb follows the exact
    // same skew/range as the attached parameter.
    const float proportion = static_cast<float> (valueToProportionOfLength (getValue()));
    const float thumbX = trackX + thumbRadius + (trackW - thumbRadius * 2.0f) * proportion;

    const auto track = juce::Rectangle<float> (trackX + thumbRadius,
                                                trackY - trackH * 0.5f,
                                                trackW - thumbRadius * 2.0f,
                                                trackH);

    g.setColour (lineColour);
    g.fillRoundedRectangle (track, trackH * 0.5f);

    const auto filled = track.withWidth (track.getWidth() * proportion);
    g.setColour (activeAccent.withAlpha (0.90f));
    if (filled.getWidth() > 0.1f)
        g.fillRoundedRectangle (filled, trackH * 0.5f);

    // Subtle center line gives the control a physical, hardware-like feel.
    g.setColour (activeAccent.withAlpha (0.13f));
    g.fillEllipse (thumbX - 10.0f, trackY - 10.0f, 20.0f, 20.0f);

    g.setColour (juce::Colours::white);
    g.fillEllipse (thumbX - thumbRadius, trackY - thumbRadius,
                   thumbRadius * 2.0f, thumbRadius * 2.0f);

    const auto valueBox = juce::Rectangle<float> (bounds.getRight() - valueW, 2.0f,
                                                   valueW, 28.0f);
    g.setColour (surfaceRaised);
    g.fillRoundedRectangle (valueBox, 8.0f);
    g.setColour (lineColour);
    g.drawRoundedRectangle (valueBox, 8.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms",
                valueBox.reduced (7.0f, 0.0f).toNearestInt(),
                juce::Justification::centred);
}


HomeSidechainTriggerAudioProcessorEditor::~HomeSidechainTriggerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypefaceName ("Helvetica");
    setSize (720, 410);
    setResizable (false, false);
    setLookAndFeel (&homeSeriesLaf);

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
    link.setColour (juce::ComboBox::textColourId, text.withAlpha (0.86f));
    link.setColour (juce::ComboBox::outlineColourId, lineColour);
    link.setColour (juce::ComboBox::arrowColourId, accent);
    link.setColour (juce::ComboBox::focusedOutlineColourId, accent);
}

void HomeSidechainTriggerAudioProcessorEditor::styleBypass()
{
    bypass.setClickingTogglesState (true);
    bypass.setColour (juce::ToggleButton::textColourId, text.withAlpha (0.66f));
    bypass.setColour (juce::ToggleButton::tickColourId, activeAccent);
    bypass.setColour (juce::ToggleButton::tickDisabledColourId, text.withAlpha (0.25f));
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
    g.setColour (colour.withAlpha (bright ? 0.18f : 0.055f));
    g.fillRoundedRectangle (area, area.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (bright ? 0.92f : 0.28f));
    g.drawRoundedRectangle (area, area.getHeight() * 0.5f, 1.0f);
    g.setColour (bright ? text : text.withAlpha (0.60f));
    g.setFont (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText (label, area.toNearestInt(), juce::Justification::centred);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g,
                                                            juce::Rectangle<float> area) const
{
    const juce::Font titleFont (juce::FontOptions (26.0f).withName ("Helvetica").withStyle ("Bold"));
    const auto homeText = juce::String ("Home-");
    const auto triggerText = juce::String ("Trigger");
    const int homeWidth = juce::GlyphArrangement::getStringWidthInt (titleFont, homeText);
    const int triggerWidth = juce::GlyphArrangement::getStringWidthInt (titleFont, triggerText);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (homeText, area.getX() + 1.0f, area.getY() + 1.0f, homeWidth, 32.0f, juce::Justification::left);
    g.drawText (triggerText, area.getX() + homeWidth + 1.0f, area.getY() + 1.0f, triggerWidth, 32.0f, juce::Justification::left);
    g.setColour (text);
    g.drawText (homeText, area.getX(), area.getY(), homeWidth, 32.0f, juce::Justification::left);
    g.setColour (activeAccent);
    g.drawText (triggerText, area.getX() + homeWidth, area.getY(), triggerWidth, 32.0f, juce::Justification::left);

    g.setColour (text.withAlpha (0.40f));
    g.setFont (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("DUBTACH DSP", area.getX(), area.getY() + 32.0f, 150.0f, 14.0f, juce::Justification::left);

    const auto meter = processor.getTriggerMeter();
    const bool firing = meter > 0.10f;
    drawPill (g, { area.getRight() - 270.0f, area.getY() + 2.0f, 82.0f, 24.0f },
              firing ? triggerHot : activeAccent, firing ? "TRIGGER" : "READY", firing);
    drawPill (g, { area.getRight() - 178.0f, area.getY() + 2.0f, 104.0f, 24.0f },
              accent, "AUDIO + MIDI", false);
}


void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g,
                                                           juce::Rectangle<float> area) const
{
    // Card itself is supplied by paint(). The graph keeps its own dark inset scope,
    // matching the Home-series EQ graph treatment.
    auto plot = area.reduced (36.0f, 30.0f);
    plot.removeFromLeft (10.0f);

    const float thresholdDb = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, thresholdDb - 30.0f);
    constexpr float maxDb = 0.0f;
    const float gridStep = (maxDb - minDb) > 42.0f ? 12.0f : 6.0f;

    for (float db = maxDb; db >= minDb - 0.1f; db -= gridStep)
    {
        const float y = yForDb (db);
        g.setColour (juce::Colours::black.withAlpha (db == 0.0f ? 0.26f : 0.14f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
        g.drawText (juce::String (juce::roundToInt (db)), area.getX() + 7.0f, y - 6.0f, 25.0f, 12.0f,
                    juce::Justification::left);
    }

    if (processor.getWaveformPointCount() > 1)
    {
        const int pointCount = processor.getWaveformPointCount();
        const int latestTriggerPoint = processor.getLatestTriggerPointIndex();
        juce::Path line, fill;
        const float baseline = plot.getBottom();
        fill.startNewSubPath (plot.getX(), baseline);
        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float y = yForDb (homeSidechain::linearToDb (peak));
            const float x = plot.getX() + plot.getWidth() * (float)i / (float)(pointCount - 1);
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (plot.getRight(), baseline); fill.closeSubPath();
        g.setColour (juce::Colour (0xff000000).withAlpha (0.08f)); g.fillPath (fill);
        g.setColour (accent.withAlpha (0.16f)); g.strokePath (line, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
        g.setColour (juce::Colour (0xff09090b)); g.strokePath (line, juce::PathStrokeType (2.3f, juce::PathStrokeType::curved));
        g.setColour (juce::Colours::white.withAlpha (0.92f)); g.strokePath (line, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));

        if (latestTriggerPoint >= 0 && latestTriggerPoint < pointCount)
        {
            const int startIndex = juce::jmax (0, latestTriggerPoint - 5);
            const int endIndex = juce::jmin (pointCount - 1, latestTriggerPoint + 6);
            juce::Path highlight;
            for (int i = startIndex; i <= endIndex; ++i)
            {
                const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
                const float y = yForDb (homeSidechain::linearToDb (peak));
                const float x = plot.getX() + plot.getWidth() * (float)i / (float)(pointCount - 1);
                if (i == startIndex) highlight.startNewSubPath (x, y); else highlight.lineTo (x, y);
            }
            g.setColour (triggerHot.withAlpha (0.34f)); g.strokePath (highlight, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved));
            g.setColour (triggerHot); g.strokePath (highlight, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));
        }
    }

    const float thresholdY = yForDb (thresholdDb);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (plot.getX(), thresholdY - 3.0f, plot.getWidth(), 6.0f, 3.0f);
    g.setColour (juce::Colour (0xff09090b));
    g.drawLine (plot.getX(), thresholdY + 0.8f, plot.getRight(), thresholdY + 0.8f, 3.0f);
    g.setColour (accent);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 1.4f);

    const float labelY = juce::jlimit (plot.getY(), plot.getBottom() - 22.0f, thresholdY - 9.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (plot.getRight() - 108.0f, labelY, 102.0f, 22.0f, 4.0f);
    g.setColour (juce::Colour (0xff09090b));
    g.fillRoundedRectangle (plot.getRight() - 110.0f, labelY - 1.0f, 102.0f, 22.0f, 4.0f);
    g.setFont (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (text);
    g.drawText ("THRESHOLD  " + juce::String (thresholdDb, 1) + " dB", plot.getRight() - 104.0f, labelY + 4.0f, 90.0f, 14.0f, juce::Justification::centredRight);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("INPUT", plot.getX(), area.getBottom() - 17.0f, 40.0f, 12.0f, juce::Justification::left);
    g.drawText ("NOW", plot.getRight() - 32.0f, area.getBottom() - 17.0f, 32.0f, 12.0f, juce::Justification::right);
}


void HomeSidechainTriggerAudioProcessorEditor::drawControlStrip (juce::Graphics& g,
                                                                  juce::Rectangle<float> area) const
{
    g.setColour (juce::Colours::black.withAlpha (0.14f));
    g.drawLine (area.getX() + 6.0f, area.getY() + 2.0f, area.getRight() - 6.0f, area.getY() + 2.0f, 1.0f);
    g.setFont (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.drawText ("COOL DOWN", area.getX(), area.getY(), 100.0f, 18.0f, juce::Justification::left);
    g.setFont (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("MINIMUM TIME BETWEEN TRIGGERS", area.getX(), area.getY() + 17.0f, 180.0f, 12.0f, juce::Justification::left);
}


void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff09090b));

    g.setColour (juce::Colour (0xff111114));
    g.fillRoundedRectangle (10.0f, 10.0f, 700.0f, 390.0f, 8.0f);
    g.setColour (juce::Colour (0xff222228));
    g.drawRoundedRectangle (10.0f, 10.0f, 700.0f, 390.0f, 8.0f, 1.5f);

    drawHeader (g, { 25.0f, 14.0f, 670.0f, 48.0f });
    g.setColour (juce::Colour (0xff1e1e24));
    g.drawLine (25.0f, 65.0f, 695.0f, 65.0f, 2.0f);

    auto drawShadedCard = [&g](juce::Rectangle<float> bounds, juce::Colour baseColour)
    {
        for (int i = 1; i <= 7; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.12f * (8 - i)));
            g.fillRoundedRectangle (bounds.expanded ((float)i * 0.4f).translated (0.0f, 2.0f + i * 0.5f), 8.0f);
        }
        juce::ColourGradient grad (baseColour, bounds.getX(), bounds.getY(),
                                   baseColour.darker (0.20f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 6.0f);
        juce::Rectangle<float> sheenArea = bounds.withHeight (bounds.getHeight() * 0.42f);
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.10f), sheenArea.getX(), sheenArea.getY(),
                                    juce::Colours::white.withAlpha (0.0f), sheenArea.getX(), sheenArea.getBottom(), false);
        juce::Path sheenPath; sheenPath.addRoundedRectangle (bounds, 6.0f);
        g.saveState(); g.reduceClipRegion (sheenPath); g.setGradientFill (sheen); g.fillRect (sheenArea); g.restoreState();
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        for (float y = bounds.getY() + 4.0f; y < bounds.getBottom() - 2.0f; y += 4.0f) g.drawLine (bounds.getX() + 2.0f, y, bounds.getRight() - 2.0f, y, 1.2f);
        for (float x = bounds.getX() + 4.0f; x < bounds.getRight() - 2.0f; x += 4.0f) g.drawLine (x, bounds.getY() + 2.0f, x, bounds.getBottom() - 2.0f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.50f));
        g.drawRoundedRectangle (bounds, 6.0f, 2.0f);
    };

    // Exact Home-series card language: cyan graph, green timing control,
    // purple link/status column, pink output/bypass area.
    const juce::Rectangle<float> graphCard (20.0f, 80.0f, 490.0f, 255.0f);
    const juce::Rectangle<float> controlCard (20.0f, 340.0f, 490.0f, 50.0f);
    const juce::Rectangle<float> statusCard (520.0f, 80.0f, 90.0f, 310.0f);
    const juce::Rectangle<float> bypassCard (620.0f, 80.0f, 80.0f, 310.0f);

    drawShadedCard (graphCard, juce::Colour (0xff00e5ff));
    drawShadedCard (controlCard, juce::Colour (0xff00ff87));
    drawShadedCard (statusCard, juce::Colour (0xffb900ff));
    drawShadedCard (bypassCard, juce::Colour (0xffff007f));

    // Graph card title.
    g.setFont (juce::FontOptions (14.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText ("TRIGGER GRAPH", 20, 88, 490, 20, juce::Justification::centred);
    g.setColour (juce::Colour (0xff09090b));
    g.drawText ("TRIGGER GRAPH", 20, 86, 490, 20, juce::Justification::centred);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (225.0f, 108.0f, 305.0f, 108.0f, 1.2f);

    auto graphInner = graphCard.reduced (12.0f, 34.0f);
    drawGraph (g, graphInner);

    // Green timing card.
    g.setFont (juce::FontOptions (14.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (juce::Colour (0xff09090b));
    g.drawText ("TIMING", 20, 346, 120, 18, juce::Justification::centred);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (62.0f, 365.0f, 98.0f, 365.0f, 1.2f);

    // Purple status/link card.
    g.setFont (juce::FontOptions (14.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (juce::Colour (0xff09090b));
    g.drawText ("LINK", 520, 90, 90, 20, juce::Justification::centred);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (548.0f, 111.0f, 582.0f, 111.0f, 1.2f);

    const bool firing = processor.getTriggerMeter() > 0.10f;
    g.setColour (firing ? juce::Colour (0xffff4f70) : juce::Colour (0xff00ff87));
    g.fillEllipse (545.0f, 132.0f, 40.0f, 40.0f);
    g.setColour (juce::Colour (0xff09090b));
    g.fillEllipse (551.0f, 138.0f, 28.0f, 28.0f);
    g.setFont (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold"));
    g.setColour (juce::Colour (0xff09090b));
    g.drawText (firing ? "TRIGGER" : "READY", 522, 178, 86, 18, juce::Justification::centred);

    // Pink bypass card.
    g.setColour (juce::Colour (0xff09090b));
    g.setFont (juce::FontOptions (14.0f).withName ("Helvetica").withStyle ("Bold"));
    g.drawText ("OUTPUT", 620, 90, 80, 20, juce::Justification::centred);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (642.0f, 111.0f, 678.0f, 111.0f, 1.2f);
    g.setColour (juce::Colour (0xff09090b));
    g.drawText ("AUDIO", 622, 144, 76, 18, juce::Justification::centred);
    g.drawText ("+ MIDI", 622, 160, 76, 18, juce::Justification::centred);

    drawControlStrip (g, controlCard.reduced (14.0f, 6.0f));
}


void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    graphBounds = { 32.0f, 126.0f, 466.0f, 194.0f };
    link.setBounds (533, 206, 64, 26);
    bypass.setBounds (628, 204, 64, 26);
    retrigger.setBounds (156, 349, 340, 38);
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
