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

    // Called from the editor's timer with the same UI-side smoothed values
    // it already computed -- the scope doesn't re-derive its own smoothing.
    void setLevels (float inputLevel, float triggerActivity) noexcept;

private:
    HomeSidechainTriggerAudioProcessor& processor;

    bool dragging = false;
    bool hovering = false;
    float cachedInputLevel = 0.0f;
    float cachedTriggerActivity = 0.0f;

    juce::Rectangle<float> plotBounds() const noexcept;
    float dbToY (float db) const noexcept;
    float yToDb (float y) const noexcept;
    void setThresholdFromY (float y, bool fine);

    void drawGrid (juce::Graphics&, juce::Rectangle<float>) const;
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>) const;
    void drawThreshold (juce::Graphics&, juce::Rectangle<float>) const;
    void drawSendingLamp (juce::Graphics&, juce::Rectangle<float>) const;
    void drawLevelBar (juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerScope)
};

// =============================================================================

class HomeSidechainTriggerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    static constexpr int designWidth = 720;
    static constexpr int designHeight = 320;

    explicit HomeSidechainTriggerAudioProcessorEditor (HomeSidechainTriggerAudioProcessor&);
    ~HomeSidechainTriggerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainTriggerAudioProcessor& processor;

    TriggerScope scope;

    homeUI::LinkSelector linkSelector { homeSidechain::linkNames() };

    homeUI::PowerButton power;

    homeUI::Knob thresholdKnob { "THRESHOLD", homeUI::pink };
    homeUI::Knob cooldownKnob { "COOL DOWN", homeUI::pink };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> thresholdAttachment, cooldownAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    float inputSmoothed = 0.0f;
    float triggerSmoothed = 0.0f;

    // Input now spans the full content height -- Activity and Sending were
    // removed and their two indicators (a sending lamp, an input level bar)
    // live inside this card instead of occupying their own cards below.
    // Input and Sensitivity fill the content height between them, same idea
    // as before -- just not stretched out further than the content
    // actually needs. 220px gives the graph (plus its lamp/legend/level
    // bar) and the two knobs comfortable room without padding for its own
    // sake.
    static juce::Rectangle<float> scopeCard()  { return { 20.0f,  76.0f, 470.0f, 220.0f }; }
    static juce::Rectangle<float> senseCard()  { return { 500.0f, 76.0f, 200.0f, 220.0f }; }

    void timerCallback() override;
    void refreshFromParameters();
    void drawHeader (juce::Graphics&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessorEditor)
};
