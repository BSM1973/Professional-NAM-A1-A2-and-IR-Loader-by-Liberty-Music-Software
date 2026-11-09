#pragma once
#include <JuceHeader.h>
// Never give non-ASCII UTF-8 literals directly to JUCE's implicit
// const-char* conversion: it causes garbled French labels on macOS.
namespace libertyText
{
inline juce::String u8(const char* bytes)
{
    return juce::String::fromUTF8(bytes);
}
}
