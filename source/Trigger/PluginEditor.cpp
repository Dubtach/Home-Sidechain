#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0        (0xff07080a);
    const juce::Colour bg1        (0xff101217);
    const juce::Colour panel      (0xff14171d);
    const juce::Colour panel2     (0xff191d24);
    const juce::Colour edge       (0xff2b3139);
    const juce::Colour white      (0xfff5f7fa);
    const juce::Colour muted      (0xff9ca4b0);
    const juce::Colour cyan       (0xff19d7ff);
    const juce::Colour green      (0xff43ef9b);
    const juce::Colour magenta    (0xffff4fa3);
    const juce::Colour red        (0xffff5f64);
    const juce::Colour yellow     (0xffffd95a);
    const juce::Colour black      (0xff07090c);

    juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica").withStyle (bold ? "Bold" : "Plain"));
    }

    void drawSoftShadow (juce::Graphics& g, juce::Rectangle<float> r, float radius)
    {
        g.setColour (juce::Colours::black.withAlpha (0.42f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), radius + 1.0f);
    }
}

HomeSeriesTriggerLookAndFeel::HomeSeriesTriggerLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panel2);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::textColourId, white);
    setColour (juce::ComboBox::arrowColourId, cyan);
    setColour (juce::ToggleButton::textColourId, white);
    setColour (juce::ToggleButton::tickColourId, green);
}

void HomeSeriesTriggerLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                                  int buttonX, int buttonY, int buttonW, int buttonH,
                                                  juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f);
    auto fill = box.hasKeyboardFocus (true) || isButtonDown ? panel2.brighter (0.08f) : panel2;

    drawSoftShadow (g, r, 6.0f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    g.setColour (cyan.withAlpha (0.95f));
    juce::Path arrow;
    const auto cx = (float) buttonX + (float) buttonW * 0.5f;
    const auto cy = (float) buttonY + (float) buttonH * 0.5f;
    arrow.addTriangle (cx - 4.0f, cy - 1.5f, cx + 4.0f, cy - 1.5f, cx, cy + 3.0f);
    g.fillPath (arrow);
}

void HomeSeriesTriggerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                       bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool bypassed = button.getToggleState();

    if (button.getName() == "BYPASS_SWITCH")
    {
        g.setFont (font (9.5f, true));
        g.setColour (white.withAlpha (0.72f));
        g.drawText ("BYPASS", bounds.removeFromLeft (48.0f).toNearestInt(), juce::Justification::centredLeft);

        const float switchW = 44.0f;
        const float switchH = 20.0f;
        const auto sw = juce::Rectangle<float> (bounds.getRight() - switchW, bounds.getCentreY() - switchH * 0.5f,
                                                switchW, switchH);

        g.setColour (black.withAlpha (0.5f));
        g.fillRoundedRectangle (sw.translated (0.0f, 1.5f), 10.0f);
        g.setColour (bypassed ? red.withAlpha (0.18f) : panel2);
        g.fillRoundedRectangle (sw, 10.0f);
        g.setColour (bypassed ? red.withAlpha (0.85f) : edge.brighter (0.15f));
        g.drawRoundedRectangle (sw, 10.0f, 1.0f);

        const float knob = 14.0f;
        const float leftX = sw.getX() + 3.0f;
        const float rightX = sw.getRight() - knob - 3.0f;
        const float x = bypassed ? rightX : leftX;
        const auto dot = juce::Rectangle<float> (x, sw.getCentreY() - knob * 0.5f, knob, knob);
        g.setColour (bypassed ? red : white);
        g.fillEllipse (dot);
        g.setColour (black.withAlpha (0.22f));
        g.drawEllipse (dot, 1.0f);

        g.setFont (font (7.0f, true));
        g.setColour (bypassed ? red : muted);
        g.drawText (bypassed ? "ON" : "OFF", sw.toNearestInt(), juce::Justification::centred);

        if (highlighted)
        {
            g.setColour (white.withAlpha (0.07f));
            g.drawRoundedRectangle (sw.expanded (2.0f), 12.0f, 1.0f);
        }
        return;
    }

    g.setColour (green);
    g.fillRoundedRectangle (bounds.removeFromLeft (14.0f).withHeight (14.0f).withY (bounds.getCentreY() - 7.0f), 3.0f);
    g.setFont (font (9.0f, bypassed));
    g.setColour (bypassed ? white : white.withAlpha (0.62f));
    g.drawText (button.getButtonText(), bounds.toNearestInt(), juce::Justification::centredLeft);
}

