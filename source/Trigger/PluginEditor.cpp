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

juce::String HomeSidechainTriggerAudioProcessorEditor::midiNoteName (int note)
{
    static constexpr const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const auto clamped = juce::jlimit (0, 127, note);
    return juce::String (names[clamped % 12]) + juce::String (clamped / 12 - 1);
}

HomeSidechainTriggerAudioProcessorEditor::HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (640, 350);
    setResizable (false, false);

    mode.addItemList ({ "SMART", "AUDIO", "MIDI", "BOTH" }, 1);
    link.addItemList (homeSidechain::linkNames(), 1);
    addAndMakeVisible (mode);
    addAndMakeVisible (link);
    addAndMakeVisible (testTrigger);
    addAndMakeVisible (bypass);

    for (auto* s : { &threshold, &sensitivity, &retrigger, &midiNote, &velocity })
    {
        addAndMakeVisible (*s);
        styleSlider (*s);
    }

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "MODE", mode);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LINK", link);
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "THRESHOLD", threshold);
    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SENSITIVITY", sensitivity);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "RETRIGGER", retrigger);
    midiNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIDI_NOTE", midiNote);
    velocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "VELOCITY", velocity);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "BYPASS", bypass);

    threshold.setTextValueSuffix (" dB");
    retrigger.setTextValueSuffix (" ms");
    midiNote.textFromValueFunction = [] (double v) { return midiNoteName (juce::roundToInt (v)); };
    midiNote.valueFromTextFunction = [] (const juce::String& s) { return juce::roundToInt (s.getDoubleValue()); };

    testTrigger.onClick = [this] { processor.manualTrigger(); };

    startTimerHz (30);
}

void HomeSidechainTriggerAudioProcessorEditor::styleSlider (juce::Slider& slider)
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
    const auto bounds = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 14.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 14.0f, 1.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    g.drawText ("HOME-SIDECHAIN", 22, 15, 230, 24, juce::Justification::left);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("TRIGGER • SMART AUDIO + MIDI", 23, 38, 210, 14, juce::Justification::left);

    g.setColour (panel2);
    g.fillRoundedRectangle (20.0f, 58.0f, 600.0f, 42.0f, 9.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (20.0f, 58.0f, 600.0f, 42.0f, 9.0f, 1.0f);

    const auto meter = processor.triggerMeter.load (std::memory_order_relaxed);
    const bool midi = processor.midiMeter.load (std::memory_order_relaxed) > 0.1f;
    const auto modeText = mode.getText().isNotEmpty() ? mode.getText() : "SMART";

    g.setColour (muted);
    g.drawText ("HOME-LINK", 32, 69, 72, 16, juce::Justification::left);
    g.setColour (accent.withAlpha (0.15f + 0.6f * meter));
    g.fillRoundedRectangle (108, 71, 220.0f * meter, 14, 7.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (108, 71, 220, 14, 7.0f, 1.0f);
    g.setColour (midi ? accent : muted);
    g.fillEllipse (350, 72, 12, 12);
    g.setColour (text);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText (modeText + " • LINK " + homeSidechain::linkName (processor.getLink()), 370, 68, 145, 18, juce::Justification::left);
    g.drawText ("TRIG " + juce::String (processor.triggerCount.load()), 520, 68, 88, 18, juce::Justification::right);

    const juce::String labels[] = { "THRESHOLD", "SENSITIVITY", "RETRIGGER", "MIDI NOTE", "VELOCITY" };
    const int xs[] = { 18, 142, 266, 390, 514 };
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (labels[i], xs[i], 112, 108, 14, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (8.5f));
    g.drawText ("TEST sends one Home-Link + MIDI trigger. Smart mode uses MIDI when present, otherwise audio detection.",
                20, 326, 600, 12, juce::Justification::centred);
}

void HomeSidechainTriggerAudioProcessorEditor::resized()
{
    mode.setBounds (390, 17, 88, 24);
    link.setBounds (482, 17, 52, 24);
    testTrigger.setBounds (540, 16, 52, 25);
    bypass.setBounds (22, 62, 72, 26);

    threshold.setBounds (12, 126, 116, 174);
    sensitivity.setBounds (136, 126, 116, 174);
    retrigger.setBounds (260, 126, 116, 174);
    midiNote.setBounds (384, 126, 116, 174);
    velocity.setBounds (508, 126, 116, 174);
}

void HomeSidechainTriggerAudioProcessorEditor::timerCallback()
{
    repaint();
}
