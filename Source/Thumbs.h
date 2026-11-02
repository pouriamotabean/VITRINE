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
}
