#include "PluginEditor.h"
#include <cmath>

using namespace homeUI;

namespace
{
    constexpr float minDb = -48.0f;
    constexpr float maxDb = 0.0f;

    float levelToDb (float linear) noexcept
    {
        return juce::jlimit (minDb, maxDb,
                             juce::Decibels::gainToDecibels (juce::jmax (linear, 0.0000025f)));
    }
}

// =============================================================================
// TriggerScope
// =============================================================================

TriggerScope::TriggerScope (HomeSidechainTriggerAudioProcessor& p)
    : processor (p)
{
}

juce::Rectangle<float> TriggerScope::plotBounds() const noexcept
{
    return getLocalBounds().toFloat().reduced (8.0f, 8.0f);
}

float TriggerScope::dbToY (float db) const noexcept
{
    const auto plot = plotBounds();
    const float normalised = (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb);
    return plot.getBottom() - normalised * plot.getHeight();
}

float TriggerScope::yToDb (float y) const noexcept
{
    const auto plot = plotBounds();
    const float normalised = juce::jlimit (0.0f, 1.0f, (plot.getBottom() - y) / juce::jmax (1.0f, plot.getHeight()));
    return minDb + normalised * (maxDb - minDb);
}

void TriggerScope::setThresholdFromY (float y, bool fine)
{
    auto* parameter = processor.apvts.getParameter ("THRESHOLD");

    if (parameter == nullptr)
        return;

    float target = yToDb (y);

    if (fine)
        target = processor.getThresholdDb() + (target - processor.getThresholdDb()) * 0.25f;

    parameter->setValueNotifyingHost (parameter->convertTo0to1 (juce::jlimit (minDb, maxDb, target)));
    repaint();
}

void TriggerScope::drawGrid (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    static const float marks[] = { -6.0f, -18.0f, -30.0f, -42.0f };

    for (auto db : marks)
    {
        const float y = dbToY (db);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    // One vertical line per second of the three-second history.
    const int seconds = juce::roundToInt (HomeSidechainTriggerAudioProcessor::waveformHistorySeconds);

    for (int i = 1; i < seconds; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (static_cast<float> (i) / static_cast<float> (seconds));
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());

        g.setFont (font (7.5f, false));
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawText ("-" + juce::String (seconds - i) + "s",
                    juce::Rectangle<float> (x + 3.0f, plot.getBottom() - 12.0f, 24.0f, 10.0f),
                    juce::Justification::centredLeft, false);
    }
}

void TriggerScope::drawWaveform (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    const int count = processor.getWaveformPointCount();

    if (count <= 1)
        return;

    const float step = plot.getWidth() / static_cast<float> (count - 1);
    const float baseline = plot.getBottom();

    for (int i = 0; i < count; ++i)
    {
        const float level = processor.getWaveformPoint (i);
        const float y = dbToY (levelToDb (level));
        const float x = plot.getX() + step * static_cast<float> (i);
        const float height = juce::jmax (1.0f, baseline - y);

        const bool fired = processor.getWaveformTriggered (i);
        const bool midi = processor.getWaveformMidiInput (i);

        const auto colour = fired ? pink : (midi ? purple : cyan);
        g.setColour (colour.withAlpha (fired || midi ? 0.95f : 0.55f));
        g.fillRect (juce::Rectangle<float> (x, y, juce::jmax (1.0f, step * 0.9f), height));

        if (fired)
        {
            g.setColour (pink.withAlpha (0.28f));
            g.fillRect (juce::Rectangle<float> (x - 1.0f, plot.getY(), step + 2.0f, plot.getHeight()));
        }
    }
}

void TriggerScope::drawThreshold (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    const float db = processor.getThresholdDb();
    const float y = dbToY (db);
    const bool live = dragging || hovering;

    g.setColour (green.withAlpha (0.10f));
    g.fillRect (juce::Rectangle<float> (plot.getX(), plot.getY(), plot.getWidth(), y - plot.getY()));

    g.setColour (green.withAlpha (live ? 1.0f : 0.80f));

    for (float x = plot.getX(); x < plot.getRight(); x += 8.0f)
        g.drawLine (x, y, juce::jmin (x + 4.5f, plot.getRight()), y, live ? 2.2f : 1.6f);

    const auto badge = juce::Rectangle<float> (plot.getRight() - 54.0f, y - 9.0f, 50.0f, 18.0f);
    g.setColour (green);
    g.fillRoundedRectangle (badge, 4.0f);
    g.setFont (font (9.5f));
    g.setColour (ink);
    g.drawText (juce::String (db, 1) + " dB", badge, juce::Justification::centred, false);
}

void TriggerScope::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot = plotBounds();

    drawWell (g, bounds, 5.0f);
    drawGrid (g, plot);
    drawWaveform (g, plot);
    drawThreshold (g, plot);
}

