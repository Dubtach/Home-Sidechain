#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
    struct PresetPoint
    {
        float x, y, tension;
    };

    struct PresetDefinition
    {
        const char* name;
        int count;
        PresetPoint points[8];
    };

    // Factory shapes.
    //
    // Every one of these is loop-safe: the height at phase 1.0 is identical to
    // the height at phase 0.0, and the last few percent of the cycle is a ramp
    // back down to it rather than a vertical jump. A drawn sidechain envelope
    // is cycled like an LFO, so whatever value the shape ends on is the value
    // it steps away from on the next trigger -- end high and you get a step
    // discontinuity in gain, which is a click. The short fall at the end is the
    // same trick VolumeShaper's trigger pre-smoothing applies just before a
    // transient, and it doubles as the "hold before the next cycle" that gives
    // four-to-the-floor pumping its shape.
    const PresetDefinition presets[HomeSidechainReceiverAudioProcessor::numPresets] =
    {
        { "Classic", 3, { { 0.00f, 0.00f, 0.34f }, { 0.95f, 1.00f, 0.50f }, { 1.00f, 0.00f, 0.50f } } },
        { "Tight",   4, { { 0.00f, 0.00f, 0.20f }, { 0.50f, 1.00f, 0.50f }, { 0.95f, 1.00f, 0.50f },
                          { 1.00f, 0.00f, 0.50f } } },
        { "Long",    3, { { 0.00f, 0.00f, 0.50f }, { 0.95f, 1.00f, 0.50f }, { 1.00f, 0.00f, 0.50f } } },
        { "Deep",    4, { { 0.00f, 0.00f, 0.50f }, { 0.16f, 0.00f, 0.30f }, { 0.95f, 1.00f, 0.50f },
                          { 1.00f, 0.00f, 0.50f } } },
        { "Soft",    3, { { 0.00f, 0.30f, 0.42f }, { 0.95f, 1.00f, 0.50f }, { 1.00f, 0.30f, 0.50f } } },
        { "Gate",    5, { { 0.00f, 0.00f, 0.50f }, { 0.46f, 0.00f, 0.50f }, { 0.52f, 1.00f, 0.50f },
                          { 0.95f, 1.00f, 0.50f }, { 1.00f, 0.00f, 0.50f } } },
        { "Double",  5, { { 0.00f, 0.00f, 0.30f }, { 0.44f, 1.00f, 0.50f }, { 0.50f, 0.00f, 0.32f },
                          { 0.95f, 1.00f, 0.50f }, { 1.00f, 0.00f, 0.50f } } },
        { "Triplet", 7, { { 0.00f, 0.00f, 0.28f }, { 0.29f, 1.00f, 0.50f }, { 0.34f, 0.00f, 0.28f },
                          { 0.62f, 1.00f, 0.50f }, { 0.67f, 0.00f, 0.28f }, { 0.95f, 1.00f, 0.50f },
                          { 1.00f, 0.00f, 0.50f } } }
    };

    // Defaults mirror the Classic preset, including its fall at the end.
    constexpr float defaultNodeX[HomeSidechainReceiverAudioProcessor::maxNodes] =
    {
        0.0f, 0.95f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
    };

    constexpr float defaultNodeY[HomeSidechainReceiverAudioProcessor::maxNodes] =
    {
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
    };

    constexpr float defaultTension[HomeSidechainReceiverAudioProcessor::maxNodes] =
    {
        0.34f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f
    };

    constexpr bool defaultNodeActive[HomeSidechainReceiverAudioProcessor::maxNodes] =
    {
        true, true, true, false, false, false, false, false,
        false, false, false, false, false, false, false, false
    };

    inline float onePoleCoefficient (float frequencyHz, double sampleRate) noexcept
    {
        const auto sr = static_cast<float> (juce::jmax (1.0, sampleRate));
        const float f = juce::jlimit (1.0f, sr * 0.49f, frequencyHz);
        return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * f / sr);
    }
}

// =============================================================================
// Construction and parameters
// =============================================================================

HomeSidechainReceiverAudioProcessor::HomeSidechainReceiverAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    activeCurve = buildSnapshot();
}

