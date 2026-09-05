#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

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

class ReceiverShaperGraph : public juce::Component
{
public:
    explicit ReceiverShaperGraph (HomeSidechainReceiverAudioProcessor&);
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
    int hoveredNode = -1;
    int hoveredHandle = -1;
    bool draggingHandle = false;

    juce::Rectangle<float> plotBounds() const noexcept;
    juce::Point<float> nodePoint (int index) const noexcept;
    juce::Point<float> handlePoint (int segment) const noexcept;
    int nearestNode (juce::Point<float>) const noexcept;
    int nearestHandle (juce::Point<float>) const noexcept;
    float xToPhase (float x) const noexcept;
    float yToValue (float y) const noexcept;
    float phaseToX (float phase) const noexcept;
    float valueToY (float value) const noexcept;
    std::array<int, HomeSidechainReceiverAudioProcessor::maxNodes> sortedNodeIndices (int& count) const noexcept;
    bool canDeleteNode (int index) const noexcept;
    void deleteNode (int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverShaperGraph)
};

class ReceiverFilterEditor : public juce::Component
{
public:
    explicit ReceiverFilterEditor (HomeSidechainReceiverAudioProcessor&);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    HomeSidechainReceiverAudioProcessor& processor;
    bool draggingLow = false;
    bool draggingHigh = false;
    bool hoveredLow = false;
    bool hoveredHigh = false;
    juce::Point<float> filterPointForHz (double) const noexcept;
    double hzForX (float) const noexcept;
    void updateFromX (float, bool lowCut);

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
    ReceiverShaperGraph shaperGraph;
    ReceiverFilterEditor filterEditor;

    juce::Slider mixKnob;
    juce::Slider depthKnob;
    juce::ToggleButton bypassButton;
    juce::ToggleButton syncButton;

    juce::TextButton linkButtons[3];
    juce::TextButton rateButtons[4];
    juce::TextButton presetButtons[12];
    juce::TextButton testButton;
    juce::TextButton resetButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;

    void timerCallback() override;
    void selectLink (int);
    void selectRate (int);
    void selectPreset (int);
    void requestTest();
    void resetShape();
    void refreshButtons();
    void drawHeader (juce::Graphics&, juce::Rectangle<float>) const;
    void drawGraphFrame (juce::Graphics&, juce::Rectangle<float>) const;
    void drawBottomControls (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCurvePreset (juce::Graphics&, juce::Rectangle<float>, int, bool) const;
    void drawStatus (juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
