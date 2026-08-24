#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0       (0xff05070a);
    const juce::Colour bg1       (0xff080d12);
    const juce::Colour panel     (0xff0c1319);
    const juce::Colour panelEdge (0xff26343f);
    const juce::Colour grid      (0xff17313d);
    const juce::Colour white     (0xfff5f8fb);
    const juce::Colour muted     (0xff7f8e9a);
    const juce::Colour cyan      (0xff1fe4ff);
    const juce::Colour cyanSoft  (0xff16b9cf);
    const juce::Colour red       (0xffff5b66);
    const juce::Colour green     (0xff39e69a);
    const juce::Colour black     (0xff020407);

    juce::Font uiFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica").withStyle (bold ? "Bold" : "Plain"));
    }

    void roundedPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                       float radius = 12.0f)
    {
        g.setColour (black.withAlpha (0.50f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), radius + 1.0f);

        juce::ColourGradient fill (panel.brighter (0.05f), r.getX(), r.getY(),
                                   bg1, r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);

        juce::ColourGradient topGlow (accent.withAlpha (0.11f), r.getX(), r.getY(),
                                      juce::Colours::transparentBlack, r.getX(), r.getY() + 70.0f, false);
        g.setGradientFill (topGlow);
        g.fillRoundedRectangle (r.reduced (1.0f), radius - 1.0f);

        g.setColour (panelEdge);
        g.drawRoundedRectangle (r, radius, 1.0f);
        g.setColour (accent.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius - 0.5f, 0.8f);
    }
}

HomeSeriesTriggerLookAndFeel::HomeSeriesTriggerLookAndFeel() = default;

void HomeSeriesTriggerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                      bool highlighted, bool)
{
    if (button.getName() != "BYPASS_SWITCH")
        return;

    const auto b = button.getLocalBounds().toFloat();
    const bool bypassed = button.getToggleState();
    const auto sw = juce::Rectangle<float> (b.getRight() - 58.0f, b.getCentreY() - 13.0f, 58.0f, 26.0f);

    g.setFont (uiFont (9.0f, true));
    g.setColour (muted.withAlpha (0.75f));
    g.drawText ("BYPASS", b.getX(), b.getY(), 48.0f, b.getHeight(), juce::Justification::centredLeft);

    g.setColour (black.withAlpha (0.65f));
    g.fillRoundedRectangle (sw.translated (0.0f, 2.0f), 13.0f);
    g.setColour (juce::Colour (0xff0d151b));
    g.fillRoundedRectangle (sw, 13.0f);

    const auto accent = bypassed ? red : green;
    g.setColour (accent.withAlpha (highlighted ? 0.98f : 0.80f));
    g.drawRoundedRectangle (sw, 13.0f, 1.2f);

    const float knob = 18.0f;
    const float left = sw.getX() + 4.0f;
    const float right = sw.getRight() - knob - 4.0f;
    const float x = bypassed ? right : left;
    const auto dot = juce::Rectangle<float> (x, sw.getCentreY() - knob * 0.5f, knob, knob);

    g.setColour (accent.withAlpha (0.10f));
    g.fillEllipse (dot.expanded (3.0f));
    g.setColour (white);
    g.fillEllipse (dot);

    g.setFont (uiFont (6.5f, true));
    g.setColour (accent);
    g.drawText (bypassed ? "OFF" : "ON", sw.toNearestInt(), juce::Justification::centred);
}

HomeSidechainTriggerLinkSelector::HomeSidechainTriggerLinkSelector (HomeSidechainTriggerAudioProcessor& p)
    : processor (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void HomeSidechainTriggerLinkSelector::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const int active = processor.getLink();
    const float arrowX = b.getRight() - 16.0f;

    g.setColour (black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.translated (0.0f, 2.0f), b.getHeight() * 0.5f);
    g.setColour (juce::Colour (0xff0f161d));
    g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    g.setColour (cyan.withAlpha (0.65f));
    g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.0f);

    const float segmentW = (arrowX - b.getX() - 20.0f) / 3.0f;
    for (int i = 0; i < 3; ++i)
    {
        const float x = b.getX() + i * segmentW;
        const bool selected = i == active;
        g.setFont (uiFont (14.0f, true));
        g.setColour (selected ? cyan : white.withAlpha (0.58f));
        g.drawText (juce::String::charToString ('A' + i), juce::Rectangle<float> (x, b.getY(), segmentW, b.getHeight()), juce::Justification::centred);
        if (selected)
        {
            g.setColour (cyan);
            g.fillRoundedRectangle (x + 9.0f, b.getBottom() - 4.0f, segmentW - 18.0f, 2.0f, 1.0f);
        }
    }

    g.setColour (cyan);
    juce::Path triangle;
    triangle.addTriangle (arrowX - 3.5f, b.getCentreY() - 1.5f,
                          arrowX + 3.5f, b.getCentreY() - 1.5f,
                          arrowX, b.getCentreY() + 3.0f);
    g.fillPath (triangle);
}

