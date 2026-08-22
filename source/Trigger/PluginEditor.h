#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HomeSeriesTriggerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HomeSeriesTriggerLookAndFeel();

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class HomeSidechainTriggerGapSlider : public juce::Slider
{
public:
    HomeSidechainTriggerGapSlider();
    void paint (juce::Graphics&) override;
};

class HomeSidechainTriggerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor&);
    ~HomeSidechainTriggerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    HomeSidechainTriggerAudioProcessor& processor;
    HomeSeriesTriggerLookAndFeel homeSeriesLaf;

    HomeSidechainTriggerGapSlider retrigger;
    juce::ComboBox link;
    juce::ToggleButton bypass { "BYPASS" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retriggerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> linkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<float> graphBounds;
    bool draggingThreshold = false;

    void timerCallback() override;
    void styleComboBox();
    void styleBypass();
    float thresholdForY (float y) const noexcept;
    float yForDb (float db) const noexcept;
    void setThresholdFromY (float y);
    void drawHeader (juce::Graphics&, juce::Rectangle<float>) const;
    void drawGraph (juce::Graphics&, juce::Rectangle<float>) const;
    void drawUtilityPanel (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCooldownPanel (juce::Graphics&, juce::Rectangle<float>) const;
    void drawBackgroundTexture (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCard (juce::Graphics&, juce::Rectangle<float>, juce::Colour, bool brightHeader = false) const;
    void drawTinyStatus (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour, bool active) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessorEditor)
};
