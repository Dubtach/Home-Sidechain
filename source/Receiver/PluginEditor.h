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
    void resetShape();

private:
    HomeSidechainReceiverAudioProcessor& processor;
    int draggedPoint = -1;
    int hoveredPoint = -1;

    juce::Rectangle<float> plotBounds() const;
    juce::Point<float> pointForIndex (int index) const;
    int nearestPoint (juce::Point<float>) const;

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
    class ReceiverLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        ReceiverLookAndFeel();
        void drawRotarySlider (juce::Graphics&, int, int, int, int, float,
                               float, float, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                                   bool, bool) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    };

    static constexpr int numPresets = 16;
    HomeSidechainReceiverAudioProcessor& processor;
    ReceiverLookAndFeel lookAndFeel;
    ReceiverShaperGraph graph;

    juce::Slider mixKnob;
    juce::Slider bandSlider;
    juce::Slider depthSlider;
    juce::ToggleButton syncButton;

    juce::TextButton linkButtons[3];
    juce::TextButton bypassButton;
    juce::TextButton testButton;
    juce::TextButton resetButton;
    juce::TextButton fullBandButton;
    juce::TextButton lowBandButton;
    juce::TextButton highBandButton;
    juce::TextButton rateButtons[4];
    juce::TextButton presetButtons[numPresets];

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    void timerCallback() override;
    void selectLink (int index);
    void selectBand (int index);
    void selectRate (int index);
    void selectPreset (int index);
    void requestTest();
    void resetShape();
    void styleSlider (juce::Slider& slider, const juce::String& suffix);
    void refreshButtonStates();
    void drawRotaryLabel (juce::Graphics&, const juce::String&, juce::Rectangle<float>, float) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
