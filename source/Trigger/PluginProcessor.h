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
    std::atomic<int> triggerCount { 0 };
    std::atomic<int> homeLinkCount { 0 };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float getThresholdDb() const noexcept;
    int getLink() const noexcept;

    int getHomeLinkDroppedCount() const noexcept { return homeLinkSender.getDroppedCount(); }

    // Automatic smart trigger input: audio peaks and incoming MIDI notes are
    // handled together; there is deliberately no mode switch in the UI.

private:
    homeSidechain::HomeLinkSender homeLinkSender;
    double sampleRate = 44100.0;
    bool wasAboveThreshold = false;
    int samplesSinceLastTrigger = 100000000;
    bool pendingNoteOff = false;
    int pendingNote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainTriggerAudioProcessor)
};
