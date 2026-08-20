#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ShaperGraph : public juce::Component
{
public:
    explicit ShaperGraph (HomeSidechainReceiverAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    int selectedPoint = -1;

    juce::Point<float> pointForIndex (int index) const;
    int nearestPoint (juce::Point<float> p) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShaperGraph)
};

class HomeSidechainReceiverAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    explicit HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor&);
    ~HomeSidechainReceiverAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    ShaperGraph graph;

    juce::ComboBox mode;
    juce::ComboBox link;
    juce::ComboBox bars;
    juce::ToggleButton sync { "SYNC" };
    juce::ToggleButton bypass { "BYPASS" };

    juce::Slider depth, attack, hold, release, curve, mix;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> linkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> barsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> holdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> curveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    void styleSlider (juce::Slider& slider);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