juce::StringArray HomeSidechainReceiverAudioProcessor::rateNames()
{
    return
    {
        // Straight (indices 0-5, unchanged from earlier versions so an old
        // session still opens on the right rate).
        "1/16", "1/8", "1/4", "1/2", "1/1", "2/1",
        // Triplet: 3 in the time of 2, i.e. straight duration * 2/3.
        "1/16T", "1/8T", "1/4T", "1/2T", "1/1T",
        // Dotted: straight duration * 1.5.
        "1/16.", "1/8.", "1/4.", "1/2.", "1/1.",
        // Poly-groove: the bar split into N equal pulses. Some of these land
        // on the same duration as a triplet above (a bar in 3 is the same
        // length as a half-note triplet) -- that's musically correct, not a
        // bug, and having both makes the grouping obvious in the menu.
        "1/3", "1/5", "1/6", "1/7", "1/9", "1/12"
    };
}

double HomeSidechainReceiverAudioProcessor::beatsForRate (int rate) noexcept
{
    // One bar in 4/4 is 4 quarter-note beats; that's the reference "1/1"
    // duration everything else, including the poly-groove entries, divides.
    static constexpr double barBeats = 4.0;

    static constexpr double beats[numRates] =
    {
        // Straight
        0.25, 0.5, 1.0, 2.0, 4.0, 8.0,
        // Triplet (straight * 2/3)
        0.25 * 2.0 / 3.0, 0.5 * 2.0 / 3.0, 1.0 * 2.0 / 3.0, 2.0 * 2.0 / 3.0, 4.0 * 2.0 / 3.0,
        // Dotted (straight * 1.5)
        0.25 * 1.5, 0.5 * 1.5, 1.0 * 1.5, 2.0 * 1.5, 4.0 * 1.5,
        // Poly-groove (bar / N)
        barBeats / 3.0, barBeats / 5.0, barBeats / 6.0, barBeats / 7.0, barBeats / 9.0, barBeats / 12.0
    };

    return beats[juce::jlimit (0, numRates - 1, rate)];
}

juce::String HomeSidechainReceiverAudioProcessor::presetName (int index)
{
    return presets[juce::jlimit (0, numPresets - 1, index)].name;
}

receiverCurve::Snapshot HomeSidechainReceiverAudioProcessor::presetSnapshot (int index)
{
    const auto& preset = presets[juce::jlimit (0, numPresets - 1, index)];
    receiverCurve::Snapshot snapshot;

    for (int i = 0; i < preset.count; ++i)
        snapshot.add (preset.points[i].x, preset.points[i].y, preset.points[i].tension);

    return snapshot;
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
        "LINK", "Link", homeSidechain::linkNames(), 0, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "SOURCE", "Source", juce::StringArray { "Home-Link", "MIDI", "Both" }, 2, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "RUN", "Run Mode", juce::StringArray { "Trigger", "Host Sync" }, 0, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "SYNC", "Tempo Sync", true, BoolAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "RATE", "Rate", rateNames(), 2, ChoiceAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "LENGTH", "Length", juce::NormalisableRange<float> (20.0f, 4000.0f, 0.1f, 0.4f), 500.0f,
        FloatAttributes{}.withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "DEPTH", "Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "MIX", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, FloatAttributes{}));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "SMOOTH", "Smooth", juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f, 0.6f), 4.0f,
        FloatAttributes{}.withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "LOW_CUT", "Low Cut", juce::NormalisableRange<float> (20.0f, 2000.0f, 0.1f, 0.35f), 20.0f,
        FloatAttributes{}.withLabel ("Hz")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "HIGH_CUT", "High Cut", juce::NormalisableRange<float> (500.0f, 20000.0f, 0.1f, 0.35f), 20000.0f,
        FloatAttributes{}.withLabel ("Hz")));

    for (int i = 1; i <= maxNodes; ++i)
    {
        const auto suffix = juce::String (i);
        const auto slot = static_cast<size_t> (i - 1);

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "NODE_X_" + suffix, "Node X " + suffix,
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), defaultNodeX[slot], FloatAttributes{}));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "NODE_Y_" + suffix, "Node Y " + suffix,
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0005f), defaultNodeY[slot], FloatAttributes{}));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "TENSION_" + suffix, "Tension " + suffix,
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0005f), defaultTension[slot], FloatAttributes{}));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            "NODE_ON_" + suffix, "Node On " + suffix, defaultNodeActive[slot], BoolAttributes{}));
    }

    return { params.begin(), params.end() };
}

// =============================================================================
// Parameter access
// =============================================================================

