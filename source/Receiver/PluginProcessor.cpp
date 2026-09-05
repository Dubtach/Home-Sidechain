#include "PluginProcessor.h"
#include "PluginEditor.h"

HomeSidechainReceiverAudioProcessor::HomeSidechainReceiverAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HomeSidechainReceiverAudioProcessor::createParameters()
{
    using FloatAttributes = juce::AudioParameterFloatAttributes;
    using BoolAttributes = juce::AudioParameterBoolAttributes;
    using ChoiceAttributes = juce::AudioParameterChoiceAttributes;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "BYPASS", "Bypass", false, BoolAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "MODE", "Mode", juce::StringArray { "Duck", "Pump", "Gate", "Shape" }, 0, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "LINK", "Link", homeSidechain::linkNames(), 0, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "SOURCE", "Source", juce::StringArray { "Home-Link", "MIDI", "Both" }, 2, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "SYNC", "Sync", false, BoolAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "BARS", "Bars", juce::StringArray { "1/4", "1/2", "1", "2", "4" }, 2, ChoiceAttributes{}));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "DEPTH", "Depth", juce::NormalisableRange<float> (0.0f, 48.0f, 0.01f), 12.0f,
        FloatAttributes{}.withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ATTACK", "Attack", juce::NormalisableRange<float> (0.1f, 250.0f, 0.1f, 0.35f), 2.0f,
        FloatAttributes{}.withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "HOLD", "Hold", juce::NormalisableRange<float> (0.0f, 1000.0f, 0.1f, 0.4f), 0.0f,
        FloatAttributes{}.withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RELEASE", "Release", juce::NormalisableRange<float> (5.0f, 2000.0f, 0.1f, 0.4f), 180.0f,
        FloatAttributes{}.withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "CURVE", "Curve", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "MIX", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, FloatAttributes{}));

    for (int i = 1; i <= 5; ++i)
    {
        const auto id = "SHAPE_" + juce::String (i);
        const float defaults[] = { 1.0f, 0.35f, 0.0f, 0.25f, 0.85f };
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id, "Shape " + juce::String (i), juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
            defaults[i - 1], FloatAttributes{}));
    }

    return { params.begin(), params.end() };
}

void HomeSidechainReceiverAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    testTriggerRequested.store (false, std::memory_order_release);
    envelopePhase = 0.0f;
    envelopeActiveInternal = false;
    envelopeActiveForUI.store (false, std::memory_order_relaxed);
    remainingSamples = 0;
    envelopeDisplayPhase.store (0.0f, std::memory_order_relaxed);
    gainSmoother.reset (sampleRate, 0.008);
    gainSmoother.setCurrentAndTargetValue (1.0f);

    const int link = getLink();
    homeLinkLastLink = link;
    homeLinkLastSequence = homeLinkService().latestSequence (link);
    juce::ignoreUnused (samplesPerBlock);
}

void HomeSidechainReceiverAudioProcessor::releaseResources()
{
}

bool HomeSidechainReceiverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn = layouts.getMainInputChannelSet();

    if ((mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        || (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo()))
        return false;

    return mainOut == mainIn;
}

int HomeSidechainReceiverAudioProcessor::getLink() const noexcept
{
    return static_cast<int> (apvts.getRawParameterValue ("LINK")->load());
}

double HomeSidechainReceiverAudioProcessor::getHostBpm() const noexcept
{
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto bpm = position->getBpm())
                if (*bpm > 1.0)
                    return *bpm;
    return 120.0;
}

double HomeSidechainReceiverAudioProcessor::cycleSamples() const noexcept
{
    const double attack = apvts.getRawParameterValue ("ATTACK")->load();
    const double hold = apvts.getRawParameterValue ("HOLD")->load();
    const double release = apvts.getRawParameterValue ("RELEASE")->load();
    return juce::jmax (1.0, (attack + hold + release) * 0.001 * sampleRate);
}

float HomeSidechainReceiverAudioProcessor::shapeValue (float phase) const noexcept
{
    phase = juce::jlimit (0.0f, 1.0f, phase);

    const float values[5] = {
        getShapePoint (0), getShapePoint (1), getShapePoint (2), getShapePoint (3), getShapePoint (4)
    };

    const float scaled = phase * 4.0f;
    const int index = juce::jlimit (0, 3, static_cast<int> (std::floor (scaled)));
    const float t0 = scaled - static_cast<float> (index);
    const float curve = apvts.getRawParameterValue ("CURVE")->load();

    float t = t0;
    if (curve > 0.001f)
        t = 1.0f - std::pow (1.0f - t0, 1.0f + curve * 7.0f);
    else if (curve < -0.001f)
        t = std::pow (t0, 1.0f + (-curve) * 7.0f);

    return juce::jmap (t, values[index], values[index + 1]);
}

float HomeSidechainReceiverAudioProcessor::modulationGain (float shape) const noexcept
{
    const auto depth = apvts.getRawParameterValue ("DEPTH")->load();
    const float duckAmount = juce::jlimit (0.0f, 1.0f, shape);
    return juce::Decibels::decibelsToGain (-depth * duckAmount);
}

float HomeSidechainReceiverAudioProcessor::getShapePoint (int index) const noexcept
{
    const auto safeIndex = juce::jlimit (0, 4, index);
    const auto id = "SHAPE_" + juce::String (safeIndex + 1);
    return apvts.getRawParameterValue (id)->load();
}

