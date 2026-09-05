#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "../Shared/SidechainCommon.h"

class HomeSidechainReceiverAudioProcessor : public juce::AudioProcessor
{
public:
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

    juce::AudioProcessorValueTreeState apvts;

    // UI telemetry. These are written atomically from the audio thread and
    // consumed by the editor timer; they are never used for audio decisions.
    std::atomic<float> triggerActivity { 0.0f };
    std::atomic<float> envelopeDisplayPhase { 0.0f };
    std::atomic<bool> envelopeActiveForUI { false };
    std::atomic<float> midiActivity { 0.0f };
    std::atomic<int> triggerCount { 0 };
    std::atomic<int> midiEventCount { 0 };
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<int> lastMidiChannel { 0 };
    std::atomic<float> homeLinkActivity { 0.0f };
    std::atomic<int> homeLinkTriggerCount { 0 };
    std::atomic<bool> homeLinkConnected { false };
    std::atomic<int> lastHomeLinkVelocity { 0 };

    int getLink() const noexcept;
    double getSampleRate() const noexcept { return sampleRate; }
    double getHostBpm() const noexcept;
    double cycleSamples() const noexcept;

    // Shape editor API used by the graph. There are up to eight persistent
    // breakpoints; X controls their position and Y controls duck depth.
    static constexpr int maxNodes = 8;
    float getNodeX (int index) const noexcept;
    float getNodeY (int index) const noexcept;
    float getHandle (int segment) const noexcept;
    bool isNodeActive (int index) const noexcept;
    void setNodeX (int index, float value);
    void setNodeY (int index, float value);
    void setHandle (int segment, float value);
    void setNodeActive (int index, bool active);
    int activeNodeCount() const noexcept;
    float shapeValue (float phase) const noexcept;

    // UI-only test trigger.
    void requestTestTrigger() noexcept { testTriggerRequested.store (true, std::memory_order_release); }

    float getMix() const noexcept;
    float getDepth() const noexcept;

private:
    double sampleRate = 44100.0;
    float envelopePhase = 0.0f;
    bool envelopeActiveInternal = false;
    int remainingSamples = 0;

    juce::LinearSmoothedValue<float> gainSmoother;
    std::array<float, 2> lowCutState { 0.0f, 0.0f };
    std::array<float, 2> highCutState { 0.0f, 0.0f };
    std::array<float, 2> crossoverState { 0.0f, 0.0f };
    std::array<float, maxNodes> cachedNodeX {};
    std::array<float, maxNodes> cachedNodeY {};
    std::array<float, maxNodes - 1> cachedHandle {};
    int cachedNodeCount = 0;

    void refreshShapeCache() noexcept;
    float shapeValueCached (float phase) const noexcept;

    float modulationGain (float shape) const noexcept;
    void triggerEnvelope();

    homeSidechain::HomeLinkReceiverService& homeLinkService() noexcept
    {
        return homeSidechain::HomeLinkReceiverService::instance();
    }

    std::atomic<bool> testTriggerRequested { false };
    uint64_t homeLinkLastSequence = 0;
    int homeLinkLastLink = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessor)
};
