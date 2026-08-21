#include "PluginEditor.h"

namespace
{
    const juce::Colour bg (0xff0a0b0d);
    const juce::Colour panel (0xff111317);
    const juce::Colour panel2 (0xff171a20);
    const juce::Colour outline (0xff292e36);
    const juce::Colour accent (0xff00e5ff);
    const juce::Colour text (0xfff4f5f7);
    const juce::Colour muted (0xff8a919c);
}

juce::Point<float> ShaperGraph::pointForIndex (int index) const
{
    const auto area = getLocalBounds().toFloat().reduced (8.0f);
    const float x = area.getX() + area.getWidth() * (static_cast<float> (index) / 4.0f);
    const float y = area.getBottom() - area.getHeight() * processor.getShapePoint (index);
    return { x, y };
}

int ShaperGraph::nearestPoint (juce::Point<float> p) const
{
    int best = -1;
    float bestDistance = 24.0f;
    for (int i = 0; i < 5; ++i)
    {
        const auto d = pointForIndex (i).getDistanceFrom (p);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

void ShaperGraph::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (panel2);
    g.fillRoundedRectangle (area, 10.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (area, 10.0f, 1.0f);

    g.setColour (outline.withAlpha (0.7f));
    for (int i = 1; i < 4; ++i)
    {
        const float x = area.getX() + area.getWidth() * (static_cast<float> (i) / 4.0f);
        const float y = area.getY() + area.getHeight() * (static_cast<float> (i) / 4.0f);
        g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
        g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }

    juce::Path curve;
    curve.startNewSubPath (pointForIndex (0));
    for (int i = 1; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        const auto last = curve.getCurrentPosition();
        const auto midX = (last.x + p.x) * 0.5f;
        curve.cubicTo (midX, last.y, midX, p.y, p.x, p.y);
    }

    juce::Path fill = curve;
    fill.lineTo (area.getRight(), area.getBottom());
    fill.lineTo (area.getX(), area.getBottom());
    fill.closeSubPath();
    g.setColour (accent.withAlpha (0.10f));
    g.fillPath (fill);
    g.setColour (accent.withAlpha (0.24f));
    g.strokePath (curve, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        g.setColour (selectedPoint == i ? text : accent);
        g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
    }
}

void ShaperGraph::mouseDown (const juce::MouseEvent& event)
{
    selectedPoint = nearestPoint (event.position);
    repaint();
}

void ShaperGraph::mouseDrag (const juce::MouseEvent& event)
{
    if (selectedPoint < 0)
        return;

    const auto area = getLocalBounds().toFloat().reduced (8.0f);
    const float normalized = 1.0f - juce::jlimit (0.0f, 1.0f,
        (event.position.y - area.getY()) / juce::jmax (1.0f, area.getHeight()));
    processor.setShapePoint (selectedPoint, normalized);
    repaint();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), graph (p)
{
    setSize (700, 500);
    setResizable (false, false);

    mode.addItemList ({ "DUCK", "PUMP", "GATE", "SHAPE" }, 1);
    link.addItemList (homeSidechain::linkNames(), 1);
    source.addItemList ({ "HOME-LINK", "MIDI", "BOTH" }, 1);
    bars.addItemList ({ "1/4", "1/2", "1", "2", "4" }, 1);
    preset.addItemList ({ "Classic Duck", "Deep Duck", "EDM Pump", "Fast Pump", "Slow Pump",
                          "Kick Pocket", "Bass Duck", "Reverse", "Hard Gate", "Ghost Pump" }, 1);

    for (auto* c : { &mode, &link, &source, &bars, &preset, &sync, &bypass, &resetShape, &flipShape, &smoothShape, &snapShape, &graph })
        addAndMakeVisible (*c);

    for (auto* s : { &depth, &attack, &hold, &release, &curve, &mix, &offset })
    {
        addAndMakeVisible (*s);
        styleSlider (*s);
    }

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "MODE", mode);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LINK", link);
    barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "BARS", bars);
    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "SOURCE", source);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "SYNC", sync);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "DEPTH", depth);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ATTACK", attack);
    holdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "HOLD", hold);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RELEASE", release);
    curveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "CURVE", curve);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mix);
    offsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "OFFSET", offset);

    depth.setTextValueSuffix (" dB");
    attack.setTextValueSuffix (" ms");
    hold.setTextValueSuffix (" ms");
    release.setTextValueSuffix (" ms");
    mix.setTextValueSuffix (" %");
    offset.setTextValueSuffix (" %");

    resetShape.onClick = [this] { processor.resetShape(); };
    flipShape.onClick = [this] { processor.flipShape(); };
    smoothShape.onClick = [this] { processor.smoothShape(); };
    snapShape.onClick = [this] { processor.snapShape(); };
    preset.onChange = [this] { if (preset.getSelectedId() > 0) processor.applyPreset (preset.getSelectedId() - 1); };

    startTimerHz (30);
}