void HomeSidechainTriggerLinkSelector::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;

    juce::PopupMenu menu;
    menu.setLookAndFeel (&juce::LookAndFeel::getDefaultLookAndFeel());
    menu.addItem (1, "A", true, processor.getLink() == 0);
    menu.addItem (2, "B", true, processor.getLink() == 1);
    menu.addItem (3, "C", true, processor.getLink() == 2);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [this] (int result)
                        {
                            if (result >= 1 && result <= 3)
                                if (auto* p = processor.apvts.getParameter ("LINK"))
                                    p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (static_cast<float> (result - 1)));
                            repaint();
                        });
}

HomeSidechainTriggerGapSlider::HomeSidechainTriggerGapSlider()
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId, cyan);
    setWantsKeyboardFocus (false);
}

float HomeSidechainTriggerGapSlider::trackStartX() const noexcept
{
    return 205.0f;
}

float HomeSidechainTriggerGapSlider::trackEndX() const noexcept
{
    return juce::jmax (trackStartX() + 120.0f, static_cast<float> (getWidth()) - 120.0f);
}

void HomeSidechainTriggerGapSlider::setValueFromMouseX (float x)
{
    const float start = trackStartX();
    const float end = trackEndX();
    const double proportion = juce::jlimit (0.0, 1.0,
                                            static_cast<double> (x - start) / juce::jmax (1.0, static_cast<double> (end - start)));
    setValue (proportionOfLengthToValue (proportion), juce::sendNotificationSync);
}

void HomeSidechainTriggerGapSlider::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled() || ! e.mods.isLeftButtonDown())
        return juce::Slider::mouseDown (e);

    const auto hit = juce::Rectangle<float> (trackStartX() - 30.0f, getLocalBounds().getCentreY() - 24.0f,
                                             trackEndX() - trackStartX() + 60.0f, 48.0f);
    if (hit.contains (e.position))
    {
        manualMouseTracking = true;
        setValueFromMouseX (e.position.x);
        return;
    }

    juce::Slider::mouseDown (e);
}

void HomeSidechainTriggerGapSlider::mouseDrag (const juce::MouseEvent& e)
{
    if (manualMouseTracking)
    {
        setValueFromMouseX (e.position.x);
        return;
    }
    juce::Slider::mouseDrag (e);
}

void HomeSidechainTriggerGapSlider::mouseUp (const juce::MouseEvent& e)
{
    if (manualMouseTracking)
    {
        manualMouseTracking = false;
        setValueFromMouseX (e.position.x);
        return;
    }
    juce::Slider::mouseUp (e);
}

