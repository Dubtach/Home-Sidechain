#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
    constexpr int legacyShapeCount = 5;
    constexpr float defaultLegacyShape[legacyShapeCount] = { 1.0f, 0.35f, 0.0f, 0.25f, 0.85f };
    constexpr float defaultNodeX[HomeSidechainReceiverAudioProcessor::maxNodes] =
        { 0.0f, 0.12f, 0.28f, 0.50f, 0.68f, 0.84f, 1.0f, 1.0f };
    constexpr float defaultNodeY[HomeSidechainReceiverAudioProcessor::maxNodes] =
        { 1.0f, 0.18f, 0.06f, 0.12f, 0.30f, 0.62f, 1.0f, 1.0f };
    constexpr bool defaultNodeActive[HomeSidechainReceiverAudioProcessor::maxNodes] =
        { true, true, true, true, true, true, true, false };

    const float presetY[12][HomeSidechainReceiverAudioProcessor::maxNodes] =
    {
        { 1.0f, 0.18f, 0.06f, 0.12f, 0.30f, 0.62f, 1.0f, 1.0f },
        { 1.0f, 0.03f, 0.00f, 0.02f, 0.05f, 0.12f, 1.0f, 1.0f },
        { 1.0f, 0.55f, 0.16f, 0.10f, 0.24f, 0.48f, 1.0f, 1.0f },
        { 1.0f, 0.72f, 0.25f, 0.08f, 0.10f, 0.22f, 1.0f, 1.0f },
        { 1.0f, 0.25f, 0.12f, 0.50f, 0.70f, 0.84f, 1.0f, 1.0f },
        { 1.0f, 0.65f, 0.04f, 0.04f, 0.04f, 0.08f, 1.0f, 1.0f },
        { 1.0f, 0.20f, 0.55f, 0.25f, 0.10f, 0.20f, 1.0f, 1.0f },
        { 1.0f, 0.10f, 0.10f, 0.18f, 0.55f, 0.84f, 1.0f, 1.0f },
        { 1.0f, 0.90f, 0.42f, 0.12f, 0.08f, 0.12f, 1.0f, 1.0f },
        { 1.0f, 0.12f, 0.78f, 0.12f, 0.05f, 0.10f, 1.0f, 1.0f },
        { 1.0f, 0.38f, 0.72f, 0.28f, 0.12f, 0.42f, 1.0f, 1.0f },
        { 1.0f, 0.08f, 0.08f, 0.78f, 0.80f, 0.86f, 1.0f, 1.0f }
    };
}

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
        "SYNC", "Sync", true, BoolAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "BARS", "Bars", juce::StringArray { "1/4", "1/2", "1", "2", "4" }, 2, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "RATE", "Rate", juce::StringArray { "1/8", "1/4", "1/2", "1/1" }, 2, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "LENGTH", "Length", juce::NormalisableRange<float> (100.0f, 4000.0f, 0.1f, 0.4f), 1000.0f,
        FloatAttributes{}.withLabel ("ms")));

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
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "BAND", "Band", juce::StringArray { "Full", "Low", "High" }, 0, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "CROSSOVER", "Crossover", juce::NormalisableRange<float> (50.0f, 800.0f, 1.0f, 0.35f), 150.0f,
        FloatAttributes{}.withLabel ("Hz")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "LOW_CUT", "Low Cut", juce::NormalisableRange<float> (20.0f, 4000.0f, 0.1f, 0.35f), 20.0f,
        FloatAttributes{}.withLabel ("Hz")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "HIGH_CUT", "High Cut", juce::NormalisableRange<float> (1000.0f, 20000.0f, 0.1f, 0.35f), 20000.0f,
        FloatAttributes{}.withLabel ("Hz")));

    for (int i = 1; i <= maxNodes; ++i)
    {
        const auto suffix = juce::String (i);
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "NODE_X_" + suffix, "Node X " + suffix,
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), defaultNodeX[i - 1], FloatAttributes{}));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "NODE_Y_" + suffix, "Node Y " + suffix,
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), defaultNodeY[i - 1], FloatAttributes{}));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            "NODE_ACTIVE_" + suffix, "Node Active " + suffix, defaultNodeActive[i - 1], BoolAttributes{}));
        if (i < maxNodes)
        {
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                "HANDLE_" + suffix, "Handle " + suffix,
                juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, FloatAttributes{}));
        }
        else
        {
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                "HANDLE_" + suffix, "Handle " + suffix,
                juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, FloatAttributes{}));
        }
    }

    // Legacy parameters SHAPE_1..5 remain available for old sessions that
    // reference them. They are not used by the new graph unless migrated.
    for (int i = 1; i <= legacyShapeCount; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "SHAPE_" + juce::String (i), "Legacy Shape " + juce::String (i),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), defaultLegacyShape[i - 1], FloatAttributes{}));
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
    lowCutState = { 0.0f, 0.0f };
    highCutState = { 0.0f, 0.0f };
    crossoverState = { 0.0f, 0.0f };
    refreshShapeCache();

    const int link = getLink();
    homeLinkLastLink = link;
    homeLinkLastSequence = homeLinkService().latestSequence (link);
    juce::ignoreUnused (samplesPerBlock);
}

