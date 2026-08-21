#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HomeSeriesTriggerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HomeSeriesTriggerLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a1e));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2a2a30));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff00e5ff));
        setColour (juce::ToggleButton::textColourId, juce::Colours::white.withAlpha (0.66f));
        setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff00ff87));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f);
        auto base = juce::Colour (0xff1a1a1e);
        if (isButtonDown || box.hasKeyboardFocus (true))
            base = base.brighter (0.10f);

        g.setColour (base);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colour (0xff2a2a30));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        g.setColour (juce::Colour (0xff00e5ff));
        juce::Path arrow;
        arrow.addTriangle ((float) buttonX + buttonW * 0.5f - 3.5f, (float) buttonY + buttonH * 0.5f - 1.5f,
                           (float) buttonX + buttonW * 0.5f + 3.5f, (float) buttonY + buttonH * 0.5f - 1.5f,
                           (float) buttonX + buttonW * 0.5f,       (float) buttonY + buttonH * 0.5f + 2.5f);
        g.fillPath (arrow);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto box = juce::Rectangle<float> (0.0f, bounds.getCentreY() - 7.0f, 14.0f, 14.0f);
        const bool on = button.getToggleState();

        g.setColour (juce::Colour (0xff09090b));
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (on ? juce::Colour (0xff00ff87) : juce::Colours::white.withAlpha (0.28f));
        g.drawRoundedRectangle (box, 3.0f, 1.0f);

        if (on)
        {
            g.setColour (juce::Colour (0xff00ff87));
            juce::Path tick;
            tick.startNewSubPath (box.getX() + 3.0f, box.getCentreY());
            tick.lineTo (box.getCentreX() - 1.0f, box.getBottom() - 3.0f);
            tick.lineTo (box.getRight() - 2.0f, box.getY() + 2.5f);
            g.strokePath (tick, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setFont (juce::FontOptions (10.0f).withName ("Helvetica").withStyle (on ? "Bold" : "Plain"));
        g.setColour (on ? juce::Colours::white : juce::Colours::white.withAlpha (0.62f));
        g.drawText (button.getButtonText(), bounds.withTrimmedLeft (20.0f).toNearestInt(), juce::Justification::centredLeft);

        if (shouldDrawButtonAsHighlighted)
        {
            g.setColour (juce::Colour (0xff2a2a30).withAlpha (0.30f));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
        }
    }

};

class HomeSidechainTriggerGapSlider : public juce::Slider
{
public:
    HomeSidechainTriggerGapSlider();
    void paint (juce::Graphics&) override;
};

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
    HomeSeriesTriggerLookAndFeel homeSeriesLaf;

    HomeSidechainTriggerGapSlider retrigger;
    juce::ComboBox link;
    juce::ToggleButton bypass { "BYPASS" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retriggerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> linkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<float> graphBounds;
    bool draggingThreshold = false;

    void timerCallback() override;
    void styleComboBox();
    void styleBypass();
    float thresholdForY (float y) const noexcept;
    float yForDb (float db) const noexcept;
    void setThresholdFromY (float y);
    void drawHeader (juce::Graphics&, juce::Rectangle<float>) const;
    void drawGraph (juce::Graphics&, juce::Rectangle<float>) const;
    void drawControlStrip (juce::Graphics&, juce::Rectangle<float>) const;
    void drawPill (juce::Graphics&, juce::Rectangle<float>, juce::Colour, const juce::String&, bool) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessorEditor)
};
