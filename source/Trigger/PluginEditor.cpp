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
    // Bottom is trimmed to make room for the level bar drawn in
    // drawLevelBar() -- it lives inside this card now rather than in its
    // own Activity card below.
    return getLocalBounds().toFloat().reduced (8.0f, 8.0f).withTrimmedBottom (18.0f);
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

        // A MIDI note carries no audio amplitude, so the bar above this
        // point can be a single pixel even though a trigger really fired --
        // that's what made incoming MIDI invisible on the scope. The
        // full-height tint and the flag at the top don't depend on level,
        // so a MIDI trigger is always visible here even on silent audio.
        if (midi)
        {
            g.setColour (purple.withAlpha (0.22f));
            g.fillRect (juce::Rectangle<float> (x - 1.0f, plot.getY(), step + 2.0f, plot.getHeight()));

            juce::Path flag;
            flag.addTriangle (x, plot.getY(), x + 6.0f, plot.getY(), x, plot.getY() + 8.0f);
            g.setColour (purple);
            g.fillPath (flag);
        }

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

void TriggerScope::drawSendingLamp (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    // Replaces the old standalone Sending card -- a compact lamp inside the
    // graph itself, lit for a moment whenever a trigger actually fires.
    const auto dot = [&g] (juce::Point<float> centre, juce::Colour colour, float alpha)
    {
        if (alpha > 0.02f)
        {
            g.setColour (colour.withAlpha (alpha * 0.35f));
            g.fillEllipse (juce::Rectangle<float> (centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f));
        }

        g.setColour (colour.withAlpha (juce::jmax (0.18f, alpha)));
        g.fillEllipse (juce::Rectangle<float> (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f));
    };

    const float y = plot.getY() + 7.0f;
    const float x = plot.getX() + 4.0f;

    dot ({ x, y }, green, cachedTriggerActivity);
    g.setFont (font (7.5f, false));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText ("SENDING", juce::Rectangle<float> (x + 8.0f, y - 5.0f, 56.0f, 10.0f),
                juce::Justification::centredLeft, false);

    float lx = plot.getRight() - 108.0f;
    dot ({ lx, y }, cyan, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText ("AUDIO", juce::Rectangle<float> (lx + 6.0f, y - 5.0f, 40.0f, 10.0f),
                juce::Justification::centredLeft, false);

    lx += 54.0f;
    dot ({ lx, y }, purple, 1.0f);
    g.drawText ("MIDI", juce::Rectangle<float> (lx + 6.0f, y - 5.0f, 36.0f, 10.0f),
                juce::Justification::centredLeft, false);
}

void TriggerScope::drawLevelBar (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    const auto bounds = getLocalBounds().toFloat();
    const auto bar = juce::Rectangle<float> (plot.getX(), plot.getBottom() + 6.0f, plot.getWidth(),
                                             bounds.getBottom() - (plot.getBottom() + 6.0f));

    drawWell (g, bar, 3.0f);

    auto fillArea = bar.reduced (2.0f);
    const float level = juce::jlimit (0.0f, 1.0f, cachedInputLevel);

    juce::ColourGradient gradient (cyan, fillArea.getX(), fillArea.getY(),
                                   pink, fillArea.getRight(), fillArea.getY(), false);
    g.setGradientFill (gradient);
    g.fillRect (fillArea.withWidth (fillArea.getWidth() * level));

    const float thresholdProportion = (processor.getThresholdDb() - minDb) / (maxDb - minDb);
    const float markX = fillArea.getX() + fillArea.getWidth() * juce::jlimit (0.0f, 1.0f, thresholdProportion);
    g.setColour (green);
    g.drawLine (markX, fillArea.getY() - 1.0f, markX, fillArea.getBottom() + 1.0f, 1.6f);
}

void TriggerScope::setLevels (float inputLevel, float triggerActivity) noexcept
{
    cachedInputLevel = inputLevel;
    cachedTriggerActivity = triggerActivity;
}

void TriggerScope::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot = plotBounds();

    drawWell (g, bounds, 5.0f);
    drawGrid (g, plot);
    drawWaveform (g, plot);
    drawThreshold (g, plot);
    drawSendingLamp (g, plot);
    drawLevelBar (g, plot);
}

