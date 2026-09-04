#pragma once

#include <JuceHeader.h>
#include <cstdint>
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
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<int> homeLinkCount { 0 };

    static constexpr size_t waveformPointCount = 320;
    static constexpr double waveformHistorySeconds = 3.00;
    int waveformSampleStride = 64;
    std::array<std::atomic<float>, waveformPointCount> waveformBuffer {};
    std::array<std::atomic<bool>, waveformPointCount> waveformTriggered {};
    std::array<std::atomic<bool>, waveformPointCount> waveformMidiInput {};
    std::atomic<size_t> waveformWriteIndex { 0 };
    std::atomic<std::uint64_t> waveformWriteSerial { 0 };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float getThresholdDb() const noexcept;
    int getLink() const noexcept;
    int getWaveformPointCount() const noexcept { return static_cast<int> (waveformPointCount); }
    float getWaveformPoint (int index) const noexcept;
    bool getWaveformTriggered (int index) const noexcept;
    bool getWaveformMidiInput (int index) const noexcept;

    int getHomeLinkDroppedCount() const noexcept { return homeLinkSender.getDroppedCount(); }
    float getTriggerMeter() const noexcept { return triggerMeter.load (std::memory_order_relaxed); }
    float getInputLevel() const noexcept { return inputLevel.load (std::memory_order_relaxed); }
    int getLatestTriggerPointIndex() const noexcept;
    void requestTestTrigger() noexcept { testTriggerPending.store (true, std::memory_order_release); }

    // Automatic smart trigger input: audio peaks and incoming MIDI notes are
    // handled together; there is deliberately no mode switch in the UI.

private:
    homeSidechain::HomeLinkSender homeLinkSender;
    double sampleRate = 44100.0;
    bool wasAboveThreshold = false;
    int samplesSinceLastTrigger = 100000000;
    bool pendingNoteOff = false;
    int pendingNote = -1;
    float waveformAccumPeak = 0.0f;
    int waveformAccumSamples = 0;
    bool waveformAccumTriggered = false;
    bool waveformAccumMidiInput = false;
    std::atomic<std::uint64_t> latestTriggerSerial { 0 };
    std::atomic<bool> testTriggerPending { false };
    int lastSelectedLink = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessor)
};
