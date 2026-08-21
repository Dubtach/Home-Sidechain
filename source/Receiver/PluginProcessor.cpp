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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> ("BYPASS", "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("MODE", "Mode",
        juce::StringArray { "Duck", "Pump", "Gate", "Shape" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("LINK", "Link", homeSidechain::linkNames(), 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("SYNC", "Sync", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("BARS", "Bars",
        juce::StringArray { "1/4", "1/2", "1", "2", "4" }, 2));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("DEPTH", "Depth", 0.0f, 48.0f, 12.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ATTACK", "Attack", juce::NormalisableRange<float> (0.1f, 250.0f, 0.1f, 0.35f), 2.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "HOLD", "Hold", juce::NormalisableRange<float> (0.0f, 1000.0f, 0.1f, 0.4f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RELEASE", "Release", juce::NormalisableRange<float> (5.0f, 2000.0f, 0.1f, 0.4f), 180.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("CURVE", "Curve", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("MIX", "Mix", 0.0f, 1.0f, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SHAPE_1", "Shape 1", 0.0f, 1.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SHAPE_2", "Shape 2", 0.0f, 1.0f, 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SHAPE_3", "Shape 3", 0.0f, 1.0f, 0.05f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SHAPE_4", "Shape 4", 0.0f, 1.0f, 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SHAPE_5", "Shape 5", 0.0f, 1.0f, 0.75f));

    return { params.begin(), params.end() };
}

void HomeSidechainReceiverAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    envelopePhase = 0.0f;
    envelopeActive = false;
    remainingSamples = 0;
    gainSmoother.reset (sampleRate, 0.01);
    gainSmoother.setCurrentAndTargetValue (1.0f);
    juce::ignoreUnused (samplesPerBlock);
}

void HomeSidechainReceiverAudioProcessor::releaseResources()
{
}

bool HomeSidechainReceiverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == mainIn;
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
    if (apvts.getRawParameterValue ("SYNC")->load() < 0.5f)
    {
        const auto attack = apvts.getRawParameterValue ("ATTACK")->load();
        const auto hold = apvts.getRawParameterValue ("HOLD")->load();
        const auto release = apvts.getRawParameterValue ("RELEASE")->load();
        return juce::jmax (1.0, (attack + hold + release) * 0.001 * sampleRate);
    }

    static constexpr double barValues[] = { 0.25, 0.5, 1.0, 2.0, 4.0 };
    const auto index = juce::jlimit (0, 4, static_cast<int> (apvts.getRawParameterValue ("BARS")->load()));
    const auto seconds = (60.0 / getHostBpm()) * 4.0 * barValues[index];
    return juce::jmax (1.0, seconds * sampleRate);
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
    const auto mode = static_cast<int> (apvts.getRawParameterValue ("MODE")->load());

    if (mode == 1) // Pump: make the dip more pronounced.
        shape = std::pow (juce::jlimit (0.0f, 1.0f, shape), 1.7f);
    else if (mode == 2) // Gate: squash the upper part of the curve into a sharper gate.
        shape = shape > 0.45f ? 1.0f : shape * 0.15f;

    return juce::Decibels::decibelsToGain (-depth * juce::jlimit (0.0f, 1.0f, shape));
}

float HomeSidechainReceiverAudioProcessor::getShapePoint (int index) const noexcept
{
    const auto id = "SHAPE_" + juce::String (juce::jlimit (1, 5, index + 1));
    return apvts.getRawParameterValue (id)->load();
}

void HomeSidechainReceiverAudioProcessor::setShapePoint (int index, float value)
{
    if (auto* p = apvts.getParameter ("SHAPE_" + juce::String (juce::jlimit (1, 5, index + 1))))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::triggerEnvelope()
{
    envelopeActive = true;
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

    const int targetNote = homeSidechain::midiNoteForLink (
        static_cast<int> (apvts.getRawParameterValue ("LINK")->load()));
    const bool bypassed = apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
    const float mix = juce::jlimit (0.0f, 1.0f,
                                    apvts.getRawParameterValue ("MIX")->load());

    triggerActivity.store (triggerActivity.load (std::memory_order_relaxed) * 0.94f,
                           std::memory_order_relaxed);
    midiActivity.store (midiActivity.load (std::memory_order_relaxed) * 0.94f,
                        std::memory_order_relaxed);

    // Keep this allocation-free on the real-time audio thread. In normal DAW use
    // there will be only a handful of MIDI events per block, so a small fixed
    // stack buffer is sufficient. Extra events are still counted/diagnosed but
    // matching triggers beyond this capacity are ignored for this block.
    std::array<int, 64> triggerPositions {};
    int triggerPositionCount = 0;
    int incomingMidiEvents = 0;
    int mostRecentNote = -1;
    int mostRecentChannel = 0;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        ++incomingMidiEvents;

        if (message.getChannel() > 0)
            mostRecentChannel = message.getChannel();

        if (message.isNoteOn())
        {
            mostRecentNote = message.getNoteNumber();

            if (message.getNoteNumber() == targetNote && triggerPositionCount < static_cast<int> (triggerPositions.size()))
                triggerPositions[static_cast<size_t> (triggerPositionCount++)]
                    = juce::jlimit (0, samples - 1, metadata.samplePosition);
        }
    }

    if (incomingMidiEvents > 0)
    {
        midiActivity.store (1.0f, std::memory_order_relaxed);
        midiEventCount.fetch_add (incomingMidiEvents, std::memory_order_relaxed);
        lastMidiNote.store (mostRecentNote, std::memory_order_relaxed);
        lastMidiChannel.store (juce::jmax (0, mostRecentChannel), std::memory_order_relaxed);
    }

    if (! bypassed)
    {
        int triggerIndex = 0;
        double totalCycle = cycleSamples();

        for (int i = 0; i < samples; ++i)
        {
            while (triggerIndex < triggerPositionCount && triggerPositions[static_cast<size_t> (triggerIndex)] == i)
            {
                triggerEnvelope();
                ++triggerIndex;
            }

            float targetGain = 1.0f;
            if (envelopeActive && remainingSamples > 0)
            {
                const float phase = 1.0f - static_cast<float> (
                    static_cast<double> (remainingSamples) / juce::jmax (1.0, totalCycle));
                targetGain = modulationGain (shapeValue (phase));

                --remainingSamples;
                if (remainingSamples <= 0)
                {
                    remainingSamples = 0;
                    envelopeActive = false;
                }
            }

            const float currentGain = juce::jmap (mix, 1.0f, targetGain);
            gainSmoother.setTargetValue (currentGain);
            const float gain = gainSmoother.getNextValue();

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample (channel, i, buffer.getSample (channel, i) * gain);
        }
    }
    else if (triggerPositionCount > 0)
    {
        // Bypass should not process audio, but the Receiver must still confirm
        // that the routed MIDI trigger actually arrived.
        triggerActivity.store (1.0f, std::memory_order_relaxed);
        triggerCount.fetch_add (triggerPositionCount, std::memory_order_relaxed);
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
