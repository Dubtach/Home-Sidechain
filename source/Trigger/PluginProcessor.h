#pragma once

#include <JuceHeader.h>
#include "../Shared/SidechainCommon.h"

class HomeSidechainTriggerAudioProcessor : public juce::AudioProcessor
{
public:
    HomeSidechainTriggerAudioProcessor();
    ~HomeSidechainTriggerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Home-Sidechain Trigger"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
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
    std::atomic<float> triggerMeter { 0.0f };
    std::atomic<float> midiMeter { 0.0f };
    std::atomic<int> triggerCount { 0 };
    std::atomic<int> homeLinkCount { 0 };
    std::atomic<int> audioTriggerCount { 0 };
    std::atomic<int> midiTriggerCount { 0 };
    std::atomic<int> lastInputMidiNote { -1 };
    std::atomic<int> lastInputMidiChannel { 0 };
    std::atomic<int> manualTriggerRequests { 0 };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float getThresholdDb() const noexcept;
    int getLink() const noexcept;
    int getMidiNote() const noexcept;
    void manualTrigger() noexcept { manualTriggerRequests.fetch_add (1, std::memory_order_relaxed); }

    int getHomeLinkDroppedCount() const noexcept { return homeLinkSender.getDroppedCount(); }

private:
    homeSidechain::HomeLinkSender homeLinkSender;
    double sampleRate = 44100.0;
    bool wasAboveThreshold = false;
    int samplesSinceLastTrigger = 100000000;
    bool pendingNoteOff = false;
    int pendingNote = -1;
    int64_t localFallbackSample = 0;
    uint32_t lastHeartbeatMs = 0;

    int64_t getBlockStartSample (int numSamples) noexcept;
    void emitTrigger (int sampleOffset, int velocity, uint8_t source,
                      juce::MidiBuffer& midi, int note, int numSamples,
                      int64_t blockStartSample);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessor)
};
