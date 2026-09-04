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
        juce::NormalisableRange<float> (-48.0f, 0.0f, 0.01f), -18.0f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RETRIGGER", "Cool Down",
        juce::NormalisableRange<float> (50.0f, 2000.0f, 1.0f, 0.42f), 80.0f,
        FloatAttributes{}.withLabel ("ms")));
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

float HomeSidechainTriggerAudioProcessor::getWaveformPoint (int index) const noexcept
{
    if (index < 0 || index >= static_cast<int> (waveformPointCount))
        return 0.0f;

    const auto write = waveformWriteIndex.load (std::memory_order_acquire);
    const size_t offset = static_cast<size_t> (index);
    const size_t slot = (write + offset) % waveformPointCount;
    return waveformBuffer[slot].load (std::memory_order_relaxed);
}

bool HomeSidechainTriggerAudioProcessor::getWaveformMidiInput (int index) const noexcept
{
    if (index < 0 || index >= static_cast<int> (waveformPointCount))
        return false;

    const auto write = waveformWriteIndex.load (std::memory_order_acquire);
    const size_t offset = static_cast<size_t> (index);
    const size_t slot = (write + offset) % waveformPointCount;
    return waveformMidiInput[slot].load (std::memory_order_relaxed);
}

bool HomeSidechainTriggerAudioProcessor::getWaveformTriggered (int index) const noexcept
{
    if (index < 0 || index >= static_cast<int> (waveformPointCount))
        return false;

    const auto write = waveformWriteIndex.load (std::memory_order_acquire);
    const size_t offset = static_cast<size_t> (index);
    const size_t slot = (write + offset) % waveformPointCount;
    return waveformTriggered[slot].load (std::memory_order_relaxed);
}

int HomeSidechainTriggerAudioProcessor::getLatestTriggerPointIndex() const noexcept
{
    const auto writeSerial = waveformWriteSerial.load (std::memory_order_acquire);
    const auto historyCount = static_cast<std::uint64_t> (waveformPointCount);
    const auto oldestSerial = writeSerial > historyCount ? writeSerial - historyCount : 0;
    const auto latestSerial = latestTriggerSerial.load (std::memory_order_acquire);

    if (writeSerial == 0 || latestSerial < oldestSerial || latestSerial >= writeSerial)
        return -1;

    return static_cast<int> (latestSerial - oldestSerial);
}


void HomeSidechainTriggerAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    wasAboveThreshold = false;
    samplesSinceLastTrigger = 100000000;
    pendingNoteOff = false;
    pendingNote = -1;
    triggerMeter.store (0.0f);
    inputLevel.store (0.0f, std::memory_order_relaxed);
    for (auto& value : waveformBuffer) value.store (0.0f, std::memory_order_relaxed);
    for (auto& value : waveformTriggered) value.store (false, std::memory_order_relaxed);
    for (auto& value : waveformMidiInput) value.store (false, std::memory_order_relaxed);
    waveformWriteIndex.store (0, std::memory_order_release);
    waveformWriteSerial.store (0, std::memory_order_release);
    waveformAccumPeak = 0.0f;
    waveformAccumSamples = 0;
    waveformSampleStride = juce::jmax (16, static_cast<int> (std::round ((waveformHistorySeconds * sampleRate)
                                                                      / static_cast<double> (waveformPointCount))));
    waveformAccumTriggered = false;
    waveformAccumMidiInput = false;
    latestTriggerSerial.store (0, std::memory_order_release);
    testTriggerPending.store (false, std::memory_order_release);
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
    if (numSamples <= 0)
        return;

    const bool bypassed = apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const bool manualTrigger = testTriggerPending.exchange (false, std::memory_order_acq_rel);
    const float thresholdDb = getThresholdDb();
    const float thresholdLinear = juce::Decibels::decibelsToGain (thresholdDb);
    const int selectedLink = getLink();
    const int note = homeSidechain::midiNoteForLink (selectedLink);
    const int retriggerSamples = static_cast<int> (
        apvts.getRawParameterValue ("RETRIGGER")->load() * 0.001 * sampleRate);

    // The selected link is block-stable, so don't write the sender atomic every block.
    if (selectedLink != lastSelectedLink)
    {
        homeLinkSender.setLink (selectedLink);
        lastSelectedLink = selectedLink;
    }

    // MIDI is an automatic second trigger source. A note-on from a piano-roll
    // or external controller is treated exactly like an audio transient.
    std::array<int, 128> midiTriggerPositions {};
    std::array<int, 128> midiTriggerVelocities {};
    int midiTriggerCount = 0;
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn (true) && midiTriggerCount < static_cast<int> (midiTriggerPositions.size()))
        {
            midiTriggerPositions[static_cast<size_t> (midiTriggerCount)] =
                juce::jlimit (0, juce::jmax (0, numSamples - 1), static_cast<int> (metadata.samplePosition));
            midiTriggerVelocities[static_cast<size_t> (midiTriggerCount)] =
                juce::jlimit (1, 127, static_cast<int> (message.getVelocity()));
            ++midiTriggerCount;
        }
    }

    // Finish a note from the previous block before generating new triggers.
    if (pendingNoteOff && pendingNote >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pendingNote), 0);
        pendingNoteOff = false;
        pendingNote = -1;
    }

    if (bypassed)
    {
        // Reset the edge detector while bypassed so re-enabling the plugin
        // always starts from a clean trigger state instead of inheriting a
        // stale "already above threshold" state.
        wasAboveThreshold = false;
        samplesSinceLastTrigger = retriggerSamples + 1;
        triggerMeter.store (0.0f, std::memory_order_relaxed);
        return;
    }

    // Keep atomics out of the per-sample loop. This materially reduces audio-thread
    // contention on high buffer/sample-rate sessions while preserving sample accuracy.
    float blockPeak = 0.0f;
    int midiTriggerIndex = 0;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float peak = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = juce::jmax (peak, std::abs (buffer.getSample (channel, sample)));

        blockPeak = juce::jmax (blockPeak, peak);
        waveformAccumPeak = juce::jmax (waveformAccumPeak, peak);

        // Compare in linear amplitude rather than converting every sample through log10.
        // This is equivalent to the displayed dB threshold and much cheaper on the audio thread.
        const bool above = peak >= thresholdLinear;
        const bool audioTrigger = above && ! wasAboveThreshold;
        bool midiTrigger = false;
        int midiVelocity = 127;

        if (manualTrigger && sample == 0)
            midiTrigger = true;

        while (midiTriggerIndex < midiTriggerCount
               && midiTriggerPositions[static_cast<size_t> (midiTriggerIndex)] < sample)
            ++midiTriggerIndex;

        if (midiTriggerIndex < midiTriggerCount
            && midiTriggerPositions[static_cast<size_t> (midiTriggerIndex)] == sample)
        {
            midiTrigger = true;
            waveformAccumMidiInput = true;
            midiVelocity = midiTriggerVelocities[static_cast<size_t> (midiTriggerIndex)];
            ++midiTriggerIndex;
        }

        if ((audioTrigger || midiTrigger) && samplesSinceLastTrigger >= retriggerSamples)
        {
            const int triggerVelocity = midiTrigger ? midiVelocity : 127;

            midi.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (triggerVelocity)),
                           sample);

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
            if (audioTrigger)
            {
                // Keep the trigger attached to the transient's waveform bin.
                // Also retain the next bin so very short isolated transients
                // have a visible red waveform segment at the display zoom.
                waveformAccumTriggered = true;
            }
        }

        wasAboveThreshold = above;
        ++samplesSinceLastTrigger;

        ++waveformAccumSamples;
        if (waveformAccumSamples >= waveformSampleStride)
        {
            const auto writeSerial = waveformWriteSerial.load (std::memory_order_relaxed);
            const size_t writeSlot = static_cast<size_t> (writeSerial % waveformPointCount);
            waveformBuffer[writeSlot].store (waveformAccumPeak, std::memory_order_relaxed);
            waveformTriggered[writeSlot].store (waveformAccumTriggered, std::memory_order_relaxed);
            waveformMidiInput[writeSlot].store (waveformAccumMidiInput, std::memory_order_relaxed);
            if (waveformAccumTriggered)
                latestTriggerSerial.store (writeSerial, std::memory_order_release);

            waveformWriteIndex.store ((writeSlot + 1) % waveformPointCount, std::memory_order_release);
            waveformWriteSerial.store (writeSerial + 1, std::memory_order_release);
            waveformAccumPeak = 0.0f;
            waveformAccumSamples = 0;
            waveformAccumTriggered = false;
            waveformAccumMidiInput = false;
        }
    }

    samplesSinceLastTrigger = juce::jmin (100000000, samplesSinceLastTrigger);
    inputLevel.store (juce::jmax (inputLevel.load (std::memory_order_relaxed) * 0.80f, blockPeak),
                      std::memory_order_relaxed);
    triggerMeter.store (triggerMeter.load (std::memory_order_relaxed) * 0.88f,
                        std::memory_order_relaxed);
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
