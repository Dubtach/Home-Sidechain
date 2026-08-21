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

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (540, 300);
    setResizable (false, false);

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

    threshold.setTextValueSuffix (" dB");
    retrigger.setTextValueSuffix (" ms");

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleSlider (juce::Slider& slider, bool)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel2);
    slider.setColour (juce::Slider::textBoxOutlineColourId, outline);
}

void HomeSidechainTriggerAudioProcessorEditor::paint (juce::Graphics& g)
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
    g.drawText ("TRIGGER", 25, 39, 100, 14, juce::Justification::left);

    g.setColour (panel2);
    g.fillRoundedRectangle (20.0f, 58.0f, 500.0f, 40.0f, 9.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (20.0f, 58.0f, 500.0f, 40.0f, 9.0f, 1.0f);

    const float meter = processor.triggerMeter.load (std::memory_order_relaxed);
    g.setColour (muted);
    g.drawText ("TRIGGER", 32, 70, 64, 16, juce::Justification::left);
    g.setColour (outline);
    g.fillRoundedRectangle (96, 72, 238, 12, 6.0f);
    g.setColour (accent.withAlpha (0.9f));
    g.fillRoundedRectangle (96, 72, 238.0f * meter, 12, 6.0f);
    g.setColour (text);
    g.drawText ("LINK " + homeSidechain::linkName (static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load()))
                + "  •  MIDI " + juce::String (homeSidechain::midiNoteForLink (static_cast<int> (processor.apvts.getRawParameterValue ("LINK")->load()))),
                344, 68, 166, 18, juce::Justification::right);

    const juce::String labels[] = { "THRESHOLD", "SENSITIVITY", "RETRIGGER", "VELOCITY" };
    const int xs[] = { 24, 152, 280, 408 };
    for (int i = 0; i < 4; ++i)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.drawText (labels[i], xs[i], 112, 104, 16, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("Audio peaks are converted to short MIDI triggers for the matching Receiver link.",
                20, 278, 500, 12, juce::Justification::centred);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    link.setBounds (348, 18, 64, 23);
    bypass.setBounds (438, 18, 72, 23);

    threshold.setBounds (18, 128, 118, 134);
    sensitivity.setBounds (142, 128, 118, 134);
    retrigger.setBounds (266, 128, 118, 134);
    velocity.setBounds (390, 128, 118, 134);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    repaint();
}