void HomeSidechainReceiverAudioProcessor::releaseResources() {}

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
    return juce::jlimit (0, 7, static_cast<int> (apvts.getRawParameterValue ("LINK")->load()));
}

double HomeSidechainReceiverAudioProcessor::getHostBpm() const noexcept
{
    if (auto* playHead = getPlayHead())
        if (auto pos = playHead->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 1.0)
                    return *bpm;
    return 120.0;
}

double HomeSidechainReceiverAudioProcessor::cycleSamples() const noexcept
{
    const bool sync = apvts.getRawParameterValue ("SYNC")->load() > 0.5f;
    if (sync)
    {
        static constexpr double beats[] = { 0.5, 1.0, 2.0, 4.0 };
        const int rate = juce::jlimit<int> (0, 3, static_cast<int> (apvts.getRawParameterValue ("RATE")->load()));
        const double bpm = juce::jmax<double> (1.0, getHostBpm());
        return juce::jmax<double> (1.0, beats[rate] * 60.0 / bpm * sampleRate);
    }
    const double lengthMs = apvts.getRawParameterValue ("LENGTH")->load();
    return juce::jmax<double> (1.0, lengthMs * 0.001 * sampleRate);
}

float HomeSidechainReceiverAudioProcessor::getMix() const noexcept
{
    return juce::jlimit<float> (0.0f, 1.0f, apvts.getRawParameterValue ("MIX")->load());
}

float HomeSidechainReceiverAudioProcessor::getDepth() const noexcept
{
    return juce::jmax<float> (0.0f, apvts.getRawParameterValue ("DEPTH")->load());
}

bool HomeSidechainReceiverAudioProcessor::isNodeActive (int index) const noexcept
{
    const int i = juce::jlimit<int> (0, maxNodes - 1, index);
    return apvts.getRawParameterValue ("NODE_ACTIVE_" + juce::String (i + 1))->load() > 0.5f;
}

float HomeSidechainReceiverAudioProcessor::getNodeX (int index) const noexcept
{
    const int i = juce::jlimit<int> (0, maxNodes - 1, index);
    return juce::jlimit<float> (0.0f, 1.0f, apvts.getRawParameterValue ("NODE_X_" + juce::String (i + 1))->load());
}

float HomeSidechainReceiverAudioProcessor::getNodeY (int index) const noexcept
{
    const int i = juce::jlimit<int> (0, maxNodes - 1, index);
    return juce::jlimit<float> (0.0f, 1.0f, apvts.getRawParameterValue ("NODE_Y_" + juce::String (i + 1))->load());
}

float HomeSidechainReceiverAudioProcessor::getHandle (int segment) const noexcept
{
    const int i = juce::jlimit<int> (0, maxNodes - 2, segment);
    return juce::jlimit<float> (-1.0f, 1.0f, apvts.getRawParameterValue ("HANDLE_" + juce::String (i + 1))->load());
}

