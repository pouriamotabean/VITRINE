#pragma once
#include "Library.h"
#include "View.h"

namespace Filter
{
    constexpr juce::int64 forgottenAfterMs = 30LL * 24 * 3600 * 1000;   // 30 days

    // Plugins in a view, in display order (decks keep the user's order).
    inline std::vector<PluginEntry*> pluginsFor (Library& lib, const ViewSel& v)
    {
        std::vector<PluginEntry*> r;
        auto& items = lib.plugins();
        const auto now = juce::Time::currentTimeMillis();

        if (v.kind == ViewSel::deck)
        {
            if (juce::isPositiveAndBelow (v.deckIndex, (int) lib.decks().size()))
                for (auto& id : lib.decks()[(size_t) v.deckIndex].ids)
                    if (auto* e = lib.find (id))
                        if (! e->hidden)
                            r.push_back (e);
            return r;
        }

        for (auto& e : items)
        {
            if (e.hidden != (v.kind == ViewSel::hidden))
                continue;

            bool ok = false;
            switch (v.kind)
            {
                case ViewSel::all:       ok = true; break;
                case ViewSel::favorites: ok = e.favorite; break;
                case ViewSel::recent:    ok = e.lastUsed > 0; break;
                case ViewSel::forgotten: ok = e.lastUsed == 0 || now - e.lastUsed > forgottenAfterMs; break;
                case ViewSel::category:  ok = e.inCategory (v.name); break;
                case ViewSel::maker:     ok = e.maker == v.name; break;
                case ViewSel::deck:      break;
                case ViewSel::hidden:    ok = true; break;
            }
            if (ok)
                r.push_back (&e);
        }

        if (v.kind == ViewSel::recent)
            std::sort (r.begin(), r.end(), [] (auto* a, auto* b) { return a->lastUsed > b->lastUsed; });
        else if (v.kind == ViewSel::forgotten)
            std::sort (r.begin(), r.end(), [] (auto* a, auto* b)
            {
                if (a->lastUsed != b->lastUsed) return a->lastUsed < b->lastUsed;
                return a->name.compareNatural (b->name) < 0;
            });
        else if (lib.sort == Library::Sort::leastUsed)
            std::stable_sort (r.begin(), r.end(), [] (auto* a, auto* b)
            {
                if (a->useCount != b->useCount) return a->useCount < b->useCount;
                return a->name.compareNatural (b->name) < 0;
            });
        else if (lib.sort == Library::Sort::mostUsed)
            std::stable_sort (r.begin(), r.end(), [] (auto* a, auto* b)
            {
                if (a->useCount != b->useCount) return a->useCount > b->useCount;
                return a->name.compareNatural (b->name) < 0;
            });
        else
            std::sort (r.begin(), r.end(), [] (auto* a, auto* b) { return a->name.compareNatural (b->name) < 0; });

        return r;
    }

    inline bool matchesSearch (const PluginEntry& e, const juce::StringArray& words)
    {
        const auto hay = (e.name + " " + e.maker + " " + e.categories().joinIntoString (" ")).toLowerCase();
        for (auto& w : words)
            if (! hay.contains (w))
                return false;
        return true;
    }
}
