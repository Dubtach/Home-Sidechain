#include "PluginProcessor.h"
#include "PluginEditor.h"

HomeSidechainTriggerAudioProcessor::HomeSidechainTriggerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HomeSidechainTriggerAudioProcessor::createParameters()
{
    using FloatAttributes = juce::AudioParameterFloatAttributes;
    using BoolAttributes = juce::AudioParameterBoolAttributes;
    using ChoiceAttributes = juce::AudioParameterChoiceAttributes;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "BYPASS", "Bypass", false, BoolAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "THRESHOLD", "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.01f), -18.0f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "SENSITIVITY", "Sensitivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RETRIGGER", "Retrigger",
        juce::NormalisableRange<float> (5.0f, 1000.0f, 1.0f, 0.4f), 80.0f,
        FloatAttributes{}.withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "VELOCITY", "Velocity",
        juce::NormalisableRange<float> (1.0f, 127.0f, 1.0f), 127.0f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "LINK", "Link", homeSidechain::linkNames(), 0, ChoiceAttributes{}));

    return { params.begin(), params.end() };
}

float HomeSidechainTriggerAudioProcessor::getThresholdDb() const noexcept
{
    return apvts.getRawParameterValue ("THRESHOLD")->load();
}

int HomeSidechainTriggerAudioProcessor::getLink() const noexcept
{
    return static_cast<int> (apvts.getRawParameterValue ("LINK")->load());
}

void HomeSidechainTriggerAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    wasAboveThreshold = false;
    samplesSinceLastTrigger = 100000000;
    pendingNoteOff = false;
    pendingNote = -1;
    triggerMeter.store (0.0f);
    homeLinkSender.start();
}

void HomeSidechainTriggerAudioProcessor::releaseResources()
{
    homeLinkSender.stop();
}

bool HomeSidechainTriggerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == mainIn;
}

void HomeSidechainTriggerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    samplesSinceLastTrigger = juce::jmin (100000000, samplesSinceLastTrigger + numSamples);
    triggerMeter.store (triggerMeter.load() * 0.94f, std::memory_order_relaxed);

    const bool bypassed = apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const float threshold = getThresholdDb();
    const float sensitivity = apvts.getRawParameterValue ("SENSITIVITY")->load();
    const float velocityParam = apvts.getRawParameterValue ("VELOCITY")->load();
    const int selectedLink = getLink();
    homeLinkSender.setLink (selectedLink);
    const int note = homeSidechain::midiNoteForLink (selectedLink);
    const int retriggerSamples = static_cast<int> (
        apvts.getRawParameterValue ("RETRIGGER")->load() * 0.001 * sampleRate);

    // MIDI is an automatic second trigger source. A note-on from a piano-roll
    // or external controller is treated exactly like an audio transient. Any
    // incoming MIDI note can act as the trigger; the selected Home-Link still
    // determines the generated output note.
    std::array<int, 128> midiTriggerPositions {};
    std::array<int, 128> midiTriggerVelocities {};
    int midiTriggerCount = 0;
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn (true) && midiTriggerCount < static_cast<int> (midiTriggerPositions.size()))
        {
            midiTriggerPositions[static_cast<size_t> (midiTriggerCount)] =
                juce::jlimit (0, numSamples - 1, metadata.samplePosition);
            midiTriggerVelocities[static_cast<size_t> (midiTriggerCount)] =
                juce::jlimit (1, 127, static_cast<int> (message.getVelocity()));
            ++midiTriggerCount;
        }
    }

    int midiTriggerIndex = 0;

    // Finish a note from the previous block before generating new triggers.
    if (pendingNoteOff && pendingNote >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pendingNote), 0);
        pendingNoteOff = false;
        pendingNote = -1;
    }

    if (bypassed)
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float peak = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = juce::jmax (peak, std::abs (buffer.getSample (channel, sample)));

        const float peakDb = homeSidechain::linearToDb (peak);
        const float dynamicThreshold = threshold - sensitivity * 12.0f;
        const bool above = peakDb >= dynamicThreshold;

        const bool audioTrigger = above && ! wasAboveThreshold;
        const bool midiTrigger = midiTriggerIndex < midiTriggerCount
                              && midiTriggerPositions[static_cast<size_t> (midiTriggerIndex)] == sample;

        if (midiTrigger)
            ++midiTriggerIndex;

        if ((audioTrigger || midiTrigger) && samplesSinceLastTrigger >= retriggerSamples)
        {
            const int audioVelocity = juce::jlimit (1, 127, juce::roundToInt (velocityParam));
            const int triggerVelocity = midiTrigger
                                      ? midiTriggerVelocities[static_cast<size_t> (midiTriggerIndex - 1)]
                                      : audioVelocity;

            midi.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (triggerVelocity)),
                           sample);

            // Home-Link is the zero-routing path. It runs alongside normal MIDI
            // output so advanced users can still use conventional host routing.
            // The transport work is offloaded from the audio thread.
            homeLinkSender.enqueueTrigger (selectedLink, triggerVelocity, sample,
                                           homeSidechain::currentHostTicks());
            homeLinkCount.fetch_add (1, std::memory_order_relaxed);

            if (sample + 1 < numSamples)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), sample + 1);
            else
            {
                pendingNoteOff = true;
                pendingNote = note;
            }

            samplesSinceLastTrigger = 0;
            triggerMeter.store (1.0f, std::memory_order_relaxed);
            triggerCount.fetch_add (1, std::memory_order_relaxed);
        }

        wasAboveThreshold = above;
    }

}

void HomeSidechainTriggerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HomeSidechainTriggerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* HomeSidechainTriggerAudioProcessor::createEditor()
{
    return new HomeSidechainTriggerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HomeSidechainTriggerAudioProcessor();
}
