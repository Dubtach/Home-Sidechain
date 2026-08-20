#pragma once

#include <JuceHeader.h>
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

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setShapePoint (int index, float value);
    float getShapePoint (int index) const noexcept;
    double getHostBpm() const noexcept;

private:
    double sampleRate = 44100.0;
    int noteOffCounter = 0;
    float envelopePhase = 0.0f;
    bool envelopeActive = false;
    int remainingSamples = 0;
    float smoothedGain = 1.0f;
    juce::LinearSmoothedValue<float> gainSmoother;

    float shapeValue (float phase) const noexcept;
    float modulationGain (float shape) const noexcept;
    double cycleSamples() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeSidechainReceiverAudioProcessor)
};