HomeSidechainTriggerGapSlider::HomeSidechainTriggerGapSlider()
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId, green);
}

void HomeSidechainTriggerGapSlider::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float valueW = 62.0f;
    const float x0 = 78.0f;
    const float x1 = b.getRight() - valueW - 6.0f;
    const float cy = b.getCentreY() + 5.0f;
    const float h = 5.0f;
    const float thumbR = 7.5f;
    const double proportion = valueToProportionOfLength (getValue());
    const float px = x0 + (x1 - x0) * (float) proportion;

    g.setFont (font (9.0f, true));
    g.setColour (black.withAlpha (0.78f));
    g.drawText ("COOL DOWN", 10, 0, 82, 18, juce::Justification::left);
    g.setFont (font (7.0f, true));
    g.setColour (black.withAlpha (0.52f));
    g.drawText ("MINIMUM TIME", 10, 15, 82, 10, juce::Justification::left);

    const auto track = juce::Rectangle<float> (x0, cy - h * 0.5f, x1 - x0, h);
    g.setColour (black.withAlpha (0.22f));
    g.fillRoundedRectangle (track, h * 0.5f);

    auto fill = track.withWidth ((px - track.getX()) + 0.1f);
    g.setColour (green.withAlpha (0.90f));
    g.fillRoundedRectangle (fill, h * 0.5f);

    g.setColour (green.withAlpha (0.12f));
    g.fillEllipse (px - 12.0f, cy - 12.0f, 24.0f, 24.0f);
    g.setColour (white);
    g.fillEllipse (px - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);

    const auto valueBox = juce::Rectangle<float> (b.getRight() - valueW, 1.0f, valueW, 26.0f);
    g.setColour (black.withAlpha (0.18f));
    g.fillRoundedRectangle (valueBox, 7.0f);
    g.setColour (black.withAlpha (0.22f));
    g.drawRoundedRectangle (valueBox, 7.0f, 1.0f);
    g.setColour (black);
    g.setFont (font (9.0f, true));
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms", valueBox.toNearestInt(), juce::Justification::centred);
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
    setSize (640, 340);
    setResizable (false, false);
    setLookAndFeel (&homeSeriesLaf);

    addAndMakeVisible (retrigger);
    addAndMakeVisible (link);
    addAndMakeVisible (bypass);

    styleComboBox();
    styleBypass();
    link.addItemList (homeSidechain::linkNames(), 1);

    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RETRIGGER", retrigger);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LINK", link);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleComboBox()
{
    link.setJustificationType (juce::Justification::centredLeft);
    link.setColour (juce::ComboBox::backgroundColourId, panel2);
    link.setColour (juce::ComboBox::textColourId, white);
    link.setColour (juce::ComboBox::outlineColourId, edge);
    link.setColour (juce::ComboBox::arrowColourId, cyan);
    link.setColour (juce::ComboBox::focusedOutlineColourId, cyan);
}

void HomeSidechainTriggerAudioProcessorEditor::styleBypass()
{
    bypass.setClickingTogglesState (true);
    bypass.setName ("BYPASS_SWITCH");
    bypass.setTooltip ("Bypass the Trigger engine");
    bypass.setColour (juce::ToggleButton::tickColourId, green);
}

