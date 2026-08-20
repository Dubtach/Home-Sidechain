#include "PluginEditor.h"

namespace
{
    const juce::Colour bg (0xff090a0c);
    const juce::Colour panel (0xff111419);
    const juce::Colour outline (0xff292e36);
    const juce::Colour accent (0xff00e5ff);
    const juce::Colour text (0xfff4f5f7);
    const juce::Colour muted (0xff7f8791);
}

juce::Point<float> ShaperGraph::pointForIndex (int index) const
{
    static constexpr float x[] = { 0.03f, 0.24f, 0.47f, 0.72f, 0.97f };
    const float y = 1.0f - processor.getShapePoint (index);
    return { getWidth() * x[index], getHeight() * (0.08f + y * 0.84f) };
}

int ShaperGraph::nearestPoint (juce::Point<float> p) const
{
    int nearest = -1;
    float best = 999999.0f;
    for (int i = 0; i < 5; ++i)
    {
        const auto distance = p.getDistanceFrom (pointForIndex (i));
        if (distance < best && distance < 28.0f)
        {
            best = distance;
            nearest = i;
        }
    }
    return nearest;
}

void ShaperGraph::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);

    const auto bounds = getLocalBounds().toFloat();
    g.setColour (outline.withAlpha (0.8f));

    for (int i = 0; i <= 4; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (float) i / 4.0f;
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());
    }

    for (int i = 0; i <= 4; ++i)
    {
        const float y = bounds.getY() + bounds.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX(), bounds.getRight());
    }

    juce::Path curve;
    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        if (i == 0) curve.startNewSubPath (p);
        else curve.lineTo (p);
    }

    juce::Path fill = curve;
    fill.lineTo (bounds.getRight(), bounds.getBottom());
    fill.lineTo (bounds.getX(), bounds.getBottom());
    fill.closeSubPath();

    g.setColour (accent.withAlpha (0.09f));
    g.fillPath (fill);
    g.setColour (accent.withAlpha (0.18f));
    g.strokePath (curve, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (int i = 0; i < 5; ++i)
    {
        const auto p = pointForIndex (i);
        g.setColour (selectedPoint == i ? text : accent);
        g.fillEllipse (p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
        g.setColour (bg);
        g.fillEllipse (p.x - 2.0f, p.y - 2.0f, 4.0f, 4.0f);
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

    const float normalized = 1.0f - juce::jlimit (0.08f, 0.92f, event.position.y / (float) getHeight());
    processor.setShapePoint (selectedPoint, normalized);
    repaint();
}

HomeSidechainReceiverAudioProcessorEditor::HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), graph (p)
{
    setSize (930, 620);
    addAndMakeVisible (graph);

    mode.addItemList ({ "Duck", "Pump", "Gate", "Shape" }, 1);
    link.addItemList (homeSidechain::linkNames(), 1);
    bars.addItemList ({ "1/4 Bar", "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1);

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
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 22);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

void HomeSidechainReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    auto bounds = getLocalBounds().toFloat().reduced (20.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 18.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 18.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (25.0f).withStyle ("Bold"));
    g.drawText ("HOME-SIDECHAIN", 40, 34, 320, 32, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("RECEIVER / SHAPER", 41, 67, 220, 18, juce::Justification::left);

    g.setColour (accent.withAlpha (0.16f));
    g.fillRoundedRectangle (40, 100, 850, 315, 16.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (40, 100, 850, 315, 16.0f, 1.0f);

    g.setColour (muted);
    g.drawText ("SOURCE", 61, 120, 80, 18, juce::Justification::left);
    g.drawText ("MODE", 190, 120, 80, 18, juce::Justification::left);
    g.drawText ("TIME", 350, 120, 80, 18, juce::Justification::left);
    g.drawText ("TRIGGER", 600, 120, 100, 18, juce::Justification::left);

    g.setColour (outline);
    g.fillRoundedRectangle (60, 153, 120, 38, 10.0f);
    g.fillRoundedRectangle (190, 153, 140, 38, 10.0f);
    g.fillRoundedRectangle (350, 153, 210, 38, 10.0f);
    g.fillRoundedRectangle (600, 153, 230, 38, 10.0f);

    g.setColour (accent);
    g.fillEllipse (615, 165, 14, 14);
    g.setColour (text);
    g.drawText ("Link " + homeSidechain::linkName (static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load())),
                638, 159, 80, 22, juce::Justification::left);
    g.setColour (muted);
    g.drawText ("MIDI note " + juce::String (homeSidechain::midiNoteForLink (static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load()))),
                711, 159, 105, 22, juce::Justification::left);

    g.setColour (muted);
    g.drawText ("DRAG THE NODES TO DRAW THE DUCKING SHAPE", 62, 210, 340, 18, juce::Justification::left);
    g.drawText ("0 dB", 840, 225, 38, 18, juce::Justification::right);
    g.drawText ("-48 dB", 829, 390, 49, 18, juce::Justification::right);

    g.setColour (muted);
    g.drawText ("LIVE TRIGGER", 760, 67, 110, 18, juce::Justification::right);
    g.setColour (accent.withAlpha (0.2f));
    g.fillRoundedRectangle (850, 61, 19, 9, 4.0f);
    g.setColour (accent.withAlpha (processor.triggerActivity.load() * 0.95f));
    g.fillRoundedRectangle (850, 61, 19 * processor.triggerActivity.load(), 9, 4.0f);

    const bool syncOn = processor.apvts.getRawParameterValue ("SYNC")->load() > 0.5f;
    g.setColour (muted);
    g.drawText (syncOn ? "SYNCED TO DAW TEMPO" : "FREE TIME", 61, 430, 200, 18, juce::Justification::left);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    mode.setBounds (198, 157, 122, 30);
    link.setBounds (68, 157, 104, 30);
    bars.setBounds (438, 157, 108, 30);
    sync.setBounds (354, 157, 70, 30);
    bypass.setBounds (776, 34, 86, 30);

    graph.setBounds (60, 228, 814, 170);

    depth.setBounds (52, 468, 125, 135);
    attack.setBounds (190, 468, 125, 135);
    hold.setBounds (328, 468, 125, 135);
    release.setBounds (466, 468, 125, 135);
    curve.setBounds (604, 468, 125, 135);
    mix.setBounds (742, 468, 125, 135);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
