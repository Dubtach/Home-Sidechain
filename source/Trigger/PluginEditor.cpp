#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0       (0xff040609);
    const juce::Colour bg1       (0xff090d12);
    const juce::Colour panel     (0xff0c1319);
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
        g.setColour (black.withAlpha (0.70f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.5f), radius + 1.0f);

        juce::ColourGradient fill (panel.brighter (0.04f), r.getX(), r.getY(),
                                   bg1, r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);

        juce::ColourGradient glow (accent.withAlpha (0.065f), r.getX(), r.getY(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r.reduced (1.0f), radius - 1.0f);

        g.setColour (edge.withAlpha (0.98f));
        g.drawRoundedRectangle (r, radius, 1.1f);
        g.setColour (accent.withAlpha (0.48f));
        g.drawRoundedRectangle (r.reduced (0.8f), radius - 0.8f, 0.8f);
    }
}

HomeSeriesTriggerLookAndFeel::HomeSeriesTriggerLookAndFeel() = default;

void HomeSeriesTriggerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                          const juce::Colour& backgroundColour,
                                                          bool highlighted, bool down)
{
    // Keep the required Button LookAndFeel hooks fully defined even though
    // the current Trigger UI only uses a custom ToggleButton for bypass.
    // Delegate to JUCE for any future buttons instead of leaving unresolved
    // vtable symbols at link time.
    juce::LookAndFeel_V4::drawButtonBackground (g, button, backgroundColour, highlighted, down);
}

void HomeSeriesTriggerLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                                    bool highlighted, bool down)
{
    juce::LookAndFeel_V4::drawButtonText (g, button, highlighted, down);
}

void HomeSeriesTriggerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                      bool highlighted, bool)
{
    if (button.getName() != "BYPASS_SWITCH")
        return;

    const auto b = button.getLocalBounds().toFloat().reduced (3.0f);
    const bool bypassed = button.getToggleState();
    const auto accent = bypassed ? red : cyan;
    const float size = juce::jmin (b.getWidth(), b.getHeight());
    const auto iconBox = juce::Rectangle<float> (b.getCentreX() - size * 0.5f,
                                                   b.getCentreY() - size * 0.5f,
                                                   size, size);

    g.setColour (black.withAlpha (0.70f));
    g.fillEllipse (iconBox.translated (0.0f, 2.0f));
    g.setColour (juce::Colour (0xff0d141a));
    g.fillEllipse (iconBox);
    g.setColour (accent.withAlpha (highlighted ? 1.0f : 0.86f));
    g.drawEllipse (iconBox, 1.5f);

    const float cx = iconBox.getCentreX();
    const float cy = iconBox.getCentreY();
    const float r = size * 0.24f;
    juce::Path arc;
    arc.addCentredArc (cx, cy, r, r, 0.0f,
                       juce::MathConstants<float>::pi * 0.22f,
                       juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi * 0.22f,
                       true);

    g.setColour (accent.withAlpha (0.15f));
    g.strokePath (arc, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    g.setColour (white);
    g.drawLine (cx, cy - r - 2.0f, cx, cy + 0.5f, 1.8f);
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

    g.setColour (black.withAlpha (0.72f));
    g.fillRoundedRectangle (b.translated (0.0f, 2.0f), b.getHeight() * 0.5f + 1.0f);
    g.setColour (juce::Colour (0xff0d1319));
    g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    g.setColour (edge.withAlpha (0.96f));
    g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.0f);

    const float inset = 4.0f;
    const float contentX = b.getX() + inset;
    const float contentW = b.getWidth() - inset * 2.0f;
    const float segmentW = contentW / 3.0f;

    if (active >= 0 && active < 3)
    {
        const float x = contentX + segmentW * static_cast<float> (active);
        const auto activeRect = juce::Rectangle<float> (x, b.getY() + 4.0f, segmentW, b.getHeight() - 8.0f);
        g.setColour (cyan.withAlpha (0.10f));
        g.fillRoundedRectangle (activeRect, 11.0f);
        g.setColour (cyan.withAlpha (0.82f));
        g.drawRoundedRectangle (activeRect, 11.0f, 1.0f);
        g.setColour (cyan);
        g.fillRoundedRectangle (x + 13.0f, b.getBottom() - 4.0f, segmentW - 26.0f, 2.0f, 1.0f);
    }

    for (int i = 0; i < 3; ++i)
    {
        const float x = contentX + segmentW * static_cast<float> (i);
        g.setFont (uiFont (13.0f, true));
        g.setColour (i == active ? white : white.withAlpha (0.48f));
        g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + i)),
                    juce::Rectangle<float> (x, b.getY(), segmentW, b.getHeight()),
                    juce::Justification::centred, true);
    }
}