void HomeSidechainTriggerGapSlider::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float cy = b.getCentreY() + 2.0f;
    const float x0 = trackStartX();
    const float x1 = trackEndX();
    const float trackH = 8.0f;
    const double p = juce::jlimit (0.0, 1.0, valueToProportionOfLength (getValue()));
    const float px = x0 + (x1 - x0) * static_cast<float> (p);

    g.setFont (uiFont (13.0f, true));
    g.setColour (cyan);
    g.drawText ("COOL DOWN", 28, 12, 120, 18, juce::Justification::left);
    g.setFont (uiFont (8.0f));
    g.setColour (muted);
    g.drawText ("TIME BETWEEN TRIGGERS", 28, 31, 155, 12, juce::Justification::left);

    const auto track = juce::Rectangle<float> (x0, cy - trackH * 0.5f, x1 - x0, trackH);
    g.setColour (black.withAlpha (0.85f));
    g.fillRoundedRectangle (track.expanded (2.0f, 2.0f), 5.0f);
    g.setColour (juce::Colour (0xff0c151b));
    g.fillRoundedRectangle (track, 4.0f);
    g.setColour (cyan);
    g.fillRoundedRectangle (track.withWidth (juce::jmax (0.0f, px - x0)), 4.0f);

    for (int i = 0; i <= 18; ++i)
    {
        const float tx = x0 + (x1 - x0) * static_cast<float> (i) / 18.0f;
        g.setColour (white.withAlpha (i % 3 == 0 ? 0.15f : 0.06f));
        g.drawLine (tx, cy + 10.0f, tx, cy + 14.0f, 1.0f);
    }

    g.setColour (cyan.withAlpha (0.13f));
    g.fillEllipse (px - 19.0f, cy - 19.0f, 38.0f, 38.0f);
    g.setColour (white);
    g.fillEllipse (px - 11.0f, cy - 11.0f, 22.0f, 22.0f);
    g.setColour (cyan);
    g.drawEllipse (px - 11.0f, cy - 11.0f, 22.0f, 22.0f, 2.0f);

    const auto valueBox = juce::Rectangle<float> (b.getRight() - 92.0f, cy - 18.0f, 78.0f, 36.0f);
    g.setColour (black.withAlpha (0.65f));
    g.fillRoundedRectangle (valueBox, 9.0f);
    g.setColour (cyan.withAlpha (0.35f));
    g.drawRoundedRectangle (valueBox, 9.0f, 1.0f);
    g.setFont (uiFont (13.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms", valueBox.toNearestInt(), juce::Justification::centred);

    g.setFont (uiFont (7.0f));
    g.setColour (muted.withAlpha (0.65f));
    g.drawText ("5 ms", static_cast<int> (x0 - 10), static_cast<int> (cy + 16), 35, 10, juce::Justification::left);
    g.drawText ("500 ms", static_cast<int> (x0 + (x1 - x0) * 0.5f - 18), static_cast<int> (cy + 16), 45, 10, juce::Justification::centred);
    g.drawText ("1000 ms", static_cast<int> (x1 - 35), static_cast<int> (cy + 16), 40, 10, juce::Justification::right);
}

HomeSidechainTriggerAudioProcessorEditor::~HomeSidechainTriggerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), linkSelector (p)
{
    juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypefaceName ("Helvetica");
    setSize (720, 410);
    setResizable (false, false);
    setLookAndFeel (&homeSeriesLaf);

    addAndMakeVisible (cooldown);
    addAndMakeVisible (linkSelector);
    addAndMakeVisible (bypass);

    styleBypass();

    cooldownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RETRIGGER", cooldown);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    startTimerHz (30);
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
    constexpr float minDb = -48.0f;
    constexpr float maxDb = 0.0f;
    const float n = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
    return graphPlotBounds.getBottom() - n * graphPlotBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    constexpr float minDb = -48.0f;
    constexpr float maxDb = 0.0f;
    const float n = juce::jlimit (0.0f, 1.0f, (graphPlotBounds.getBottom() - y) / graphPlotBounds.getHeight());
    return minDb + n * (maxDb - minDb);
}

void HomeSidechainTriggerAudioProcessorEditor::setThresholdFromY (float y)
{
    const float clampedY = juce::jlimit (graphPlotBounds.getY(), graphPlotBounds.getBottom(), y);
    const float db = thresholdForY (clampedY);
    if (auto* parameter = processor.apvts.getParameter ("THRESHOLD"))
        parameter->setValueNotifyingHost (parameter->getNormalisableRange().convertTo0to1 (db));
}

