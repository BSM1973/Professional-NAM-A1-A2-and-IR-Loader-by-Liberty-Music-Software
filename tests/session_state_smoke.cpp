#include <JuceHeader.h>
#include "SessionStateCodec.h"
#include <iostream>

int main()
{
    const juce::Identifier rootType("LNM_SNAPSHOT");
    juce::ValueTree root(rootType);
    root.setProperty("inputGain", 7.0, nullptr);
    libertySession::Snapshot snapshot;
    snapshot.paths = {"/tests/JCM800.nam", "/tests/Plexi_A2.nam",
                      "/tests/V30_A.wav", "/tests/V30_B.wav"};
    snapshot.visualFamilies = {2, -1};
    snapshot.photos = {"/photos/MyAmplifier.png", juce::String()};
    libertySession::write(root, snapshot);
    auto xml = root.createXml();
    if (!xml) return 1;
    auto restoredTree = juce::ValueTree::fromXml(*xml);
    libertySession::Snapshot restored;
    if (!libertySession::read(restoredTree, rootType, restored)) return 2;
    if (restored.paths != snapshot.paths || restored.visualFamilies != snapshot.visualFamilies
        || restored.photos != snapshot.photos || !restored.visualsPresent) return 3;
    if ((double)restoredTree.getProperty("inputGain") != 7.0) return 4;
    if (libertySession::read(restoredTree, juce::Identifier("ANOTHER_PLUGIN"), restored)) return 5;
    juce::ValueTree legacy(rootType);
    if (!libertySession::read(legacy, rootType, restored) || restored.visualsPresent) return 6;
    for (const auto& file : restored.paths) if (file.isNotEmpty()) return 7;
    std::cout << "Session codec: 4 paths, 2 visual families, photos, old preset and invalid type OK\n";
    return 0;
}
