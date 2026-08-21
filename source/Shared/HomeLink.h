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
    // Home-Link transport
    //
    // Trigger and Receiver are separate plugin binaries, so a normal C++
    // static singleton is NOT shared between them. Home-Link therefore uses a
    // localhost UDP transport. It is automatic (no DAW MIDI routing), local to
    // the current DAW process, and the real-time audio thread never performs
    // socket I/O. Trigger -> queue -> sender thread -> localhost -> Receiver
    // listener thread -> fixed event ring -> audio thread.
    //
    // The transport adds no declared audio latency and does not intentionally
    // delay the audio signal. Actual trigger arrival is still subject to the
    // DAW's plugin scheduling order and normal thread scheduling.
    // -------------------------------------------------------------------------

    inline uint32_t currentProcessId() noexcept
    {
       #if JUCE_WINDOWS
        return static_cast<uint32_t> (::GetCurrentProcessId());
       #elif JUCE_MAC || JUCE_LINUX
        return static_cast<uint32_t> (::getpid());
       #else
        return 1u;
       #endif
    }

    inline int homeLinkPort() noexcept
    {
        // A deterministic localhost port derived from the DAW process. This
        // keeps independent DAW processes isolated without user configuration.
        return 40000 + static_cast<int> (currentProcessId() % 20000u);
    }

    inline constexpr uint32_t homeLinkMagic = 0x484C4E4Bu; // "HLNK"
    inline constexpr uint8_t homeLinkVersion = 2;
    inline constexpr uint8_t packetHeartbeat = 1;
    inline constexpr uint8_t packetTrigger = 2;

    #pragma pack(push, 1)
    struct HomeLinkPacket
    {
        uint32_t magic = homeLinkMagic;
        uint8_t version = homeLinkVersion;
        uint8_t type = packetHeartbeat;
        uint8_t link = 0;
        uint8_t source = 0;
        uint16_t velocity = 127;
        int64_t absoluteSample = -1;
        uint64_t senderSequence = 0;
    };
    #pragma pack(pop)

    static_assert (sizeof (HomeLinkPacket) == 26, "Unexpected Home-Link packet packing");

    struct HomeLinkEvent
    {
        uint64_t sequence = 0; // Receiver-local sequence.
        int64_t absoluteSample = -1;
        uint16_t velocity = 127;
        uint8_t source = 0;
    };

    class HomeLinkSender : private juce::Thread
    {
    public:
        HomeLinkSender()
            : juce::Thread ("HomeSidechain-Link-Sender")
        {
        }

        ~HomeLinkSender() override
        {
            stop();
        }

        void start()
        {
            if (! isThreadRunning())
                startThread (juce::Thread::Priority::high);
        }

        void stop()
        {
            signalThreadShouldExit();
            queueReady.signal();
            stopThread (500);
        }

        void setLink (int link) noexcept
        {
            currentLink.store (juce::jlimit (0, numberOfLinks - 1, link), std::memory_order_relaxed);
        }

        void heartbeat (int link) noexcept
        {
            setLink (link);
        }

        void publishTrigger (int link, int velocity, int64_t absoluteSample, uint8_t source) noexcept
        {
            const auto safeLink = juce::jlimit (0, numberOfLinks - 1, link);
            const auto w = writeIndex.load (std::memory_order_relaxed);
            const auto r = readIndex.load (std::memory_order_acquire);

            if (w - r >= queueSize)
            {
                droppedCount.fetch_add (1, std::memory_order_relaxed);
                return;
            }

            auto& slot = queue[static_cast<size_t> (w % queueSize)];
            slot.magic = homeLinkMagic;
            slot.version = homeLinkVersion;
            slot.type = packetTrigger;
            slot.link = static_cast<uint8_t> (safeLink);
            slot.source = source;
            slot.velocity = static_cast<uint16_t> (juce::jlimit (1, 127, velocity));
            slot.absoluteSample = absoluteSample;
            slot.senderSequence = sequence.fetch_add (1, std::memory_order_relaxed) + 1;

            writeIndex.store (w + 1, std::memory_order_release);
            queueReady.signal();
        }

        // Compatibility helper for older code.
        void enqueueTrigger (int link, int velocity, int sampleOffset, uint64_t hostTicks) noexcept
        {
            juce::ignoreUnused (hostTicks);
            publishTrigger (link, velocity, static_cast<int64_t> (sampleOffset), 1);
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

        void sendPacket (const HomeLinkPacket& packet) noexcept
        {
            const auto bytesSent = socket.write ("127.0.0.1", homeLinkPort(), &packet, sizeof (packet));
            if (bytesSent != static_cast<int> (sizeof (packet)))
                droppedCount.fetch_add (1, std::memory_order_relaxed);
        }

        void run() override
        {
            // Bind an ephemeral local port for the sender. We never receive on it.
            socket.setEnablePortReuse (true);

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
                    heartbeat.link = static_cast<uint8_t> (currentLink.load (std::memory_order_relaxed));
                    heartbeat.senderSequence = sequence.fetch_add (1, std::memory_order_relaxed) + 1;
                    sendPacket (heartbeat);
                    lastHeartbeatMs = now;
                    sentAnything = true;
                }

                if (! sentAnything)
                    queueReady.wait (20);
            }

            socket.shutdown();
        }
    };

    class HomeLinkReceiverService : private juce::Thread
    {
    public:
        HomeLinkReceiverService()
            : juce::Thread ("HomeSidechain-Link-Receiver")
        {
            socket.setEnablePortReuse (true);
            const auto result = socket.bindToPort (homeLinkPort(), "127.0.0.1");
            socketBound.store (result, std::memory_order_release);
            startThread (juce::Thread::Priority::high);
        }

        ~HomeLinkReceiverService() override
        {
            signalThreadShouldExit();
            socket.shutdown();
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

        bool readEvent (int link, uint64_t sequenceNumber, HomeLinkEvent& result) const noexcept
        {
            if (sequenceNumber == 0)
                return false;

            const auto safeLink = juce::jlimit (0, numberOfLinks - 1, link);
            const auto& slot = rings[static_cast<size_t> (safeLink)][static_cast<size_t> (sequenceNumber % ringSize)];
            const auto published = slot.sequence.load (std::memory_order_acquire);

            if (published != sequenceNumber)
                return false;

            result = slot.event;
            return true;
        }

        uint32_t lastHeartbeatMs (int link) const noexcept
        {
            return heartbeatMs[static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link))]
                .load (std::memory_order_relaxed);
        }

        bool isListening (int) const noexcept
        {
            return socketBound.load (std::memory_order_acquire);
        }

        int totalTriggerCount (int link) const noexcept
        {
            return triggerCounts[static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, link))]
                .load (std::memory_order_relaxed);
        }

        uint64_t ringCapacity() const noexcept
        {
            return ringSize;
        }

    private:
        static constexpr size_t ringSize = 256;

        struct Slot
        {
            HomeLinkEvent event {};
            std::atomic<uint64_t> sequence { 0 };
        };

        std::array<std::array<Slot, ringSize>, numberOfLinks> rings {};
        std::array<std::atomic<uint64_t>, numberOfLinks> latestSeq {};
        std::array<std::atomic<uint32_t>, numberOfLinks> heartbeatMs {};
        std::array<std::atomic<int>, numberOfLinks> triggerCounts {};
        std::atomic<bool> socketBound { false };
        juce::DatagramSocket socket;

        void publish (const HomeLinkPacket& packet)
        {
            if (packet.link >= numberOfLinks)
                return;

            const auto link = static_cast<int> (packet.link);
            const auto index = static_cast<size_t> (link);

            if (packet.type == packetHeartbeat)
            {
                heartbeatMs[index].store (juce::Time::getMillisecondCounter(), std::memory_order_relaxed);
                return;
            }

            if (packet.type != packetTrigger)
                return;

            // Receiver-local sequencing avoids collisions when multiple Trigger
            // instances use the same Home-Link.
            const auto sequenceNumber = latestSeq[index].fetch_add (1, std::memory_order_relaxed) + 1;
            auto& slot = rings[index][static_cast<size_t> (sequenceNumber % ringSize)];

            slot.event.sequence = sequenceNumber;
            slot.event.absoluteSample = packet.absoluteSample;
            slot.event.velocity = static_cast<uint16_t> (juce::jlimit (1, 127, static_cast<int> (packet.velocity)));
            slot.event.source = packet.source;
            slot.sequence.store (sequenceNumber, std::memory_order_release);
            triggerCounts[index].fetch_add (1, std::memory_order_relaxed);
        }

        void run() override
        {
            std::array<uint8_t, 512> receiveBuffer {};

            while (! threadShouldExit())
            {
                if (socket.waitUntilReady (true, 1) > 0)
                {
                    const auto bytes = socket.read (receiveBuffer.data(), static_cast<int> (receiveBuffer.size()), false);

                    if (bytes == static_cast<int> (sizeof (HomeLinkPacket)))
                    {
                        HomeLinkPacket packet;
                        std::memcpy (&packet, receiveBuffer.data(), sizeof (packet));

                        if (packet.magic == homeLinkMagic && packet.version == homeLinkVersion)
                            publish (packet);
                    }
                }
                else
                {
                    wait (1);
                }
            }
        }
    };

    inline HomeLinkReceiverService& getHomeLinkReceiverService() noexcept
    {
        return HomeLinkReceiverService::instance();
    }
}
