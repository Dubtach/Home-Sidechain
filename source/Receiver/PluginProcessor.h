#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include "../Shared/SidechainCommon.h"

// =============================================================================
// Shared curve model.
//
// The audio thread and the editor both evaluate the shape through this type,
// so the curve you see drawn is exactly the curve you hear. A Snapshot is a
// plain value type: the audio thread copies one per block, the editor builds
// one per repaint. Nothing here allocates.
//
// y == 1.0 means "full level", y == 0.0 means "fully ducked", which matches
// the way Kickstart / VolumeShaper style envelopes read on screen.
// =============================================================================
namespace receiverCurve
{
    inline constexpr int maxNodes = 16;

    // tension: 0.5 is a straight line, below 0.5 moves fast then settles
    // (classic sidechain release), above 0.5 lingers then snaps.
    inline float applyTension (float t, float tension) noexcept
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        const float k = juce::jlimit (0.0f, 1.0f, tension);
        const float e = std::pow (2.0f, (k - 0.5f) * 6.0f);

        if (std::abs (e - 1.0f) < 1.0e-4f)
            return t;

        return std::pow (t, e);
    }

    // Inverse of applyTension: the tension that makes the segment pass
    // through normalised height u at normalised position t. Used so a curve
    // handle follows the mouse exactly instead of approximately.
    inline float tensionForPoint (float t, float u) noexcept
    {
        t = juce::jlimit (0.02f, 0.98f, t);
        u = juce::jlimit (0.002f, 0.998f, u);

        const float e = std::log (u) / std::log (t);
        const float clamped = juce::jlimit (0.125f, 8.0f, e);
        return juce::jlimit (0.0f, 1.0f, 0.5f + std::log2 (clamped) / 6.0f);
    }

    struct Snapshot
    {
        int count = 0;
        std::array<float, maxNodes> x {};
        std::array<float, maxNodes> y {};
        std::array<float, maxNodes> tension {};

        void add (float nx, float ny, float nt) noexcept
        {
            if (count >= maxNodes)
                return;

            const auto i = static_cast<size_t> (count);
            x[i] = juce::jlimit (0.0f, 1.0f, nx);
            y[i] = juce::jlimit (0.0f, 1.0f, ny);
            tension[i] = juce::jlimit (0.0f, 1.0f, nt);
            ++count;
        }

        float valueAt (float phase) const noexcept
        {
            if (count <= 0)
                return 1.0f;

            phase = juce::jlimit (0.0f, 1.0f, phase);

            if (count == 1)
                return y[0];

            const auto last = static_cast<size_t> (count - 1);

            if (phase <= x[0])
                return y[0];

            if (phase >= x[last])
                return y[last];

            int segment = count - 2;

            for (int i = 0; i < count - 1; ++i)
            {
                if (phase <= x[static_cast<size_t> (i + 1)])
                {
                    segment = i;
                    break;
                }
            }

            const auto a = static_cast<size_t> (segment);
            const auto b = static_cast<size_t> (segment + 1);
            const float dx = x[b] - x[a];

            if (dx <= 1.0e-6f)
                return y[b];

            const float t = applyTension ((phase - x[a]) / dx, tension[a]);
            return y[a] + (y[b] - y[a]) * t;
        }
    };
}

// =============================================================================

class HomeSidechainReceiverAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int maxNodes = receiverCurve::maxNodes;
    static constexpr int numPresets = 7;

    // Rate table layout. Straight values come first (and keep the same
    // indices v45 used, so a v45 session still opens on the right rate),
    // then triplet, dotted, and poly-groove subdivisions. The UI's rate
    // selector uses these counts to build its categorised menu without
    // duplicating the boundaries by hand.
    static constexpr int numStraightRates = 6;
    static constexpr int numTripletRates = 5;
    static constexpr int numDottedRates = 5;
    static constexpr int numGrooveRates = 6;
    static constexpr int numRates = numStraightRates + numTripletRates + numDottedRates + numGrooveRates;

    HomeSidechainReceiverAudioProcessor();
    ~HomeSidechainReceiverAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Home-Sidechain Receiver"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    static juce::StringArray rateNames();
    static double beatsForRate (int rate) noexcept;
    static juce::String presetName (int index);
    static receiverCurve::Snapshot presetSnapshot (int index);

    juce::AudioProcessorValueTreeState apvts;

    // -------------------------------------------------------------------------
    // UI telemetry. Written from the audio thread, read by the editor timer.
    // Never used to make an audio decision.
    // -------------------------------------------------------------------------
    std::atomic<float> triggerActivity { 0.0f };
    std::atomic<float> midiActivity { 0.0f };
    std::atomic<float> homeLinkActivity { 0.0f };
    std::atomic<float> envelopeDisplayPhase { 0.0f };
    std::atomic<bool> envelopeActiveForUI { false };
    std::atomic<float> currentGainForUI { 1.0f };
    std::atomic<float> outputLevelForUI { 0.0f };
    std::atomic<int> triggerCount { 0 };
    std::atomic<int> midiEventCount { 0 };
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<int> homeLinkTriggerCount { 0 };
    std::atomic<bool> homeLinkConnected { false };
    std::atomic<float> hostBpmForUI { 120.0f };

    // -------------------------------------------------------------------------
    // Parameter helpers
    // -------------------------------------------------------------------------
    int getLink() const noexcept;
    void setLink (int link);
    int getRunMode() const noexcept;        // 0 = triggered, 1 = host sync
    void setRunMode (int mode);
    int getSource() const noexcept;         // 0 = Home-Link, 1 = MIDI, 2 = both
    void setSource (int source);
    int getRate() const noexcept;
    void setRate (int rate);
    bool isSynced() const noexcept;
    bool isBypassed() const noexcept;
    float getMix() const noexcept;
    float getDepth() const noexcept;
    double getHostBpm() const noexcept;
    double cycleSamples() const noexcept;
    int gridDivisions() const noexcept;

    // -------------------------------------------------------------------------
    // Curve editing. Nodes live in fixed parameter slots and carry an on/off
    // flag, so adding and removing points never reorders automation.
    // -------------------------------------------------------------------------
    receiverCurve::Snapshot buildSnapshot() const noexcept;
    bool isNodeActive (int slot) const noexcept;
    float getNodeX (int slot) const noexcept;
    float getNodeY (int slot) const noexcept;
    float getTension (int slot) const noexcept;
    void setNodeX (int slot, float value);
    void setNodeY (int slot, float value);
    void setTension (int slot, float value);
    void setNodeActive (int slot, bool active);

    // The first and last points are the loop seam. They always carry the same
    // height, so the cycle can repeat without a step in gain.
    void setEndpointY (float value);
    bool isEndpoint (int slot) const noexcept;
    int activeNodeCount() const noexcept;
    int addNode (float x, float y);
    void removeNode (int slot);
    void applyPreset (int index);
    void resetCurve();

    void requestTestTrigger() noexcept { testTriggerRequested.store (true, std::memory_order_release); }

private:
    double currentSampleRate = 44100.0;

    // Envelope transport
    double envelopePhase = 0.0;
    bool envelopeActiveInternal = false;
    double remainingSamples = 0.0;

    // Output smoothing (one pole, so changing the time never snaps the value)
    float gainState = 1.0f;
    float smoothCoefficient = 0.0f;

    // Sidechain band filters
    std::array<float, 2> lowCutState { 0.0f, 0.0f };
    std::array<float, 2> highCutState { 0.0f, 0.0f };

    receiverCurve::Snapshot activeCurve;

    void triggerEnvelope();
    float gainForPhase (float phase, float depth) const noexcept;

    static homeSidechain::HomeLinkReceiverService& homeLinkService() noexcept
    {
        return homeSidechain::HomeLinkReceiverService::instance();
    }

    std::atomic<bool> testTriggerRequested { false };
    uint64_t homeLinkLastSequence = 0;
    int homeLinkLastLink = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessor)
};