void TriggerScope::mouseMove (const juce::MouseEvent& e)
{
    const bool near = std::abs (e.position.y - dbToY (processor.getThresholdDb())) <= 7.0f;

    if (near != hovering)
    {
        hovering = near;
        setMouseCursor (near ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void TriggerScope::mouseExit (const juce::MouseEvent&)
{
    hovering = false;
    repaint();
}

void TriggerScope::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    setThresholdFromY (e.position.y, e.mods.isShiftDown());
}

void TriggerScope::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        setThresholdFromY (e.position.y, e.mods.isShiftDown());
}

void TriggerScope::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    repaint();
}

void TriggerScope::mouseDoubleClick (const juce::MouseEvent&)
{
    if (auto* parameter = processor.apvts.getParameter ("THRESHOLD"))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (-18.0f));

    repaint();
}

// =============================================================================
// Editor
// =============================================================================

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (
    HomeSidechainTriggerAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), scope (p)
{
    addAndMakeVisible (scope);

    for (int i = 0; i < homeSidechain::numberOfLinks; ++i)
    {
        auto pill = std::make_unique<Pill> (homeSidechain::linkName (i), cyan);
        pill->setFontSize (9.5f);
        pill->setCornerRadius (4.0f);
        pill->onClick = [this, i]
        {
            if (auto* parameter = processor.apvts.getParameter ("LINK"))
                parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (i)));

            refreshFromParameters();
        };
        addAndMakeVisible (*pill);
        linkPills[static_cast<size_t> (i)] = std::move (pill);
    }

    testPill.setFontSize (9.5f);
    testPill.onClick = [this] { processor.requestTestTrigger(); };
    addAndMakeVisible (testPill);

    addAndMakeVisible (power);

    thresholdKnob.valueText = [] (double value) { return juce::String (value, 1) + "dB"; };
    cooldownKnob.valueText = [] (double value) { return juce::String (juce::roundToInt (value)) + "ms"; };
    addAndMakeVisible (thresholdKnob);
    addAndMakeVisible (cooldownKnob);

    thresholdAttachment = std::make_unique<SliderAttachment> (processor.apvts, "THRESHOLD", thresholdKnob);
    cooldownAttachment = std::make_unique<SliderAttachment> (processor.apvts, "RETRIGGER", cooldownKnob);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "BYPASS", power);

    setSize (designWidth, designHeight);
    refreshFromParameters();
    startTimerHz (30);
}

