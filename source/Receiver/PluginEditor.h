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

    // Off by default: dragging is free-hand unless the person turns on
    // "Always snap" in the advanced panel. Holding Shift snaps a drag to the
    // grid on demand, whichever state Always Snap is in.
    bool snapEnabled = false;

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
    float snapPhase (float, bool shiftHeld) const noexcept;
    float snapValue (float, bool shiftHeld) const noexcept;

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
// Rate selector. Straight rates are a handful of buttons, but triplet, dotted,
// and poly-groove subdivisions add up to 22 total -- too many to lay out as
// pills without the timing card turning into a wall of tiny buttons. This
// shows the current rate as a large readout with prev/next steppers, and
// clicking the readout opens the full list as a categorised menu.
// =============================================================================

class ReceiverRateSelector : public juce::Component
{
public:
    explicit ReceiverRateSelector (HomeSidechainReceiverAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    // Distinct from Component::setEnabled: also greys/blocks the stepper
    // buttons, which have their own independent enabled state.
    void setUsable (bool usable);

    void refresh() { repaint(); }

private:
    HomeSidechainReceiverAudioProcessor& processor;

    homeUI::Pill prevPill { "<", homeUI::green };
    homeUI::Pill nextPill { ">", homeUI::green };
    juce::Rectangle<int> labelArea;
    bool usable = true;

    void step (int delta);
    void openMenu();

    static juce::String categoryName (int rateIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverRateSelector)
};

// =============================================================================
// Advanced controls live behind the ADV button, the way Home-Disto hides its
// settings, so the front face stays down to what you touch while writing.
// =============================================================================

class ReceiverSettingsPanel : public juce::Component
{
public:
    ReceiverSettingsPanel (HomeSidechainReceiverAudioProcessor&, ReceiverCurveEditor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    void refresh();

private:
    HomeSidechainReceiverAudioProcessor& processor;
    ReceiverCurveEditor& curveEditor;

    homeUI::Knob smoothKnob { "SMOOTH" };
    homeUI::Knob lowCutKnob { "LOW CUT" };
    homeUI::Knob highCutKnob { "HIGH CUT" };

    std::array<std::unique_ptr<homeUI::Pill>, 3> sourcePills;
    homeUI::Checkbox alwaysSnapPill { "Always snap to grid", homeUI::green };
    homeUI::Pill closePill { "CLOSE", homeUI::warn };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> smoothAttachment, lowCutAttachment, highCutAttachment;

    static juce::Rectangle<float> cardBounds() noexcept { return { 110.0f, 76.0f, 500.0f, 270.0f }; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReceiverSettingsPanel)
};

// =============================================================================

class HomeSidechainReceiverAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    static constexpr int designWidth = 720;
    static constexpr int designHeight = 416;

    explicit HomeSidechainReceiverAudioProcessorEditor (HomeSidechainReceiverAudioProcessor&);
    ~HomeSidechainReceiverAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HomeSidechainReceiverAudioProcessor& processor;

    ReceiverCurveEditor curveEditor;
    ReceiverShapeStrip shapeStrip;
    ReceiverRateSelector rateSelector;
    ReceiverSettingsPanel settingsPanel;

    std::array<std::unique_ptr<homeUI::Pill>, homeSidechain::numberOfLinks> linkPills;
    std::array<std::unique_ptr<homeUI::Pill>, 2> runPills;

    homeUI::Checkbox syncPill { "Sync", homeUI::green };
    homeUI::Pill testPill { "TEST", homeUI::cyan };
    homeUI::Pill advPill { "ADV", homeUI::cyan };
    homeUI::ResetButton resetIcon;
    homeUI::PowerButton power;

    homeUI::Knob depthKnob { "DEPTH" };
    homeUI::Knob mixKnob { "MIX" };

    // Lives in the timing card, not Advanced: it only means anything when
    // Sync is off, so it swaps in for the rate selector right where the rate
    // selector would otherwise be, rather than being buried a click away.
    homeUI::Knob lengthKnob { "LENGTH" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> depthAttachment, mixAttachment, lengthAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment, syncAttachment;

    // X position of the divider line drawn between the TRIG/HOST pair and
    // the Sync checkbox, so Sync doesn't read as a third mode option.
    int syncDividerX = 0;

    // Graph and Shape keep their slots (Shape's shorter height stays --
    // that's the one you said you liked). Timing is shorter than the
    // original, and Output is back to its original 200x122 size, moved up
    // to sit directly under Timing instead of down in Shape's row.
    static juce::Rectangle<float> graphCard()   { return { 20.0f,  76.0f, 470.0f, 202.0f }; }
    static juce::Rectangle<float> timingCard()  { return { 500.0f, 76.0f, 200.0f, 170.0f }; }
    static juce::Rectangle<float> shapeCard()   { return { 20.0f, 288.0f, 470.0f, 100.0f }; }
    static juce::Rectangle<float> outputCard()  { return { 500.0f, 258.0f, 200.0f, 122.0f }; }

    void timerCallback() override;
    void refreshFromParameters();
    void drawHeader (juce::Graphics&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessorEditor)
};
