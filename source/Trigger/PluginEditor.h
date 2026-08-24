#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HomeSeriesTriggerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HomeSeriesTriggerLookAndFeel();

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class HomeSidechainTriggerLinkSelector : public juce::Component
{
public:
    explicit HomeSidechainTriggerLinkSelector (HomeSidechainTriggerAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    HomeSidechainTriggerAudioProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerLinkSelector)
};

class HomeSidechainTriggerGapSlider : public juce::Slider
{
public:
    HomeSidechainTriggerGapSlider();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void setValueFromMouseX (float x);
    float trackStartX() const noexcept;
    float trackEndX() const noexcept;
    bool manualMouseTracking = false;
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

    HomeSidechainTriggerGapSlider cooldown;
    HomeSidechainTriggerLinkSelector linkSelector;
    juce::ToggleButton bypass { "BYPASS" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cooldownAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<float> graphPlotBounds;
    bool draggingThreshold = false;

    void timerCallback() override;
    void styleBypass();
    float thresholdForY (float y) const noexcept;
    float yForDb (float db) const noexcept;
    void setThresholdFromY (float y);

    void drawBackground (juce::Graphics&, juce::Rectangle<float>) const;
    void drawLogo (juce::Graphics&, float x, float y) const;
    void drawHeader (juce::Graphics&, juce::Rectangle<float>) const;
    void drawGraphCard (juce::Graphics&, juce::Rectangle<float>) const;
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCooldownCard (juce::Graphics&, juce::Rectangle<float>) const;
    void drawStatusPill (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessorEditor)
};
