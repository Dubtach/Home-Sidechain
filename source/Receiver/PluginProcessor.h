#pragma once

#include <JuceHeader.h>
#include <array>
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

    std::atomic<float> triggerActivity { 0.0f };
    std::atomic<float> midiActivity { 0.0f };
    std::atomic<int> triggerCount { 0 };
    std::atomic<int> midiEventCount { 0 };
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<int> lastMidiChannel { 0 };
    std::atomic<float> homeLinkActivity { 0.0f };
    std::atomic<int> homeLinkTriggerCount { 0 };
    std::atomic<bool> homeLinkConnected { false };
    std::atomic<int> lastHomeLinkVelocity { 0 };
    std::atomic<int> lastHomeLinkSource { 0 };

    int getLink() const noexcept;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setShapePoint (int index, float value);
    float getShapePoint (int index) const noexcept;
    double getHostBpm() const noexcept;

    void resetShape();
    void flipShape();
    void smoothShape();
    void snapShape();
    void applyPreset (int presetIndex);

private:
    double sampleRate = 44100.0;
    bool envelopeActive = false;
    double envelopePhase = 0.0;
    int64_t remainingSamples = 0;
    uint64_t homeLinkLastSequence = 0;
    int homeLinkLastLink = 0;
    int64_t localFallbackSample = 0;

    int64_t getBlockStartSample (int numSamples) noexcept;
    double cycleSamples() const noexcept;
    float shapeValue (float phase) const noexcept;
    float modulationGain (float shape) const noexcept;
    void triggerEnvelope (double startPhase = 0.0);

    homeSidechain::HomeLinkReceiverService& homeLinkService() noexcept
    {
        return homeSidechain::HomeLinkReceiverService::instance();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessor)
};
