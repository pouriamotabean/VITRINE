#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Library.h"

// Card pictures. A real screenshot of the plugin is used when one exists in
// <data>/thumbs (filled by the picture step in a later version); otherwise a
// calm placeholder: the category icon in the maker's colour.
namespace Thumbs
{
    juce::File fileFor (const PluginEntry& e);
    void draw (juce::Graphics& g, juce::Rectangle<float> r, const PluginEntry& e, float corner);
    bool hasPicture (const PluginEntry& e);
    void migrateOldFiles (const std::vector<PluginEntry>& items);   // pictures of version 0.4.0 -> new names

    // child-process entry point: open the plugin's window, photograph it, write a PNG
    int runChildShot (const juce::String& path, const juce::String& name, const juce::File& out);
}
