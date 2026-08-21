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
    setSize (640, 450);
    setResizable (false, false);

    mode.addItemList ({ "Duck", "Pump", "Gate", "Shape" }, 1);
    link.addItemList (homeSidechain::linkNames(), 1);
    source.addItemList ({ "HOME-LINK", "MIDI", "BOTH" }, 1);
    bars.addItemList ({ "1/4 Bar", "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1);

    addAndMakeVisible (graph);
    addAndMakeVisible (mode);
    addAndMakeVisible (link);
    addAndMakeVisible (source);
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
    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "SOURCE", source);
    barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "BARS", bars);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "SYNC", sync);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "DEPTH", depth);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ATTACK", attack);
    holdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "HOLD", hold);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RELEASE", release);
    curveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "CURVE", curve);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mix);

    depth.setTextValueSuffix (" dB");
    attack.setTextValueSuffix (" ms");
    hold.setTextValueSuffix (" ms");
    release.setTextValueSuffix (" ms");
    mix.setTextValueSuffix (" %");

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
    g.drawText ("SOURCE", 191, 60, 60, 12, juce::Justification::left);
    g.drawText ("TIMING", 337, 60, 60, 12, juce::Justification::left);

    const float midiLed = juce::jlimit (0.0f, 1.0f, processor.midiActivity.load (std::memory_order_relaxed));
    const float triggerLed = juce::jlimit (0.0f, 1.0f, processor.triggerActivity.load (std::memory_order_relaxed));
    const float homeLed = juce::jlimit (0.0f, 1.0f, processor.homeLinkActivity.load (std::memory_order_relaxed));
    const bool hasMidi = midiLed > 0.10f;
    const bool hasTrigger = triggerLed > 0.10f;
    const bool connected = processor.homeLinkConnected.load (std::memory_order_relaxed);
    const int lastNote = processor.lastMidiNote.load (std::memory_order_relaxed);
    const int lastChannel = processor.lastMidiChannel.load (std::memory_order_relaxed);

    g.setColour (panel2);
    g.fillRoundedRectangle (392.0f, 54.0f, 230.0f, 32.0f, 8.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (392.0f, 54.0f, 230.0f, 32.0f, 8.0f, 1.0f);
    g.setColour (connected ? accent : (hasMidi ? accent : outline));
    g.fillEllipse (402.0f, 64.0f, 12.0f, 12.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (connected ? (hasTrigger ? "HOME-LINK SIGNAL" : "HOME-LINK CONNECTED") : (hasMidi ? "MIDI IN" : "WAITING"), 421, 59, 92, 18, juce::Justification::left);
    g.setColour (muted);
    g.setFont (juce::FontOptions (8.5f));
    const auto diagnostic = lastNote >= 0
        ? ("NOTE " + juce::String (lastNote) + "  CH " + juce::String (juce::jmax (1, lastChannel)))
        : "NO MIDI EVENTS";
    g.drawText (diagnostic, 510, 59, 100, 14, juce::Justification::right);

    const float led = juce::jmax (midiLed, juce::jmax (triggerLed, homeLed));
    g.setColour (accent.withAlpha (0.16f + 0.30f * led));
    g.fillRoundedRectangle (22, 92, 596, 202, 12.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (22, 92, 596, 202, 12.0f, 1.0f);

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("SHAPER — drag the nodes", 34, 102, 180, 14, juce::Justification::left);
    g.drawText ("0 dB", 566, 107, 40, 14, juce::Justification::right);
    g.drawText ("-DEPTH", 560, 279, 46, 14, juce::Justification::right);

    const juce::String labels[] = { "DEPTH", "ATTACK", "HOLD", "RELEASE", "CURVE", "MIX" };
    const int xs[] = { 12, 110, 208, 306, 404, 502 };
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (labels[i], xs[i], 303, 96, 14, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("DEPTH = attenuation • ATTACK/HOLD/RELEASE = timing • CURVE = transition • MIX = dry/wet",
                18, 430, 604, 12, juce::Justification::centred);
}

void HomeSidechainReceiverAudioProcessorEditor::resized()
{
    link.setBounds (20, 66, 60, 23);
    mode.setBounds (88, 66, 96, 23);
    source.setBounds (192, 66, 98, 23);
    sync.setBounds (300, 65, 62, 24);
    bars.setBounds (367, 66, 82, 23);
    bypass.setBounds (565, 18, 58, 23);

    graph.setBounds (28, 112, 584, 170);

    depth.setBounds (16, 314, 96, 104);
    attack.setBounds (114, 314, 96, 104);
    hold.setBounds (212, 314, 96, 104);
    release.setBounds (310, 314, 96, 104);
    curve.setBounds (408, 314, 96, 104);
    mix.setBounds (506, 314, 96, 104);
}

void HomeSidechainReceiverAudioProcessorEditor::timerCallback()
{
    graph.repaint();
    repaint();
}
