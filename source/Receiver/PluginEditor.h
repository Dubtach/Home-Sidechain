#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include "PluginProcessor.h"
#include "../Shared/HomeSeriesUI.h"

// =============================================================================
// The shaper graph. This is the plugin; everything else supports it.
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

    static constexpr int maxNodes = HomeSidechainReceiverAudioProcessor::maxNodes;

    HomeSidechainReceiverAudioProcessor& processor;

    int gridDivisions = 4;
    bool snapEnabled = true;

    int draggedSlot = -1;
    int draggedSegment = -1;
    int hoveredSlot = -1;
    int hoveredSegment = -1;

    int buildSorted (std::array<SortedNode, maxNodes>&) const;

    juce::Rectangle<float> plotBounds() const noexcept;
    float phaseToX (float) const noexcept;
    float valueToY (float) const noexcept;
    float xToPhase (float) const noexcept;
    float yToValue (float) const noexcept;
    float snapPhase (float, bool fine) const noexcept;
    float snapValue (float, bool fine) const noexcept;

    int nodeAt (juce::Point<float>) const;
    int segmentHandleAt (juce::Point<float>) const;
    bool canDeleteNode (int slot) const;
    juce::Point<float> handlePosition (const SortedNode&, const SortedNode&) const noexcept;

    void drawGrid (juce::Graphics&, juce::Rectangle<float>) const;
    void drawCurve (juce::Graphics&, juce::Rectangle<float>) const;
    void drawPlayhead (juce::Graphics&, juce::Rectangle<float>) const;
    void drawNodes (juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverCurveEditor)
};

// =============================================================================

class ReceiverShapeStrip : public juce::Component
{
public:
    ReceiverShapeStrip() = default;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

    void setSelected (int index);

    std::function<void (int)> onSelect;

private:
    int selected = 0;
    int hovered = -1;

    juce::Rectangle<float> cellBounds (int) const noexcept;
    int cellAt (juce::Point<float>) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverShapeStrip)
};

// =============================================================================
// Advanced controls live behind the ADV button, the way Home-Disto hides its
// settings, so the front face stays down to what you touch while writing.
// =============================================================================

class ReceiverSettingsPanel : public juce::Component
{
public:
    explicit ReceiverSettingsPanel (HomeSidechainReceiverAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    void refresh();

private:
    HomeSidechainReceiverAudioProcessor& processor;

    homeUI::Knob smoothKnob { "SMOOTH" };
    homeUI::Knob lowCutKnob { "LOW CUT" };
    homeUI::Knob highCutKnob { "HIGH CUT" };
    homeUI::Knob lengthKnob { "LENGTH" };

    std::array<std::unique_ptr<homeUI::Pill>, 3> sourcePills;
    homeUI::Pill closePill { "CLOSE", homeUI::warn };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> smoothAttachment, lowCutAttachment, highCutAttachment, lengthAttachment;

    static juce::Rectangle<float> cardBounds() noexcept { return { 110.0f, 96.0f, 500.0f, 240.0f }; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverSettingsPanel)
};

// =============================================================================

class HomeSidechainReceiverAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    static constexpr int designWidth = 720;
    static constexpr int designHeight = 430;

    explicit HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor&);
    ~HomeSidechainReceiverAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainReceiverAudioProcessor& processor;

    ReceiverCurveEditor curveEditor;
    ReceiverShapeStrip shapeStrip;
    ReceiverSettingsPanel settingsPanel;

    std::array<std::unique_ptr<homeUI::Pill>, homeSidechain::numberOfLinks> linkPills;
    std::array<std::unique_ptr<homeUI::Pill>, HomeSidechainReceiverAudioProcessor::numRates> ratePills;
    std::array<std::unique_ptr<homeUI::Pill>, 2> runPills;

    homeUI::Pill syncPill { "SYNC", homeUI::green };
    homeUI::Pill snapPill { "SNAP", homeUI::green };
    homeUI::Pill testPill { "TEST", homeUI::cyan };
    homeUI::Pill advPill { "ADV", homeUI::cyan };
    homeUI::Pill resetPill { "RESET", homeUI::purple };
    homeUI::PowerButton power;

    homeUI::Knob depthKnob { "DEPTH" };
    homeUI::Knob mixKnob { "MIX" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> depthAttachment, mixAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment, syncAttachment;

    static juce::Rectangle<float> graphCard()  { return { 20.0f,  76.0f, 470.0f, 202.0f }; }
    static juce::Rectangle<float> outputCard() { return { 500.0f, 76.0f, 200.0f, 202.0f }; }
    static juce::Rectangle<float> shapeCard()  { return { 20.0f, 288.0f, 470.0f, 122.0f }; }
    static juce::Rectangle<float> timingCard() { return { 500.0f, 288.0f, 200.0f, 122.0f }; }

    void timerCallback() override;
    void refreshFromParameters();
    void drawHeader (juce::Graphics&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
