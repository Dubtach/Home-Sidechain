#include "PluginEditor.h"

namespace
{
    const juce::Colour bg (0xff0a0b0d);
    const juce::Colour panel (0xff111317);
    const juce::Colour outline (0xff262a31);
    const juce::Colour accent (0xff00e5ff);
    const juce::Colour text (0xfff4f5f7);
    const juce::Colour muted (0xff848b95);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (760, 430);

    for (auto* slider : { &threshold, &sensitivity, &retrigger, &velocity })
    {
        addAndMakeVisible (*slider);
        styleSlider (*slider);
    }

    addAndMakeVisible (link);
    link.addItemList (homeSidechain::linkNames(), 1);

    addAndMakeVisible (bypass);

    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "THRESHOLD", threshold);
    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SENSITIVITY", sensitivity);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RETRIGGER", retrigger);
    velocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "VELOCITY", velocity);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LINK", link);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleSlider (juce::Slider& slider, bool)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 22);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    auto bounds = getLocalBounds().toFloat().reduced (22.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 18.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 18.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (25.0f).withStyle ("Bold"));
    g.drawText ("HOME-SIDECHAIN", 44, 38, 300, 32, juce::Justification::left);

    g.setColour (muted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("TRIGGER", 45, 70, 120, 18, juce::Justification::left);

    g.setColour (accent.withAlpha (0.25f));
    g.fillRoundedRectangle (44, 104, 672, 86, 14.0f);
    g.setColour (accent.withAlpha (0.55f));
    g.drawRoundedRectangle (44, 104, 672, 86, 14.0f, 1.0f);

    g.setColour (muted);
    g.drawText ("AUDIO THRESHOLD", 64, 124, 150, 18, juce::Justification::left);
    g.drawText ("DETECTS PEAKS", 64, 146, 150, 18, juce::Justification::left);

    const float meter = processor.triggerMeter.load (std::memory_order_relaxed);
    g.setColour (outline);
    g.fillRoundedRectangle (220, 132, 448, 18, 9.0f);
    g.setColour (accent.withAlpha (0.9f));
    g.fillRoundedRectangle (220, 132, 448 * meter, 18, 9.0f);

    g.setColour (text);
    g.drawText ("MIDI LINK  " + homeSidechain::linkName (static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load())),
                220, 160, 220, 18, juce::Justification::left);
    g.setColour (muted);
    g.drawText ("Output note: " + juce::String (homeSidechain::midiNoteForLink (static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load()))),
                430, 160, 180, 18, juce::Justification::left);

    g.setColour (muted);
    g.drawText ("Every trigger is sent as a short MIDI note to the matching Receiver link.", 45, 213, 650, 18, juce::Justification::left);
    g.drawText ("Trigger count: " + juce::String (processor.triggerCount.load()), 45, 234, 300, 18, juce::Justification::left);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    const auto area = getLocalBounds().reduced (22);
    threshold.setBounds (54, 270, 140, 145);
    sensitivity.setBounds (214, 270, 140, 145);
    retrigger.setBounds (374, 270, 140, 145);
    velocity.setBounds (534, 270, 140, 145);

    link.setBounds (570, 42, 90, 28);
    bypass.setBounds (670, 42, 70, 28);
    juce::ignoreUnused (area);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    repaint (44, 100, 675, 95);
}