void HomeSidechainTriggerAudioProcessorEditor::drawBackground (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setColour (bg0);
    g.fillAll();

    juce::ColourGradient bg (bg1, area.getX(), area.getY(), bg0, area.getRight(), area.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (area, 14.0f);

    // Very subtle cyan and magenta light leaks like the Home-family reference.
    juce::ColourGradient leftGlow (cyan.withAlpha (0.035f), area.getX() + 100.0f, area.getY() + 20.0f,
                                   juce::Colours::transparentBlack, area.getRight() * 0.60f, area.getBottom(), true);
    g.setGradientFill (leftGlow);
    g.fillEllipse (area.getX() - 100.0f, area.getY() - 50.0f, area.getWidth() * 0.80f, area.getHeight() * 0.75f);

    juce::ColourGradient rightGlow (juce::Colour (0xff7e3cff).withAlpha (0.022f), area.getRight() - 120.0f, area.getY() + 10.0f,
                                    juce::Colours::transparentBlack, area.getX() + 280.0f, area.getBottom(), true);
    g.setGradientFill (rightGlow);
    g.fillEllipse (area.getRight() - 300.0f, area.getY() - 40.0f, 320.0f, 230.0f);

    g.setColour (white.withAlpha (0.008f));
    for (float y = area.getY() + 1.0f; y < area.getBottom(); y += 6.0f)
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
    for (float x = area.getX(); x < area.getRight(); x += 9.0f)
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);

    g.setColour (panelEdge.withAlpha (0.9f));
    g.drawRoundedRectangle (area, 14.0f, 1.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawLogo (juce::Graphics& g, float x, float y) const
{
    g.setColour (cyan.withAlpha (0.95f));
    juce::Path h;
    h.startNewSubPath (x + 3.0f, y + 4.0f);
    h.lineTo (x + 3.0f, y + 26.0f);
    h.lineTo (x + 8.0f, y + 26.0f);
    h.lineTo (x + 8.0f, y + 18.0f);
    h.lineTo (x + 17.0f, y + 18.0f);
    h.lineTo (x + 17.0f, y + 26.0f);
    h.lineTo (x + 22.0f, y + 26.0f);
    h.lineTo (x + 22.0f, y + 4.0f);
    h.lineTo (x + 17.0f, y + 4.0f);
    h.lineTo (x + 17.0f, y + 13.0f);
    h.lineTo (x + 8.0f, y + 13.0f);
    h.lineTo (x + 8.0f, y + 4.0f);
    h.closeSubPath();
    g.strokePath (h, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.fillEllipse (x + 27.0f, y + 2.0f, 3.0f, 3.0f);
    g.fillEllipse (x + 28.0f, y + 10.0f, 2.0f, 2.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawLogo (g, area.getX(), area.getY() + 3.0f);

    const auto titleFont = uiFont (21.0f, true);
    const float baseX = area.getX() + 42.0f;
    float x = baseX;

    const juce::String p1 = "HOME-SIDECHAIN";
    const juce::String p2 = " TRIGGER";
    g.setFont (titleFont);
    g.setColour (white);
    const float p1w = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, p1);
    g.drawText (p1, x, area.getY() + 2.0f, p1w + 2.0f, 28.0f, juce::Justification::left);
    x += p1w - 2.0f;
    g.setColour (cyan);
    g.drawText (p2, x, area.getY() + 2.0f, 95.0f, 28.0f, juce::Justification::left);

    g.setFont (uiFont (7.5f, true));
    g.setColour (muted.withAlpha (0.9f));
    g.drawText ("D U B T A C H   D S P", baseX + 1.0f, area.getY() + 31.0f, 160.0f, 10.0f, juce::Justification::left);

    const float controlY = area.getY() + 7.0f;
    const float bypassW = 94.0f;
    bypass.setBounds (juce::roundToInt (area.getRight() - bypassW), juce::roundToInt (controlY - 2.0f), (int) bypassW, 30);
}

void HomeSidechainTriggerAudioProcessorEditor::drawStatusPill (juce::Graphics& g, juce::Rectangle<float> r,
                                                                const juce::String& text, juce::Colour colour) const
{
    g.setColour (colour.withAlpha (0.09f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (0.55f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (colour);
    g.fillEllipse (r.getX() + 11.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setFont (uiFont (10.0f, true));
    g.drawText (text, r.getX() + 27.0f, r.getY(), r.getWidth() - 33.0f, r.getHeight(), juce::Justification::centredLeft);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraphCard (juce::Graphics& g, juce::Rectangle<float> area) const
{
    roundedPanel (g, area, cyan, 12.0f);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool triggering = processor.getTriggerMeter() > 0.35f && ! bypassed;
    const auto statusColour = bypassed ? muted : (triggering ? red : cyan);
    const auto statusText = bypassed ? "BYPASSED" : (triggering ? "TRIGGERING" : "READY");
    drawStatusPill (g, { area.getX() + 18.0f, area.getY() + 17.0f, 102.0f, 28.0f }, statusText, statusColour);

    g.setColour (panelEdge.withAlpha (0.7f));
    g.drawLine (area.getX() + 145.0f, area.getY() + 17.0f, area.getX() + 145.0f, area.getY() + 45.0f, 1.0f);

    g.setFont (uiFont (10.0f, true));
    g.setColour (muted.withAlpha (0.92f));
    g.drawText ("LIVE INPUT", area.getX() + 164.0f, area.getY() + 18.0f, 100.0f, 14.0f, juce::Justification::left);

    drawWaveform (g, { area.getX() + 18.0f, area.getY() + 55.0f, area.getWidth() - 36.0f, area.getHeight() - 75.0f });
}

void HomeSidechainTriggerAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto plot = area.reduced (4.0f, 4.0f);
    const float thresholdDb = processor.getThresholdDb();
    const auto labelX = plot.getX();

    g.setColour (black.withAlpha (0.72f));
    g.fillRoundedRectangle (plot, 7.0f);

    // Grid and dB labels deliberately stop at -48 dB: the detector range is -48..0.
    for (int db = 0; db >= -48; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (white.withAlpha (db == 0 ? 0.10f : 0.045f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setFont (uiFont (8.0f));
        g.setColour (muted.withAlpha (0.76f));
        g.drawText (db == 0 ? "0" : juce::String (db), labelX + 2.0f, y - 5.0f, 24.0f, 10.0f, juce::Justification::left);
    }

    constexpr int divisions = 7;
    for (int d = 1; d < divisions; ++d)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (d) / static_cast<float> (divisions);
        g.setColour (grid.withAlpha (0.35f));
        for (float yy = plot.getY(); yy < plot.getBottom(); yy += 10.0f)
            g.drawLine (x, yy, x, juce::jmin (plot.getBottom(), yy + 4.0f), 1.0f);
    }

    const int pointCount = processor.getWaveformPointCount();
    const int latestTriggerPoint = processor.getLatestTriggerPointIndex();

    if (pointCount > 1)
    {
        juce::Path line;
        juce::Path fill;
        juce::Path hot;
        bool fillStarted = false;
        int hotStart = -1;
        int hotEnd = -1;
        constexpr int hotRadius = 12;

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float db = homeSidechain::linearToDb (juce::jmax (peak, 0.000001f));
            const float y = yForDb (db);
            const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (pointCount - 1);

            if (! fillStarted)
            {
                fill.startNewSubPath (x, plot.getBottom());
                fill.lineTo (x, y);
                fillStarted = true;
            }
            else
                fill.lineTo (x, y);

            if (i == 0) line.startNewSubPath (x, y);
            else line.lineTo (x, y);

            const bool isHot = latestTriggerPoint >= 0 && std::abs (i - latestTriggerPoint) <= hotRadius;
            if (isHot)
            {
                if (hotStart < 0)
                {
                    hotStart = i;
                    hot.startNewSubPath (x, y);
                }
                else
                    hot.lineTo (x, y);
                hotEnd = i;
            }
        }

        if (fillStarted)
        {
            fill.lineTo (plot.getRight(), plot.getBottom());
            fill.closeSubPath();
            g.setColour (cyan.withAlpha (0.08f));
            g.fillPath (fill);
        }

        g.setColour (cyan.withAlpha (0.12f));
        g.strokePath (line, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.25f));
        g.strokePath (line, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved));
        g.setColour (white);
        g.strokePath (line, juce::PathStrokeType (1.15f, juce::PathStrokeType::curved));

        if (! hot.isEmpty())
        {
            const float hx0 = plot.getX() + plot.getWidth() * static_cast<float> (hotStart) / static_cast<float> (pointCount - 1);
            const float hx1 = plot.getX() + plot.getWidth() * static_cast<float> (hotEnd) / static_cast<float> (pointCount - 1);
            g.setColour (red.withAlpha (0.035f));
            g.fillRoundedRectangle (juce::jmax (plot.getX(), hx0 - 7.0f), plot.getY(),
                                    juce::jmin (plot.getRight(), hx1 + 7.0f) - juce::jmax (plot.getX(), hx0 - 7.0f),
                                    plot.getHeight(), 6.0f);
            g.setColour (red.withAlpha (0.22f));
            g.strokePath (hot, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
            g.setColour (red);
            g.strokePath (hot, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        }
    }

    const float thresholdY = yForDb (thresholdDb);
    for (float x = plot.getX(); x < plot.getRight(); x += 12.0f)
    {
        g.setColour (cyan.withAlpha (0.75f));
        g.drawLine (x, thresholdY, juce::jmin (plot.getRight(), x + 7.0f), thresholdY, 1.5f);
    }

    const auto handle = juce::Point<float> (plot.getRight(), thresholdY);
    g.setColour (cyan.withAlpha (0.12f));
    g.fillEllipse (handle.x - 13.0f, handle.y - 13.0f, 26.0f, 26.0f);
    g.setColour (cyan);
    g.fillEllipse (handle.x - 7.0f, handle.y - 7.0f, 14.0f, 14.0f);
    g.setColour (white);
    g.fillEllipse (handle.x - 2.5f, handle.y - 2.5f, 5.0f, 5.0f);

    const auto badge = juce::Rectangle<float> (plot.getRight() - 122.0f, juce::jlimit (plot.getY() + 6.0f, plot.getBottom() - 56.0f, thresholdY - 28.0f), 112.0f, 52.0f);
    g.setColour (black.withAlpha (0.92f));
    g.fillRoundedRectangle (badge, 8.0f);
    g.setColour (cyan.withAlpha (0.38f));
    g.drawRoundedRectangle (badge, 8.0f, 1.0f);
    g.setFont (uiFont (7.0f, true));
    g.setColour (muted);
    g.drawText ("THRESHOLD", badge.getX() + 10.0f, badge.getY() + 8.0f, 90.0f, 10.0f, juce::Justification::left);
    g.setFont (uiFont (13.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (thresholdDb, 1) + " dB", badge.getX() + 10.0f, badge.getY() + 21.0f, 92.0f, 19.0f, juce::Justification::left);

    g.setFont (uiFont (8.0f));
    g.setColour (muted.withAlpha (0.55f));
    g.drawText ("PAST", plot.getX(), plot.getBottom() + 6.0f, 28.0f, 10.0f, juce::Justification::left);
    g.drawText ("NOW", plot.getRight() - 28.0f, plot.getBottom() + 6.0f, 28.0f, 10.0f, juce::Justification::right);
    g.drawText ("DRAG THRESHOLD", plot.getX() + 70.0f, plot.getBottom() + 6.0f, 120.0f, 10.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::drawCooldownCard (juce::Graphics& g, juce::Rectangle<float> area) const
{
    roundedPanel (g, area, cyan, 12.0f);
    g.setFont (uiFont (16.0f, true));
    g.setColour (cyan);
    g.drawText ("COOL DOWN", area.getX() + 28.0f, area.getY() + 13.0f, 130.0f, 20.0f, juce::Justification::left);
    g.setFont (uiFont (8.0f));
    g.setColour (muted);
    g.drawText ("TIME BETWEEN TRIGGERS", area.getX() + 28.0f, area.getY() + 35.0f, 155.0f, 12.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto frame = getLocalBounds().toFloat().reduced (6.0f);
    drawBackground (g, frame);
    drawHeader (g, { frame.getX() + 18.0f, frame.getY() + 16.0f, frame.getWidth() - 36.0f, 48.0f });

    const auto graphCard = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 78.0f,
                                                    frame.getWidth() - 36.0f, 272.0f);
    const auto cooldownCard = juce::Rectangle<float> (frame.getX() + 18.0f, graphCard.getBottom() + 16.0f,
                                                       frame.getWidth() - 36.0f, 54.0f);

    drawGraphCard (g, graphCard);
    drawCooldownCard (g, cooldownCard);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    const auto frame = getLocalBounds().toFloat().reduced (6.0f);
    const auto graphCard = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 78.0f,
                                                    frame.getWidth() - 36.0f, 272.0f);
    const auto cooldownCard = juce::Rectangle<float> (frame.getX() + 18.0f, graphCard.getBottom() + 16.0f,
                                                       frame.getWidth() - 36.0f, 54.0f);

    bypass.setBounds (juce::roundToInt (frame.getRight() - 108.0f), juce::roundToInt (frame.getY() + 20.0f), 102, 34);
    linkSelector.setBounds (juce::roundToInt (frame.getRight() - 306.0f), juce::roundToInt (frame.getY() + 18.0f), 148, 38);
    cooldown.setBounds (juce::roundToInt (cooldownCard.getX()), juce::roundToInt (cooldownCard.getY()),
                        juce::roundToInt (cooldownCard.getWidth()), juce::roundToInt (cooldownCard.getHeight()));

    graphPlotBounds = { graphCard.getX() + 22.0f + 4.0f,
                        graphCard.getY() + 55.0f + 4.0f,
                        graphCard.getWidth() - 44.0f - 8.0f,
                        graphCard.getHeight() - 79.0f - 8.0f };
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;

    const auto hitArea = graphPlotBounds.expanded (12.0f, 8.0f);
    if (hitArea.contains (e.position))
    {
        draggingThreshold = true;
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setThresholdFromY (e.position.y);
    }
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingThreshold)
    {
        setThresholdFromY (e.position.y);
        return;
    }
}

void HomeSidechainTriggerAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    draggingThreshold = false;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    linkSelector.repaint();
    repaint();
}
