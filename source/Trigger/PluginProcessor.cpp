#include "PluginProcessor.h"
#include "PluginEditor.h"

HomeSidechainTriggerAudioProcessor::HomeSidechainTriggerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("HomeSidechain Trigger Link"),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    startThread();
}

HomeSidechainTriggerAudioProcessor::~HomeSidechainTriggerAudioProcessor()
{
    signalThreadShouldExit();
    stopThread (1000);
}

juce::AudioProcessorValueTreeState::ParameterLayout HomeSidechainTriggerAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterBool> ("BYPASS", "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "THRESHOLD", "Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.01f), -18.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "SENSITIVITY", "Sensitivity", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RETRIGGER", "Retrigger", juce::NormalisableRange<float> (5.0f, 1000.0f, 1.0f, 0.4f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "VELOCITY", "Velocity", 1.0f, 127.0f, 127.0f));
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

void HomeSidechainTriggerAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sampleRate = newSampleRate;
    wasAboveThreshold = false;
    samplesSinceLastTrigger = 100000000;
    pendingNoteOff = false;
    pendingNote = -1;
    triggerMeter.store (0.0f);
}

void HomeSidechainTriggerAudioProcessor::releaseResources() {}

bool HomeSidechainTriggerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn = layouts.getMainInputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo()) return false;
    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo()) return false;
    return mainOut == mainIn;
}

void HomeSidechainTriggerAudioProcessor::enqueueHomeTrigger (int link, float velocity, int sampleOffset)
{
    int start = 0, size = 0;
    if (! outgoingFifo.prepareToWrite (1, start, size) || size == 0)
        return;

    homeSidechain::LinkEvent event;
    event.link = link;
    event.velocity = velocity;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto samplePos = position->getTimeInSamples())
            {
                event.samplePosition = static_cast<uint64_t> (juce::jmax<int64_t> (0, *samplePos + sampleOffset));
                event.hasHostPosition = true;
            }
            if (auto seconds = position->getTimeInSeconds())
                event.timeSeconds = *seconds + (static_cast<double> (sampleOffset) / sampleRate);
        }
    }

    outgoingQueue[static_cast<size_t> (start)].event = event;
    outgoingFifo.finishedWrite (1);
}

void HomeSidechainTriggerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;

    samplesSinceLastTrigger = juce::jmin (100000000, samplesSinceLastTrigger + numSamples);
    triggerMeter.store (triggerMeter.load() * 0.94f, std::memory_order_relaxed);

    const bool bypassed = apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const float threshold = getThresholdDb();
    const float sensitivity = apvts.getRawParameterValue ("SENSITIVITY")->load();
    const float velocityParam = apvts.getRawParameterValue ("VELOCITY")->load();
    const int link = getLink();
    const int note = homeSidechain::midiNoteForLink (link);
    const int retriggerSamples = static_cast<int> (apvts.getRawParameterValue ("RETRIGGER")->load() * 0.001 * sampleRate);

    if (pendingNoteOff && pendingNote >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pendingNote), 0);
        pendingNoteOff = false;
        pendingNote = -1;
    }

    if (bypassed) return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float peak = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = juce::jmax (peak, std::abs (buffer.getSample (channel, sample)));

        const float peakDb = homeSidechain::linearToDb (peak);
        const bool above = peakDb >= (threshold - sensitivity * 12.0f);

        if (above && ! wasAboveThreshold && samplesSinceLastTrigger >= retriggerSamples)
        {
            const int velocity = juce::jlimit (1, 127, juce::roundToInt (velocityParam));
            midi.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (velocity)), sample);
            if (sample + 1 < numSamples)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), sample + 1);
            else
            {
                pendingNoteOff = true;
                pendingNote = note;
            }

            enqueueHomeTrigger (link, velocity / 127.0f, sample);
            samplesSinceLastTrigger = 0;
            triggerMeter.store (1.0f, std::memory_order_relaxed);
            triggerCount.fetch_add (1, std::memory_order_relaxed);
        }

        wasAboveThreshold = above;
    }
}

void HomeSidechainTriggerAudioProcessor::threadRun()
{
    lastHeartbeatMs = 0;

    while (! threadShouldExit())
    {
        const auto now = juce::Time::currentTimeMillis();
        const int link = getLink();

        if (now - lastHeartbeatMs >= 250)
        {
            if (linkBus.updateHeartbeat (link))
                lastHeartbeatMs = now;
        }

        int start = 0, size = 0;
        if (outgoingFifo.prepareToRead (queueCapacity, start, size) && size > 0)
        {
            for (int i = 0; i < size; ++i)
                linkBus.send (outgoingQueue[static_cast<size_t> (start + i)].event);
            outgoingFifo.finishedRead (size);
        }

        homeLinkActive.store (linkBus.heartbeatAgeMs (link) < 1000, std::memory_order_relaxed);
        wait (2);
    }
}

void HomeSidechainTriggerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void HomeSidechainTriggerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* HomeSidechainTriggerAudioProcessor::createEditor()
{
    return new HomeSidechainTriggerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HomeSidechainTriggerAudioProcessor();
}