int HomeSidechainReceiverAudioProcessor::getLink() const noexcept
{
    return juce::jlimit (0, homeSidechain::numberOfLinks - 1,
                         static_cast<int> (apvts.getRawParameterValue ("LINK")->load()));
}

void HomeSidechainReceiverAudioProcessor::setLink (int link)
{
    if (auto* p = apvts.getParameter ("LINK"))
        p->setValueNotifyingHost (p->convertTo0to1 (
            static_cast<float> (juce::jlimit (0, homeSidechain::numberOfLinks - 1, link))));
}

int HomeSidechainReceiverAudioProcessor::getRunMode() const noexcept
{
    return juce::jlimit (0, 1, static_cast<int> (apvts.getRawParameterValue ("RUN")->load()));
}

void HomeSidechainReceiverAudioProcessor::setRunMode (int mode)
{
    if (auto* p = apvts.getParameter ("RUN"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (juce::jlimit (0, 1, mode))));
}

int HomeSidechainReceiverAudioProcessor::getSource() const noexcept
{
    return juce::jlimit (0, 2, static_cast<int> (apvts.getRawParameterValue ("SOURCE")->load()));
}

void HomeSidechainReceiverAudioProcessor::setSource (int source)
{
    if (auto* p = apvts.getParameter ("SOURCE"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (juce::jlimit (0, 2, source))));
}

int HomeSidechainReceiverAudioProcessor::getRate() const noexcept
{
    return juce::jlimit (0, numRates - 1, static_cast<int> (apvts.getRawParameterValue ("RATE")->load()));
}

void HomeSidechainReceiverAudioProcessor::setRate (int rate)
{
    if (auto* p = apvts.getParameter ("RATE"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (juce::jlimit (0, numRates - 1, rate))));
}

bool HomeSidechainReceiverAudioProcessor::isSynced() const noexcept
{
    return apvts.getRawParameterValue ("SYNC")->load() > 0.5f;
}

bool HomeSidechainReceiverAudioProcessor::isBypassed() const noexcept
{
    return apvts.getRawParameterValue ("BYPASS")->load() > 0.5f;
}

float HomeSidechainReceiverAudioProcessor::getMix() const noexcept
{
    return juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("MIX")->load());
}

float HomeSidechainReceiverAudioProcessor::getDepth() const noexcept
{
    return juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("DEPTH")->load());
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
    // Host-sync always follows the grid; free length only applies to triggers.
    if (getRunMode() == 1 || isSynced())
    {
        const double bpm = juce::jmax (1.0, getHostBpm());
        return juce::jmax (1.0, beatsForRate (getRate()) * 60.0 / bpm * currentSampleRate);
    }

    const double lengthMs = apvts.getRawParameterValue ("LENGTH")->load();
    return juce::jmax (1.0, lengthMs * 0.001 * currentSampleRate);
}

int HomeSidechainReceiverAudioProcessor::gridDivisions() const noexcept
{
    const double beats = beatsForRate (getRate());
    return juce::jlimit (4, 32, static_cast<int> (std::lround (beats * 4.0)));
}

// =============================================================================
// Curve access
// =============================================================================

bool HomeSidechainReceiverAudioProcessor::isNodeActive (int slot) const noexcept
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    return apvts.getRawParameterValue ("NODE_ON_" + juce::String (i + 1))->load() > 0.5f;
}

float HomeSidechainReceiverAudioProcessor::getNodeX (int slot) const noexcept
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    return juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("NODE_X_" + juce::String (i + 1))->load());
}

float HomeSidechainReceiverAudioProcessor::getNodeY (int slot) const noexcept
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    return juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("NODE_Y_" + juce::String (i + 1))->load());
}

float HomeSidechainReceiverAudioProcessor::getTension (int slot) const noexcept
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    return juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("TENSION_" + juce::String (i + 1))->load());
}

