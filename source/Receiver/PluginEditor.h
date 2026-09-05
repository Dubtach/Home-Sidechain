#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>

class ReceiverHomeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ReceiverHomeLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class ReceiverShapeEditor : public juce::Component
{
public:
    explicit ReceiverShapeEditor (HomeSidechainReceiverAudioProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    int draggedNode = -1;
    int draggedHandle = -1;
    bool draggingHandle = false;
    int hoveredNode = -1;
    int hoveredHandle = -1;

    juce::Rectangle<float> plotBounds() const noexcept;
    juce::Point<float> nodePoint (int index) const noexcept;
    juce::Point<float> handlePoint (int segment) const noexcept;
    int nearestNode (juce::Point<float>) const noexcept;
    int nearestHandle (juce::Point<float>) const noexcept;
    float xToPhase (float x) const noexcept;
    float yToValue (float y) const noexcept;
    float phaseToX (float phase) const noexcept;
    float valueToY (float value) const noexcept;
    void sortNodesByX (int movingIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverShapeEditor)
};

class ReceiverFilterEditor : public juce::Component
{
public:
    explicit ReceiverFilterEditor (HomeSidechainReceiverAudioProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    bool draggingLow = false;
    bool draggingHigh = false;
    juce::Point<float> filterPointForHz (double hz) const noexcept;
    double hzForX (float x) const noexcept;
    void updateFromX (float x, bool lowCut);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverFilterEditor)
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
    ReceiverHomeLookAndFeel lookAndFeel;
    ReceiverShapeEditor shapeEditor;
    ReceiverFilterEditor filterEditor;

    juce::Slider mixKnob;
    juce::Slider depthSlider;
    juce::ToggleButton syncButton;
    juce::ToggleButton bypassButton;

    juce::TextButton linkButtons[3];
    juce::TextButton testButton;
    juce::TextButton resetButton;
    juce::TextButton rateButtons[4];
    juce::TextButton presetButtons[12];

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    void timerCallback() override;
    void selectLink (int index);
    void selectRate (int index);
    void selectPreset (int index);
    void requestTest();
    void resetShape();
    void styleSlider (juce::Slider& slider, const juce::String& suffix);
    void refreshLinkButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
