#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ReceiverEnvelopeView : public juce::Component
{
public:
    explicit ReceiverEnvelopeView (HomeSidechainReceiverAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    int draggedPoint = -1;

    juce::Point<float> pointForIndex (int index) const;
    int nearestPoint (juce::Point<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverEnvelopeView)
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
    ReceiverEnvelopeView envelopeView;

    juce::TextButton linkButtons[3];
    juce::TextButton testButton { "TEST" };
    juce::ToggleButton bypassButton;

    juce::Slider depthSlider, attackSlider, holdSlider, releaseSlider, mixSlider, curveSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> depthAttachment, attackAttachment, holdAttachment,
                                    releaseAttachment, mixAttachment, curveAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    void styleSlider (juce::Slider&, const juce::String& suffix);
    void selectLink (int index);
    void requestTest();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
