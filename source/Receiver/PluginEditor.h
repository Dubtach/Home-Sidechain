#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ReceiverShaperGraph : public juce::Component
{
public:
    explicit ReceiverShaperGraph (HomeSidechainReceiverAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    int draggedPoint = -1;
    int hoveredPoint = -1;

    juce::Rectangle<float> plotBounds() const;
    juce::Point<float> pointForIndex (int index) const;
    int nearestPoint (juce::Point<float>) const;
    void resetShape();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverShaperGraph)
};

class HomeSidechainReceiverAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    explicit HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor&);
    ~HomeSidechainReceiverAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    ReceiverShaperGraph graph;

    juce::TextButton linkButtons[3];
    juce::ToggleButton bypassButton;
    juce::TextButton testButton;
    juce::TextButton resetButton;

    juce::Slider depthSlider, attackSlider, holdSlider, releaseSlider, mixSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> depthAttachment, attackAttachment, holdAttachment,
                                    releaseAttachment, mixAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    void styleSlider (juce::Slider&, const juce::String& suffix);
    void selectLink (int index);
    void requestTest();
    void resetShape();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