void TriggerScope::mouseMove (const juce::MouseEvent& e)
{
    // Not "near": windows.h defines that as an empty macro.
    const bool overLine = std::abs (e.position.y - dbToY (processor.getThresholdDb())) <= 7.0f;

    if (overLine != hovering)
    {
        hovering = overLine;
        setMouseCursor (overLine ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
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

    linkSelector.setFontSize (9.5f);
    linkSelector.onChange = [this] (int index)
    {
        if (auto* parameter = processor.apvts.getParameter ("LINK"))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (index)));

        refreshFromParameters();
    };
    addAndMakeVisible (linkSelector);

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
    linkSelector.setBounds (320, 29, 190, 22);

    power.setBounds (660, 20, 30, 30);

    scope.setBounds (scopeCard().reduced (12.0f, 0.0f)
                                .withTrimmedTop (26.0f)
                                .withTrimmedBottom (10.0f).toNearestInt());

    const auto sense = senseCard().toNearestInt();
    thresholdKnob.setBounds (sense.getX() + 16, sense.getY() + 30, 168, 84);
    cooldownKnob.setBounds (sense.getX() + 16, sense.getY() + 126, 168, 84);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g) const
{
    drawBrand (g, "Sidechain", cyan, 25.0f, 16.0f, 695.0f);

    // More breathing room from the title than before, and both lines share
    // the same weight (regular, not bold) so they read as one consistent
    // subtitle block instead of two mismatched labels -- matches Receiver.
    g.setFont (font (8.5f, false));
    g.setColour (cyan.withAlpha (0.75f));
    g.drawText ("TRIGGER", juce::Rectangle<float> (235.0f, 22.0f, 80.0f, 13.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("NOTE " + juce::MidiMessage::getMidiNoteName (
                    homeSidechain::midiNoteForLink (processor.getLink()), true, true, 3),
                juce::Rectangle<float> (235.0f, 37.0f, 80.0f, 12.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawText ("LINK", juce::Rectangle<float> (286.0f, 33.0f, 34.0f, 14.0f),
                juce::Justification::centredLeft, false);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (chassis);

    g.setColour (face);
    g.fillRoundedRectangle (10.0f, 10.0f, 700.0f, 300.0f, 8.0f);
    g.setColour (faceEdge);
    g.drawRoundedRectangle (10.0f, 10.0f, 700.0f, 300.0f, 8.0f, 1.5f);

    drawHeader (g);

    drawCard (g, scopeCard(), cyan);
    drawCard (g, senseCard(), pink);

    drawCardTitle (g, "INPUT", scopeCard());
    drawCardTitle (g, "SENSITIVITY", senseCard());

    drawCardText (g, "DRAG THE LINE", juce::Rectangle<float> (scopeCard().getRight() - 106.0f,
                                                             scopeCard().getY() + 7.0f, 94.0f, 15.0f),
                  8.5f, juce::Justification::centredRight, 0.55f);

    // Bypass overlay, matching Home-Disto's exactly: dim the whole face
    // plate except the button that turns it back off, and say so in the
    // middle of it.
    if (power.getToggleState())
    {
        g.excludeClipRegion (power.getBounds());

        g.setColour (juce::Colours::black.withAlpha (0.70f));
        g.fillRoundedRectangle (10.0f, 10.0f, 700.0f, 300.0f, 8.0f);

        g.setFont (juce::FontOptions (48.0f).withName ("Helvetica").withStyle ("Bold"));
        g.setColour (juce::Colours::white);
        g.drawText ("BYPASSED", 10, 10, 700, 300, juce::Justification::centred);
    }
}

void HomeSidechainTriggerAudioProcessorEditor::refreshFromParameters()
{
    linkSelector.setSelectedIndex (processor.getLink(), juce::dontSendNotification);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    inputSmoothed = juce::jmax (processor.getInputLevel(), inputSmoothed * 0.72f);
    triggerSmoothed = juce::jmax (processor.getTriggerMeter(), triggerSmoothed * 0.68f);

    // dB-scaled rather than linear, so quiet signals still show meaningful
    // movement on the bar instead of sitting near zero the whole time.
    const float levelProportion = (levelToDb (inputSmoothed) - minDb) / (maxDb - minDb);
    scope.setLevels (levelProportion, triggerSmoothed);

    refreshFromParameters();
    scope.repaint();
    repaint (juce::Rectangle<int> (10, 10, 700, 62));
}
