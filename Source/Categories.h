#pragma once
#include <juce_core/juce_core.h>

// Automatic categories. The plugin's own VST3 sub-categories come first,
// then words in its name and maker, then "Other".
namespace Categories
{
    const juce::StringArray& effects();      // EQ, Compressor, ...
    const juce::StringArray& instruments();  // Synth, Sampler, ...
    juce::StringArray all();                 // effects + instruments + Other
    bool isInstrumentCategory (const juce::String& c);

    juce::String guess (const juce::String& name, const juce::String& maker,
                        const juce::String& vstCategory, bool isInstrument);
}