float HomeSidechainTriggerAudioProcessorEditor::yForDb (float db) const noexcept
{
    const float threshold = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, threshold - 30.0f);
    const float maxDb = juce::jmin (0.0f, threshold + 12.0f);
    const float span = juce::jmax (1.0f, maxDb - minDb);
    const float n = juce::jlimit (0.0f, 1.0f, (db - minDb) / span);
    return graphBounds.getBottom() - n * graphBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    const float threshold = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, threshold - 30.0f);
    const float maxDb = juce::jmin (0.0f, threshold + 12.0f);
    const float span = juce::jmax (1.0f, maxDb - minDb);
    const float n = juce::jlimit (0.0f, 1.0f, (graphBounds.getBottom() - y) / graphBounds.getHeight());
    return minDb + n * span;
}

void HomeSidechainTriggerAudioProcessorEditor::setThresholdFromY (float y)
{
    const float db = thresholdForY (juce::jlimit (graphBounds.getY(), graphBounds.getBottom(), y));
    if (auto* param = processor.apvts.getParameter ("THRESHOLD"))
        param->setValueNotifyingHost (param->convertTo0to1 (db));
}

void HomeSidechainTriggerAudioProcessorEditor::drawTinyStatus (juce::Graphics& g, juce::Rectangle<float> area,
                                                                const juce::String& label, juce::Colour colour,
                                                                bool active) const
{
    g.setColour (colour.withAlpha (active ? 0.14f : 0.07f));
    g.fillRoundedRectangle (area, area.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (active ? 0.88f : 0.42f));
    g.drawRoundedRectangle (area, area.getHeight() * 0.5f, 1.0f);
    g.setColour (colour);
    g.fillEllipse (area.getX() + 7.0f, area.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setFont (font (7.5f, true));
    g.drawText (label, area.getX() + 19.0f, area.getY(), area.getWidth() - 23.0f, area.getHeight(), juce::Justification::centredLeft);
}

void HomeSidechainTriggerAudioProcessorEditor::drawBackgroundTexture (juce::Graphics& g, juce::Rectangle<float> area) const
{
    juce::ColourGradient glow1 (cyan.withAlpha (0.055f), area.getX(), area.getY(),
                                juce::Colours::transparentBlack, area.getRight(), area.getBottom(), true);
    g.setGradientFill (glow1);
    g.fillEllipse (area.getX() - 50.0f, area.getY() - 70.0f, area.getWidth() * 0.95f, area.getHeight() * 0.90f);

    juce::ColourGradient glow2 (magenta.withAlpha (0.045f), area.getRight(), area.getY() + 25.0f,
                                juce::Colours::transparentBlack, area.getX() + area.getWidth() * 0.62f, area.getBottom(), true);
    g.setGradientFill (glow2);
    g.fillEllipse (area.getRight() - 230.0f, area.getY() + 8.0f, 250.0f, 210.0f);

    g.setColour (juce::Colours::white.withAlpha (0.018f));
    for (float y = area.getY() + 3.0f; y < area.getBottom(); y += 5.0f)
        g.drawLine (area.getX(), y, area.getRight(), y);

    g.setColour (juce::Colours::white.withAlpha (0.009f));
    for (float x = area.getX(); x < area.getRight(); x += 5.0f)
        g.drawLine (x, area.getY(), x, area.getBottom());
}

void HomeSidechainTriggerAudioProcessorEditor::drawCard (juce::Graphics& g, juce::Rectangle<float> r,
                                                           juce::Colour colour, bool brightHeader) const
{
    drawSoftShadow (g, r, 9.0f);

    juce::ColourGradient base (colour.withAlpha (brightHeader ? 0.18f : 0.08f), r.getX(), r.getY(),
                               juce::Colours::transparentBlack, r.getRight(), r.getBottom(), false);
    g.setColour (panel);
    g.fillRoundedRectangle (r, 9.0f);
    g.setGradientFill (base);
    g.fillRoundedRectangle (r, 9.0f);

    g.setColour (edge);
    g.drawRoundedRectangle (r, 9.0f, 1.0f);

    const auto headerStrip = juce::Rectangle<float> (r.getX(), r.getY(), r.getWidth(), 27.0f);
    juce::Path clip;
    clip.addRoundedRectangle (r, 9.0f);
    g.saveState();
    g.reduceClipRegion (clip);
    juce::ColourGradient sheen (colour.withAlpha (0.16f), headerStrip.getX(), headerStrip.getY(),
                                colour.withAlpha (0.0f), headerStrip.getX(), headerStrip.getBottom(), false);
    g.setGradientFill (sheen);
    g.fillRect (headerStrip);
    g.restoreState();
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto title = font (23.0f, true);
    const auto subtitle = font (7.5f, true);

    const auto home = juce::String ("HOME ");
    const auto trig = juce::String ("TRIGGER");
    const int homeW = juce::GlyphArrangement::getStringWidthInt (title, home);

    g.setFont (title);
    g.setColour (white);
    g.drawText (home, area.getX(), area.getY(), homeW + 4, 28, juce::Justification::left);
    g.setColour (green);
    g.drawText (trig, area.getX() + homeW - 1, area.getY(), 118, 28, juce::Justification::left);

    g.setFont (subtitle);
    g.setColour (muted.withAlpha (0.55f));
    g.drawText ("TRIGGER ENGINE  •  SMART INPUT", area.getX(), area.getY() + 28.0f, 170, 12, juce::Justification::left);

    const bool firing = processor.getTriggerMeter() > 0.10f;
    drawTinyStatus (g, { area.getX() + 190.0f, area.getY() + 3.0f, 138.0f, 24.0f },
                    "AUDIO + MIDI", firing ? red : green, true);

    const auto headerStatus = juce::Rectangle<float> (area.getRight() - 176.0f, area.getY() + 3.0f, 82.0f, 24.0f);
    g.setColour (firing ? red.withAlpha (0.14f) : green.withAlpha (0.10f));
    g.fillRoundedRectangle (headerStatus, 12.0f);
    g.setColour (firing ? red.withAlpha (0.8f) : green.withAlpha (0.68f));
    g.drawRoundedRectangle (headerStatus, 12.0f, 1.0f);
    g.setColour (firing ? red : green);
    g.setFont (font (7.5f, true));
    g.drawText (firing ? "TRIGGER" : "READY", headerStatus.toNearestInt(), juce::Justification::centred);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto inner = area.reduced (12.0f, 10.0f);
    const auto plot = juce::Rectangle<float> (inner.getX() + 26.0f, inner.getY() + 14.0f,
                                               inner.getWidth() - 34.0f, inner.getHeight() - 28.0f);
    const float thresholdDb = processor.getThresholdDb();
    const float minDb = juce::jlimit (-60.0f, -24.0f, thresholdDb - 30.0f);
    const float maxDb = juce::jmin (0.0f, thresholdDb + 12.0f);
    const float gridStep = (maxDb - minDb) > 42.0f ? 12.0f : 6.0f;

    g.setColour (black.withAlpha (0.46f));
    g.fillRoundedRectangle (plot.expanded (1.0f), 6.0f);
    g.setColour (edge.withAlpha (0.8f));
    g.drawRoundedRectangle (plot.expanded (1.0f), 6.0f, 1.0f);

    for (float db = maxDb; db >= minDb - 0.1f; db -= gridStep)
    {
        const float y = yForDb (db);
        g.setColour (white.withAlpha (db == 0.0f ? 0.10f : 0.055f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setFont (font (7.0f, true));
        g.setColour (muted.withAlpha (0.65f));
        g.drawText (juce::String (juce::roundToInt (db)), inner.getX(), y - 5.0f, 22.0f, 10.0f, juce::Justification::left);
    }

    g.setFont (font (6.8f, true));
    g.setColour (muted.withAlpha (0.55f));
    g.drawText ("PAST", plot.getX(), plot.getBottom() + 4.0f, 30.0f, 10.0f, juce::Justification::left);
    g.drawText ("NOW", plot.getRight() - 30.0f, plot.getBottom() + 4.0f, 30.0f, 10.0f, juce::Justification::right);

    if (processor.getWaveformPointCount() > 1)
    {
        const int pointCount = processor.getWaveformPointCount();
        const int latestTriggerPoint = processor.getLatestTriggerPointIndex();
        juce::Path line;
        juce::Path redLine;
        bool redStarted = false;
        const int redRadius = 7;

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float y = yForDb (homeSidechain::linearToDb (peak));
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (pointCount - 1);
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);

            const bool hot = latestTriggerPoint >= 0 && std::abs (i - latestTriggerPoint) <= redRadius;
            if (hot)
            {
                if (! redStarted) { redLine.startNewSubPath (x, y); redStarted = true; }
                else redLine.lineTo (x, y);
            }
            else
            {
                redStarted = false;
            }
        }

        g.setColour (cyan.withAlpha (0.16f));
        g.strokePath (line, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
        g.setColour (white.withAlpha (0.92f));
        g.strokePath (line, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.78f));
        g.strokePath (line, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));

        if (! redLine.isEmpty())
        {
            g.setColour (red.withAlpha (0.14f));
            g.strokePath (redLine, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
            g.setColour (red);
            g.strokePath (redLine, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));
        }
    }

    const float thresholdY = yForDb (thresholdDb);
    g.setColour (cyan.withAlpha (0.16f));
    g.fillRoundedRectangle (plot.getX(), thresholdY - 4.0f, plot.getWidth(), 8.0f, 4.0f);
    g.setColour (black);
    g.drawLine (plot.getX(), thresholdY + 0.8f, plot.getRight(), thresholdY + 0.8f, 2.8f);
    g.setColour (cyan);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 1.2f);

    const auto handle = juce::Rectangle<float> (plot.getX() - 5.0f, thresholdY - 5.0f, 10.0f, 10.0f);
    g.setColour (black.withAlpha (0.5f));
    g.fillEllipse (handle.expanded (2.0f));
    g.setColour (cyan);
    g.fillEllipse (handle);

    const auto tag = juce::Rectangle<float> (plot.getRight() - 110.0f, thresholdY - 12.0f, 106.0f, 24.0f);
    g.setColour (black.withAlpha (0.84f));
    g.fillRoundedRectangle (tag, 6.0f);
    g.setColour (cyan.withAlpha (0.62f));
    g.drawRoundedRectangle (tag, 6.0f, 1.0f);
    g.setFont (font (8.5f, true));
    g.setColour (white);
    g.drawText ("THRESHOLD", tag.getX() + 8.0f, tag.getY() + 4.0f, 54.0f, 15.0f, juce::Justification::left);
    g.setColour (cyan);
    g.drawText (juce::String (thresholdDb, 1) + " dB", tag.getX() + 61.0f, tag.getY() + 4.0f, 38.0f, 15.0f, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::drawUtilityPanel (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setFont (font (12.0f, true));
    g.setColour (magenta);
    g.drawText ("LINK + OUTPUT", area.getX() + 14.0f, area.getY() + 9.0f, area.getWidth() - 28.0f, 17.0f, juce::Justification::left);
    g.setFont (font (7.0f, true));
    g.setColour (muted.withAlpha (0.55f));
    g.drawText ("ONE ENGINE • AUDIO + MIDI", area.getX() + 14.0f, area.getY() + 25.0f, area.getWidth() - 28.0f, 11.0f, juce::Justification::left);

    g.setColour (edge);
    g.drawLine (area.getX() + 12.0f, area.getY() + 42.0f, area.getRight() - 12.0f, area.getY() + 42.0f);

    g.setFont (font (7.5f, true));
    g.setColour (black.withAlpha (0.72f));
    g.drawText ("DESTINATION", area.getX() + 12.0f, area.getY() + 51.0f, area.getWidth() - 24.0f, 10.0f, juce::Justification::left);

    g.setColour (black.withAlpha (0.18f));
    g.fillRoundedRectangle (area.getX() + 12.0f, area.getY() + 67.0f, area.getWidth() - 24.0f, 32.0f, 8.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (area.getX() + 12.0f, area.getY() + 67.0f, area.getWidth() - 24.0f, 32.0f, 8.0f, 1.0f);

    g.setFont (font (8.0f, true));
    g.setColour (muted.withAlpha (0.62f));
    g.drawText ("TRIGGER OUTPUT", area.getX() + 14.0f, area.getY() + 109.0f, area.getWidth() - 28.0f, 11.0f, juce::Justification::left);
    g.setFont (font (10.0f, true));
    g.setColour (white);
    g.drawText ("AUDIO  +  MIDI", area.getX() + 14.0f, area.getY() + 122.0f, area.getWidth() - 28.0f, 15.0f, juce::Justification::left);

    const bool firing = processor.getTriggerMeter() > 0.10f;
    drawTinyStatus (g, { area.getX() + 12.0f, area.getY() + 150.0f, area.getWidth() - 24.0f, 22.0f },
                    firing ? "TRIGGERING" : "READY", firing ? red : green, true);

    g.setFont (font (7.0f, true));
    g.setColour (muted.withAlpha (0.52f));
    g.drawText ("SMART DETECTION", area.getX() + 14.0f, area.getBottom() - 31.0f, area.getWidth() - 28.0f, 10.0f, juce::Justification::left);
    g.drawText ("NO MODE SWITCH", area.getX() + 14.0f, area.getBottom() - 18.0f, area.getWidth() - 28.0f, 10.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::drawCooldownPanel (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setFont (font (10.0f, true));
    g.setColour (green);
    g.drawText ("COOL DOWN", area.getX() + 12.0f, area.getY() + 7.0f, 92.0f, 14.0f, juce::Justification::left);
    g.setFont (font (7.0f, true));
    g.setColour (black.withAlpha (0.58f));
    g.drawText ("MINIMUM GAP BETWEEN TRIGGERS", area.getX() + 12.0f, area.getY() + 20.0f, 160.0f, 10.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);
    const auto frame = juce::Rectangle<float> (7.0f, 7.0f, 626.0f, 326.0f);
    g.setColour (bg1);
    g.fillRoundedRectangle (frame, 12.0f);
    drawBackgroundTexture (g, frame);
    g.setColour (edge.withAlpha (0.9f));
    g.drawRoundedRectangle (frame, 12.0f, 1.2f);

    drawHeader (g, { 20.0f, 14.0f, 600.0f, 47.0f });

    const auto graphCard = juce::Rectangle<float> (18.0f, 72.0f, 432.0f, 214.0f);
    const auto utilityCard = juce::Rectangle<float> (458.0f, 72.0f, 164.0f, 214.0f);
    const auto cooldownCard = juce::Rectangle<float> (18.0f, 294.0f, 604.0f, 30.0f);

    drawCard (g, graphCard, cyan, true);
    drawCard (g, utilityCard, magenta, true);
    drawCard (g, cooldownCard, green, false);

    g.setColour (black.withAlpha (0.28f));
    g.drawLine (graphCard.getX() + 12.0f, graphCard.getY() + 29.0f,
                graphCard.getRight() - 12.0f, graphCard.getY() + 29.0f, 1.0f);
    g.setFont (font (11.5f, true));
    g.setColour (white);
    g.drawText ("TRIGGER GRAPH", graphCard.getX() + 13.0f, graphCard.getY() + 6.0f, 180.0f, 18.0f, juce::Justification::left);
    g.setFont (font (7.0f, true));
    g.setColour (muted.withAlpha (0.62f));
    g.drawText ("DRAG THE THRESHOLD LINE", graphCard.getRight() - 160.0f, graphCard.getY() + 8.0f, 146.0f, 12.0f, juce::Justification::right);

    drawGraph (g, graphCard.reduced (8.0f, 30.0f));
    drawUtilityPanel (g, utilityCard);
    drawCooldownPanel (g, cooldownCard);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    // Match the exact plot rectangle used by drawGraph().
    graphBounds = { 64.0f, 126.0f, 358.0f, 106.0f };
    link.setBounds (471, 139, 138, 32);
    bypass.setBounds (544, 24, 70, 24);
    retrigger.setBounds (114, 295, 490, 28);
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
