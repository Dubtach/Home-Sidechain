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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> ("BYPASS", "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "MODE", "Trigger Mode", juce::StringArray { "Smart", "Audio", "MIDI", "Both" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "THRESHOLD", "Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.01f), -18.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "SENSITIVITY", "Sensitivity", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RETRIGGER", "Retrigger", juce::NormalisableRange<float> (1.0f, 1000.0f, 1.0f, 0.4f), 80.0f, "ms"));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        "MIDI_NOTE", "MIDI Note", 0, 127, 36));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "VELOCITY", "Velocity", juce::NormalisableRange<float> (1.0f, 127.0f, 1.0f), 127.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "LINK", "Link", homeSidechain::linkNames(), 0));

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

int HomeSidechainTriggerAudioProcessor::getMidiNote() const noexcept
{
    return static_cast<int> (apvts.getRawParameterValue ("MIDI_NOTE")->load());
}

int64_t HomeSidechainTriggerAudioProcessor::getBlockStartSample (int numSamples) noexcept
{
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto timeSamples = position->getTimeInSamples())
            {
                localFallbackSample = *timeSamples + numSamples;
                return *timeSamples;
            }

    const auto start = localFallbackSample;
    localFallbackSample += numSamples;
    return start;
}

void HomeSidechainTriggerAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    wasAboveThreshold = false;
    samplesSinceLastTrigger = 100000000;
    pendingNoteOff = false;
    pendingNote = -1;
    localFallbackSample = 0;
    triggerMeter.store (0.0f);
    midiMeter.store (0.0f);
    homeLinkSender.start();
    juce::ignoreUnused (samplesPerBlock);
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

void HomeSidechainTriggerAudioProcessor::emitTrigger (int sampleOffset, int velocity, uint8_t source,
                                                      juce::MidiBuffer& midi, int note, int numSamples,
                                                      int64_t blockStartSample)
{
    const auto safeSample = juce::jlimit (0, juce::jmax (0, numSamples - 1), sampleOffset);
    const auto selectedLink = getLink();
    const auto safeVelocity = juce::jlimit (1, 127, velocity);

    midi.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (safeVelocity)), safeSample);

    if (safeSample + 1 < numSamples)
        midi.addEvent (juce::MidiMessage::noteOff (1, note), safeSample + 1);
    else
    {
        pendingNoteOff = true;
        pendingNote = note;
    }

    homeLinkSender.setLink (selectedLink);
    homeLinkSender.publishTrigger (selectedLink, safeVelocity,
                                   blockStartSample + safeSample, source);

    triggerCount.fetch_add (1, std::memory_order_relaxed);
    homeLinkCount.fetch_add (1, std::memory_order_relaxed);
    triggerMeter.store (1.0f, std::memory_order_relaxed);

    if (source == 1) audioTriggerCount.fetch_add (1, std::memory_order_relaxed);
    if (source == 2) midiTriggerCount.fetch_add (1, std::memory_order_relaxed);
}

void HomeSidechainTriggerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const auto blockStartSample = getBlockStartSample (numSamples);
    const auto link = getLink();
    homeLinkSender.setLink (link);
    homeLinkSender.heartbeat (link);

    const auto nowMs = juce::Time::getMillisecondCounter();
    if (nowMs - lastHeartbeatMs > 100)
        lastHeartbeatMs = nowMs;

    samplesSinceLastTrigger = juce::jmin (100000000, samplesSinceLastTrigger + numSamples);
    triggerMeter.store (triggerMeter.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    midiMeter.store (midiMeter.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);

    const bool bypassed = apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const int mode = static_cast<int> (apvts.getRawParameterValue ("MODE")->load());
    const float threshold = getThresholdDb();
    const float sensitivity = apvts.getRawParameterValue ("SENSITIVITY")->load();
    const float velocityParam = apvts.getRawParameterValue ("VELOCITY")->load();
    const int note = getMidiNote();
    const int retriggerSamples = juce::jmax (1, static_cast<int> (
        apvts.getRawParameterValue ("RETRIGGER")->load() * 0.001 * sampleRate));

    if (pendingNoteOff && pendingNote >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pendingNote), 0);
        pendingNoteOff = false;
        pendingNote = -1;
    }

    // Consume external MIDI. In Smart mode, a matching incoming note wins over
    // audio detection for this block so a kick carrying MIDI does not double-fire.
    bool sawMatchingMidi = false;
    std::array<int, 128> midiTriggerPositions {};
    int midiTriggerPositionCount = 0;

    if (! bypassed && (mode == 0 || mode == 2 || mode == 3))
    {
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (! message.isNoteOn())
                continue;

            lastInputMidiNote.store (message.getNoteNumber(), std::memory_order_relaxed);
            lastInputMidiChannel.store (message.getChannel(), std::memory_order_relaxed);
            midiMeter.store (1.0f, std::memory_order_relaxed);

            if (message.getNoteNumber() == note)
            {
                sawMatchingMidi = true;
                if (midiTriggerPositionCount < static_cast<int> (midiTriggerPositions.size()))
                    midiTriggerPositions[static_cast<size_t> (midiTriggerPositionCount++)] = juce::jlimit (0, numSamples - 1, metadata.samplePosition);
            }
        }
    }

    const bool allowAudio = ! bypassed && (mode == 0 || mode == 1 || mode == 3) && ! sawMatchingMidi;
    const bool allowMidi = ! bypassed && (mode == 0 || mode == 2 || mode == 3);
    const int manualRequests = manualTriggerRequests.exchange (0, std::memory_order_acq_rel);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (! bypassed && allowMidi)
        {
            for (int midiIndex = 0; midiIndex < midiTriggerPositionCount; ++midiIndex)
            {
                if (midiTriggerPositions[static_cast<size_t> (midiIndex)] == sample)
                {
                    emitTrigger (sample, juce::roundToInt (velocityParam), 2, midi, note, numSamples, blockStartSample);
                    midiTriggerPositions[static_cast<size_t> (midiIndex)] = -1;
                    samplesSinceLastTrigger = 0;
                }
            }
        }

        if (manualRequests > 0 && sample == 0 && ! bypassed)
        {
            for (int i = 0; i < manualRequests; ++i)
                emitTrigger (0, juce::roundToInt (velocityParam), 3, midi, note, numSamples, blockStartSample);
            samplesSinceLastTrigger = 0;
        }

        if (! allowAudio)
            continue;

        float peak = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = juce::jmax (peak, std::abs (buffer.getSample (channel, sample)));

        const auto peakDb = homeSidechain::linearToDb (peak);
        const auto dynamicThreshold = threshold - sensitivity * 12.0f;
        const bool above = peakDb >= dynamicThreshold;

        if (above && ! wasAboveThreshold && samplesSinceLastTrigger >= retriggerSamples)
        {
            emitTrigger (sample, juce::roundToInt (velocityParam), 1, midi, note, numSamples, blockStartSample);
            samplesSinceLastTrigger = 0;
        }

        wasAboveThreshold = above;
        if (! above)
            wasAboveThreshold = false;
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
