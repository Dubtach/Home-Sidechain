#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0      (0xff05070a);
    const juce::Colour bg1      (0xff0a0d12);
    const juce::Colour panel    (0xff0d1218);
    const juce::Colour panel2   (0xff111821);
    const juce::Colour edge     (0xff27313b);
    const juce::Colour edgeSoft (0xff1a232c);
    const juce::Colour white    (0xfff5f8fb);
    const juce::Colour muted    (0xff85919e);
    const juce::Colour cyan     (0xff18dfff);
    const juce::Colour cyanDark (0xff0b7186);
    const juce::Colour green    (0xff39e69a);
    const juce::Colour red      (0xffff5c64);
    const juce::Colour black    (0xff030507);

    juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size).withName ("Helvetica").withStyle (bold ? "Bold" : "Plain"));
    }

    void drawSoftShadow (juce::Graphics& g, juce::Rectangle<float> r, float radius)
    {
        g.setColour (juce::Colours::black.withAlpha (0.50f));
        g.fillRoundedRectangle (r.translated (0.0f, 4.0f), radius + 1.0f);
    }

    void drawGradientPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent)
    {
        drawSoftShadow (g, r, 11.0f);

        juce::ColourGradient base (juce::Colour (0xff10161d), r.getX(), r.getY(),
                                   juce::Colour (0xff0a0e14), r.getRight(), r.getBottom(), false);
        g.setGradientFill (base);
        g.fillRoundedRectangle (r, 11.0f);

        juce::ColourGradient glow (accent.withAlpha (0.09f), r.getX(), r.getY(),
                                   juce::Colours::transparentBlack, r.getX(), r.getY() + 90.0f, false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r, 11.0f);

        g.setColour (edgeSoft);
        g.drawRoundedRectangle (r, 11.0f, 1.0f);
        g.setColour (accent.withAlpha (0.32f));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.5f, 0.8f);

        juce::ColourGradient sheen (white.withAlpha (0.028f), r.getX(), r.getY(),
                                    juce::Colours::transparentWhite, r.getX(), r.getY() + 36.0f, false);
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (r.reduced (1.0f), 10.0f);
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
    const bool active = box.hasKeyboardFocus (true) || isButtonDown;

    g.setColour (black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 10.0f);
    g.setColour (panel2.brighter (active ? 0.06f : 0.0f));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (cyan.withAlpha (active ? 0.75f : 0.45f));
    g.drawRoundedRectangle (r, 10.0f, 1.1f);

    const auto cx = (float) buttonX + (float) buttonW * 0.5f;
    const auto cy = (float) buttonY + (float) buttonH * 0.5f;
    g.setColour (cyan.withAlpha (0.95f));
    juce::Path arrow;
    arrow.addTriangle (cx - 4.0f, cy - 1.0f, cx + 4.0f, cy - 1.0f, cx, cy + 3.5f);
    g.fillPath (arrow);
}

void HomeSeriesTriggerLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.setColour (bg0);
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 11.0f);
    g.setColour (edge.withAlpha (0.95f));
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 11.0f, 1.0f);
}

void HomeSeriesTriggerLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                                       bool isSeparator, bool isActive, bool isHighlighted,
                                                       bool isTicked, bool, const juce::String& text,
                                                       const juce::String&, const juce::Drawable*, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (edgeSoft);
        g.drawHorizontalLine (area.getCentreY(), area.getX() + 10, area.getRight() - 10);
        return;
    }

    auto r = area.toFloat().reduced (7.0f, 2.0f);
    if (isHighlighted)
    {
        g.setColour (cyan.withAlpha (0.13f));
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (cyan.withAlpha (0.55f));
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
    }

    g.setFont (font (15.0f, isHighlighted || isTicked));
    g.setColour ((textColour != nullptr ? *textColour : (isActive ? white : muted)).withAlpha (isActive ? 1.0f : 0.48f));
    g.drawText (text.trim(), r.toNearestInt(), juce::Justification::centred);

    if (isTicked)
    {
        g.setColour (cyan);
        g.fillEllipse (r.getX() + 10.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
    }
}

void HomeSeriesTriggerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                       bool highlighted, bool)
{
    if (button.getName() != "BYPASS_SWITCH")
        return;

    auto bounds = button.getLocalBounds().toFloat();
    const bool bypassed = button.getToggleState();
    const auto sw = juce::Rectangle<float> (bounds.getRight() - 52.0f, bounds.getCentreY() - 13.0f, 52.0f, 26.0f);
    const auto accent = bypassed ? red : cyan;

    g.setFont (font (9.0f, true));
    g.setColour (muted.withAlpha (0.75f));
    g.drawText ("BYPASS", bounds.getX(), bounds.getY(), 48.0f, bounds.getHeight(), juce::Justification::centredLeft);

    g.setColour (black.withAlpha (0.55f));
    g.fillRoundedRectangle (sw.translated (0.0f, 2.0f), 13.0f);
    g.setColour (panel2);
    g.fillRoundedRectangle (sw, 13.0f);
    g.setColour (accent.withAlpha (highlighted ? 0.95f : 0.72f));
    g.drawRoundedRectangle (sw, 13.0f, 1.2f);

    const float knob = 18.0f;
    const float leftX = sw.getX() + 4.0f;
    const float rightX = sw.getRight() - knob - 4.0f;
    const float x = bypassed ? rightX : leftX;
    const auto dot = juce::Rectangle<float> (x, sw.getCentreY() - knob * 0.5f, knob, knob);
    g.setColour (accent.withAlpha (0.14f));
    g.fillEllipse (dot.expanded (3.0f));
    g.setColour (white);
    g.fillEllipse (dot);

    g.setFont (font (6.5f, true));
    g.setColour (accent);
    g.drawText (bypassed ? "OFF" : "ON", sw.toNearestInt(), juce::Justification::centred);
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

float HomeSidechainTriggerGapSlider::trackStartX() const noexcept { return 168.0f; }
float HomeSidechainTriggerGapSlider::trackEndX() const noexcept { return juce::jmax (trackStartX() + 90.0f, (float) getWidth() - 118.0f); }

void HomeSidechainTriggerGapSlider::setValueFromMouseX (float x)
{
    const float start = trackStartX();
    const float end = trackEndX();
    const double proportion = juce::jlimit (0.0, 1.0, (double) (x - start) / juce::jmax (1.0, (double) (end - start)));
    setValue (proportionOfLengthToValue (proportion), juce::sendNotificationSync);
}

void HomeSidechainTriggerGapSlider::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled() || ! e.mods.isLeftButtonDown())
        return juce::Slider::mouseDown (e);

    const auto hit = juce::Rectangle<float> (trackStartX() - 24.0f,
                                             getLocalBounds().getCentreY() - 18.0f,
                                             trackEndX() - trackStartX() + 48.0f, 36.0f);
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
    const float cy = b.getCentreY();
    const float x0 = trackStartX();
    const float x1 = trackEndX();
    const float h = 6.0f;
    const float thumbR = 11.0f;
    const double proportion = juce::jlimit (0.0, 1.0, valueToProportionOfLength (getValue()));
    const float px = x0 + (x1 - x0) * (float) proportion;

    g.setFont (font (13.0f, true));
    g.setColour (cyan);
    g.drawText ("COOL DOWN", 24, cy - 16.0f, 120, 18, juce::Justification::left);
    g.setFont (font (8.0f, false));
    g.setColour (muted);
    g.drawText ("TIME BETWEEN TRIGGERS", 24, cy + 2.0f, 140, 14, juce::Justification::left);

    const auto track = juce::Rectangle<float> (x0, cy - h * 0.5f, x1 - x0, h);
    g.setColour (black.withAlpha (0.62f));
    g.fillRoundedRectangle (track.expanded (2.0f, 3.0f), 5.0f);
    g.setColour (juce::Colour (0xff0a1117));
    g.fillRoundedRectangle (track, 3.0f);

    const auto fill = track.withWidth (juce::jmax (0.0f, px - track.getX()));
    g.setColour (cyan.withAlpha (0.90f));
    g.fillRoundedRectangle (fill, 3.0f);

    // Subtle scale ticks, matching the visual language of the reference mockup.
    for (int i = 0; i <= 10; ++i)
    {
        const float tx = x0 + (x1 - x0) * (float) i / 10.0f;
        g.setColour (white.withAlpha (i == 0 || i == 5 || i == 10 ? 0.18f : 0.08f));
        g.drawLine (tx, cy + 9.0f, tx, cy + 14.0f, 1.0f);
    }

    g.setColour (cyan.withAlpha (0.12f));
    g.fillEllipse (px - 19.0f, cy - 19.0f, 38.0f, 38.0f);
    g.setColour (white);
    g.fillEllipse (px - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    g.setColour (cyan);
    g.drawEllipse (px - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f, 2.0f);

    const auto valueBox = juce::Rectangle<float> (b.getRight() - 94.0f, cy - 18.0f, 82.0f, 36.0f);
    g.setColour (black.withAlpha (0.52f));
    g.fillRoundedRectangle (valueBox, 9.0f);
    g.setColour (cyan.withAlpha (0.40f));
    g.drawRoundedRectangle (valueBox, 9.0f, 1.0f);
    g.setFont (font (13.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms", valueBox.toNearestInt(), juce::Justification::centred);

    g.setFont (font (7.0f, false));
    g.setColour (muted.withAlpha (0.72f));
    g.drawText ("5", (int) x0 - 5, (int) cy + 15, 20, 11, juce::Justification::left);
    g.drawText ("500", (int) (x0 + (x1 - x0) * 0.5f - 10), (int) cy + 15, 30, 11, juce::Justification::centred);
    g.drawText ("1000", (int) x1 - 25, (int) cy + 15, 28, 11, juce::Justification::right);
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
    setSize (720, 460);
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

    startTimerHz (24);
}

void HomeSidechainTriggerAudioProcessorEditor::styleComboBox()
{
    link.setJustificationType (juce::Justification::centred);
    link.setColour (juce::ComboBox::backgroundColourId, panel2);
    link.setColour (juce::ComboBox::textColourId, white);
    link.setColour (juce::ComboBox::outlineColourId, cyan.withAlpha (0.42f));
    link.setColour (juce::ComboBox::arrowColourId, cyan);
    link.setColour (juce::ComboBox::focusedOutlineColourId, cyan);
    link.setTooltip ("Home-Link destination A, B or C");
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
    return graphBounds.getBottom() - n * graphBounds.getHeight();
}

float HomeSidechainTriggerAudioProcessorEditor::thresholdForY (float y) const noexcept
{
    constexpr float minDb = -48.0f;
    constexpr float maxDb = 0.0f;
    const float n = juce::jlimit (0.0f, 1.0f, (graphBounds.getBottom() - y) / graphBounds.getHeight());
    return minDb + n * (maxDb - minDb);
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
    g.setColour (colour.withAlpha (active ? 0.13f : 0.05f));
    g.fillRoundedRectangle (area, area.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (active ? 0.85f : 0.35f));
    g.drawRoundedRectangle (area, area.getHeight() * 0.5f, 1.0f);
    g.setColour (colour);
    g.fillEllipse (area.getX() + 9.0f, area.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setFont (font (9.0f, true));
    g.drawText (label, area.getX() + 22.0f, area.getY(), area.getWidth() - 26.0f, area.getHeight(), juce::Justification::centredLeft);
}

void HomeSidechainTriggerAudioProcessorEditor::drawBackgroundTexture (juce::Graphics& g, juce::Rectangle<float> area) const
{
    juce::ColourGradient cyanGlow (cyan.withAlpha (0.055f), area.getX() + 80.0f, area.getY() + 10.0f,
                                   juce::Colours::transparentBlack, area.getRight() - 180.0f, area.getBottom(), true);
    g.setGradientFill (cyanGlow);
    g.fillEllipse (area.getX() - 50.0f, area.getY() - 60.0f, area.getWidth() * 0.72f, area.getHeight() * 0.80f);

    juce::ColourGradient rightGlow (juce::Colour (0xff185a73).withAlpha (0.035f), area.getRight() - 100.0f, area.getY(),
                                    juce::Colours::transparentBlack, area.getX() + 120.0f, area.getBottom(), true);
    g.setGradientFill (rightGlow);
    g.fillEllipse (area.getRight() - 300.0f, area.getY() - 20.0f, 330.0f, 260.0f);

    g.setColour (white.withAlpha (0.009f));
    for (float y = area.getY() + 2.0f; y < area.getBottom(); y += 7.0f)
        g.drawLine (area.getX(), y, area.getRight(), y);

    g.setColour (white.withAlpha (0.007f));
    for (float x = area.getX(); x < area.getRight(); x += 7.0f)
        g.drawLine (x, area.getY(), x, area.getBottom());
}

void HomeSidechainTriggerAudioProcessorEditor::drawCard (juce::Graphics& g, juce::Rectangle<float> r,
                                                           juce::Colour colour, bool brightHeader) const
{
    drawGradientPanel (g, r, colour);
    if (brightHeader)
    {
        g.setColour (colour.withAlpha (0.07f));
        g.fillRoundedRectangle (r.getX() + 1.0f, r.getY() + 1.0f, r.getWidth() - 2.0f, 48.0f, 10.0f);
    }
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setFont (font (8.0f, true));
    g.setColour (cyan.withAlpha (0.75f));
    g.drawText ("D U B T A C H   D S P", area.getX(), area.getY() + 37.0f, 170, 12, juce::Justification::left);

    const auto titleFont = font (29.0f, true);
    const juce::String prefix = "HOME-SIDECHAIN";
    const juce::String product = " TRIGGER";
    const float prefixW = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, prefix);

    g.setFont (titleFont);
    g.setColour (white);
    g.drawText (prefix, area.getX(), area.getY(), prefixW + 2.0f, 34.0f, juce::Justification::left);
    g.setColour (cyan);
    g.drawText (product, area.getX() + prefixW - 2.0f, area.getY(), 170.0f, 34.0f, juce::Justification::left);

    // Link selector label and control, in the same compact top-row language as the mockup.
    g.setFont (font (9.0f, true));
    g.setColour (muted.withAlpha (0.80f));
    g.drawText ("LINK", area.getRight() - 250.0f, area.getY() + 12.0f, 36.0f, 12.0f, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto inner = area.reduced (14.0f, 12.0f);
    const auto plot = juce::Rectangle<float> (inner.getX() + 30.0f, inner.getY() + 8.0f,
                                               inner.getWidth() - 38.0f, inner.getHeight() - 32.0f);
    const float thresholdDb = processor.getThresholdDb();

    g.setColour (black.withAlpha (0.92f));
    g.fillRoundedRectangle (plot.expanded (2.0f), 8.0f);
    g.setColour (cyan.withAlpha (0.08f));
    g.drawRoundedRectangle (plot.expanded (2.0f), 8.0f, 1.0f);

    for (int db = 0; db >= -48; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (white.withAlpha (db == 0 ? 0.11f : 0.045f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setFont (font (8.0f, false));
        g.setColour (muted.withAlpha (0.72f));
        g.drawText (db == 0 ? "0" : juce::String (db), inner.getX(), y - 5.0f, 24.0f, 10.0f, juce::Justification::left);
    }

    constexpr int divisions = 8;
    for (int d = 1; d < divisions; ++d)
    {
        const float x = plot.getX() + plot.getWidth() * (float) d / (float) divisions;
        g.setColour (white.withAlpha (0.025f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }

    g.setFont (font (8.0f, true));
    g.setColour (muted.withAlpha (0.50f));
    g.drawText ("PAST", plot.getX(), plot.getBottom() + 7.0f, 32.0f, 11.0f, juce::Justification::left);
    g.drawText ("NOW", plot.getRight() - 34.0f, plot.getBottom() + 7.0f, 34.0f, 11.0f, juce::Justification::right);

    const int pointCount = processor.getWaveformPointCount();
    const int latestTriggerPoint = processor.getLatestTriggerPointIndex();
    if (pointCount > 1)
    {
        juce::Path line;
        juce::Path fill;
        juce::Path redLine;
        bool fillStarted = false;
        bool redStarted = false;
        int hotStart = -1;
        int hotEnd = -1;
        constexpr int hotRadius = 5;

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float db = homeSidechain::linearToDb (peak);
            const float y = yForDb (db);
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (pointCount - 1);

            if (! fillStarted)
            {
                fill.startNewSubPath (x, plot.getBottom());
                fill.lineTo (x, y);
                fillStarted = true;
            }
            else
                fill.lineTo (x, y);

            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);

            const bool hot = latestTriggerPoint >= 0 && std::abs (i - latestTriggerPoint) <= hotRadius;
            if (hot)
            {
                if (hotStart < 0) hotStart = i;
                hotEnd = i;
                if (! redStarted) { redLine.startNewSubPath (x, y); redStarted = true; }
                else redLine.lineTo (x, y);
            }
        }

        if (fillStarted)
        {
            fill.lineTo (plot.getRight(), plot.getBottom());
            fill.closeSubPath();
            g.setColour (cyan.withAlpha (0.065f));
            g.fillPath (fill);
        }

        g.setColour (cyan.withAlpha (0.10f));
        g.strokePath (line, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.24f));
        g.strokePath (line, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.55f));
        g.strokePath (line, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved));
        g.setColour (white);
        g.strokePath (line, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));

        if (! redLine.isEmpty())
        {
            const float hotX0 = plot.getX() + plot.getWidth() * (float) hotStart / (float) (pointCount - 1);
            const float hotX1 = plot.getX() + plot.getWidth() * (float) hotEnd / (float) (pointCount - 1);
            const float left = juce::jmax (plot.getX(), hotX0 - 10.0f);
            const float right = juce::jmin (plot.getRight(), hotX1 + 10.0f);
            g.setColour (red.withAlpha (0.045f));
            g.fillRoundedRectangle (left, plot.getY(), right - left, plot.getHeight(), 6.0f);
            g.setColour (red.withAlpha (0.20f));
            g.strokePath (redLine, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved));
            g.setColour (red);
            g.strokePath (redLine, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));
        }
    }

    // Threshold line and badge.
    const float thresholdY = yForDb (thresholdDb);
    g.setColour (cyan.withAlpha (0.10f));
    g.fillRoundedRectangle (plot.getX(), thresholdY - 4.0f, plot.getWidth(), 8.0f, 4.0f);
    g.setColour (black.withAlpha (0.82f));
    g.drawLine (plot.getX(), thresholdY + 1.0f, plot.getRight(), thresholdY + 1.0f, 3.0f);
    g.setColour (cyan);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 1.5f);

    const float handleR = 8.0f;
    const auto handle = juce::Point<float> (plot.getRight(), thresholdY);
    g.setColour (cyan.withAlpha (0.16f));
    g.fillEllipse (handle.x - 13.0f, handle.y - 13.0f, 26.0f, 26.0f);
    g.setColour (cyan);
    g.fillEllipse (handle.x - handleR, handle.y - handleR, handleR * 2.0f, handleR * 2.0f);
    g.setColour (white);
    g.fillEllipse (handle.x - 3.0f, handle.y - 3.0f, 6.0f, 6.0f);

    const auto tag = juce::Rectangle<float> (plot.getRight() - 150.0f, thresholdY - 29.0f, 140.0f, 46.0f);
    g.setColour (black.withAlpha (0.94f));
    g.fillRoundedRectangle (tag, 9.0f);
    g.setColour (cyan.withAlpha (0.28f));
    g.drawRoundedRectangle (tag, 9.0f, 1.0f);
    g.setFont (font (8.0f, true));
    g.setColour (muted);
    g.drawText ("THRESHOLD", tag.getX() + 12.0f, tag.getY() + 7.0f, 100.0f, 12.0f, juce::Justification::left);
    g.setFont (font (15.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (thresholdDb, 1) + " dB", tag.getX() + 12.0f, tag.getY() + 20.0f, tag.getWidth() - 24.0f, 20.0f, juce::Justification::left);

    g.setFont (font (8.0f, false));
    g.setColour (muted.withAlpha (0.62f));
    g.drawText ("DRAG THRESHOLD", plot.getX(), area.getBottom() - 8.0f, 110.0f, 11.0f, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::drawUtilityPanel (juce::Graphics&, juce::Rectangle<float>) const
{
}

void HomeSidechainTriggerAudioProcessorEditor::drawCooldownPanel (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawGradientPanel (g, area, cyan);
    g.setColour (cyan.withAlpha (0.035f));
    g.fillRoundedRectangle (area.reduced (1.0f), 11.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);

    const auto frame = juce::Rectangle<float> (8.0f, 8.0f, 704.0f, 444.0f);
    g.setColour (bg1);
    g.fillRoundedRectangle (frame, 15.0f);
    drawBackgroundTexture (g, frame);
    g.setColour (edge.withAlpha (0.95f));
    g.drawRoundedRectangle (frame, 15.0f, 1.2f);

    drawHeader (g, { 24.0f, 19.0f, 670.0f, 46.0f });

    // Main analysis card and full-width cooldown card.
    const auto graphCard = juce::Rectangle<float> (20.0f, 88.0f, 680.0f, 286.0f);
    const auto cooldownCard = juce::Rectangle<float> (20.0f, 386.0f, 680.0f, 52.0f);

    drawCard (g, graphCard, cyan, true);
    drawCooldownPanel (g, cooldownCard);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool triggering = processor.getTriggerMeter() > 0.35f && ! bypassed;
    const auto statusColour = triggering ? red : (bypassed ? muted : cyan);
    const auto statusText = triggering ? "TRIGGERING" : (bypassed ? "BYPASSED" : "READY");
    const auto status = juce::Rectangle<float> (graphCard.getX() + 18.0f, graphCard.getY() + 14.0f, 122.0f, 28.0f);
    drawTinyStatus (g, status, statusText, statusColour, true);

    g.setFont (font (8.0f, true));
    g.setColour (muted.withAlpha (0.48f));
    g.drawText ("LIVE INPUT", graphCard.getX() + 154.0f, graphCard.getY() + 18.0f, 80.0f, 12.0f, juce::Justification::left);

    g.setColour (edgeSoft.withAlpha (0.72f));
    g.drawLine (graphCard.getX() + 18.0f, graphCard.getY() + 52.0f,
                graphCard.getRight() - 18.0f, graphCard.getY() + 52.0f, 1.0f);

    drawGraph (g, graphCard.reduced (8.0f, 54.0f));
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    graphBounds = { 72.0f, 160.0f, 598.0f, 126.0f };
    link.setBounds (510, 26, 104, 40);
    bypass.setBounds (618, 24, 84, 44);
    retrigger.setBounds (20, 386, 680, 52);
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() && graphBounds.expanded (12.0f).contains (e.position))
    {
        draggingThreshold = true;
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
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
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    repaint();
}