void HomeSidechainReceiverAudioProcessor::setNodeX (int slot, float value)
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    if (auto* p = apvts.getParameter ("NODE_X_" + juce::String (i + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::setNodeY (int slot, float value)
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    if (auto* p = apvts.getParameter ("NODE_Y_" + juce::String (i + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::setTension (int slot, float value)
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    if (auto* p = apvts.getParameter ("TENSION_" + juce::String (i + 1)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value)));
}

void HomeSidechainReceiverAudioProcessor::setNodeActive (int slot, bool active)
{
    const int i = juce::jlimit (0, maxNodes - 1, slot);
    if (auto* p = apvts.getParameter ("NODE_ON_" + juce::String (i + 1)))
        p->setValueNotifyingHost (active ? 1.0f : 0.0f);
}

void HomeSidechainReceiverAudioProcessor::setEndpointY (float value)
{
    int firstSlot = -1, lastSlot = -1;
    float firstX = 2.0f, lastX = -1.0f;

    for (int i = 0; i < maxNodes; ++i)
    {
        if (! isNodeActive (i))
            continue;

        const float x = getNodeX (i);

        if (x <= firstX) { firstX = x; firstSlot = i; }
        if (x >= lastX)  { lastX = x;  lastSlot = i; }
    }

    if (firstSlot >= 0)
        setNodeY (firstSlot, value);

    if (lastSlot >= 0 && lastSlot != firstSlot)
        setNodeY (lastSlot, value);
}

bool HomeSidechainReceiverAudioProcessor::isEndpoint (int slot) const noexcept
{
    if (! isNodeActive (slot))
        return false;

    float lowest = 2.0f, highest = -1.0f;

    for (int i = 0; i < maxNodes; ++i)
    {
        if (! isNodeActive (i))
            continue;

        lowest = juce::jmin (lowest, getNodeX (i));
        highest = juce::jmax (highest, getNodeX (i));
    }

    const float x = getNodeX (slot);
    return x <= lowest || x >= highest;
}

int HomeSidechainReceiverAudioProcessor::activeNodeCount() const noexcept
{
    int count = 0;

    for (int i = 0; i < maxNodes; ++i)
        if (isNodeActive (i))
            ++count;

    return count;
}

receiverCurve::Snapshot HomeSidechainReceiverAudioProcessor::buildSnapshot() const noexcept
{
    struct Entry { float x, y, tension; };
    std::array<Entry, maxNodes> entries {};
    int count = 0;

    for (int i = 0; i < maxNodes; ++i)
    {
        if (! isNodeActive (i))
            continue;

        entries[static_cast<size_t> (count)] = { getNodeX (i), getNodeY (i), getTension (i) };
        ++count;
    }

    std::sort (entries.begin(), entries.begin() + count,
               [] (const Entry& a, const Entry& b) { return a.x < b.x; });

    receiverCurve::Snapshot snapshot;

    for (int i = 0; i < count; ++i)
    {
        const auto& entry = entries[static_cast<size_t> (i)];
        snapshot.add (entry.x, entry.y, entry.tension);
    }

    return snapshot;
}

int HomeSidechainReceiverAudioProcessor::addNode (float x, float y)
{
    for (int i = 0; i < maxNodes; ++i)
    {
        if (isNodeActive (i))
            continue;

        setNodeX (i, x);
        setNodeY (i, y);
        setTension (i, 0.5f);
        setNodeActive (i, true);
        return i;
    }

    return -1;
}

void HomeSidechainReceiverAudioProcessor::removeNode (int slot)
{
    if (activeNodeCount() <= 2)
        return;

    setNodeActive (slot, false);
}

void HomeSidechainReceiverAudioProcessor::applyPreset (int index)
{
    const auto& preset = presets[juce::jlimit (0, numPresets - 1, index)];

    for (int i = 0; i < maxNodes; ++i)
    {
        const bool used = i < preset.count;
        setNodeActive (i, used);

        if (used)
        {
            setNodeX (i, preset.points[i].x);
            setNodeY (i, preset.points[i].y);
            setTension (i, preset.points[i].tension);
        }
    }
}

void HomeSidechainReceiverAudioProcessor::resetCurve()
{
    applyPreset (0);
}

// =============================================================================
// Audio
// =============================================================================

void HomeSidechainReceiverAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = juce::jmax (1.0, newSampleRate);
    testTriggerRequested.store (false, std::memory_order_release);

    envelopePhase = 0.0;
    envelopeActiveInternal = false;
    remainingSamples = 0.0;
    envelopeActiveForUI.store (false, std::memory_order_relaxed);
    envelopeDisplayPhase.store (0.0f, std::memory_order_relaxed);
    currentGainForUI.store (1.0f, std::memory_order_relaxed);

    gainState = 1.0f;
    smoothCoefficient = 0.0f;
    lowCutState = { 0.0f, 0.0f };
    highCutState = { 0.0f, 0.0f };

    activeCurve = buildSnapshot();

    const int link = getLink();
    homeLinkLastLink = link;
    homeLinkLastSequence = homeLinkService().latestSequence (link);
}

void HomeSidechainReceiverAudioProcessor::releaseResources() {}

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

void HomeSidechainReceiverAudioProcessor::triggerEnvelope()
{
    envelopeActiveInternal = true;
    envelopeActiveForUI.store (true, std::memory_order_relaxed);
    envelopePhase = 0.0;
    remainingSamples = juce::jmax (1.0, cycleSamples());
    triggerActivity.store (1.0f, std::memory_order_relaxed);
    triggerCount.fetch_add (1, std::memory_order_relaxed);
}

float HomeSidechainReceiverAudioProcessor::gainForPhase (float phase, float depth) const noexcept
{
    const float shape = juce::jlimit (0.0f, 1.0f, activeCurve.valueAt (phase));
    return 1.0f - depth * (1.0f - shape);
}

void HomeSidechainReceiverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                        juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const int link = getLink();

    if (link != homeLinkLastLink)
    {
        homeLinkLastLink = link;
        homeLinkLastSequence = homeLinkService().latestSequence (link);
    }

    const int targetNote = homeSidechain::midiNoteForLink (link);
    const int source = getSource();
    const bool acceptHomeLink = source != 1;
    const bool acceptMidi = source != 0;
    const bool bypassed = isBypassed();
    const int runMode = getRunMode();

    triggerActivity.store (triggerActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    midiActivity.store (midiActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    homeLinkActivity.store (homeLinkActivity.load (std::memory_order_relaxed) * 0.90f, std::memory_order_relaxed);
    hostBpmForUI.store (static_cast<float> (getHostBpm()), std::memory_order_relaxed);

    const auto heartbeatAge = static_cast<uint32_t> (
        juce::Time::getMillisecondCounter() - homeLinkService().lastHeartbeatMs (link));
    homeLinkConnected.store (heartbeatAge < 450u, std::memory_order_relaxed);

    // -------------------------------------------------------------------------
    // Collect trigger positions for this block. No allocation, no socket I/O.
    // -------------------------------------------------------------------------
    std::array<int, 256> triggerPositions {};
    int triggerPositionCount = 0;

    if (testTriggerRequested.exchange (false, std::memory_order_acq_rel))
        triggerPositions[static_cast<size_t> (triggerPositionCount++)] = 0;

    if (acceptHomeLink)
    {
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
            }

            ++next;
        }

        if (latest > homeLinkLastSequence)
            homeLinkLastSequence = latest;
    }

    int incomingMidiEvents = 0;
    int mostRecentNote = -1;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (! message.isNoteOn())
            continue;

        ++incomingMidiEvents;
        mostRecentNote = message.getNoteNumber();

        if (acceptMidi
            && message.getNoteNumber() == targetNote
            && triggerPositionCount < static_cast<int> (triggerPositions.size()))
        {
            triggerPositions[static_cast<size_t> (triggerPositionCount++)] =
                juce::jlimit (0, numSamples - 1, metadata.samplePosition);
            midiActivity.store (1.0f, std::memory_order_relaxed);
        }
    }

    if (incomingMidiEvents > 0)
    {
        midiEventCount.fetch_add (incomingMidiEvents, std::memory_order_relaxed);
        lastMidiNote.store (mostRecentNote, std::memory_order_relaxed);
    }

    if (bypassed)
    {
        envelopeActiveInternal = false;
        remainingSamples = 0.0;
        gainState = 1.0f;
        envelopeActiveForUI.store (false, std::memory_order_relaxed);
        envelopeDisplayPhase.store (0.0f, std::memory_order_relaxed);
        currentGainForUI.store (1.0f, std::memory_order_relaxed);
        return;
    }

    // -------------------------------------------------------------------------
    // Block-stable settings. The curve is sorted once per block, never per
    // sample, and no parameter is read inside the sample loop.
    // -------------------------------------------------------------------------
    activeCurve = buildSnapshot();

    const double totalCycle = juce::jmax (1.0, cycleSamples());
    const double phaseIncrement = 1.0 / totalCycle;
    const float mix = getMix();
    const float depth = getDepth();
    const float smoothMs = juce::jlimit (0.0f, 40.0f, apvts.getRawParameterValue ("SMOOTH")->load());
    const float lowCutHz = juce::jlimit (20.0f, 2000.0f, apvts.getRawParameterValue ("LOW_CUT")->load());
    const float highCutHz = juce::jmax (lowCutHz * 1.05f,
                                        juce::jlimit (500.0f, 20000.0f,
                                                      apvts.getRawParameterValue ("HIGH_CUT")->load()));

    const float lowAlpha = onePoleCoefficient (lowCutHz, currentSampleRate);
    const float highAlpha = onePoleCoefficient (highCutHz, currentSampleRate);

    // A one-pole smoother never snaps when the time constant changes, so the
    // Smooth control is safe to automate.
    smoothCoefficient = smoothMs <= 0.01f
        ? 0.0f
        : std::exp (-1.0f / juce::jmax (1.0f, smoothMs * 0.001f * static_cast<float> (currentSampleRate)));

    // -------------------------------------------------------------------------
    // Host-sync mode locks the shape to the transport, which is what makes the
    // plugin usable with no trigger source at all.
    // -------------------------------------------------------------------------
    if (runMode == 1)
    {
        envelopeActiveInternal = true;
        envelopeActiveForUI.store (true, std::memory_order_relaxed);

        if (auto* playHead = getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (position->getIsPlaying())
                {
                    if (auto ppq = position->getPpqPosition())
                    {
                        const double beats = juce::jmax (0.0001, beatsForRate (getRate()));
                        double wrapped = std::fmod (*ppq / beats, 1.0);

                        if (wrapped < 0.0)
                            wrapped += 1.0;

                        envelopePhase = wrapped;
                    }
                }
            }
        }

        triggerPositionCount = 0;
    }

    std::sort (triggerPositions.begin(), triggerPositions.begin() + triggerPositionCount);

    int triggerIndex = 0;
    const int numChannels = buffer.getNumChannels();
    float blockPeak = 0.0f;
    float lastGain = gainState;

    for (int i = 0; i < numSamples; ++i)
    {
        while (triggerIndex < triggerPositionCount
               && triggerPositions[static_cast<size_t> (triggerIndex)] == i)
        {
            triggerEnvelope();
            ++triggerIndex;
        }

        float targetGain = 1.0f;

        if (runMode == 1)
        {
            targetGain = gainForPhase (static_cast<float> (envelopePhase), depth);
            envelopePhase += phaseIncrement;

            if (envelopePhase >= 1.0)
                envelopePhase -= std::floor (envelopePhase);
        }
        else if (envelopeActiveInternal && remainingSamples > 0.0)
        {
            envelopePhase = juce::jlimit (0.0, 1.0, 1.0 - remainingSamples / totalCycle);
            targetGain = gainForPhase (static_cast<float> (envelopePhase), depth);
            remainingSamples -= 1.0;

            if (remainingSamples <= 0.0)
            {
                remainingSamples = 0.0;
                envelopeActiveInternal = false;
                envelopeActiveForUI.store (false, std::memory_order_relaxed);
            }
        }

        gainState = targetGain + (gainState - targetGain) * smoothCoefficient;
        const float gain = gainState;
        lastGain = gain;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto state = static_cast<size_t> (juce::jmin (channel, 1));
            const float in = buffer.getSample (channel, i);

            lowCutState[state] += (in - lowCutState[state]) * lowAlpha;
            highCutState[state] += (in - highCutState[state]) * highAlpha;

            // Everything below Low Cut and above High Cut passes untouched;
            // only the band between them is ducked.
            const float below = lowCutState[state];
            const float above = in - highCutState[state];
            const float band = highCutState[state] - below;

            const float wet = below + above + band * gain;

            buffer.setSample (channel, i, in + (wet - in) * mix);

            if (channel == 0)
                blockPeak = juce::jmax (blockPeak, std::abs (in));
        }
    }

    envelopeDisplayPhase.store (juce::jlimit (0.0f, 1.0f, static_cast<float> (envelopePhase)),
                                std::memory_order_relaxed);
    currentGainForUI.store (juce::jlimit (0.0f, 1.0f, lastGain), std::memory_order_relaxed);
    inputLevelForUI.store (juce::jmax (inputLevelForUI.load (std::memory_order_relaxed) * 0.80f, blockPeak),
                           std::memory_order_relaxed);
}

// =============================================================================
// State
// =============================================================================

void HomeSidechainReceiverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
    {
        xml->setAttribute ("CurveModel", 2);
        copyXmlToBinary (*xml, destData);
    }
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