void HomeSidechainTriggerLinkSelector::mouseDown (const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    const float segmentW = juce::jmax (1.0f, static_cast<float> (getWidth()) / 3.0f);
    const int index = juce::jlimit (0, 2, static_cast<int> (std::floor (e.position.x / segmentW)));

    if (auto* parameter = processor.apvts.getParameter ("LINK"))
    {
        const float normalized = parameter->getNormalisableRange().convertTo0to1 (static_cast<float> (index));
        parameter->setValueNotifyingHost (normalized);
    }
    repaint();
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
    return 8.0f;
}

float HomeSidechainTriggerGapSlider::trackEndX() const noexcept
{
    return juce::jmax (trackStartX() + 180.0f, static_cast<float> (getWidth()) - 145.0f);
}

bool HomeSidechainTriggerGapSlider::hitTest (int x, int y)
{
    const float cy = getLocalBounds().getCentreY() - 1.0f;
    const auto hit = juce::Rectangle<float> (trackStartX() - 5.0f, cy - 9.0f,
                                              trackEndX() - trackStartX() + 10.0f, 18.0f);
    return hit.contains (static_cast<float> (x), static_cast<float> (y));
}

void HomeSidechainTriggerGapSlider::setValueFromMouseX (float x)
{
    const float start = trackStartX();
    const float end = trackEndX();
    const double denominator = juce::jmax (1.0, static_cast<double> (end - start));
    const double proportion = juce::jlimit<double> (0.0, 1.0,
        static_cast<double> (x - start) / denominator);

    // Map through the real parameter range. This keeps mouse position, the
    // displayed thumb and the skewed Cool Down parameter in perfect agreement.
    const auto range = getNormalisableRange();
    setValue (range.convertFrom0to1 (proportion), juce::sendNotificationSync);
}

void HomeSidechainTriggerGapSlider::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown() || ! hitTest (e.getMouseDownX(), e.getMouseDownY()))
    {
        manualMouseTracking = false;
        return;
    }

    manualMouseTracking = true;
    setValueFromMouseX (e.position.x);
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
    const float cy = b.getCentreY() - 1.0f;
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
        g.drawLine (tx, cy + 9.0f, tx, cy + 9.0f + th, 1.0f);
    }

    g.setColour (cyan.withAlpha (0.14f));
    g.fillEllipse (px - 24.0f, cy - 24.0f, 48.0f, 48.0f);
    g.setColour (black.withAlpha (0.80f));
    g.fillEllipse (px - 15.0f, cy - 15.0f, 30.0f, 30.0f);
    g.setColour (cyan);
    g.drawEllipse (px - 14.0f, cy - 14.0f, 28.0f, 28.0f, 2.0f);
    g.setColour (white.withAlpha (0.95f));
    g.fillEllipse (px - 9.0f, cy - 9.0f, 18.0f, 18.0f);
    g.setColour (juce::Colour (0xffb7edf3).withAlpha (0.7f));
    g.fillEllipse (px - 5.5f, cy - 5.5f, 11.0f, 11.0f);

    const auto valueBox = juce::Rectangle<float> (b.getRight() - 116.0f, cy - 20.0f, 100.0f, 40.0f);
    g.setColour (black.withAlpha (0.72f));
    g.fillRoundedRectangle (valueBox, 10.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (valueBox, 10.0f, 1.0f);
    g.setFont (uiFont (14.0f, true));
    g.setColour (cyan);
    g.drawText (juce::String (juce::roundToInt (getValue())) + " ms", valueBox.toNearestInt().reduced (2, 0), juce::Justification::centred, true);

    g.setFont (uiFont (7.8f));
    g.setColour (muted.withAlpha (0.72f));
    g.drawText ("50 ms", static_cast<int> (x0 - 4), static_cast<int> (cy + 14), 36, 10, juce::Justification::left, true);
    g.drawText ("500 ms", static_cast<int> (x0 + (x1 - x0) * 0.50f - 25), static_cast<int> (cy + 14), 50, 10, juce::Justification::centred, true);
    g.drawText ("2000 ms", static_cast<int> (x1 - 48), static_cast<int> (cy + 14), 48, 10, juce::Justification::right, true);
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
    setSize (600, 365);
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
    g.setColour (white.withAlpha (0.0065f));
    for (float y = area.getY(); y < area.getBottom(); y += 7.0f)
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
    for (float x = area.getX(); x < area.getRight(); x += 11.0f)
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);

    g.setColour (edge.withAlpha (0.98f));
    g.drawRoundedRectangle (area.reduced (0.5f), 13.0f, 1.0f);
    g.setColour (white.withAlpha (0.03f));
    g.drawLine (area.getX() + 16.0f, area.getY() + 1.0f, area.getRight() - 16.0f, area.getY() + 1.0f, 1.0f);
}


void HomeSidechainTriggerAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const float baseX = area.getX() + 2.0f;
    const float utilitiesLeft = area.getRight() - 222.0f;
    const float maxTitleWidth = juce::jmax (220.0f, utilitiesLeft - baseX - 16.0f);

    juce::Font titleFont = uiFont (20.5f, true);
    const juce::String p1 = "HOME-";
    const juce::String p2 = "SIDECHAIN";
    const juce::String p3 = "TRIGGER";

    for (float size = 20.5f; size >= 17.0f; size -= 0.5f)
    {
        const auto candidate = uiFont (size, true);
        const float total = (float) juce::GlyphArrangement::getStringWidthInt (candidate, p1)
                          + (float) juce::GlyphArrangement::getStringWidthInt (candidate, p2)
                          + (float) juce::GlyphArrangement::getStringWidthInt (candidate, p3) + 2.0f;
        if (total <= maxTitleWidth)
        {
            titleFont = candidate;
            break;
        }
    }

    float x = baseX;
    const float titleH = 23.0f;
    const float p1w = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, p1);
    const float p2w = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, p2);
    const float p3w = (float) juce::GlyphArrangement::getStringWidthInt (titleFont, p3);

    g.setFont (titleFont);
    g.setColour (white);
    g.drawText (p1, juce::Rectangle<float> (x, area.getY(), p1w + 1.0f, titleH), juce::Justification::left, true);
    x += p1w;
    g.setColour (cyan);
    g.drawText (p2, juce::Rectangle<float> (x, area.getY(), p2w + 1.0f, titleH), juce::Justification::left, true);
    x += p2w;
    g.setColour (white);
    g.drawText (p3, juce::Rectangle<float> (x + 1.0f, area.getY(), p3w + 1.0f, titleH), juce::Justification::left, true);

    g.setFont (uiFont (7.2f, true));
    g.setColour (muted.withAlpha (0.86f));
    g.drawText ("DUBTACH DSP", baseX + 1.0f, area.getY() + 26.0f, 105.0f, 10.0f, juce::Justification::left, true);

    g.setColour (edge.withAlpha (0.65f));
    g.drawLine (utilitiesLeft - 10.0f, area.getY() + 3.0f, utilitiesLeft - 10.0f, area.getBottom() - 3.0f, 1.0f);
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
    g.fillEllipse (r.getX() + 14.0f, r.getCentreY() - 4.5f, 9.0f, 9.0f);
    g.setFont (uiFont (10.6f, true));
    const auto textArea = juce::Rectangle<float> (r.getX() + 31.0f, r.getY() + 1.0f, r.getWidth() - 38.0f, r.getHeight() - 2.0f);
    g.drawText (text, textArea, juce::Justification::centredLeft, true);
}

void HomeSidechainTriggerAudioProcessorEditor::drawGraphCard (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawPanel (g, area, cyan, 12.0f);

    const bool bypassed = processor.apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool triggering = processor.getTriggerMeter() > 0.35f && !bypassed;
    const auto colour = bypassed ? muted : (triggering ? red : cyan);
    const auto text = bypassed ? "BYPASSED" : (triggering ? "TRIGGERING" : "READY");

    // Paint the scope first, then lay the status indicator over it. The scope
    // therefore truly fills the graph card while READY/TRIGGERING remains
    // a compact overlay on top of the waveform UI.
    drawWaveform (g, { area.getX() + 6.0f, area.getY() + 6.0f, area.getWidth() - 12.0f, area.getHeight() - 12.0f });
    drawStatusPill (g, { area.getX() + 18.0f, area.getY() + 13.0f, 158.0f, 32.0f }, text, colour);
}