void HomeSidechainReceiverAudioProcessorEditor::styleSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 19);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel2);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    const auto bounds = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 14.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 14.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    g.drawText ("HOME-SIDECHAIN", 20, 14, 230, 24, juce::Justification::left);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("RECEIVER • ZERO LATENCY • CREATIVE SHAPER", 21, 37, 280, 14, juce::Justification::left);

    g.setColour (panel2);
    g.fillRoundedRectangle (300.0f, 14.0f, 378.0f, 48.0f, 9.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (300.0f, 14.0f, 378.0f, 48.0f, 9.0f, 1.0f);

    const float homeLed = juce::jlimit (0.0f, 1.0f, processor.homeLinkActivity.load());
    const bool connected = processor.homeLinkConnected.load();
    const int src = static_cast<int> (processor.apvts.getRawParameterValue ("SOURCE")->load());
    const bool midi = processor.midiActivity.load() > 0.10f;
    const bool signal = src == 0 ? homeLed > 0.10f : src == 1 ? midi : (homeLed > 0.10f || midi);

    g.setColour (signal ? accent : (connected ? accent.withAlpha (0.55f) : outline));
    g.fillEllipse (311, 27, 13, 13);
    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (signal ? "SIGNAL" : (connected ? "LINKED" : "WAITING"), 332, 20, 85, 20, juce::Justification::left);
    g.setColour (muted);
    g.setFont (juce::FontOptions (8.5f));
    g.drawText ("LINK " + homeSidechain::linkName (processor.getLink())
                + " • " + juce::String (processor.homeLinkTriggerCount.load()) + " TRIGGERS",
                418, 20, 148, 18, juce::Justification::right);
    g.drawText ("ZERO LATENCY", 573, 20, 94, 18, juce::Justification::right);

    const juce::String labels[] = { "MODE", "SOURCE", "LINK", "PRESET", "TIMING", "BARS" };
    const int xs[] = { 20, 106, 208, 274, 420, 518 };
    const int widths[] = { 80, 96, 58, 140, 88, 72 };
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
        g.drawText (labels[i], xs[i], 69, widths[i], 12, juce::Justification::left);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("SHAPER • DRAG POINTS", 24, 110, 170, 14, juce::Justification::left);
    g.drawText ("RESET / FLIP / SMOOTH / SNAP", 484, 110, 188, 14, juce::Justification::right);

    const auto led = juce::jmax (homeLed, processor.triggerActivity.load());
    g.setColour (accent.withAlpha (0.06f + 0.20f * led));
    g.fillRoundedRectangle (18, 126, 664, 208, 12.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (18, 126, 664, 208, 12.0f, 1.0f);

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.5f));
    g.drawText ("0", 35, 145, 20, 12, juce::Justification::left);
    g.drawText ("100%", 38, 308, 30, 12, juce::Justification::left);

    const juce::String knobLabels[] = { "DEPTH", "ATTACK", "HOLD", "RELEASE", "CURVE", "MIX", "OFFSET" };
    const int xs2[] = { 18, 116, 214, 312, 410, 508, 606 };
    for (int i = 0; i < 7; ++i)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
        g.drawText (knobLabels[i], xs2[i], 350, 88, 14, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("Home-Link = no DAW routing. MIDI/BOTH = advanced fallback. Plugin reports 0 samples latency.",
                16, 478, 668, 12, juce::Justification::centred);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    mode.setBounds (20, 82, 80, 23);
    source.setBounds (106, 82, 94, 23);
    link.setBounds (208, 82, 58, 23);
    preset.setBounds (274, 82, 140, 23);
    sync.setBounds (420, 81, 60, 25);
    bars.setBounds (518, 82, 72, 23);
    bypass.setBounds (610, 82, 70, 23);

    graph.setBounds (28, 128, 644, 194);
    resetShape.setBounds (20, 108, 58, 20);
    flipShape.setBounds (80, 108, 52, 20);
    smoothShape.setBounds (134, 108, 70, 20);
    snapShape.setBounds (206, 108, 56, 20);

    depth.setBounds (12, 358, 92, 108);
    attack.setBounds (110, 358, 92, 108);
    hold.setBounds (208, 358, 92, 108);
    release.setBounds (306, 358, 92, 108);
    curve.setBounds (404, 358, 92, 108);
    mix.setBounds (502, 358, 92, 108);
    offset.setBounds (600, 358, 92, 108);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
