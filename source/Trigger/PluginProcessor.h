#pragma once

#include <JuceHeader.h>
#include <array>
#include "../Shared/SidechainCommon.h"
#include "../Shared/SidechainLinkBus.h"

class HomeSidechainTriggerAudioProcessor : public juce::AudioProcessor,
                                            private juce::Thread
{
public:
    HomeSidechainTriggerAudioProcessor();
    ~HomeSidechainTriggerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Home-Sidechain Trigger"; }
    bool acceptsMidi() const override { return false; }
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
    std::atomic<int> triggerCount { 0 };
    std::atomic<bool> homeLinkActive { false };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float getThresholdDb() const noexcept;
    int getLink() const noexcept;

private:
    struct PendingEvent
    {
        homeSidechain::LinkEvent event;
    };

    static constexpr int queueCapacity = 256;
    std::array<PendingEvent, queueCapacity> outgoingQueue {};
    juce::AbstractFifo outgoingFifo { queueCapacity };

    homeSidechain::LinkBus linkBus;
    double sampleRate = 44100.0;
    bool wasAboveThreshold = false;
    int samplesSinceLastTrigger = 100000000;
    bool pendingNoteOff = false;
    int pendingNote = -1;
    juce::int64 lastHeartbeatMs = 0;

    void enqueueHomeTrigger (int link, float velocity, int sampleOffset);
    void threadRun() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessor)
};