void HomeSidechainTriggerAudioProcessorEditor::drawTimeScale (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    constexpr int divisions = 6;
    const float labelY = plot.getBottom() - 15.0f;
    g.setFont (uiFont (7.6f));

    for (int i = 0; i <= divisions; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (divisions);
        const float seconds = -3.0f + 3.0f * static_cast<float> (i) / static_cast<float> (divisions);
        const juce::String label = (i == divisions) ? "NOW" : juce::String (seconds, 1) + "s";
        const float w = 40.0f;
        const float labelX = juce::jlimit (plot.getX(), plot.getRight() - w, x - w * 0.5f);
        g.setColour (muted.withAlpha (0.68f));
        g.drawText (label, juce::Rectangle<float> (labelX, labelY, w, 9.0f),
                    i == divisions ? juce::Justification::right
                                    : (i == 0 ? juce::Justification::left : juce::Justification::centred), true);
    }
}

void HomeSidechainTriggerAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> area) const
{
    // Use almost the entire graph card for the scope. The status indicator is
    // intentionally overlaid on the scope instead of consuming a separate row.
    const auto plot = area.reduced (4.0f);
    const float thresholdDb = processor.getThresholdDb();

    g.setColour (plotBg);
    g.fillRoundedRectangle (plot, 8.0f);
    g.setColour (black.withAlpha (0.55f));
    g.drawRoundedRectangle (plot, 8.0f, 1.0f);

    // Fixed dB scale: the visual line and detector use the same scale.
    for (int db = 0; db >= -48; db -= 12)
    {
        const float y = yForDb (static_cast<float> (db));
        g.setColour (white.withAlpha (db == 0 ? 0.12f : 0.055f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setFont (uiFont (7.8f));
        g.setColour (muted.withAlpha (0.80f));
        g.drawText (db == 0 ? "0" : juce::String (db),
                    juce::Rectangle<float> (plot.getX() + 5.0f, y - 5.0f, 24.0f, 10.0f),
                    juce::Justification::left, true);
    }

    // Subtle time grid. Keep it visually quiet so the waveform remains the focus.
    constexpr int divisions = 6;
    const float dashes[] = { 4.0f, 6.0f };
    for (int d = 0; d <= divisions; ++d)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (d) / static_cast<float> (divisions);
        g.setColour (grid.withAlpha (d == divisions ? 0.22f : 0.15f));
        g.drawDashedLine (juce::Line<float> (x, plot.getY(), x, plot.getBottom()), dashes, 2, 1.0f);
    }

    const int pointCount = processor.getWaveformPointCount();

    if (pointCount > 1)
    {
        juce::Path line;
        bool started = false;

        // Each waveform bin remembers whether that bin contained the audio
        // transient that actually caused a trigger. The highlight therefore
        // stays attached to the waveform for the full visible history instead
        // of fading with the transient meter. Old highlights disappear only
        // when their waveform bin naturally scrolls out of the history.
        juce::Path hotLine;
        bool hotStarted = false;

        for (int i = 0; i < pointCount; ++i)
        {
            const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
            const float db = homeSidechain::linearToDb (juce::jmax (peak, 0.000001f));
            const float y = yForDb (db);
            const float x = plot.getX() + plot.getWidth() * static_cast<float> (i)
                          / static_cast<float> (pointCount - 1);

            if (!started)
            {
                line.startNewSubPath (x, y);
                started = true;
            }
            else
                line.lineTo (x, y);

            const bool hot = processor.getWaveformTriggered (i);
            if (hot)
            {
                // Keep the red section attached to the real waveform. Extend
                // one point on either side only to make the transient legible
                // at the current graph scale; there is no fade or timeout.
                if (!hotStarted)
                {
                    const int begin = juce::jmax (0, i - 1);
                    const float beginPeak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (begin));
                    const float beginDb = homeSidechain::linearToDb (juce::jmax (beginPeak, 0.000001f));
                    const float beginY = yForDb (beginDb);
                    const float beginX = plot.getX() + plot.getWidth() * static_cast<float> (begin)
                                       / static_cast<float> (pointCount - 1);
                    hotLine.startNewSubPath (beginX, beginY);
                    hotStarted = true;
                }
                hotLine.lineTo (x, y);
            }
            else if (hotStarted)
            {
                // Finish this highlighted transient segment and start a fresh
                // sub-path if another trigger bin appears later in the history.
                hotLine.startNewSubPath (x, y);
                hotStarted = false;
            }
        }

        // Quiet fill under the normal waveform.
        juce::Path fill = line;
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        juce::ColourGradient areaFill (cyan.withAlpha (0.15f), plot.getX(), plot.getY(),
                                       juce::Colours::transparentBlack, plot.getX(), plot.getBottom(), false);
        g.setGradientFill (areaFill);
        g.fillPath (fill);

        // Cyan waveform core.
        g.setColour (cyan.withAlpha (0.12f));
        g.strokePath (line, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (cyan.withAlpha (0.28f));
        g.strokePath (line, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (white.withAlpha (0.96f));
        g.strokePath (line, juce::PathStrokeType (1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Red only on the actual transient waveform segments. Stroke the full
        // path even when the highlighted region ends before the final sample;
        // the previous implementation only drew while the last region was
        // still open, which made isolated trigger bins effectively invisible.
        if (! hotLine.isEmpty())
        {
            g.setColour (red.withAlpha (0.22f));
            g.strokePath (hotLine, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (red);
            g.strokePath (hotLine, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Make a single-bin transient visible at this zoom level too.
            for (int i = 0; i < pointCount; ++i)
            {
                if (! processor.getWaveformTriggered (i))
                    continue;

                const float peak = juce::jlimit (0.0f, 1.0f, processor.getWaveformPoint (i));
                const float db = homeSidechain::linearToDb (juce::jmax (peak, 0.000001f));
                const float y = yForDb (db);
                const float x = plot.getX() + plot.getWidth() * static_cast<float> (i)
                              / static_cast<float> (pointCount - 1);
                g.setColour (red.withAlpha (0.95f));
                g.fillEllipse (x - 2.8f, y - 2.8f, 5.6f, 5.6f);
            }
        }
    }

    // Threshold line and handle use the same y mapping as the detector.
    const float thresholdY = yForDb (thresholdDb);
    const float dashW = 9.0f;
    const float gapW = 6.0f;
    for (float x = plot.getX(); x < plot.getRight(); x += dashW + gapW)
    {
        g.setColour (cyan.withAlpha (0.86f));
        g.drawLine (x, thresholdY, juce::jmin (plot.getRight(), x + dashW), thresholdY, 1.5f);
    }

    const float handleX = plot.getRight() - 1.0f;
    g.setColour (cyan.withAlpha (0.10f));
    g.fillEllipse (handleX - 11.0f, thresholdY - 11.0f, 22.0f, 22.0f);
    g.setColour (cyan);
    g.fillEllipse (handleX - 6.0f, thresholdY - 6.0f, 12.0f, 12.0f);
    g.setColour (white);
    g.fillEllipse (handleX - 2.1f, thresholdY - 2.1f, 4.2f, 4.2f);

    // Compact threshold badge, intentionally overlaid inside the graph.
    const auto badge = juce::Rectangle<float> (plot.getRight() - 132.0f,
                                                juce::jlimit (plot.getY() + 8.0f,
                                                              plot.getBottom() - 54.0f,
                                                              thresholdY - 27.0f),
                                                124.0f, 52.0f);
    g.setColour (black.withAlpha (0.92f));
    g.fillRoundedRectangle (badge, 9.0f);
    g.setColour (edge.withAlpha (0.94f));
    g.drawRoundedRectangle (badge, 9.0f, 1.0f);
    g.setFont (uiFont (7.2f, true));
    g.setColour (muted);
    g.drawText ("THRESHOLD", badge.withTrimmedLeft (10.0f).withTrimmedRight (10.0f).withTrimmedBottom (34.0f),
                juce::Justification::left, true);
    g.setFont (uiFont (12.4f, true));
    g.setColour (cyan);
    g.drawText (juce::String (thresholdDb, 1) + " dB",
                badge.withTrimmedLeft (10.0f).withTrimmedRight (8.0f).withTrimmedTop (19.0f).withTrimmedBottom (7.0f),
                juce::Justification::left, true);

    // Timeline is kept entirely inside the graph card.
    g.setFont (uiFont (7.3f));
    g.setColour (muted.withAlpha (0.68f));
    const float baselineY = plot.getBottom() - 12.0f;
    g.drawText ("PAST", plot.getX() + 6.0f, baselineY, 30.0f, 9.0f, juce::Justification::left, true);
    g.drawText ("NOW", plot.getRight() - 34.0f, baselineY, 30.0f, 9.0f, juce::Justification::right, true);
}

void HomeSidechainTriggerAudioProcessorEditor::drawCooldownCard (juce::Graphics& g, juce::Rectangle<float> area) const
{
    drawPanel (g, area, cyan, 12.0f);
    const float textX = area.getX() + 24.0f;
    const float textW = 138.0f;
    const float centerY = area.getCentreY() + 1.5f;
    g.setFont (uiFont (14.0f, true));
    g.setColour (cyan);
    g.drawText ("COOL DOWN", juce::Rectangle<float> (textX, centerY - 16.0f, textW + 8.0f, 18.0f), juce::Justification::left, true);
    g.setFont (uiFont (8.0f));
    g.setColour (muted.withAlpha (0.88f));
    g.drawText ("TIME BETWEEN TRIGGERS", juce::Rectangle<float> (textX, centerY + 4.0f, textW + 20.0f, 11.0f), juce::Justification::left, true);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto frame = getLocalBounds().toFloat().reduced (4.0f);
    drawBackground (g, frame);

    const auto header = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 15.0f,
                                                 frame.getWidth() - 36.0f, 52.0f);
    drawHeader (g, header);

    const auto graph = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 72.0f,
                                                frame.getWidth() - 36.0f, 208.0f);
    const auto cooldownCard = juce::Rectangle<float> (frame.getX() + 18.0f, graph.getBottom() + 10.0f,
                                                       frame.getWidth() - 36.0f, 62.0f);

    drawGraphCard (g, graph);
    drawCooldownCard (g, cooldownCard);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    const auto frame = getLocalBounds().toFloat().reduced (4.0f);
    const auto graph = juce::Rectangle<float> (frame.getX() + 18.0f, frame.getY() + 72.0f,
                                                frame.getWidth() - 36.0f, 208.0f);
    const auto cooldownCard = juce::Rectangle<float> (frame.getX() + 18.0f, graph.getBottom() + 10.0f,
                                                       frame.getWidth() - 36.0f, 62.0f);

    // Keep the header controls aligned to the same right edge as the graph
    // and Cool Down cards below. The bypass icon sits just after the A/B/C
    // selector with the overall group ending exactly at the card edge.
    const int headerBypassWidth = 34;
    const int headerGap = 10;
    const int headerLinkWidth = 132;
    const int rightEdge = juce::roundToInt (frame.getRight() - 18.0f);
    bypass.setBounds (rightEdge - headerBypassWidth,
                      juce::roundToInt (frame.getY() + 10.0f), headerBypassWidth, 42);
    linkSelector.setBounds (rightEdge - headerBypassWidth - headerGap - headerLinkWidth,
                            juce::roundToInt (frame.getY() + 10.0f), headerLinkWidth, 42);

    // The slider component only covers the actual control row. The surrounding
    // Cool Down card remains purely visual/non-interactive.
    const int sliderX = juce::roundToInt (cooldownCard.getX() + 160.0f);
    const int sliderY = juce::roundToInt (cooldownCard.getY() + 11.0f);
    const int sliderW = juce::jmax (220, juce::roundToInt (cooldownCard.getRight() - 24.0f - sliderX));
    const int sliderH = juce::jmax (34, juce::roundToInt (cooldownCard.getHeight() - 22.0f));
    cooldown.setBounds (sliderX, sliderY, sliderW, sliderH);

    graphPlotBounds = { graph.getX() + 10.0f, graph.getY() + 10.0f,
                        graph.getWidth() - 20.0f, graph.getHeight() - 20.0f };
}

void HomeSidechainTriggerAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    const auto hit = graphPlotBounds.expanded (7.0f, 7.0f);
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

    const auto hitArea = graphPlotBounds.expanded (7.0f, 7.0f);
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
