#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0       (0xff040609);
    const juce::Colour bg1       (0xff090d12);
    const juce::Colour panel     (0xff0b1117);
    const juce::Colour plotBg    (0xff05090d);
    const juce::Colour edge      (0xff263640);
    const juce::Colour grid      (0xff21414e);
    const juce::Colour white     (0xfff1f6fa);
    const juce::Colour muted     (0xff8796a3);
    const juce::Colour cyan      (0xff1ee7ff);
    const juce::Colour cyanDim   (0xff0fa8be);
    const juce::Colour red       (0xffff5965);
    const juce::Colour green     (0xff36e79a);
    const juce::Colour black     (0xff010204);

    juce::Font uiFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica")
                                                     .withStyle (bold ? "Bold" : "Plain"));
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float radius)
    {
        g.setColour (black.withAlpha (0.60f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), radius + 1.0f);

        juce::ColourGradient fill (panel.brighter (0.04f), r.getX(), r.getY(),
                                   bg1, r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);

        juce::ColourGradient glow (accent.withAlpha (0.065f), r.getX(), r.getY(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r.reduced (1.0f), radius - 1.0f);

        g.setColour (edge.withAlpha (0.95f));
        g.drawRoundedRectangle (r, radius, 1.0f);
        g.setColour (accent.withAlpha (0.40f));
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

    const float labelW = 46.0f;
    const auto sw = juce::Rectangle<float> (b.getRight() - 61.0f, b.getCentreY() - 13.0f, 61.0f, 26.0f);

    g.setFont (uiFont (9.0f, true));
    g.setColour (muted.withAlpha (0.88f));
    g.drawText ("BYPASS", b.getX(), b.getY(), labelW, b.getHeight(), juce::Justification::centredLeft);

    const auto accent = bypassed ? red : cyan;
    g.setColour (black.withAlpha (0.85f));
    g.fillRoundedRectangle (sw.translated (0.0f, 2.0f), 13.0f);
    g.setColour (juce::Colour (0xff0d141a));
    g.fillRoundedRectangle (sw, 13.0f);
    g.setColour (accent.withAlpha (highlighted ? 0.98f : 0.78f));
    g.drawRoundedRectangle (sw, 13.0f, 1.2f);

    const float knob = 18.0f;
    const float left = sw.getX() + 4.0f;
    const float right = sw.getRight() - knob - 4.0f;
    const float x = bypassed ? left : right;
    const auto dot = juce::Rectangle<float> (x, sw.getCentreY() - knob * 0.5f, knob, knob);

    g.setColour (accent.withAlpha (0.12f));
    g.fillEllipse (dot.expanded (3.0f));
    g.setColour (white.withAlpha (0.98f));
    g.fillEllipse (dot);

    g.setFont (uiFont (6.5f, true));
    g.setColour (accent.withAlpha (0.95f));
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
    const int active = juce::jlimit (0, 2, processor.getLink());

    g.setColour (black.withAlpha (0.70f));
    g.fillRoundedRectangle (b.translated (0.0f, 2.0f), b.getHeight() * 0.5f + 1.0f);
    g.setColour (juce::Colour (0xff0d1319));
    g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    g.setColour (edge.withAlpha (0.95f));
    g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.0f);

    const float left = b.getX() + 12.0f;
    const float arrowZone = 26.0f;
    const float contentW = b.getWidth() - 24.0f - arrowZone;
    const float segmentW = contentW / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        const float x = left + segmentW * static_cast<float> (i);
        const bool selected = i == active;
        g.setFont (uiFont (14.0f, true));
        g.setColour (selected ? cyan : white.withAlpha (0.66f));
        g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + i)),
                    juce::Rectangle<float> (x, b.getY(), segmentW, b.getHeight()),
                    juce::Justification::centred);
        if (selected)
        {
            g.setColour (cyan);
            g.fillRoundedRectangle (x + 12.0f, b.getBottom() - 4.0f, segmentW - 24.0f, 2.0f, 1.0f);
        }
    }

    g.setColour (cyan);
    juce::Path triangle;
    const float ax = b.getRight() - 15.0f;
    const float ay = b.getCentreY();
    triangle.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.fillPath (triangle);
}

