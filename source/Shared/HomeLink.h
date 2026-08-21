#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <algorithm>

namespace homeSidechain
{
    inline constexpr int firstLinkMidiNote = 36; // C2
    inline constexpr int numberOfLinks = 8;

    inline int midiNoteForLink (int link) noexcept
    {
        return firstLinkMidiNote + juce::jlimit (0, numberOfLinks - 1, link);
    }

    inline juce::String linkName (int link)
    {
        static constexpr const char* names[] = { "A", "B", "C", "D", "E", "F", "G", "H" };
        return names[juce::jlimit (0, numberOfLinks - 1, link)];
    }

    inline juce::StringArray linkNames()
    {
        return { "A", "B", "C", "D", "E", "F", "G", "H" };
    }

    inline float linearToDb (float linear) noexcept
    {
        return juce::Decibels::gainToDecibels (juce::jmax (linear, 0.000001f));
    }

    // -------------------------------------------------------------------------
    // Home-Link v2: process-local, lock-free-ish broadcast event bus.
    //
    // There is intentionally NO socket, thread, wait, or heap allocation in the
    // audio callback. Trigger writes a fixed-size event directly into a shared
    // per-link ring. Each Receiver owns its own read cursor, so all receivers on
    // the same Link see the same event stream.
    //
    // Timing is carried as an absolute host sample position when the host
    // provides it. This lets the Receiver place a trigger at the exact sample
    // within its current block when the Trigger track is processed first.
    // -------------------------------------------------------------------------

    struct HomeLinkEvent
    {
        uint64_t sequence = 0;
        int64_t absoluteSample = -1;
        uint16_t velocity = 127;
        uint8_t source = 0; // 1 = audio, 2 = midi, 3 = manual
        uint8_t reserved = 0;
    };

    class HomeLinkBus
    {
    public:
        static HomeLinkBus& instance() noexcept
        {
            static HomeLinkBus bus;
            return bus;
        }

        uint64_t publishTrigger (int link, const HomeLinkEvent& input) noexcept
        {
            const auto l = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            const auto sequence = writeSequence[l].fetch_add (1, std::memory_order_acq_rel) + 1;
            auto& slot = rings[l][static_cast<size_t> (sequence % ringSize)];

            auto event = input;
            event.sequence = sequence;
            slot.event = event;
            slot.publishedSequence.store (sequence, std::memory_order_release);
            triggerCount[l].fetch_add (1, std::memory_order_relaxed);
            return sequence;
        }

        void markHeartbeat (int link) noexcept
        {
            const auto l = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            heartbeatMs[l].store (juce::Time::getMillisecondCounter(), std::memory_order_relaxed);
        }

        uint64_t latestSequence (int link) const noexcept
        {
            const auto l = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            return writeSequence[l].load (std::memory_order_acquire);
        }

        bool readEvent (int link, uint64_t sequence, HomeLinkEvent& result) const noexcept
        {
            if (sequence == 0)
                return false;

            const auto l = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            const auto& slot = rings[l][static_cast<size_t> (sequence % ringSize)];
            const auto published = slot.publishedSequence.load (std::memory_order_acquire);

            if (published != sequence)
                return false;

            result = slot.event;
            return true;
        }

        uint32_t lastHeartbeatMs (int link) const noexcept
        {
            const auto l = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            return heartbeatMs[l].load (std::memory_order_relaxed);
        }

        int totalTriggerCount (int link) const noexcept
        {
            const auto l = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            return triggerCount[l].load (std::memory_order_relaxed);
        }

        bool isEventAvailable (int link, uint64_t sequence) const noexcept
        {
            HomeLinkEvent ignored;
            return readEvent (link, sequence, ignored);
        }

        static constexpr uint64_t ringCapacity() noexcept { return ringSize; }

    private:
        static constexpr size_t ringSize = 1024;

        struct Slot
        {
            HomeLinkEvent event {};
            std::atomic<uint64_t> publishedSequence { 0 };
        };

        std::array<std::array<Slot, ringSize>, numberOfLinks> rings {};
        std::array<std::atomic<uint64_t>, numberOfLinks> writeSequence {};
        std::array<std::atomic<uint32_t>, numberOfLinks> heartbeatMs {};
        std::array<std::atomic<int>, numberOfLinks> triggerCount {};
    };

    class HomeLinkSender
    {
    public:
        void start() noexcept {}
        void stop() noexcept {}

        void setLink (int link) noexcept
        {
            currentLink.store (juce::jlimit (0, numberOfLinks - 1, link), std::memory_order_relaxed);
        }

        void heartbeat (int link) noexcept
        {
            HomeLinkBus::instance().markHeartbeat (link);
        }

        uint64_t publishTrigger (int link, int velocity, int64_t absoluteSample, uint8_t source) noexcept
        {
            HomeLinkEvent event;
            event.absoluteSample = absoluteSample;
            event.velocity = static_cast<uint16_t> (juce::jlimit (1, 127, velocity));
            event.source = source;
            return HomeLinkBus::instance().publishTrigger (link, event);
        }

        // Compatibility helper for code that still calls enqueueTrigger.
        void enqueueTrigger (int link, int velocity, int sampleOffset, uint64_t hostTicks) noexcept
        {
            juce::ignoreUnused (hostTicks);
            publishTrigger (link, velocity, static_cast<int64_t> (sampleOffset), 1);
        }

        int getDroppedCount() const noexcept { return 0; }

    private:
        std::atomic<int> currentLink { 0 };
    };

    class HomeLinkReceiverService
    {
    public:
        static HomeLinkReceiverService& instance() noexcept
        {
            static HomeLinkReceiverService service;
            return service;
        }

        uint64_t latestSequence (int link) const noexcept
        {
            return HomeLinkBus::instance().latestSequence (link);
        }

        bool readEvent (int link, uint64_t sequence, HomeLinkEvent& result) const noexcept
        {
            return HomeLinkBus::instance().readEvent (link, sequence, result);
        }

        uint32_t lastHeartbeatMs (int link) const noexcept
        {
            return HomeLinkBus::instance().lastHeartbeatMs (link);
        }

        int totalTriggerCount (int link) const noexcept
        {
            return HomeLinkBus::instance().totalTriggerCount (link);
        }

        bool isListening (int) const noexcept { return true; }
        uint64_t ringCapacity() const noexcept { return HomeLinkBus::ringCapacity(); }
    };
}
