#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HomeSidechainTriggerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor&);
    ~HomeSidechainTriggerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    HomeSidechainTriggerAudioProcessor& processor;

    juce::Slider sensitivity;
    juce::Slider retrigger;
    juce::ComboBox link;
    juce::ToggleButton bypass { "BYPASS" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensitivityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retriggerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> linkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<float> graphBounds;
    bool draggingThreshold = false;

    void timerCallback() override;
    void styleSlider (juce::Slider& slider);
    float thresholdForY (float y) const noexcept;
    float yForDb (float db) const noexcept;
    void setThresholdFromY (float y);
    void drawGraph (juce::Graphics&, juce::Rectangle<float>) const;
    void drawStatus (juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessorEditor)
};