void HomeSidechainTriggerLinkSelector::mouseDown (const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "A", true, processor.getLink() == 0);
    menu.addItem (2, "B", true, processor.getLink() == 1);
    menu.addItem (3, "C", true, processor.getLink() == 2);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int result)
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
    setWantsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

float HomeSidechainTriggerGapSlider::trackStartX() const noexcept
{
    return 160.0f;
}

float HomeSidechainTriggerGapSlider::trackEndX() const noexcept
{
    return juce::jmax (trackStartX() + 160.0f, static_cast<float> (getWidth()) - 155.0f);
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
    if (!e.mods.isLeftButtonDown())
        return;

    const auto hit = juce::Rectangle<float> (trackStartX() - 18.0f, getLocalBounds().getCentreY() - 28.0f,
                                              trackEndX() - trackStartX() + 36.0f, 56.0f);
    if (hit.contains (e.position))
    {
        manualMouseTracking = true;
        setValueFromMouseX (e.position.x);
        return;
    }
    manualMouseTracking = false;
}

void HomeSidechainTriggerGapSlider::mouseDrag (const juce::MouseEvent& e)
{
    if (manualMouseTracking)
        setValueFromMouseX (e.position.x);
}

void HomeSidechainTriggerGapSlider::mouseUp (const juce::MouseEvent& e)
{
    if (manualMouseTracking)
    {
        setValueFromMouseX (e.position.x);
        manualMouseTracking = false;
    }
    juce::ignoreUnused (e);
}