HomeSidechainTriggerAudioProcessorEditor::~HomeSidechainTriggerAudioProcessorEditor()
{
    stopTimer();
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    int x = 320;

    for (auto& pill : linkPills)
    {
        pill->setBounds (x, 30, 20, 20);
        x += 23;
    }

    testPill.setBounds (582, 29, 44, 22);
    power.setBounds (678, 27, 26, 26);

    scope.setBounds (scopeCard().reduced (12.0f, 0.0f)
                                .withTrimmedTop (26.0f)
                                .withTrimmedBottom (10.0f).toNearestInt());

    const auto sense = senseCard().toNearestInt();
    thresholdKnob.setBounds (sense.getX() + 32, sense.getY() + 28, 136, 82);
    cooldownKnob.setBounds (sense.getX() + 32, sense.getY() + 114, 136, 80);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g) const
{
    drawBrand (g, "Sidechain", cyan, 25.0f, 16.0f, 695.0f);

    g.setFont (font (8.5f));
    g.setColour (cyan.withAlpha (0.75f));
    g.drawText ("TRIGGER", juce::Rectangle<float> (202.0f, 22.0f, 80.0f, 13.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("NOTE " + juce::MidiMessage::getMidiNoteName (
                    homeSidechain::midiNoteForLink (processor.getLink()), true, true, 3),
                juce::Rectangle<float> (202.0f, 37.0f, 80.0f, 12.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawText ("LINK", juce::Rectangle<float> (286.0f, 33.0f, 34.0f, 14.0f),
                juce::Justification::centredLeft, false);

    drawLamp (g, juce::Rectangle<float> (512.0f, 33.0f, 64.0f, 14.0f), "SENDING", green,
              triggerSmoothed, false);
}

void HomeSidechainTriggerAudioProcessorEditor::drawMeters (juce::Graphics& g) const
{
    const auto card = meterCard();

    auto well = juce::Rectangle<float> (card.getX() + 14.0f, card.getY() + 30.0f,
                                        card.getWidth() - 28.0f, 30.0f);
    drawWell (g, well, 4.0f);

    const float threshold = (processor.getThresholdDb() - minDb) / (maxDb - minDb);
    const float level = (levelToDb (inputSmoothed) - minDb) / (maxDb - minDb);
    auto bar = well.reduced (4.0f, 5.0f);

    juce::ColourGradient meter (cyan, bar.getX(), bar.getY(),
                                pink, bar.getRight(), bar.getY(), false);
    g.setGradientFill (meter);
    g.fillRoundedRectangle (bar.withWidth (juce::jmax (2.0f, bar.getWidth() * juce::jlimit (0.0f, 1.0f, level))), 3.0f);

    const float markX = bar.getX() + bar.getWidth() * juce::jlimit (0.0f, 1.0f, threshold);
    g.setColour (green);
    g.drawLine (markX, well.getY() + 2.0f, markX, well.getBottom() - 2.0f, 2.0f);

    drawCardText (g, "INPUT", juce::Rectangle<float> (card.getX() + 14.0f, card.getY() + 62.0f, 60.0f, 12.0f),
                  8.5f, juce::Justification::centredLeft, 0.7f);
    drawCardText (g, "THRESHOLD", juce::Rectangle<float> (markX - 40.0f, card.getY() + 62.0f, 80.0f, 12.0f),
                  8.5f, juce::Justification::centred, 0.7f);

    const auto stat = [&] (const juce::String& label, const juce::String& value, float cx)
    {
        drawCardText (g, value, juce::Rectangle<float> (cx - 50.0f, card.getBottom() - 42.0f, 100.0f, 18.0f), 15.0f);
        drawCardText (g, label, juce::Rectangle<float> (cx - 50.0f, card.getBottom() - 24.0f, 100.0f, 12.0f),
                      8.0f, juce::Justification::centred, 0.65f);
    };

    stat ("TRIGGERS SENT", juce::String (processor.homeLinkCount.load (std::memory_order_relaxed)),
          card.getX() + 110.0f);
    stat ("DROPPED", juce::String (processor.getHomeLinkDroppedCount()), card.getX() + 250.0f);
    stat ("COOL DOWN", juce::String (juce::roundToInt (
              processor.apvts.getRawParameterValue ("RETRIGGER")->load())) + " ms",
          card.getX() + 380.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawSend (juce::Graphics& g) const
{
    const auto card = sendCard();

    auto lamp = juce::Rectangle<float> (card.getCentreX() - 26.0f, card.getY() + 34.0f, 52.0f, 52.0f);
    const float activity = juce::jlimit (0.0f, 1.0f, triggerSmoothed);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (lamp);

    if (activity > 0.02f)
    {
        g.setColour (juce::Colours::white.withAlpha (activity * 0.35f));
        g.fillEllipse (lamp.expanded (activity * 10.0f));
    }

    g.setColour (juce::Colours::white.withAlpha (0.25f + activity * 0.75f));
    g.fillEllipse (lamp.reduced (10.0f));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (lamp, 2.0f);

    drawCardText (g, "LINK " + homeSidechain::linkName (processor.getLink()),
                  juce::Rectangle<float> (card.getX(), card.getBottom() - 32.0f, card.getWidth(), 16.0f), 12.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (chassis);

    g.setColour (face);
    g.fillRoundedRectangle (10.0f, 10.0f, 700.0f, 410.0f, 8.0f);
    g.setColour (faceEdge);
    g.drawRoundedRectangle (10.0f, 10.0f, 700.0f, 410.0f, 8.0f, 1.5f);

    drawHeader (g);

    drawCard (g, scopeCard(), cyan);
    drawCard (g, senseCard(), pink);
    drawCard (g, meterCard(), purple);
    drawCard (g, sendCard(), green);

    drawCardTitle (g, "INPUT", scopeCard());
    drawCardTitle (g, "SENSITIVITY", senseCard());
    drawCardTitle (g, "ACTIVITY", meterCard());
    drawCardTitle (g, "SENDING", sendCard());

    drawCardText (g, "DRAG THE LINE", juce::Rectangle<float> (scopeCard().getRight() - 106.0f,
                                                             scopeCard().getY() + 7.0f, 94.0f, 15.0f),
                  8.5f, juce::Justification::centredRight, 0.55f);

    drawMeters (g);
    drawSend (g);

}

void HomeSidechainTriggerAudioProcessorEditor::refreshFromParameters()
{
    const int link = processor.getLink();

    for (int i = 0; i < homeSidechain::numberOfLinks; ++i)
        linkPills[static_cast<size_t> (i)]->setToggleState (i == link, juce::dontSendNotification);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    inputSmoothed = juce::jmax (processor.getInputLevel(), inputSmoothed * 0.72f);
    triggerSmoothed = juce::jmax (processor.getTriggerMeter(), triggerSmoothed * 0.68f);

    refreshFromParameters();
    scope.repaint();
    repaint (juce::Rectangle<int> (10, 10, 700, 62));
    repaint (meterCard().toNearestInt());
    repaint (sendCard().toNearestInt());
}
