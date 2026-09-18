#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"
#include "../Shared/HomeSeriesUI.h"

// =============================================================================
// Scrolling input waveform with a draggable threshold line. Bins that fired a
// trigger are marked, so you can see exactly which transient crossed.
// =============================================================================

class TriggerScope : public juce::Component
{
public:
    explicit TriggerScope (HomeSidechainTriggerAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    HomeSidechainTriggerAudioProcessor& processor;

    bool dragging = false;
    bool hovering = false;

    juce::Rectangle<float> plotBounds() const noexcept;
    float dbToY (float db) const noexcept;
    float yToDb (float y) const noexcept;
    void setThresholdFromY (float y, bool fine);

    void drawGrid (juce::Graphics&, juce::Rectangle<float>) const;
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>) const;
    void drawThreshold (juce::Graphics&, juce::Rectangle<float>) const;
    void drawLegend (juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerScope)
};

// =============================================================================

class HomeSidechainTriggerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    static constexpr int designWidth = 720;
    static constexpr int designHeight = 430;

    explicit HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor&);
    ~HomeSidechainTriggerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainTriggerAudioProcessor& processor;

    TriggerScope scope;

    std::array<std::unique_ptr<homeUI::Pill>, homeSidechain::numberOfLinks> linkPills;

    homeUI::Pill testPill { "TEST", homeUI::cyan };
    homeUI::PowerButton power;

    homeUI::Knob thresholdKnob { "THRESHOLD" };
    homeUI::Knob cooldownKnob { "COOL DOWN" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> thresholdAttachment, cooldownAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    float inputSmoothed = 0.0f;
    float triggerSmoothed = 0.0f;

    static juce::Rectangle<float> scopeCard()  { return { 20.0f,  76.0f, 470.0f, 202.0f }; }
    static juce::Rectangle<float> senseCard()  { return { 500.0f, 76.0f, 200.0f, 202.0f }; }
    static juce::Rectangle<float> meterCard()  { return { 20.0f, 288.0f, 470.0f, 122.0f }; }
    static juce::Rectangle<float> sendCard()   { return { 500.0f, 288.0f, 200.0f, 122.0f }; }

    void timerCallback() override;
    void refreshFromParameters();
    void drawHeader (juce::Graphics&) const;
    void drawMeters (juce::Graphics&) const;
    void drawSend (juce::Graphics&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessorEditor)
};