void HomeSidechainReceiverAudioProcessor::setShapePoint (int index, float value)
{
    const auto safeIndex = juce::jlimit (0, 4, index);
    if (auto* p = apvts.getParameter ("SHAPE_" + juce::String (safeIndex + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::triggerEnvelope()
{
    envelopeActiveInternal = true;
    envelopeActiveForUI.store (true, std::memory_order_relaxed);
    envelopePhase = 0.0f;
    remainingSamples = juce::jmax (1, static_cast<int> (std::round (cycleSamples())));
    triggerActivity.store (1.0f, std::memory_order_relaxed);
    triggerCount.fetch_add (1, std::memory_order_relaxed);
}

void HomeSidechainReceiverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                        juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int samples = buffer.getNumSamples();
    if (samples <= 0)
        return;

    const int link = getLink();
    if (link != homeLinkLastLink)
    {
        homeLinkLastLink = link;
        homeLinkLastSequence = homeLinkService().latestSequence (link);
    }

    const int targetNote = homeSidechain::midiNoteForLink (link);
    const bool bypassed = apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const float mix = juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("MIX")->load());

    triggerActivity.store (triggerActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    midiActivity.store (midiActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    homeLinkActivity.store (homeLinkActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);

    const auto heartbeatAge = static_cast<uint32_t> (
        juce::Time::getMillisecondCounter() - homeLinkService().lastHeartbeatMs (link));
    const bool connected = heartbeatAge < 450u;
    homeLinkConnected.store (connected, std::memory_order_relaxed);

    std::array<int, 256> triggerPositions {};
    int triggerPositionCount = 0;

    if (testTriggerRequested.exchange (false, std::memory_order_acq_rel)
        && triggerPositionCount < static_cast<int> (triggerPositions.size()))
    {
        triggerPositions[static_cast<size_t> (triggerPositionCount++)] = 0;
        triggerActivity.store (1.0f, std::memory_order_relaxed);
    }

    auto latest = homeLinkService().latestSequence (link);
    auto next = homeLinkLastSequence + 1;
    if (latest > next && latest - next >= 256)
        next = latest - 255;

    while (next <= latest && triggerPositionCount < static_cast<int> (triggerPositions.size()))
    {
        homeSidechain::HomeLinkEvent event;
        if (homeLinkService().readEvent (link, next, event))
        {
            triggerPositions[static_cast<size_t> (triggerPositionCount++)] = 0;
            homeLinkActivity.store (1.0f, std::memory_order_relaxed);
            triggerActivity.store (1.0f, std::memory_order_relaxed);
            homeLinkTriggerCount.fetch_add (1, std::memory_order_relaxed);
            lastHomeLinkVelocity.store (static_cast<int> (event.velocity), std::memory_order_relaxed);
        }
        ++next;
    }
    if (latest > homeLinkLastSequence)
        homeLinkLastSequence = latest;

    int incomingMidiEvents = 0;
    int mostRecentNote = -1;
    int mostRecentChannel = 0;
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        ++incomingMidiEvents;
        mostRecentChannel = juce::jmax (mostRecentChannel, message.getChannel());

        if (message.isNoteOn())
        {
            mostRecentNote = message.getNoteNumber();
            if (message.getNoteNumber() == targetNote
                && triggerPositionCount < static_cast<int> (triggerPositions.size()))
            {
                triggerPositions[static_cast<size_t> (triggerPositionCount++)]
                    = juce::jlimit (0, samples - 1, metadata.samplePosition);
                midiActivity.store (1.0f, std::memory_order_relaxed);
            }
        }
    }

    if (incomingMidiEvents > 0)
    {
        midiActivity.store (1.0f, std::memory_order_relaxed);
        midiEventCount.fetch_add (incomingMidiEvents, std::memory_order_relaxed);
        lastMidiNote.store (mostRecentNote, std::memory_order_relaxed);
        lastMidiChannel.store (mostRecentChannel, std::memory_order_relaxed);
    }

    if (! bypassed)
    {
        const double totalCycle = cycleSamples();
        int triggerIndex = 0;
        float lastPhaseForDisplay = envelopeActiveInternal ? envelopeDisplayPhase.load (std::memory_order_relaxed) : 0.0f;

        for (int i = 0; i < samples; ++i)
        {
            while (triggerIndex < triggerPositionCount
                   && triggerPositions[static_cast<size_t> (triggerIndex)] == i)
            {
                triggerEnvelope();
                ++triggerIndex;
            }

            float targetGain = 1.0f;
            if (envelopeActiveInternal && remainingSamples > 0)
            {
                const float phase = 1.0f - static_cast<float> (
                    static_cast<double> (remainingSamples) / juce::jmax (1.0, totalCycle));
                lastPhaseForDisplay = phase;
                targetGain = modulationGain (shapeValue (phase));
                --remainingSamples;
                if (remainingSamples <= 0)
                {
                    remainingSamples = 0;
                    envelopeActiveInternal = false;
                    envelopeActiveForUI.store (false, std::memory_order_relaxed);
                }
            }

            const float currentGain = juce::jmap (mix, 1.0f, targetGain);
            gainSmoother.setTargetValue (currentGain);
            const float gain = gainSmoother.getNextValue();

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample (channel, i, buffer.getSample (channel, i) * gain);
        }

        envelopeDisplayPhase.store (envelopeActiveInternal ? juce::jlimit (0.0f, 1.0f, lastPhaseForDisplay) : 0.0f,
                                     std::memory_order_relaxed);
    }
    else
    {
        envelopeDisplayPhase.store (0.0f, std::memory_order_relaxed);
    }
}

void HomeSidechainReceiverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HomeSidechainReceiverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* HomeSidechainReceiverAudioProcessor::createEditor()
{
    return new HomeSidechainReceiverAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HomeSidechainReceiverAudioProcessor();
}
