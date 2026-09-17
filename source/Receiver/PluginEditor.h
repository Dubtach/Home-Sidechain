#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include "PluginProcessor.h"

// =============================================================================
// Small, self-painting controls. Each one draws itself rather than going
// through a LookAndFeel, which keeps the styling next to the thing it styles.
// =============================================================================

class ReceiverPill : public juce::Button
{
public:
    ReceiverPill (const juce::String& text, juce::Colour accentColour);

    void paintButton (juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

    void setAccent (juce::Colour newAccent);
    void setFontSize (float size) noexcept { fontSize = size; }
    void setFilled (bool shouldFill) noexcept { filled = shouldFill; }

private:
    juce::Colour accent;
    float fontSize = 11.0f;
    bool filled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverPill)
};

class ReceiverKnob : public juce::Slider
{
public:
    ReceiverKnob (const juce::String& captionText, juce::Colour accentColour);

    void paint (juce::Graphics&) override;

    std::function<juce::String (double)> valueText;

private:
    juce::String caption;
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverKnob)
};

// =============================================================================
// The curve editor is the plugin. Everything else is secondary to it.
// =============================================================================

class ReceiverCurveEditor : public juce::Component
{
public:
    explicit ReceiverCurveEditor (HomeSidechainReceiverAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void setSnapEnabled (bool shouldSnap) noexcept { snapEnabled = shouldSnap; }
    bool isSnapEnabled() const noexcept { return snapEnabled; }
    void setGridDivisions (int divisions) noexcept;

private:
    struct SortedNode
    {
        int slot = 0;
        float x = 0.0f;
        float y = 0.0f;
        float tension = 0.5f;
    };

    HomeSidechainReceiverAudioProcessor& processor;

    int gridDivisions = 4;
    bool snapEnabled = true;

    int draggedSlot = -1;
    int draggedSegment = -1;
    int hoveredSlot = -1;
    int hoveredSegment = -1;

    int buildSorted (std::array<SortedNode, HomeSidechainReceiverAudioProcessor::maxNodes>&) const;

    juce::Rectangle<float> plotBounds() const noexcept;
    float phaseToX (float phase) const noexcept;
    float valueToY (float value) const noexcept;
    float xToPhase (float x) const noexcept;
    float yToValue (float y) const noexcept;
    float snapPhase (float phase, bool fine) const noexcept;

    int nodeAt (juce::Point<float>) const;
    int segmentHandleAt (juce::Point<float>) const;
    bool canDeleteNode (int slot) const;
    juce::Point<float> handlePosition (const SortedNode& a, const SortedNode& b) const noexcept;

    void drawGrid (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCurve (juce::Graphics&, juce::Rectangle<float>) const;
    void drawPlayhead (juce::Graphics&, juce::Rectangle<float>) const;
    void drawNodes (juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverCurveEditor)
};

class ReceiverPresetStrip : public juce::Component
{
public:
    ReceiverPresetStrip();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

    void setSelected (int index);
    int getSelected() const noexcept { return selected; }

    std::function<void (int)> onSelect;

private:
    int selected = 0;
    int hovered = -1;

    juce::Rectangle<float> cellBounds (int index) const noexcept;
    int cellAt (juce::Point<float>) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverPresetStrip)
};

// =============================================================================
// The whole interface lives in one fixed-size panel that the editor scales, so
// the window can be resized without reflowing every control.
// =============================================================================

class ReceiverPanel : public juce::Component,
                      private juce::Timer
{
public:
    static constexpr int designWidth = 940;
    static constexpr int designHeight = 620;

    explicit ReceiverPanel (HomeSidechainReceiverAudioProcessor&);
    ~ReceiverPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainReceiverAudioProcessor& processor;

    ReceiverCurveEditor curveEditor;
    ReceiverPresetStrip presetStrip;

    std::array<std::unique_ptr<ReceiverPill>, homeSidechain::numberOfLinks> linkPills;
    std::array<std::unique_ptr<ReceiverPill>, HomeSidechainReceiverAudioProcessor::numRates> ratePills;
    std::array<std::unique_ptr<ReceiverPill>, 2> runPills;
    std::array<std::unique_ptr<ReceiverPill>, 3> sourcePills;

    ReceiverPill bypassPill { "Bypass", juce::Colour (0xffff5965) };
    ReceiverPill syncPill { "Sync", juce::Colour (0xff36e79a) };
    ReceiverPill snapPill { "Snap", juce::Colour (0xffb08cff) };
    ReceiverPill testPill { "Test", juce::Colour (0xff1ee7ff) };
    ReceiverPill resetPill { "Reset", juce::Colour (0xff8796a3) };

    ReceiverKnob depthKnob { "Depth", juce::Colour (0xff1ee7ff) };
    ReceiverKnob mixKnob { "Mix", juce::Colour (0xff1ee7ff) };
    ReceiverKnob smoothKnob { "Smooth", juce::Colour (0xffb08cff) };
    ReceiverKnob lowCutKnob { "Low cut", juce::Colour (0xff36e79a) };
    ReceiverKnob highCutKnob { "High cut", juce::Colour (0xff36e79a) };
    ReceiverKnob lengthKnob { "Length", juce::Colour (0xffb08cff) };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> smoothAttachment;
    std::unique_ptr<SliderAttachment> lowCutAttachment;
    std::unique_ptr<SliderAttachment> highCutAttachment;
    std::unique_ptr<SliderAttachment> lengthAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;

    juce::Rectangle<int> headerArea, presetArea, graphArea, footerArea;

    void timerCallback() override;
    void refreshFromParameters();
    void addKnob (ReceiverKnob&);

    void drawHeader (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCard (juce::Graphics&, juce::Rectangle<float>, const juce::String& title,
                   juce::Colour accent) const;
    void drawStatusLamp (juce::Graphics&, juce::Rectangle<float>, const juce::String&,
                         juce::Colour, float activity, bool connected) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverPanel)
};

class HomeSidechainReceiverAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor&);
    ~HomeSidechainReceiverAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ReceiverPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
