#pragma once

#include <JuceHeader.h>
#include <array>
#include "../Shared/SidechainCommon.h"
#include "../Shared/SidechainLinkBus.h"

class HomeSidechainReceiverAudioProcessor : public juce::AudioProcessor,
                                            private juce::Thread
{
public:
    HomeSidechainReceiverAudioProcessor();
    ~HomeSidechainReceiverAudioProcessor() override;

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
    std::atomic<float> triggerActivity { 0.0f };
    std::atomic<float> midiActivity { 0.0f };
    std::atomic<float> homeLinkActivity { 0.0f };
    std::atomic<bool> homeLinkConnected { false };
    std::atomic<int> triggerCount { 0 };
    std::atomic<int> midiEventCount { 0 };
    std::atomic<int> homeLinkEventCount { 0 };
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<int> lastMidiChannel { 0 };
    std::atomic<int> lastHomeLinkVelocity { 127 };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setShapePoint (int index, float value);
    float getShapePoint (int index) const noexcept;
    double getHostBpm() const noexcept;

private:
    struct ScheduledEvent
    {
        homeSidechain::LinkEvent event;
    };

    static constexpr int queueCapacity = 256;
    std::array<ScheduledEvent, queueCapacity> incomingQueue {};
    juce::AbstractFifo incomingFifo { queueCapacity };
    std::array<ScheduledEvent, queueCapacity> scheduledEvents {};
    int scheduledCount = 0;

    homeSidechain::LinkBus linkBus;
    size_t busCursor = homeSidechain::LinkBus::headerSize;
    double sampleRate = 44100.0;
    float envelopePhase = 0.0f;
    bool envelopeActive = false;
    int remainingSamples = 0;
    juce::LinearSmoothedValue<float> gainSmoother;
    juce::int64 lastHeartbeatCheckMs = 0;
    bool clockWasValid = false;

    float shapeValue (float phase) const noexcept;
    float modulationGain (float shape) const noexcept;
    double cycleSamples() const noexcept;
    void triggerEnvelope();
    bool useHomeLink() const noexcept;
    bool useMidi() const noexcept;
    void threadRun() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessor)
};
