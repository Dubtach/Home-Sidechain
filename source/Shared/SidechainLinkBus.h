#pragma once

#include <JuceHeader.h>
#include <array>

namespace homeSidechain
{
    struct LinkEvent
    {
        uint64_t samplePosition = 0;
        double timeSeconds = 0.0;
        float velocity = 1.0f;
        int link = 0;
        bool hasHostPosition = false;
    };

    class LinkBus
    {
    public:
        static constexpr uint32_t magic = 0x484C4E4B; // HLNK
        static constexpr uint32_t version = 1;
        static constexpr size_t headerSize = 4 + 4 + (sizeof (int64_t) * numberOfLinks);
        static constexpr size_t recordSize = 4 + 4 + 8 + 8 + 4 + 4;

        LinkBus()
        {
            file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                         .getChildFile ("Dubtach")
                         .getChildFile ("HomeSidechain-" + juce::String (juce::SystemStats::getProcessId()) + ".bin");
            file.getParentDirectory().createDirectory();
            lock = std::make_unique<juce::InterProcessLock> (
                "Dubtach_HomeSidechain_Lock_" + juce::String (juce::SystemStats::getProcessId()));
            initialiseFile();
        }

        ~LinkBus() = default;

        bool send (const LinkEvent& event)
        {
            const juce::ScopedLockType scoped (localLock);
            if (lock == nullptr)
                return false;

            juce::InterProcessLock::ScopedLockType busLock (*lock);
            if (! busLock.isLocked())
                return false;
            juce::FileOutputStream stream (file);
            if (! stream.openedOk())
                return false;

            if (! stream.setPosition (file.getSize()))
                return false;

            const uint32_t recordMagic = magic;
            const int32_t link = juce::jlimit (0, numberOfLinks - 1, event.link);
            const uint64_t sample = event.samplePosition;
            const double seconds = event.timeSeconds;
            const float velocity = event.velocity;
            const uint32_t flags = event.hasHostPosition ? 1u : 0u;

            stream.writeInt ((int) recordMagic);
            stream.writeInt ((int) link);
            stream.writeInt64 ((int64) sample);
            stream.writeDouble (seconds);
            stream.writeFloat (velocity);
            stream.writeInt ((int) flags);
            stream.flush();
            return true;
        }

        bool updateHeartbeat (int linkIndex)
        {
            if (lock == nullptr)
                return false;

            juce::InterProcessLock::ScopedLockType busLock (*lock);
            if (! busLock.isLocked())
                return false;
            if (! busLock.isLocked())
                return false;

            juce::FileOutputStream stream (file);
            if (! stream.openedOk())
                return false;

            const auto offset = 8 + (sizeof (int64_t) * static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, linkIndex)));
            if (! stream.setPosition ((int64) offset))
                return false;

            stream.writeInt64 (juce::Time::getCurrentTime().toMilliseconds());
            stream.flush();
            return true;
        }

        int64 heartbeatAgeMs (int linkIndex) const
        {
            if (lock == nullptr)
                return std::numeric_limits<int64>::max();

            juce::InterProcessLock::ScopedLockType busLock (*lock);
            if (! busLock.isLocked())
                return std::numeric_limits<int64>::max();
            if (! busLock.isLocked())
                return std::numeric_limits<int64>::max();

            juce::FileInputStream stream (file);
            if (! stream.openedOk() || file.getSize() < static_cast<int64> (headerSize))
                return std::numeric_limits<int64>::max();

            const auto offset = 8 + (sizeof (int64_t) * static_cast<size_t> (juce::jlimit (0, numberOfLinks - 1, linkIndex)));
            stream.setPosition ((int64) offset);
            const auto last = stream.readInt64();
            if (last <= 0)
                return std::numeric_limits<int64>::max();

            return juce::jmax<int64> (0, juce::Time::getCurrentTime().toMilliseconds() - last);
        }

        size_t getInitialCursor() const noexcept
        {
            return headerSize;
        }

        size_t readSince (size_t& cursor, LinkEvent* destination, int maxEvents, int wantedLink)
        {
            if (destination == nullptr || maxEvents <= 0 || lock == nullptr)
                return 0;

            juce::InterProcessLock::ScopedLockType busLock (*lock);
            if (! busLock.isLocked())
                return 0;

            juce::FileInputStream stream (file);
            if (! stream.openedOk())
                return 0;

            const auto fileSize = static_cast<size_t> (file.getSize());
            if (cursor < headerSize)
                cursor = headerSize;

            const auto available = fileSize > cursor ? (fileSize - cursor) : 0;
            const auto completeRecords = available / recordSize;
            size_t consumedRecords = 0;
            size_t outputCount = 0;

            while (consumedRecords < completeRecords)
            {
                stream.setPosition ((int64) (cursor + (consumedRecords * recordSize)));
                if (stream.readInt() != (int) magic)
                    break;

                const int eventLink = stream.readInt();
                const auto sample = static_cast<uint64_t> (stream.readInt64());
                const auto seconds = stream.readDouble();
                const auto velocity = stream.readFloat();
                const auto flags = static_cast<uint32_t> (stream.readInt());

                if (eventLink == wantedLink && outputCount < static_cast<size_t> (maxEvents))
                {
                    destination[outputCount++] = { sample, seconds, velocity, eventLink, (flags & 1u) != 0 };
                }

                ++consumedRecords;
                if (outputCount >= static_cast<size_t> (maxEvents))
                    break;
            }

            cursor += consumedRecords * recordSize;
            return outputCount;
        }

    private:
        void initialiseFile()
        {
            if (lock == nullptr)
                return;

            juce::InterProcessLock::ScopedLockType busLock (*lock);
            if (! busLock.isLocked())
                return;

            const auto minimumSize = static_cast<int64> (headerSize);
            if (! file.existsAsFile() || file.getSize() < minimumSize)
            {
                file.deleteFile();
                file.create();
                juce::FileOutputStream stream (file);
                if (! stream.openedOk())
                    return;

                stream.writeInt ((int) magic);
                stream.writeInt ((int) version);
                for (int i = 0; i < numberOfLinks; ++i)
                    stream.writeInt64 (0);
                stream.flush();
            }
            else
            {
                juce::FileInputStream stream (file);
                if (! stream.openedOk())
                    return;
                if (stream.readInt() != (int) magic || stream.readInt() != (int) version)
                {
                    stream.setPosition (0);
                    juce::FileOutputStream out (file);
                    if (out.openedOk())
                    {
                        out.writeInt ((int) magic);
                        out.writeInt ((int) version);
                        for (int i = 0; i < numberOfLinks; ++i)
                            out.writeInt64 (0);
                        out.flush();
                    }
                }
            }
        }

        juce::File file;
        std::unique_ptr<juce::InterProcessLock> lock;
        juce::CriticalSection localLock;
    };
}
