#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0        (0xff07080b);
    const juce::Colour bg1        (0xff0d1118);
    const juce::Colour panel      (0xff10141b);
    const juce::Colour panel2     (0xff151a22);
    const juce::Colour edge       (0xff252c35);
    const juce::Colour edgeSoft   (0xff1b222b);
    const juce::Colour white      (0xfff7fafc);
    const juce::Colour muted      (0xff8e98a5);
    const juce::Colour cyan       (0xff16d9ff);
    const juce::Colour green      (0xff3df1a0);
    const juce::Colour magenta    (0xffff3f9f);
    const juce::Colour red        (0xffff5d64);
    const juce::Colour yellow     (0xffffd85f);
    const juce::Colour black      (0xff05070a);

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

void HomeSeriesTriggerLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.setColour (bg0);
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 10.0f);
    g.setColour (edge.withAlpha (0.95f));
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 10.0f, 1.0f);
}

void HomeSeriesTriggerLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                                       bool isSeparator, bool isActive, bool isHighlighted,
                                                       bool isTicked, bool hasSubMenu, const juce::String& text,
                                                       const juce::String&, const juce::Drawable*,
                                                       const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (edge.withAlpha (0.55f));
        g.drawHorizontalLine (area.getCentreY(), area.getX() + 12.0f, area.getRight() - 12.0f);
        return;
    }

    auto r = area.toFloat().reduced (6.0f, 2.0f);
    if (isHighlighted)
    {
        g.setColour (cyan.withAlpha (0.12f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (cyan.withAlpha (0.60f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    const auto colour = textColour != nullptr ? *textColour : (isActive ? white : muted);
    const auto letter = text.trim();
    g.setFont (font (13.0f, isTicked || isHighlighted));
    g.setColour (colour.withAlpha (isActive ? 1.0f : 0.55f));
    g.drawText (letter, r.toNearestInt(), juce::Justification::centred);

    if (isTicked)
    {
        g.setColour (green);
        g.fillEllipse (r.getX() + 9.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
    }
}

void HomeSeriesTriggerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                       bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool bypassed = button.getToggleState();

    if (button.getName() == "BYPASS_SWITCH")
    {
        const float labelW = 38.0f;
        g.setFont (font (9.0f, true));
        g.setColour (white.withAlpha (0.78f));
        g.drawText ("BYPASS", bounds.removeFromLeft (labelW).toNearestInt(), juce::Justification::centredLeft);

        const float switchW = 42.0f;
        const float switchH = 22.0f;
        const auto sw = juce::Rectangle<float> (bounds.getRight() - switchW,
                                                bounds.getCentreY() - switchH * 0.5f,
                                                switchW, switchH);

        const auto accent = bypassed ? red : green;
        g.setColour (black.withAlpha (0.48f));
        g.fillRoundedRectangle (sw.translated (0.0f, 2.0f), 11.0f);
        g.setColour (panel2);
        g.fillRoundedRectangle (sw, 11.0f);
        g.setColour (accent.withAlpha (highlighted ? 0.95f : 0.72f));
        g.drawRoundedRectangle (sw, 11.0f, 1.2f);

        const float knob = 16.0f;
        const float leftX = sw.getX() + 3.0f;
        const float rightX = sw.getRight() - knob - 3.0f;
        const float x = bypassed ? rightX : leftX;
        const auto dot = juce::Rectangle<float> (x, sw.getCentreY() - knob * 0.5f, knob, knob);
        g.setColour (accent.withAlpha (0.20f));
        g.fillEllipse (dot.expanded (2.0f));
        g.setColour (white);
        g.fillEllipse (dot);
        g.setColour (black.withAlpha (0.18f));
        g.drawEllipse (dot, 1.0f);

        g.setFont (font (6.0f, true));
        g.setColour (accent.withAlpha (0.92f));
        g.drawText (bypassed ? "ON" : "OFF", sw.toNearestInt(), juce::Justification::centred);

        if (highlighted)
        {
            g.setColour (white.withAlpha (0.08f));
            g.drawRoundedRectangle (sw.expanded (2.0f), 13.0f, 1.0f);
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
    setWantsKeyboardFocus (false);
}

float HomeSidechainTriggerGapSlider::trackStartX() const noexcept
{
    return 112.0f;
}

float HomeSidechainTriggerGapSlider::trackEndX() const noexcept
{
    return juce::jmax (trackStartX() + 60.0f, (float) getWidth() - 84.0f);
}

void HomeSidechainTriggerGapSlider::setValueFromMouseX (float x)
{
    const float start = trackStartX();
    const float end = trackEndX();
    const double proportion = juce::jlimit (0.0, 1.0, (double) (x - start) / (double) (end - start));
    setValue (proportionOfLengthToValue (proportion), juce::sendNotificationSync);
}

void HomeSidechainTriggerGapSlider::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled() || ! e.mods.isLeftButtonDown())
        return juce::Slider::mouseDown (e);

    const auto hit = juce::Rectangle<float> (trackStartX() - 18.0f,
                                             getLocalBounds().getCentreY() - 15.0f,
                                             trackEndX() - trackStartX() + 36.0f,
                                             30.0f);
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
    g.setFont (font (10.0f, true));
    g.setColour (white);
    g.drawText ("COOL DOWN", 12, juce::roundToInt (b.getCentreY() - 9.0f), 82, 18, juce::Justification::left);
    g.setFont (font (6.5f, true));
    g.setColour (muted.withAlpha (0.58f));
    g.drawText ("MIN TIME", 12, juce::roundToInt (b.getCentreY() + 7.0f), 66, 10, juce::Justification::left);

    const float x0 = trackStartX();
    const float x1 = trackEndX();
    const float cy = b.getCentreY();
    const float h = 5.0f;
    const float thumbR = 7.5f;
    const double proportion = juce::jlimit (0.0, 1.0, valueToProportionOfLength (getValue()));
    const float px = x0 + (x1 - x0) * (float) proportion;

    // The card labels and interactive control share one aligned row.
    const auto track = juce::Rectangle<float> (x0, cy - h * 0.5f, x1 - x0, h);
    g.setColour (black.withAlpha (0.56f));
    g.fillRoundedRectangle (track.expanded (1.0f, 2.0f), 4.0f);
    g.setColour (black.withAlpha (0.34f));
    g.fillRoundedRectangle (track, 2.5f);

    const auto fill = track.withWidth (juce::jmax (0.0f, px - track.getX()));
    if (fill.getWidth() > 0.0f)
    {
        g.setColour (green.withAlpha (0.82f));
        g.fillRoundedRectangle (fill, 2.5f);
    }

    // Small glow + crisp thumb for a more precise control.
    g.setColour (green.withAlpha (0.10f));
    g.fillEllipse (px - 13.0f, cy - 13.0f, 26.0f, 26.0f);
    g.setColour (white);
    g.fillEllipse (px - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    g.setColour (green.withAlpha (0.92f));
    g.drawEllipse (px - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f, 1.2f);

    const auto valueBox = juce::Rectangle<float> (b.getRight() - 72.0f, b.getCentreY() - 14.0f, 64.0f, 28.0f);
    g.setColour (black.withAlpha (0.42f));
    g.fillRoundedRectangle (valueBox, 8.0f);
    g.setColour (edge.withAlpha (0.85f));
    g.drawRoundedRectangle (valueBox, 8.0f, 1.0f);
    g.setFont (font (9.2f, true));
    g.setColour (white);
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
    setSize (580, 310);
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
    // Subtle Home-series lighting: colour belongs to the chassis, not the cards.
    juce::ColourGradient cyanGlow (cyan.withAlpha (0.055f), area.getX() + 90.0f, area.getY() + 4.0f,
                                   juce::Colours::transparentBlack, area.getRight() - 120.0f, area.getBottom(), true);
    g.setGradientFill (cyanGlow);
    g.fillEllipse (area.getX() - 80.0f, area.getY() - 70.0f, area.getWidth() * 0.72f, area.getHeight() * 0.82f);

    juce::ColourGradient magentaGlow (magenta.withAlpha (0.045f), area.getRight() - 70.0f, area.getY() + 40.0f,
                                      juce::Colours::transparentBlack, area.getX() + area.getWidth() * 0.52f, area.getBottom(), true);
    g.setGradientFill (magentaGlow);
    g.fillEllipse (area.getRight() - 250.0f, area.getY() - 20.0f, 290.0f, 240.0f);

    g.setColour (white.withAlpha (0.012f));
    for (float y = area.getY() + 2.0f; y < area.getBottom(); y += 6.0f)
        g.drawLine (area.getX(), y, area.getRight(), y);

    g.setColour (white.withAlpha (0.008f));
    for (float x = area.getX(); x < area.getRight(); x += 6.0f)
        g.drawLine (x, area.getY(), x, area.getBottom());

    g.setColour (white.withAlpha (0.018f));
    g.drawLine (area.getX() + 14.0f, area.getY() + 53.0f, area.getRight() - 14.0f, area.getY() + 53.0f, 1.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawCard (juce::Graphics& g, juce::Rectangle<float> r,
                                                           juce::Colour colour, bool brightHeader) const
{
    drawSoftShadow (g, r, 10.0f);

    juce::ColourGradient base (panel2.withAlpha (0.98f), r.getX(), r.getY(),
                               panel.withAlpha (0.98f), r.getRight(), r.getBottom(), false);
    g.setGradientFill (base);
    g.fillRoundedRectangle (r, 10.0f);

    // Colour is concentrated at the top edge, matching the reference style
    // without turning the whole panel into a flat colour block.
    juce::ColourGradient accent (colour.withAlpha (brightHeader ? 0.20f : 0.11f),
                                 r.getX(), r.getY(),
                                 juce::Colours::transparentBlack,
                                 r.getX(), r.getY() + 38.0f, false);
    g.setGradientFill (accent);
    g.fillRoundedRectangle (r, 10.0f);

    g.setColour (edgeSoft);
    g.drawRoundedRectangle (r, 10.0f, 1.0f);
    g.setColour (colour.withAlpha (0.42f));
    g.drawRoundedRectangle (r.reduced (0.6f), 9.4f, 0.8f);

    juce::ColourGradient sheen (white.withAlpha (0.032f), r.getX(), r.getY(),
                                juce::Colours::transparentWhite, r.getX(), r.getY() + 24.0f, false);
    g.setGradientFill (sheen);
    g.fillRoundedRectangle (r.reduced (1.0f), 9.0f);
}

void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto title = font (21.0f, true);
    const auto subtitle = font (7.2f, true);
    const juce::String brand = "HOME-SIDECHAIN ";
    const juce::String product = "TRIGGER";
    const int brandW = juce::GlyphArrangement::getStringWidthInt (title, brand);

    g.setFont (title);
    g.setColour (white);
    g.drawText (brand, area.getX(), area.getY(), brandW + 4, 26, juce::Justification::left);
    g.setColour (green);
    g.drawText (product, area.getX() + brandW - 1, area.getY(), 118, 26, juce::Justification::left);

    g.setFont (subtitle);
    g.setColour (muted.withAlpha (0.52f));
    g.drawText ("DUBTACH DSP", area.getX(), area.getY() + 26.0f, 120, 10, juce::Justification::left);

    // Compact header destination control: the A/B/C selector belongs here
    // because it identifies where this Trigger instance sends its event.
    g.setFont (font (6.6f, true));
    g.setColour (muted.withAlpha (0.60f));
    g.drawText ("LINK", area.getX() + 350.0f, area.getY() + 11.0f, 30.0f, 10.0f, juce::Justification::right);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraph (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto inner = area.reduced (10.0f, 4.0f);
    const auto plot = juce::Rectangle<float> (inner.getX() + 25.0f, inner.getY() + 2.0f,
                                               inner.getWidth() - 31.0f, juce::jmax (40.0f, inner.getHeight() - 10.0f));
    const float thresholdDb = processor.getThresholdDb();

    // Inner scope surface.
    g.setColour (black.withAlpha (0.88f));
    g.fillRoundedRectangle (plot.expanded (2.0f), 7.0f);
    g.setColour (cyan.withAlpha (0.10f));
    g.drawRoundedRectangle (plot.expanded (2.0f), 7.0f, 1.0f);

    // Horizontal dB grid.
    for (int db = 0; db >= -48; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (white.withAlpha (db == 0 ? 0.10f : 0.045f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setFont (font (6.8f, true));
        g.setColour (muted.withAlpha (0.65f));
        g.drawText (juce::String (db), inner.getX(), y - 4.0f, 20.0f, 9.0f, juce::Justification::left);
    }

    // Fine time divisions.
    constexpr int divisions = 10;
    for (int d = 1; d < divisions; ++d)
    {
        const float x = plot.getX() + plot.getWidth() * (float) d / (float) divisions;
        g.setColour (white.withAlpha (0.024f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }

    g.setFont (font (6.6f, true));
    g.setColour (muted.withAlpha (0.48f));
    g.drawText ("PAST", plot.getX(), plot.getBottom() + 4.0f, 30.0f, 9.0f, juce::Justification::left);
    g.drawText ("NOW", plot.getRight() - 28.0f, plot.getBottom() + 4.0f, 28.0f, 9.0f, juce::Justification::right);

    if (processor.getWaveformPointCount() > 1)
    {
        const int pointCount = processor.getWaveformPointCount();
        const int latestTriggerPoint = processor.getLatestTriggerPointIndex();
        juce::Path line;
        juce::Path fill;
        juce::Path redLine;
        bool redStarted = false;
        const int redRadius = 6;
        int hotStart = -1;
        int hotEnd = -1;
        bool fillStarted = false;

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
            {
                fill.lineTo (x, y);
            }

            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);

            const bool hot = latestTriggerPoint >= 0 && std::abs (i - latestTriggerPoint) <= redRadius;
            if (hot)
            {
                if (hotStart < 0) hotStart = i;
                hotEnd = i;
                if (! redStarted) { redLine.startNewSubPath (x, y); redStarted = true; }
                else redLine.lineTo (x, y);
            }
            else
            {
                if (redStarted && i > hotEnd)
                    redStarted = false;
            }
        }

        if (fillStarted)
        {
            fill.lineTo (plot.getRight(), plot.getBottom());
            fill.closeSubPath();
            g.setColour (cyan.withAlpha (0.050f));
            g.fillPath (fill);
        }

        g.setColour (cyan.withAlpha (0.10f));
        g.strokePath (line, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.24f));
        g.strokePath (line, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved));
        g.setColour (cyan.withAlpha (0.58f));
        g.strokePath (line, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        g.setColour (white.withAlpha (0.96f));
        g.strokePath (line, juce::PathStrokeType (0.9f, juce::PathStrokeType::curved));

        if (! redLine.isEmpty())
        {
            const float hotX0 = plot.getX() + plot.getWidth() * (float) hotStart / (float) (pointCount - 1);
            const float hotX1 = plot.getX() + plot.getWidth() * (float) hotEnd / (float) (pointCount - 1);
            const float left = juce::jmax (plot.getX(), hotX0 - 8.0f);
            const float right = juce::jmin (plot.getRight(), hotX1 + 8.0f);
            g.setColour (red.withAlpha (0.065f));
            g.fillRoundedRectangle (left, plot.getY() + 2.0f, right - left, plot.getHeight() - 4.0f, 5.0f);
            g.setColour (red.withAlpha (0.24f));
            g.strokePath (redLine, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
            g.setColour (red);
            g.strokePath (redLine, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
        }
    }

    // Threshold control remains on the exact same -48..0 dB scale as the waveform.
    const float thresholdY = yForDb (thresholdDb);
    g.setColour (cyan.withAlpha (0.08f));
    g.fillRoundedRectangle (plot.getX(), thresholdY - 4.0f, plot.getWidth(), 8.0f, 4.0f);
    const juce::Line<float> shadowLine (plot.getX(), thresholdY + 0.9f, plot.getRight(), thresholdY + 0.9f);
    g.setColour (black.withAlpha (0.75f));
    g.drawLine (shadowLine, 3.0f);
    g.setColour (cyan);
    g.drawLine (plot.getX(), thresholdY, plot.getRight(), thresholdY, 1.2f);

    const auto handle = juce::Rectangle<float> (plot.getRight() - 7.0f, thresholdY - 7.0f, 14.0f, 14.0f);
    g.setColour (cyan.withAlpha (0.15f));
    g.fillEllipse (handle.expanded (3.0f));
    g.setColour (cyan);
    g.fillEllipse (handle);
    g.setColour (white);
    g.fillEllipse (handle.reduced (4.0f));

    const auto tag = juce::Rectangle<float> (plot.getRight() - 88.0f, thresholdY - 11.0f, 80.0f, 22.0f);
    g.setColour (black.withAlpha (0.90f));
    g.fillRoundedRectangle (tag, 6.0f);
    g.setColour (cyan.withAlpha (0.42f));
    g.drawRoundedRectangle (tag, 6.0f, 1.0f);
    g.setFont (font (6.9f, true));
    g.setColour (cyan.withAlpha (0.72f));
    g.drawText ("THRESHOLD", tag.getX() + 6.0f, tag.getY(), 41.0f, tag.getHeight(), juce::Justification::centredLeft);
    g.setColour (white);
    g.drawText (juce::String (thresholdDb, 1) + " dB", tag.getX() + 43.0f, tag.getY(), tag.getWidth() - 49.0f, tag.getHeight(), juce::Justification::centredRight);
}

void HomeSidechainTriggerAudioProcessorEditor::drawUtilityPanel (juce::Graphics&, juce::Rectangle<float>) const
{
    // Reserved for future utility content; the compact Trigger no longer
    // displays a separate Link/Output panel.
}

void HomeSidechainTriggerAudioProcessorEditor::drawCooldownPanel (juce::Graphics& g, juce::Rectangle<float> area) const
{
    auto r = area.reduced (0.5f);
    juce::ColourGradient bg (green.withAlpha (0.13f), r.getX(), r.getY(),
                             green.withAlpha (0.035f), r.getRight(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (green.withAlpha (0.40f));
    g.drawRoundedRectangle (r, 9.0f, 1.0f);

}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);

    const auto frame = juce::Rectangle<float> (7.0f, 7.0f, 566.0f, 296.0f);
    g.setColour (bg1);
    g.fillRoundedRectangle (frame, 13.0f);
    drawBackgroundTexture (g, frame);

    g.setColour (edge.withAlpha (0.95f));
    g.drawRoundedRectangle (frame, 13.0f, 1.1f);

    drawHeader (g, { 18.0f, 14.0f, 544.0f, 42.0f });

    const auto graphCard = juce::Rectangle<float> (14.0f, 64.0f, 552.0f, 198.0f);
    const auto cooldownCard = juce::Rectangle<float> (14.0f, 267.0f, 552.0f, 28.0f);

    drawCard (g, graphCard, cyan, true);
    drawCard (g, cooldownCard, green, false);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool triggering = processor.getTriggerMeter() > 0.35f && ! bypassed;
    const auto statusColour = triggering ? red : (bypassed ? muted : green);
    const auto statusText = triggering ? "TRIGGERING" : (bypassed ? "BYPASSED" : "READY");

    auto status = juce::Rectangle<float> (graphCard.getX() + 12.0f, graphCard.getY() + 7.0f, 108.0f, 20.0f);
    g.setColour (black.withAlpha (0.42f));
    g.fillRoundedRectangle (status, 8.0f);
    g.setColour (statusColour.withAlpha (triggering ? 0.70f : 0.34f));
    g.drawRoundedRectangle (status, 8.0f, 1.0f);
    g.setColour (statusColour);
    g.fillEllipse (status.getX() + 8.0f, status.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setFont (font (7.4f, true));
    g.drawText (statusText, status.getX() + 20.0f, status.getY(), status.getWidth() - 24.0f, status.getHeight(), juce::Justification::centredLeft);

    g.setFont (font (6.4f, true));
    g.setColour (muted.withAlpha (0.50f));
    g.drawText ("DRAG TO SET THRESHOLD", graphCard.getRight() - 126.0f, graphCard.getY() + 12.0f, 112.0f, 9.0f, juce::Justification::right);

    g.setColour (edgeSoft.withAlpha (0.70f));
    g.drawLine (graphCard.getX() + 12.0f, graphCard.getY() + 31.0f,
                graphCard.getRight() - 12.0f, graphCard.getY() + 31.0f, 1.0f);

    drawGraph (g, graphCard.reduced (8.0f, 34.0f));
    drawCooldownPanel (g, cooldownCard);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    // The graph and cooldown now span the full plugin width. The link
    // destination lives in the header next to bypass, so no utility panel
    // is needed.
    graphBounds = { 57.0f, 102.0f, 485.0f, 112.0f };
    link.setBounds (392, 21, 68, 25);
    bypass.setBounds (474, 19, 84, 27);
    retrigger.setBounds (14, 267, 552, 28);
}

void HomeSidechainTriggerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() && graphBounds.expanded (8.0f).contains (e.position))
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