void HomeSidechainReceiverAudioProcessor::setNodeX (int index, float value)
{
    const int i = juce::jlimit<int> (0, maxNodes - 1, index);
    if (auto* p = apvts.getParameter ("NODE_X_" + juce::String (i + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit<float> (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::setNodeY (int index, float value)
{
    const int i = juce::jlimit<int> (0, maxNodes - 1, index);
    if (auto* p = apvts.getParameter ("NODE_Y_" + juce::String (i + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit<float> (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::setHandle (int segment, float value)
{
    const int i = juce::jlimit<int> (0, maxNodes - 2, segment);
    if (auto* p = apvts.getParameter ("HANDLE_" + juce::String (i + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit<float> (-1.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::setNodeActive (int index, bool active)
{
    const int i = juce::jlimit<int> (0, maxNodes - 1, index);
    if (auto* p = apvts.getParameter ("NODE_ACTIVE_" + juce::String (i + 1)))
        p->setValueNotifyingHost (active ? 1.0f : 0.0f);
}

int HomeSidechainReceiverAudioProcessor::activeNodeCount() const noexcept
{
    int count = 0;
    for (int i = 0; i < maxNodes; ++i)
        if (isNodeActive (i))
            ++count;
    return count;
}

void HomeSidechainReceiverAudioProcessor::refreshShapeCache() noexcept
{
    struct Node { float x, y; int index; };
    std::array<Node, maxNodes> nodes {};
    int count = 0;
    for (int i = 0; i < maxNodes; ++i)
        if (isNodeActive (i))
            nodes[static_cast<size_t> (count++)] = { getNodeX (i), getNodeY (i), i };

    std::sort (nodes.begin(), nodes.begin() + count, [] (const Node& a, const Node& b) { return a.x < b.x; });
    cachedNodeCount = count;
    for (int i = 0; i < count; ++i)
    {
        cachedNodeX[static_cast<size_t> (i)] = nodes[static_cast<size_t> (i)].x;
        cachedNodeY[static_cast<size_t> (i)] = nodes[static_cast<size_t> (i)].y;
    }
    for (int i = 0; i < maxNodes - 1; ++i)
        cachedHandle[static_cast<size_t> (i)] = getHandle (i);
}

float HomeSidechainReceiverAudioProcessor::shapeValueCached (float phase) const noexcept
{
    phase = juce::jlimit<float> (0.0f, 1.0f, phase);
    if (cachedNodeCount < 2)
        return phase;
    if (phase <= cachedNodeX[0]) return cachedNodeY[0];
    if (phase >= cachedNodeX[static_cast<size_t> (cachedNodeCount - 1)])
        return cachedNodeY[static_cast<size_t> (cachedNodeCount - 1)];

    int segment = 0;
    for (int i = 0; i < cachedNodeCount - 1; ++i)
        if (phase >= cachedNodeX[static_cast<size_t> (i)] && phase <= cachedNodeX[static_cast<size_t> (i + 1)])
        {
            segment = i;
            break;
        }

    const float dx = juce::jmax<float> (0.0001f, cachedNodeX[static_cast<size_t> (segment + 1)] - cachedNodeX[static_cast<size_t> (segment)]);
    float t = juce::jlimit<float> (0.0f, 1.0f, (phase - cachedNodeX[static_cast<size_t> (segment)]) / dx);
    const float handle = cachedHandle[static_cast<size_t> (segment)];
    const float exponent = std::pow (2.5f, std::abs (handle));
    if (handle > 0.001f)
        t = std::pow (t, 1.0f / exponent);
    else if (handle < -0.001f)
        t = 1.0f - std::pow (1.0f - t, 1.0f / exponent);

    return juce::jmap (t, cachedNodeY[static_cast<size_t> (segment)], cachedNodeY[static_cast<size_t> (segment + 1)]);
}

float HomeSidechainReceiverAudioProcessor::shapeValue (float phase) const noexcept
{
    phase = juce::jlimit<float> (0.0f, 1.0f, phase);

    struct Node { float x, y; int index; };
    std::array<Node, maxNodes> nodes {};
    int count = 0;
    for (int i = 0; i < maxNodes; ++i)
        if (isNodeActive (i))
            nodes[static_cast<size_t> (count++)] = { getNodeX (i), getNodeY (i), i };

    if (count < 2)
        return phase;

    std::sort (nodes.begin(), nodes.begin() + count, [](const Node& a, const Node& b) { return a.x < b.x; });

    if (phase <= nodes[0].x) return nodes[0].y;
    if (phase >= nodes[count - 1].x) return nodes[count - 1].y;

    int segment = 0;
    for (int i = 0; i < count - 1; ++i)
    {
        if (phase >= nodes[i].x && phase <= nodes[i + 1].x)
        {
            segment = i;
            break;
        }
    }

    const float dx = juce::jmax<float> (0.0001f, nodes[segment + 1].x - nodes[segment].x);
    float t = juce::jlimit<float> (0.0f, 1.0f, (phase - nodes[segment].x) / dx);
    const float handle = getHandle (segment);

    // Handle is a normalized bend amount. Positive bends toward the next
    // point; negative bends toward the previous point.
    const float exponent = std::pow (2.5f, std::abs (handle));
    if (handle > 0.001f)
        t = std::pow (t, 1.0f / exponent);
    else if (handle < -0.001f)
        t = 1.0f - std::pow (1.0f - t, 1.0f / exponent);

    return juce::jmap (t, nodes[segment].y, nodes[segment + 1].y);
}

float HomeSidechainReceiverAudioProcessor::modulationGain (float shape) const noexcept
{
    const float depth = getDepth();
    const float duckAmount = juce::jlimit<float> (0.0f, 1.0f, shape);
    return juce::Decibels::decibelsToGain (-depth * duckAmount);
}

void HomeSidechainReceiverAudioProcessor::triggerEnvelope()
{
    envelopeActiveInternal = true;
    envelopeActiveForUI.store (true, std::memory_order_relaxed);
    envelopePhase = 0.0f;
    remainingSamples = juce::jmax<int> (1, static_cast<int> (std::lround (cycleSamples())));
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

    triggerActivity.store (triggerActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    midiActivity.store (midiActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    homeLinkActivity.store (homeLinkActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);

    const auto heartbeatAge = static_cast<uint32_t> (
        juce::Time::getMillisecondCounter() - homeLinkService().lastHeartbeatMs (link));
    homeLinkConnected.store (heartbeatAge < 450u, std::memory_order_relaxed);

    std::array<int, 256> triggerPositions {};
    int triggerPositionCount = 0;

    if (testTriggerRequested.exchange (false, std::memory_order_acq_rel))
        triggerPositions[static_cast<size_t> (triggerPositionCount++)] = 0;

    const auto latest = homeLinkService().latestSequence (link);
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
        mostRecentChannel = juce::jmax<int> (mostRecentChannel, message.getChannel());
        if (message.isNoteOn())
        {
            mostRecentNote = message.getNoteNumber();
            if (message.getNoteNumber() == targetNote
                && triggerPositionCount < static_cast<int> (triggerPositions.size()))
            {
                triggerPositions[static_cast<size_t> (triggerPositionCount++)] =
                    juce::jlimit<int> (0, samples - 1, metadata.samplePosition);
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

    if (bypassed)
    {
        envelopeActiveInternal = false;
        remainingSamples = 0;
        envelopeActiveForUI.store (false, std::memory_order_relaxed);
        envelopeDisplayPhase.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const double totalCycle = cycleSamples();
    refreshShapeCache();
    const float mix = getMix();
    const int bandMode = juce::jlimit<int> (0, 2, static_cast<int> (apvts.getRawParameterValue ("BAND")->load()));
    const float lowCutHz = juce::jlimit<float> (20.0f, 4000.0f, apvts.getRawParameterValue ("LOW_CUT")->load());
    const float highCutHz = juce::jlimit<float> (1000.0f, 20000.0f, apvts.getRawParameterValue ("HIGH_CUT")->load());
    const float crossover = juce::jlimit<float> (50.0f, 800.0f, apvts.getRawParameterValue ("CROSSOVER")->load());

    const float safeLowCut = juce::jmin<float> (lowCutHz, juce::jmax<float> (20.0f, highCutHz * 0.98f));
    const float safeHighCut = juce::jmax<float> (juce::jmin<float> (20000.0f, highCutHz), safeLowCut * 1.02f);
    const float lowAlpha = std::exp (-juce::MathConstants<float>::twoPi * safeLowCut / static_cast<float> (juce::jmax<double> (1.0, sampleRate)));
    const float highAlpha = std::exp (-juce::MathConstants<float>::twoPi * safeHighCut / static_cast<float> (juce::jmax<double> (1.0, sampleRate)));
    const float splitAlpha = std::exp (-juce::MathConstants<float>::twoPi * crossover / static_cast<float> (juce::jmax<double> (1.0, sampleRate)));

    std::sort (triggerPositions.begin(), triggerPositions.begin() + triggerPositionCount);
    int triggerIndex = 0;
    float lastPhaseForDisplay = envelopeDisplayPhase.load (std::memory_order_relaxed);

    for (int i = 0; i < samples; ++i)
    {
        while (triggerIndex < triggerPositionCount && triggerPositions[static_cast<size_t> (triggerIndex)] == i)
        {
            triggerEnvelope();
            ++triggerIndex;
        }

        float targetGain = 1.0f;
        if (envelopeActiveInternal && remainingSamples > 0)
        {
            const float phase = 1.0f - static_cast<float> (
                static_cast<double> (remainingSamples) / juce::jmax<double> (1.0, totalCycle));
            envelopePhase = juce::jlimit<float> (0.0f, 1.0f, phase);
            lastPhaseForDisplay = envelopePhase;
            targetGain = modulationGain (shapeValueCached (envelopePhase));
            --remainingSamples;
            if (remainingSamples <= 0)
            {
                remainingSamples = 0;
                envelopeActiveInternal = false;
                envelopeActiveForUI.store (false, std::memory_order_relaxed);
            }
        }

        const float currentTarget = juce::jmap (mix, 1.0f, targetGain);
        gainSmoother.setTargetValue (currentTarget);
        const float gain = gainSmoother.getNextValue();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const int state = juce::jmin<int> (channel, 1);
            const float in = buffer.getSample (channel, i);

            lowCutState[static_cast<size_t> (state)] =
                lowAlpha * lowCutState[static_cast<size_t> (state)] + (1.0f - lowAlpha) * in;
            const float highPassed = in - lowCutState[static_cast<size_t> (state)];

            highCutState[static_cast<size_t> (state)] =
                highAlpha * highCutState[static_cast<size_t> (state)] + (1.0f - highAlpha) * highPassed;
            const float bandPassed = highCutState[static_cast<size_t> (state)];

            crossoverState[static_cast<size_t> (state)] =
                splitAlpha * crossoverState[static_cast<size_t> (state)]
                + (1.0f - splitAlpha) * highPassed;
            const float splitLow = crossoverState[static_cast<size_t> (state)];
            const float splitHigh = highPassed - splitLow;

            float wet = in;
            if (bandMode == 0)
                wet = bandPassed * gain;
            else if (bandMode == 1)
                wet = splitLow * gain + splitHigh;
            else
                wet = splitHigh * gain + splitLow;

            buffer.setSample (channel, i, juce::jmap (mix, in, wet));
        }
    }

    envelopeDisplayPhase.store (envelopeActiveInternal ? juce::jlimit<float> (0.0f, 1.0f, lastPhaseForDisplay) : 0.0f,
                                 std::memory_order_relaxed);
}

void HomeSidechainReceiverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
    {
        xml->setAttribute ("NodeModel", 1);
        copyXmlToBinary (*xml, destData);
    }
}

void HomeSidechainReceiverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            const bool hasNewNodes = xml->getStringAttribute ("NodeModel", {}).isNotEmpty();
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            if (! hasNewNodes)
            {
                for (int i = 0; i < maxNodes; ++i)
                {
                    const float x = defaultNodeX[i];
                    const float y = i < legacyShapeCount ?
                        apvts.getRawParameterValue ("SHAPE_" + juce::String (i + 1))->load() : defaultNodeY[i];
                    if (auto* px = apvts.getParameter ("NODE_X_" + juce::String (i + 1)))
                        px->setValueNotifyingHost (px->convertTo0to1 (x));
                    if (auto* py = apvts.getParameter ("NODE_Y_" + juce::String (i + 1)))
                        py->setValueNotifyingHost (py->convertTo0to1 (juce::jlimit<float> (0.0f, 1.0f, y)));
                    if (auto* pa = apvts.getParameter ("NODE_ACTIVE_" + juce::String (i + 1)))
                        pa->setValueNotifyingHost (i < 7 ? 1.0f : 0.0f);
                }
            }
        }
    }
}

juce::AudioProcessorEditor* HomeSidechainReceiverAudioProcessor::createEditor()
{
    return new HomeSidechainReceiverAudioProcessorEditor (*this);
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HomeSidechainReceiverAudioProcessor();
}
