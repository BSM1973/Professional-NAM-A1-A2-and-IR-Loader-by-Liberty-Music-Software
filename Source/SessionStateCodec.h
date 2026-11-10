#pragma once
#include <JuceHeader.h>
#include <array>
#include <utility>

// Shared, host-agnostic schema used by both the plugin and its round-trip test.
// File paths refer to files on the user's disk; no samples or NAM weights are
// embedded in the DAW preset (and missing files are handled on restore).
namespace libertySession
{
struct Snapshot
{
    std::array<juce::String, 4> paths {};
    std::array<int, 2> visualFamilies {{ -1, -1 }};
    std::array<juce::String, 2> photos {};
    bool visualsPresent = false;
};

inline void write(juce::ValueTree& tree, const Snapshot& snapshot)
{
    for (int i=0; i<4; ++i)
        tree.setProperty("path" + juce::String(i), snapshot.paths[(size_t)i], nullptr);
    for (int i=0; i<2; ++i)
    {
        tree.setProperty("visualFamily" + juce::String(i), snapshot.visualFamilies[(size_t)i], nullptr);
        tree.setProperty("visualPhoto" + juce::String(i), snapshot.photos[(size_t)i], nullptr);
    }
}

inline bool read(const juce::ValueTree& tree, const juce::Identifier& requiredType, Snapshot& result)
{
    if (!tree.isValid() || !tree.hasType(requiredType)) return false;
    Snapshot next;
    for (int i=0; i<4; ++i)
        next.paths[(size_t)i] = tree.getProperty("path" + juce::String(i)).toString();
    next.visualsPresent = tree.hasProperty("visualFamily0") || tree.hasProperty("visualPhoto0");
    for (int i=0; i<2; ++i)
    {
        next.visualFamilies[(size_t)i] = (int)tree.getProperty("visualFamily" + juce::String(i), -1);
        next.photos[(size_t)i] = tree.getProperty("visualPhoto" + juce::String(i), {}).toString();
    }
    result = std::move(next);
    return true;
}
}
