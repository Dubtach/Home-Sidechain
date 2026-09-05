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

    // Smoothed visual activity for the editor. This is intentionally independent
    // of the audio envelope so the user can clearly see that MIDI arrived.
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

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setShapePoint (int index, float value);
    float getShapePoint (int index) const noexcept;
    double getHostBpm() const noexcept;

    // UI-only diagnostic trigger. It is atomic so the editor never touches the
    // audio/envelope state directly.
    void requestTestTrigger() noexcept { testTriggerRequested.store (true, std::memory_order_release); }

private:
    double sampleRate = 44100.0;
    float envelopePhase = 0.0f;
    bool envelopeActiveInternal = false;
    int remainingSamples = 0;
    juce::LinearSmoothedValue<float> gainSmoother;

    float shapeValue (float phase) const noexcept;
    float modulationGain (float shape) const noexcept;
    double cycleSamples() const noexcept;

    homeSidechain::HomeLinkReceiverService& homeLinkService() noexcept
    {
        return homeSidechain::HomeLinkReceiverService::instance();
    }

    std::atomic<bool> testTriggerRequested { false };

    uint64_t homeLinkLastSequence = 0;
    int homeLinkLastLink = 0;

    void triggerEnvelope();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessor)
};