void HomeSidechainTriggerGapSlider::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float cy = b.getCentreY() + 1.0f;
    const float x0 = trackStartX();
    const float x1 = trackEndX();
    const float trackH = 8.0f;
    const float p = static_cast<float> (juce::jlimit (0.0, 1.0, valueToProportionOfLength (getValue())));
    const float px = x0 + (x1 - x0) * p;

    const auto track = juce::Rectangle<float> (x0, cy - trackH * 0.5f, x1 - x0, trackH);
    g.setColour (black.withAlpha (0.86f));
    g.fillRoundedRectangle (track.expanded (1.5f, 1.5f), 5.0f);
    g.setColour (juce::Colour (0xff11181d));
    g.fillRoundedRectangle (track, 4.0f);
    g.setColour (cyan.withAlpha (0.96f));
    g.fillRoundedRectangle (track.withWidth (juce::jmax (0.0f, px - x0)), 4.0f);

    for (int i = 0; i <= 20; ++i)
    {
        const float tx = x0 + (x1 - x0) * static_cast<float> (i) / 20.0f;
        const float th = (i % 5 == 0) ? 5.0f : 3.0f;
        g.setColour (white.withAlpha (i % 5 == 0 ? 0.15f : 0.065f));
        g.drawLine (tx, cy + 11.0f, tx, cy + 11.0f + th, 1.0f);
    }

    g.setColour (cyan.withAlpha (0.14f));
    g.fillEllipse (px - 22.0f, cy - 22.0f, 44.0f, 44.0f);
    g.setColour (black.withAlpha (0.80f));
    g.fillEllipse (px - 14.0f, cy - 14.0f, 28.0f, 28.0f);
    g.setColour (cyan);
    g.drawEllipse (px - 13.0f, cy - 13.0f, 26.0f, 26.0f, 2.0f);
    g.setColour (white.withAlpha (0.95f));
    g.fillEllipse (px - 8.5f, cy - 8.5f, 17.0f, 17.0f);
    g.setColour (juce::Colour (0xffb7edf3).withAlpha (0.7f));
    g.fillEllipse (px - 5.0f, cy - 5.0f, 10.0f, 10.0f);

    const auto valueBox = juce::Rectangle<float> (b.getRight() - 108.0f, cy - 22.0f, 94.0f, 44.0f);
    g.setColour (black.withAlpha (0.72f));
    g.fillRoundedRectangle (valueBox, 10.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (valueBox, 10.0f, 1.0f);
    g.setFont (uiFont (16.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms", valueBox.toNearestInt(), juce::Justification::centred);

    g.setFont (uiFont (8.0f));
    g.setColour (muted.withAlpha (0.72f));
    g.drawText ("5 ms", static_cast<int> (x0 - 10), static_cast<int> (cy + 19), 35, 10, juce::Justification::left);
    g.drawText ("500 ms", static_cast<int> (x0 + (x1 - x0) * 0.50f - 21), static_cast<int> (cy + 19), 50, 10, juce::Justification::centred);
    g.drawText ("1000 ms", static_cast<int> (x1 - 42), static_cast<int> (cy + 19), 45, 10, juce::Justification::right);
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
    setSize (660, 398);
    setResizable (false, false);
    setLookAndFeel (&homeSeriesLaf);

    addAndMakeVisible (cooldown);
    addAndMakeVisible (linkSelector);
    addAndMakeVisible (bypass);

    styleBypass();

    cooldownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RETRIGGER", cooldown);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    startTimerHz (24);
}

void HomeSidechainTriggerAudioProcessorEditor::styleBypass()
{
    bypass.setClickingTogglesState (true);
    bypass.setName ("BYPASS_SWITCH");
    bypass.setTooltip ("Bypass the Trigger engine");
    bypass.setColour (juce::ToggleButton::tickColourId, cyan);
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
    juce::ColourGradient bg (bg1, area.getX(), area.getY(), bg0, area.getRight(), area.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRect (area);

    juce::ColourGradient cyanGlow (cyan.withAlpha (0.045f), area.getX() + 60.0f, area.getY() + 20.0f,
                                   juce::Colours::transparentBlack, area.getX() + 280.0f, area.getBottom(), true);
    g.setGradientFill (cyanGlow);
    g.fillEllipse (area.getX() - 120.0f, area.getY() - 100.0f, 420.0f, 300.0f);

    juce::ColourGradient magentaGlow (juce::Colour (0xff7d4cff).withAlpha (0.025f), area.getRight() - 80.0f, area.getY() + 35.0f,
                                      juce::Colours::transparentBlack, area.getRight() - 340.0f, area.getBottom(), true);
    g.setGradientFill (magentaGlow);
    g.fillEllipse (area.getRight() - 260.0f, area.getY() - 40.0f, 320.0f, 220.0f);

    // Fine Home-series texture: tiny crosshatch rather than a heavy grid.
    g.setColour (white.withAlpha (0.008f));
    for (float y = area.getY(); y < area.getBottom(); y += 7.0f)
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
    for (float x = area.getX(); x < area.getRight(); x += 11.0f)
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);

    g.setColour (edge.withAlpha (0.98f));
    g.drawRoundedRectangle (area.reduced (0.5f), 13.0f, 1.0f);
    g.setColour (white.withAlpha (0.03f));
    g.drawLine (area.getX() + 16.0f, area.getY() + 1.0f, area.getRight() - 16.0f, area.getY() + 1.0f, 1.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawLogo (juce::Graphics& g, float x, float y) const
{
    // A compact, glowing H glyph matching the visual language of the Home family.
    g.setColour (cyan.withAlpha (0.13f));
    juce::Path glow;
    glow.startNewSubPath (x + 2.0f, y + 2.0f);
    glow.lineTo (x + 2.0f, y + 31.0f);
    glow.startNewSubPath (x + 26.0f, y + 2.0f);
    glow.lineTo (x + 26.0f, y + 31.0f);
    glow.startNewSubPath (x + 2.0f, y + 16.5f);
    glow.lineTo (x + 26.0f, y + 16.5f);
    g.strokePath (glow, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (cyan);
    juce::Path h;
    h.startNewSubPath (x + 2.0f, y + 2.0f);
    h.lineTo (x + 2.0f, y + 31.0f);
    h.startNewSubPath (x + 26.0f, y + 2.0f);
    h.lineTo (x + 26.0f, y + 31.0f);
    h.startNewSubPath (x + 2.0f, y + 16.5f);
    h.lineTo (x + 26.0f, y + 16.5f);
    g.strokePath (h, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.fillEllipse (x + 31.0f, y + 1.0f, 3.0f, 3.0f);
    g.fillEllipse (x + 33.0f, y + 11.0f, 2.0f, 2.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawLogo (g, area.getX(), area.getY() + 1.0f);

    const float baseX = area.getX() + 42.0f;
    const auto titleFont = uiFont (21.0f, true);
    const juce::String p1 = "HOME-";
    const juce::String p2 = "SIDECHAIN";
    const juce::String p3 = " TRIGGER";
    float x = baseX;

    g.setFont (titleFont);
    g.setColour (white);
    const float p1w = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, p1);
    const float p2w = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, p2);
    g.drawText (p1, x, area.getY() + 1.0f, p1w + 2.0f, 28.0f, juce::Justification::left);
    x += p1w;
    g.setColour (cyan);
    g.drawText (p2, x, area.getY() + 1.0f, p2w + 2.0f, 28.0f, juce::Justification::left);
    x += p2w;
    g.setColour (white);
    g.drawText (p3, x, area.getY() + 1.0f, 95.0f, 28.0f, juce::Justification::left);

    g.setFont (uiFont (7.4f, true));
    g.setColour (muted.withAlpha (0.95f));
    g.drawText ("D U B T A C H   D S P", baseX + 1.0f, area.getY() + 29.0f, 175.0f, 11.0f, juce::Justification::left);

    // Divider and tiny LINK caption visually integrate the two header utilities.
    const float dividerX = area.getRight() - 193.0f;
    g.setColour (edge.withAlpha (0.75f));
    g.drawLine (dividerX, area.getY() + 3.0f, dividerX, area.getBottom() - 3.0f, 1.0f);
    g.setFont (uiFont (10.0f, true));
    g.setColour (muted);
    g.drawText ("LINK", dividerX - 82.0f, area.getY() + 14.0f, 40.0f, 14.0f, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::drawStatusPill (juce::Graphics& g, juce::Rectangle<float> r,
                                                                const juce::String& text, juce::Colour colour) const
{
    g.setColour (black.withAlpha (0.50f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), r.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (0.07f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (0.45f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (colour);
    g.fillEllipse (r.getX() + 12.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setFont (uiFont (11.0f, true));
    g.drawText (text, r.getX() + 28.0f, r.getY(), r.getWidth() - 32.0f, r.getHeight(), juce::Justification::centredLeft);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraphCard (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawPanel (g, area, cyan, 12.0f);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool triggering = processor.getTriggerMeter() > 0.35f && !bypassed;
    const auto colour = bypassed ? muted : (triggering ? red : cyan);
    const auto text = bypassed ? "BYPASSED" : (triggering ? "TRIGGERING" : "READY");

    drawStatusPill (g, { area.getX() + 18.0f, area.getY() + 17.0f, 122.0f, 29.0f }, text, colour);
    g.setColour (edge.withAlpha (0.72f));
    g.drawLine (area.getX() + 158.0f, area.getY() + 17.0f, area.getX() + 158.0f, area.getY() + 46.0f, 1.0f);

    g.setFont (uiFont (10.5f, true));
    g.setColour (muted.withAlpha (0.92f));
    g.drawText ("LIVE INPUT", area.getX() + 180.0f, area.getY() + 18.0f, 100.0f, 14.0f, juce::Justification::left);

    drawWaveform (g, { area.getX() + 18.0f, area.getY() + 55.0f, area.getWidth() - 36.0f, area.getHeight() - 73.0f });
}

void HomeSidechainTriggerAudioProcessorEditor::drawTimeScale (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    constexpr int divisions = 6;
    g.setFont (uiFont (7.8f));
    g.setColour (muted.withAlpha (0.72f));
    for (int i = 0; i <= divisions; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (divisions);
        const float seconds = -3.0f + 3.0f * static_cast<float> (i) / static_cast<float> (divisions);
        const juce::String label = (i == divisions) ? "NOW" : juce::String (seconds, 1) + "s";
        g.drawText (label, x - 26.0f, plot.getBottom() + 8.0f, 52.0f, 10.0f,
                    i == divisions ? juce::Justification::right : (i == 0 ? juce::Justification::left : juce::Justification::centred));
    }
}

void HomeSidechainTriggerAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto plot = area.reduced (6.0f, 7.0f);
    const float thresholdDb = processor.getThresholdDb();

    g.setColour (plotBg);
    g.fillRoundedRectangle (plot, 7.0f);
    g.setColour (black.withAlpha (0.55f));
    g.drawRoundedRectangle (plot, 7.0f, 1.0f);

    // dB grid: match the mockup's useful display range without wasting space below -48 dB.
    for (int db = 0; db >= -48; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (white.withAlpha (db == 0 ? 0.10f : 0.05f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setFont (uiFont (7.8f));
        g.setColour (muted.withAlpha (0.78f));
        g.drawText (db == 0 ? "0" : juce::String (db), plot.getX() - 1.0f, y - 5.0f, 26.0f, 10.0f, juce::Justification::left);
    }

    constexpr int divisions = 6;
    for (int d = 0; d <= divisions; ++d)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (d) / static_cast<float> (divisions);
        g.setColour (grid.withAlpha (d == 0 || d == divisions ? 0.28f : 0.24f));

        // Keep this compatible with the JUCE version used by the project.
        // Passing a null dash array with zero entries can assert in some JUCE
        // builds and was causing pluginval to fail during the Editor test.
        const float dashes[] = { 5.0f, 5.0f };
        g.drawDashedLine (juce::Line<float> (x, plot.getY(), x, plot.getBottom()),
                          dashes, 2, 1.0f);
    }

    // The subtle horizontal mid lines from the mockup give the graph a depth without clutter.
    for (int db = -12; db >= -36; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (grid.withAlpha (0.16f));
        g.drawLine (plot.getX(), y, plot.getRight(), y, 1.0f);
    }

    const int pointCount = processor.getWaveformPointCount();
    const int latestTriggerPoint = processor.getLatestTriggerPointIndex();

    if (pointCount > 1)
    {
        juce::Path line;
        juce::Path fill;
        juce::Path hotLine;
        juce::Path hotFill;
        bool fillStarted = false;
        bool hotStarted = false;
        int hotStart = -1;

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float db = homeSidechain::linearToDb (juce::jmax (peak, 0.000001f));
            const float y = yForDb (db);
            const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (pointCount - 1);
            const bool hot = latestTriggerPoint >= 0 && i >= latestTriggerPoint;

            if (!fillStarted)
            {
                fill.startNewSubPath (x, plot.getBottom());
                fill.lineTo (x, y);
                fillStarted = true;
            }
            else
                fill.lineTo (x, y);

            if (!hotStarted && hot)
            {
                hotStarted = true;
                hotStart = i;
                hotFill.startNewSubPath (x, plot.getBottom());
                hotFill.lineTo (x, y);
                hotLine.startNewSubPath (x, y);
            }
            else if (hotStarted)
            {
                hotFill.lineTo (x, y);
                hotLine.lineTo (x, y);
            }

            if (i == 0)
                line.startNewSubPath (x, y);
            else
                line.lineTo (x, y);
        }

        if (fillStarted)
        {
            fill.lineTo (plot.getRight(), plot.getBottom());
            fill.closeSubPath();
            g.setColour (cyan.withAlpha (0.08f));
            g.fillPath (fill);
        }

        if (hotStarted)
        {
            hotFill.lineTo (plot.getRight(), plot.getBottom());
            hotFill.closeSubPath();
            g.setColour (red.withAlpha (0.09f));
            g.fillPath (hotFill);
        }

        g.setColour (cyan.withAlpha (0.11f));
        g.strokePath (line, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.28f));
        g.strokePath (line, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved));
        g.setColour (white);
        g.strokePath (line, juce::PathStrokeType (1.25f, juce::PathStrokeType::curved));

        if (hotStarted)
        {
            g.setColour (red.withAlpha (0.17f));
            g.strokePath (hotLine, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
            g.setColour (red);
            g.strokePath (hotLine, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved));

            const float hotX = plot.getX() + plot.getWidth() * static_cast<float> (hotStart) / static_cast<float> (pointCount - 1);
            g.setColour (red.withAlpha (0.11f));
            g.fillRoundedRectangle (juce::Rectangle<float> (hotX - 6.0f, plot.getY(), plot.getRight() - hotX + 6.0f, plot.getHeight()), 5.0f);
        }
    }

    const float thresholdY = yForDb (thresholdDb);
    const float dashW = 9.0f;
    const float gapW = 6.0f;
    for (float x = plot.getX(); x < plot.getRight(); x += dashW + gapW)
    {
        g.setColour (cyan.withAlpha (0.85f));
        g.drawLine (x, thresholdY, juce::jmin (plot.getRight(), x + dashW), thresholdY, 1.6f);
    }

    const auto handle = juce::Point<float> (plot.getRight(), thresholdY);
    g.setColour (cyan.withAlpha (0.10f));
    g.fillEllipse (handle.x - 12.0f, handle.y - 12.0f, 24.0f, 24.0f);
    g.setColour (cyan);
    g.fillEllipse (handle.x - 6.5f, handle.y - 6.5f, 13.0f, 13.0f);
    g.setColour (white);
    g.fillEllipse (handle.x - 2.3f, handle.y - 2.3f, 4.6f, 4.6f);

    const auto badge = juce::Rectangle<float> (plot.getRight() - 130.0f,
                                                juce::jlimit (plot.getY() + 8.0f, plot.getBottom() - 56.0f,
                                                              thresholdY - 27.0f),
                                                120.0f, 54.0f);
    g.setColour (black.withAlpha (0.94f));
    g.fillRoundedRectangle (badge, 9.0f);
    g.setColour (edge.withAlpha (0.96f));
    g.drawRoundedRectangle (badge, 9.0f, 1.0f);
    g.setFont (uiFont (8.0f, true));
    g.setColour (muted);
    g.drawText ("THRESHOLD", badge.getX() + 12.0f, badge.getY() + 9.0f, 94.0f, 10.0f, juce::Justification::left);
    g.setFont (uiFont (13.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (thresholdDb, 1) + " dB", badge.getX() + 12.0f, badge.getY() + 24.0f, 97.0f, 18.0f, juce::Justification::left);

    drawTimeScale (g, plot);
    drawDragHint (g, plot.getX() + 8.0f, plot.getBottom() + 24.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawDragHint (juce::Graphics& g, float x, float y) const
{
    const float alpha = hoveringThreshold ? 0.95f : 0.72f;
    g.setColour (muted.withAlpha (alpha));

    juce::Path hand;
    hand.startNewSubPath (x + 2.0f, y + 4.0f);
    hand.lineTo (x + 2.0f, y - 5.0f);
    hand.cubicTo (x + 2.0f, y - 8.0f, x + 6.0f, y - 8.0f, x + 6.0f, y - 5.0f);
    hand.lineTo (x + 6.0f, y + 0.0f);
    hand.lineTo (x + 8.5f, y - 4.5f);
    hand.cubicTo (x + 9.5f, y - 6.0f, x + 12.0f, y - 5.0f, x + 11.0f, y - 2.5f);
    hand.lineTo (x + 8.0f, y + 5.0f);
    hand.cubicTo (x + 7.0f, y + 7.5f, x + 3.0f, y + 7.5f, x + 2.0f, y + 4.0f);
    g.strokePath (hand, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setFont (uiFont (10.0f, true));
    g.drawText ("DRAG THRESHOLD", x + 24.0f, y - 6.0f, 120.0f, 14.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::drawCooldownCard (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawPanel (g, area, cyan, 12.0f);
    g.setFont (uiFont (15.0f, true));
    g.setColour (cyan);
    g.drawText ("COOL DOWN", area.getX() + 26.0f, area.getY() + 11.0f, 120.0f, 20.0f, juce::Justification::left);
    g.setFont (uiFont (8.0f));
    g.setColour (muted.withAlpha (0.90f));
    g.drawText ("TIME BETWEEN TRIGGERS", area.getX() + 26.0f, area.getY() + 31.0f, 150.0f, 11.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto frame = getLocalBounds().toFloat().reduced (7.0f);
    drawBackground (g, frame);

    const auto header = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 15.0f,
                                                 frame.getWidth() - 36.0f, 52.0f);
    drawHeader (g, header);

    const auto graph = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 78.0f,
                                                frame.getWidth() - 36.0f, 248.0f);
    const auto cooldownCard = juce::Rectangle<float> (frame.getX() + 18.0f, graph.getBottom() + 12.0f,
                                                       frame.getWidth() - 36.0f, 51.0f);

    drawGraphCard (g, graph);
    drawCooldownCard (g, cooldownCard);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    const auto frame = getLocalBounds().toFloat().reduced (7.0f);
    const auto graph = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 78.0f,
                                                frame.getWidth() - 36.0f, 248.0f);
    const auto cooldownCard = juce::Rectangle<float> (frame.getX() + 18.0f, graph.getBottom() + 12.0f,
                                                       frame.getWidth() - 36.0f, 51.0f);

    linkSelector.setBounds (juce::roundToInt (frame.getRight() - 195.0f),
                            juce::roundToInt (frame.getY() + 18.0f), 142, 36);
    bypass.setBounds (juce::roundToInt (frame.getRight() - 88.0f),
                      juce::roundToInt (frame.getY() + 18.0f), 82, 36);

    cooldown.setBounds (juce::roundToInt (cooldownCard.getX()), juce::roundToInt (cooldownCard.getY()),
                        juce::roundToInt (cooldownCard.getWidth()), juce::roundToInt (cooldownCard.getHeight()));

    graphPlotBounds = { graph.getX() + 24.0f, graph.getY() + 55.0f + 6.0f,
                        graph.getWidth() - 48.0f, graph.getHeight() - 79.0f - 12.0f };
}

void HomeSidechainTriggerAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    const auto hit = graphPlotBounds.expanded (16.0f, 12.0f);
    const bool over = hit.contains (e.position);
    if (over != hoveringThreshold)
    {
        hoveringThreshold = over;
        setMouseCursor (over ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void HomeSidechainTriggerAudioProcessorEditor::mouseExit (const juce::MouseEvent&)
{
    hoveringThreshold = false;
    if (!draggingThreshold)
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    const auto hitArea = graphPlotBounds.expanded (16.0f, 12.0f);
    if (hitArea.contains (e.position))
    {
        draggingThreshold = true;
        hoveringThreshold = true;
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setThresholdFromY (e.position.y);
        repaint();
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
    setMouseCursor (hoveringThreshold ? juce::MouseCursor::UpDownResizeCursor
                                      : juce::MouseCursor::NormalCursor);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    linkSelector.repaint();
    repaint();
}
