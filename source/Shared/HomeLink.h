#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

#if JUCE_WINDOWS
 #include <windows.h>
#elif JUCE_MAC || JUCE_LINUX
 #include <unistd.h>
#endif

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
    // Home-Link
    //
    // Home-Link deliberately does not depend on the DAW's MIDI routing.  The
    // Trigger publishes small UDP packets to localhost using a port derived
    // from the DAW process ID.  This keeps different DAW processes isolated,
    // requires no user routing, and works for the VST3/Standalone workflow.
    //
    // The audio thread never performs socket I/O. Trigger uses a small SPSC
    // queue and a worker thread; Receiver has one listener thread and a fixed
    // per-link event ring.
    // -------------------------------------------------------------------------

    inline uint32_t currentProcessId() noexcept
    {
       #if JUCE_WINDOWS
        return static_cast<uint32_t> (::GetCurrentProcessId());
       #elif JUCE_MAC || JUCE_LINUX
        return static_cast<uint32_t> (::getpid());
       #else
        // Fallback for platforms without a native process-id API. The VST3/AU
        // builds currently target Windows/macOS/Linux.
        return 1u;
       #endif
    }

    inline int homeLinkBasePort() noexcept
    {
        const auto pid = currentProcessId();
        return 24000 + static_cast<int> (pid % 2000u) * 8;
    }

    inline int homeLinkPort (int link) noexcept
    {
        return homeLinkBasePort() + juce::jlimit (0, numberOfLinks - 1, link);
    }

    inline constexpr uint32_t homeLinkMagic = 0x484C4E4Bu; // "HLNK"
    inline constexpr uint8_t homeLinkVersion = 1;
    inline constexpr uint8_t packetHeartbeat = 1;
    inline constexpr uint8_t packetTrigger = 2;

    #pragma pack(push, 1)
    struct HomeLinkPacket
    {
        uint32_t magic = homeLinkMagic;
        uint8_t version = homeLinkVersion;
        uint8_t type = packetHeartbeat;
        uint8_t link = 0;
        uint8_t reserved = 0;
        uint16_t velocity = 127;
        uint16_t sourceSampleOffset = 0;
        uint64_t sourceHostTicks = 0;
        uint64_t sequence = 0;
    };
    #pragma pack(pop)

    static_assert (sizeof (HomeLinkPacket) == 28, "Unexpected Home-Link packet packing");

    struct HomeLinkEvent
    {
        uint64_t sequence = 0;
        uint64_t sourceHostTicks = 0;
        uint16_t sourceSampleOffset = 0;
        uint8_t velocity = 127;
    };

    class HomeLinkSender : private juce::Thread
    {
    public:
        HomeLinkSender()
            : juce::Thread ("HomeSidechain-Link-Sender")
        {
            startThread (juce::Thread::Priority::high);
        }

        ~HomeLinkSender() override
        {
            signalThreadShouldExit();
            queueReady.signal();
            stopThread (500);
        }

        void setLink (int link) noexcept
        {
            currentLink.store (juce::jlimit (0, numberOfLinks - 1, link), std::memory_order_relaxed);
        }

        void enqueueTrigger (int link, int velocity, int sampleOffset, uint64_t hostTicks)
        {
            const auto safeLink = juce::jlimit (0, numberOfLinks - 1, link);
            currentLink.store (safeLink, std::memory_order_relaxed);

            const auto w = writeIndex.load (std::memory_order_relaxed);
            const auto r = readIndex.load (std::memory_order_acquire);

            if (w - r >= queueSize)
            {
                droppedCount.fetch_add (1, std::memory_order_relaxed);
                return;
            }

            auto& slot = queue[static_cast<size_t> (w % queueSize)];
            slot.type = packetTrigger;
            slot.link = static_cast<uint8_t> (safeLink);
            slot.velocity = static_cast<uint16_t> (juce::jlimit (1, 127, velocity));
            slot.sourceSampleOffset = static_cast<uint16_t> (juce::jlimit (0, 65535, sampleOffset));
            slot.sourceHostTicks = hostTicks;
            slot.sequence = ++sequence;
            writeIndex.store (w + 1, std::memory_order_release);
            queueReady.signal();
        }

        int getDroppedCount() const noexcept
        {
            return droppedCount.load (std::memory_order_relaxed);
        }

    private:
        static constexpr uint64_t queueSize = 256;

        std::array<HomeLinkPacket, queueSize> queue {};
        std::atomic<uint64_t> writeIndex { 0 };
        std::atomic<uint64_t> readIndex { 0 };
        std::atomic<int> droppedCount { 0 };
        std::atomic<uint64_t> sequence { 0 };
        std::atomic<int> currentLink { 0 };
        juce::WaitableEvent queueReady;
        juce::DatagramSocket socket;

        void sendPacket (const HomeLinkPacket& packet)
        {
            const auto bytesSent = socket.write ("127.0.0.1", homeLinkPort (packet.link), &packet, sizeof (packet));
            if (bytesSent != static_cast<int> (sizeof (packet)))
                droppedCount.fetch_add (1, std::memory_order_relaxed);
        }

        void run() override
        {
            uint32_t lastHeartbeatMs = 0;

            while (! threadShouldExit())
            {
                bool sentAnything = false;

                auto r = readIndex.load (std::memory_order_relaxed);
                const auto w = writeIndex.load (std::memory_order_acquire);

                while (r < w)
                {
                    sendPacket (queue[static_cast<size_t> (r % queueSize)]);
                    ++r;
                    sentAnything = true;
                }

                readIndex.store (r, std::memory_order_release);

                const auto now = juce::Time::getMillisecondCounter();

                if (now - lastHeartbeatMs >= 100)
                {
                    HomeLinkPacket heartbeat;
                    heartbeat.type = packetHeartbeat;
                    heartbeat.sequence = ++sequence;

                    // Advertise only the currently selected link so Receiver
                    // status reflects the actual Trigger/Receiver pairing.
                    heartbeat.link = static_cast<uint8_t> (
                        currentLink.load (std::memory_order_relaxed));
                    sendPacket (heartbeat);

                    lastHeartbeatMs = now;
                    sentAnything = true;
                }

                if (! sentAnything)
                    queueReady.wait (20);
            }
        }
    };

    class HomeLinkReceiverService : private juce::Thread
    {
    public:
        HomeLinkReceiverService()
            : juce::Thread ("HomeSidechain-Link-Receiver")
        {
            for (int link = 0; link < numberOfLinks; ++link)
            {
                sockets[static_cast<size_t> (link)] = std::make_unique<juce::DatagramSocket>();
                sockets[static_cast<size_t> (link)]->setEnablePortReuse (true);
                const auto result = sockets[static_cast<size_t> (link)]->bindToPort (homeLinkPort (link), "127.0.0.1");
                socketBound[static_cast<size_t> (link)].store (result, std::memory_order_release);
            }

            startThread (juce::Thread::Priority::high);
        }

        ~HomeLinkReceiverService() override
        {
            signalThreadShouldExit();

            for (auto& socket : sockets)
                if (socket != nullptr)
                    socket->shutdown();

            stopThread (500);
        }

        HomeLinkReceiverService (const HomeLinkReceiverService&) = delete;
        HomeLinkReceiverService& operator= (const HomeLinkReceiverService&) = delete;

        static HomeLinkReceiverService& instance()
        {
            static HomeLinkReceiverService service;
            return service;
        }

        uint64_t latestSequence (int link) const noexcept
        {
            return latestSeq[static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link))]
                .load (std::memory_order_acquire);
        }

        bool readEvent (int link, uint64_t sequence, HomeLinkEvent& result) const noexcept
        {
            const auto safeLink = juce::jlimit (0, numberOfLinks - 1, link);
            const auto& slot = rings[static_cast<size_t> (safeLink)][static_cast<size_t> (sequence % ringSize)];
            const auto published = slot.sequence.load (std::memory_order_acquire);

            if (published != sequence)
                return false;

            result = slot.event;
            return true;
        }

        uint32_t lastHeartbeatMs (int link) const noexcept
        {
            return heartbeatMs[static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link))]
                .load (std::memory_order_relaxed);
        }

        bool isListening (int link) const noexcept
        {
            const auto index = static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link));
            return socketBound[index].load (std::memory_order_acquire);
        }

        int totalTriggerCount (int link) const noexcept
        {
            return triggerCounts[static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link))]
                .load (std::memory_order_relaxed);
        }

    private:
        static constexpr size_t ringSize = 256;

        struct Slot
        {
            HomeLinkEvent event;
            std::atomic<uint64_t> sequence { 0 };
        };

        std::array<std::array<Slot, ringSize>, numberOfLinks> rings {};
        std::array<std::atomic<uint64_t>, numberOfLinks> latestSeq {};
        std::array<std::atomic<uint32_t>, numberOfLinks> heartbeatMs {};
        std::array<std::atomic<int>, numberOfLinks> triggerCounts {};
        std::array<std::atomic<bool>, numberOfLinks> socketBound {};
        std::array<std::unique_ptr<juce::DatagramSocket>, numberOfLinks> sockets {};

        void publish (const HomeLinkPacket& packet)
        {
            const auto link = juce::jlimit (0, numberOfLinks - 1, static_cast<int> (packet.link));

            if (packet.type == packetHeartbeat)
            {
                heartbeatMs[static_cast<size_t> (link)].store (
                    juce::Time::getMillisecondCounter(), std::memory_order_relaxed);
                return;
            }

            if (packet.type != packetTrigger)
                return;

            auto sequence = packet.sequence;
            if (sequence == 0)
                sequence = latestSeq[static_cast<size_t> (link)].load (std::memory_order_relaxed) + 1;

            auto& slot = rings[static_cast<size_t> (link)][static_cast<size_t> (sequence % ringSize)];
            slot.event.sequence = sequence;
            slot.event.sourceHostTicks = packet.sourceHostTicks;
            slot.event.sourceSampleOffset = packet.sourceSampleOffset;
            slot.event.velocity = static_cast<uint8_t> (juce::jlimit (1, 127, static_cast<int> (packet.velocity)));
            slot.sequence.store (sequence, std::memory_order_release);

            latestSeq[static_cast<size_t> (link)].store (sequence, std::memory_order_release);
            triggerCounts[static_cast<size_t> (link)].fetch_add (1, std::memory_order_relaxed);
        }

        void run() override
        {
            std::array<uint8_t, 512> receiveBuffer {};

            while (! threadShouldExit())
            {
                bool received = false;

                for (int link = 0; link < numberOfLinks; ++link)
                {
                    auto* socket = sockets[static_cast<size_t> (link)].get();
                    if (socket == nullptr)
                        continue;

                    if (socket->waitUntilReady (true, 1) > 0)
                    {
                        const auto bytes = socket->read (receiveBuffer.data(),
                                                         static_cast<int> (receiveBuffer.size()),
                                                         false);

                        if (bytes == static_cast<int> (sizeof (HomeLinkPacket)))
                        {
                            HomeLinkPacket packet;
                            std::memcpy (&packet, receiveBuffer.data(), sizeof (packet));

                            if (packet.magic == homeLinkMagic && packet.version == homeLinkVersion)
                            {
                                publish (packet);
                                received = true;
                            }
                        }
                    }
                }

                if (! received)
                    wait (1);
            }
        }
    };

    inline float hostTicksToSeconds (uint64_t ticks) noexcept
    {
        return static_cast<float> (static_cast<double> (ticks)
                                    / juce::Time::getHighResolutionTicksPerSecond());
    }

    inline uint64_t currentHostTicks() noexcept
    {
        return static_cast<uint64_t> (juce::Time::getHighResolutionTicks());
    }
}
