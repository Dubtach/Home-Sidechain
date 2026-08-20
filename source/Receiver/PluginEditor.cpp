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
    g.fillRoundedRectangle (area, 9.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (area, 9.0f, 1.0f);

    g.setColour (outline.withAlpha (0.65f));
    for (int i = 1; i < 4; ++i)
    {
        const float x = area.getX() + area.getWidth() * (static_cast<float> (i) / 4.0f);
        g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
    }
    for (int i = 1; i < 4; ++i)
    {
        const float y = area.getY() + area.getHeight() * (static_cast<float> (i) / 4.0f);
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
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (curve, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

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
    setSize (720, 520);
    setResizable (false, false);

    mode.addItemList ({ "Duck", "Pump", "Gate", "Shape" }, 1);
    link.addItemList (homeSidechain::linkNames(), 1);
    bars.addItemList ({ "1/4 Bar", "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1);

    addAndMakeVisible (graph);
    addAndMakeVisible (mode);
    addAndMakeVisible (link);
    addAndMakeVisible (bars);
    addAndMakeVisible (sync);
    addAndMakeVisible (bypass);

    for (auto* s : { &depth, &attack, &hold, &release, &curve, &mix })
    {
        addAndMakeVisible (*s);
        styleSlider (*s);
    }

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "MODE", mode);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LINK", link);
    barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "BARS", bars);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "SYNC", sync);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "DEPTH", depth);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ATTACK", attack);
    holdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "HOLD", hold);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RELEASE", release);
    curveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "CURVE", curve);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mix);

    startTimerHz (30);
}

void HomeSidechainReceiverAudioProcessorEditor::styleSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 19);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel2);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    const auto bounds = getLocalBounds().toFloat().reduced (12.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 14.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 14.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (19.0f).withStyle ("Bold"));
    g.drawText ("HOME-SIDECHAIN", 24, 16, 250, 24, juce::Justification::left);
    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("RECEIVER / SHAPER", 25, 39, 150, 14, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("LINK", 25, 60, 60, 12, juce::Justification::left);
    g.drawText ("MODE", 104, 60, 60, 12, juce::Justification::left);
    g.drawText ("TIMING", 243, 60, 60, 12, juce::Justification::left);

    const float activity = processor.triggerActivity.load (std::memory_order_relaxed);
    const bool active = activity > 0.10f;
    const float led = juce::jlimit (0.0f, 1.0f, activity);

    g.setColour (panel2);
    g.fillRoundedRectangle (445.0f, 58.0f, 176.0f, 34.0f, 8.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (445.0f, 58.0f, 176.0f, 34.0f, 8.0f, 1.0f);
    g.setColour (active ? accent : outline);
    g.fillEllipse (456.0f, 68.0f, 13.0f, 13.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (active ? "MIDI IN" : "WAITING", 477, 63, 70, 18, juce::Justification::left);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (juce::String (processor.triggerCount.load()) + " triggers", 536, 63, 74, 18, juce::Justification::right);

    g.setColour (accent.withAlpha (0.16f + 0.30f * led));
    g.fillRoundedRectangle (28, 96, 664, 238, 12.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (28, 96, 664, 238, 12.0f, 1.0f);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("SHAPER — drag the nodes", 42, 107, 180, 14, juce::Justification::left);
    g.drawText ("0 dB", 646, 112, 34, 14, juce::Justification::right);
    g.drawText ("-DEPTH", 642, 309, 38, 14, juce::Justification::right);

    const juce::String labels[] = { "DEPTH", "ATTACK", "HOLD", "RELEASE", "CURVE", "MIX" };
    const int xs[] = { 20, 134, 248, 362, 476, 590 };
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (labels[i], xs[i], 355, 110, 14, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("Depth = maximum attenuation • Curve = transition shape • Mix = dry/wet modulation",
                24, 498, 672, 12, juce::Justification::centred);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    link.setBounds (24, 71, 64, 24);
    mode.setBounds (104, 71, 120, 24);
    sync.setBounds (243, 70, 68, 25);
    bars.setBounds (317, 71, 110, 24);
    bypass.setBounds (634, 18, 64, 24);

    graph.setBounds (36, 132, 648, 190);

    depth.setBounds (18, 365, 114, 122);
    attack.setBounds (132, 365, 114, 122);
    hold.setBounds (246, 365, 114, 122);
    release.setBounds (360, 365, 114, 122);
    curve.setBounds (474, 365, 114, 122);
    mix.setBounds (588, 365, 114, 122);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
