#pragma once

#include <JuceHeader.h>

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
}
