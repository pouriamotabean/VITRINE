#pragma once
#include <juce_core/juce_core.h>

// What the grid is showing.
struct ViewSel
{
    enum Kind { all, favorites, recent, forgotten, deck, category, maker };
    Kind kind = all;
    int deckIndex = -1;         // for Kind::deck
    juce::String name;          // category or maker

    bool operator== (const ViewSel& o) const { return kind == o.kind && deckIndex == o.deckIndex && name == o.name; }
    bool operator!= (const ViewSel& o) const { return ! (*this == o); }
};

// Prefixes for drag-and-drop descriptions
inline const juce::String dragPluginPrefix = "plugin:";
inline const juce::String dragDeckPrefix   = "deck:";
