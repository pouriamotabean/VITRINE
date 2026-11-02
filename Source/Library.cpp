#include "Library.h"
#include "Categories.h"

namespace
{
    juce::var entryToVar (const PluginEntry& e)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", e.id);
        o->setProperty ("name", e.name);
        o->setProperty ("maker", e.maker);
        o->setProperty ("format", e.format);
        o->setProperty ("path", e.path);
        o->setProperty ("vstCategory", e.vstCategory);
        o->setProperty ("autoCategory", e.autoCategory);
        o->setProperty ("userCategory", e.userCategory);
        o->setProperty ("uid", e.uid);
        o->setProperty ("instrument", e.isInstrument);
        o->setProperty ("favorite", e.favorite);
        o->setProperty ("hidden", e.hidden);
        o->setProperty ("shortcut", e.shortcut);
        o->setProperty ("hotMods", e.hotMods);
        o->setProperty ("hotKey", e.hotKey);
        o->setProperty ("noPicture", e.noPicture);
        o->setProperty ("fileTime", e.fileTime);
        o->setProperty ("lastUsed", e.lastUsed);
        o->setProperty ("added", e.added);
        o->setProperty ("useCount", e.useCount);
        return juce::var (o);
    }

    PluginEntry varToEntry (const juce::var& v)
    {
        PluginEntry e;
        e.id           = v["id"].toString();
        e.name         = v["name"].toString();
        e.maker        = v["maker"].toString();
        e.format       = v["format"].toString();
        e.path         = v["path"].toString();
        e.vstCategory  = v["vstCategory"].toString();
        e.autoCategory = v["autoCategory"].toString();
        e.userCategory = v["userCategory"].toString();
        e.uid          = v["uid"].toString();
        e.isInstrument = (bool) v["instrument"];
        e.favorite     = (bool) v["favorite"];
        e.hidden       = (bool) v["hidden"];
        e.shortcut     = v["shortcut"].toString();
        e.hotMods      = (int) v["hotMods"];
        e.hotKey       = (int) v["hotKey"];
        e.noPicture    = (bool) v["noPicture"];
        e.fileTime     = (juce::int64) v["fileTime"];
        e.lastUsed     = (juce::int64) v["lastUsed"];
        e.added        = (juce::int64) v["added"];
        e.useCount     = (int) v["useCount"];
        return e;
    }
}

Library::Library() {}

juce::File Library::dataFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Vitrine");
    f.createDirectory();
    return f;
}

void Library::load()
{
    const auto file = dataFolder().getChildFile ("library.json");
    if (! file.existsAsFile())
        return;

    const auto root = juce::JSON::parse (file);
    items.clear();
    deckList.clear();
    extraFolders.clear();

    if (auto* arr = root["plugins"].getArray())
        for (auto& v : *arr)
        {
            auto e = varToEntry (v);
            if (e.id.isNotEmpty())
                items.push_back (e);
        }

    if (auto* arr = root["decks"].getArray())
        for (auto& v : *arr)
        {
            Deck d;
            d.name = v["name"].toString();
            if (auto* ids = v["ids"].getArray())
                for (auto& i : *ids)
                    d.ids.add (i.toString());
            deckList.push_back (d);
        }

    if (root.hasProperty ("sidecarColumns"))
        sidecarColumns = juce::jlimit (1, 3, (int) root["sidecarColumns"]);

    if (root.hasProperty ("cardWidth"))
        cardWidth = juce::jlimit (100, 340, (int) root["cardWidth"]);

    if (auto* arr = root["extraFolders"].getArray())
        for (auto& v : *arr)
            extraFolders.add (v.toString());

    // re-run the guesser so improvements to it reach old libraries
    for (auto& e : items)
        e.autoCategory = Categories::guess (e.name, e.maker, e.vstCategory, e.isInstrument);

    sendChangeMessage();
}

void Library::saveSoon()
{
    saver.startTimer (600);
}

void Library::save()
{
    juce::Array<juce::var> ps, ds, fs;
    for (auto& e : items) ps.add (entryToVar (e));

    for (auto& d : deckList)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", d.name);
        juce::Array<juce::var> ids;
        for (auto& i : d.ids) ids.add (i);
        o->setProperty ("ids", ids);
        ds.add (juce::var (o));
    }

    for (auto& f : extraFolders) fs.add (f);

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", 1);
    root->setProperty ("plugins", ps);
    root->setProperty ("decks", ds);
    root->setProperty ("extraFolders", fs);
    root->setProperty ("cardWidth", cardWidth);
    root->setProperty ("sidecarColumns", sidecarColumns);

    const auto file = dataFolder().getChildFile ("library.json");
    const auto tmp  = file.getSiblingFile ("library.json.tmp");
    if (tmp.replaceWithText (juce::JSON::toString (juce::var (root))))
        tmp.moveFileTo (file);
}

PluginEntry* Library::find (const juce::String& id)
{
    for (auto& e : items)
        if (e.id == id)
            return &e;
    return nullptr;
}

void Library::upsert (const PluginEntry& s)
{
    if (auto* e = find (s.id))
    {
        // refresh what the plugin says about itself, keep everything the user set
        e->name = s.name;
        e->maker = s.maker;
        e->path = s.path;
        e->vstCategory = s.vstCategory;
        e->uid = s.uid;
        e->isInstrument = s.isInstrument;
        e->fileTime = s.fileTime;
        e->autoCategory = Categories::guess (e->name, e->maker, e->vstCategory, e->isInstrument);
    }
    else
    {
        auto n = s;
        n.autoCategory = Categories::guess (n.name, n.maker, n.vstCategory, n.isInstrument);
        n.added = juce::Time::currentTimeMillis();
        items.push_back (n);
    }

    sendChangeMessage();
    saveSoon();
}

void Library::removeMissing (const juce::StringArray& keepIds, const juce::String& format)
{
    const auto before = items.size();
    items.erase (std::remove_if (items.begin(), items.end(), [&] (const PluginEntry& e)
                 { return e.format == format && ! keepIds.contains (e.id); }), items.end());

    if (items.size() != before)
    {
        sendChangeMessage();
        saveSoon();
    }
}

int Library::addDeck (const juce::String& name)
{
    deckList.push_back ({ name, {} });
    sendChangeMessage();
    saveSoon();
    return (int) deckList.size() - 1;
}

void Library::renameDeck (int i, const juce::String& name)
{
    if (juce::isPositiveAndBelow (i, (int) deckList.size()) && name.trim().isNotEmpty())
    {
        deckList[(size_t) i].name = name.trim();
        sendChangeMessage();
        saveSoon();
    }
}

void Library::removeDeck (int i)
{
    if (juce::isPositiveAndBelow (i, (int) deckList.size()))
    {
        deckList.erase (deckList.begin() + i);
        sendChangeMessage();
        saveSoon();
    }
}

void Library::moveDeck (int from, int to)
{
    const int n = (int) deckList.size();
    if (! juce::isPositiveAndBelow (from, n))
        return;
    to = juce::jlimit (0, n - 1, to);
    if (from == to)
        return;

    auto d = deckList[(size_t) from];
    deckList.erase (deckList.begin() + from);
    deckList.insert (deckList.begin() + to, d);
    sendChangeMessage();
    saveSoon();
}

bool Library::addToDeck (int i, const juce::String& id, int insertBefore)
{
    if (! juce::isPositiveAndBelow (i, (int) deckList.size()))
        return false;

    auto& ids = deckList[(size_t) i].ids;
    const int existing = ids.indexOf (id);

    if (existing >= 0)
    {
        if (insertBefore < 0 || insertBefore == existing)
            return false;
        ids.remove (existing);
        if (insertBefore > existing) --insertBefore;
    }

    if (insertBefore < 0 || insertBefore > ids.size())
        ids.add (id);
    else
        ids.insert (insertBefore, id);

    sendChangeMessage();
    saveSoon();
    return true;
}

void Library::removeFromDeck (int i, const juce::String& id)
{
    if (juce::isPositiveAndBelow (i, (int) deckList.size()))
    {
        deckList[(size_t) i].ids.removeString (id);
        sendChangeMessage();
        saveSoon();
    }
}

juce::StringArray Library::decksContaining (const juce::String& id) const
{
    juce::StringArray r;
    for (auto& d : deckList)
        if (d.ids.contains (id))
            r.add (d.name);
    return r;
}

void Library::toggleFavorite (const juce::String& id)
{
    if (auto* e = find (id))
    {
        e->favorite = ! e->favorite;
        sendChangeMessage();
        saveSoon();
    }
}

void Library::setHidden (const juce::String& id, bool h)
{
    if (auto* e = find (id))
    {
        e->hidden = h;
        sendChangeMessage();
        saveSoon();
    }
}

void Library::setUserCategory (const juce::String& id, const juce::String& cat)
{
    if (auto* e = find (id))
    {
        e->userCategory = (cat == e->autoCategory) ? juce::String() : cat;
        sendChangeMessage();
        saveSoon();
    }
}

void Library::markUsed (const juce::String& id)
{
    if (auto* e = find (id))
    {
        e->lastUsed = juce::Time::currentTimeMillis();
        ++e->useCount;
        sendChangeMessage();
        saveSoon();
    }
}

juce::StringArray Library::allMakers() const
{
    juce::StringArray m;
    for (auto& e : items)
        if (e.maker.isNotEmpty())
            m.addIfNotAlreadyThere (e.maker);
    m.sortNatural();
    return m;
}

void Library::fillDemo()
{
    struct D { const char* n; const char* m; const char* c; bool inst; };
    const D demo[] = {
        { "Pro-Q 3", "FabFilter", "Fx|EQ", false },           { "Pro-Q 4", "FabFilter", "Fx|EQ", false },
        { "Nova", "Tokyo Dawn Labs", "Fx|EQ", false },         { "Frequency", "Steinberg", "Fx|EQ", false },
        { "Pultec EQP-1A", "Universal Audio", "Fx|EQ", false },
        { "CLA-76", "Waves", "Fx|Dynamics", false },           { "1176LN", "Universal Audio", "Fx|Dynamics", false },
        { "Bus Compressor", "SSL", "Fx|Dynamics", false },     { "Pro-C 2", "FabFilter", "Fx|Dynamics", false },
        { "RComp", "Waves", "Fx|Dynamics", false },            { "Pro-L 2", "FabFilter", "Fx|Dynamics", false },
        { "ValhallaVintageVerb", "Valhalla DSP", "Fx|Reverb", false }, { "Blackhole", "Eventide", "Fx|Reverb", false },
        { "Seventh Heaven", "LiquidSonics", "Fx|Reverb", false }, { "Raum", "Native Instruments", "Fx|Reverb", false },
        { "EchoBoy", "Soundtoys", "Fx|Delay", false },         { "Timeless 3", "FabFilter", "Fx|Delay", false },
        { "H-Delay", "Waves", "Fx|Delay", false },
        { "Decapitator", "Soundtoys", "Fx|Distortion", false }, { "Saturn 2", "FabFilter", "Fx|Distortion", false },
        { "Kramer Master Tape", "Waves", "Fx", false },        { "Little AlterBoy", "Soundtoys", "Fx|Pitch Shift", false },
        { "Auto-Tune Pro", "Antares", "Fx|Pitch Shift", false }, { "MicroShift", "Soundtoys", "Fx|Modulation", false },
        { "Soothe2", "oeksound", "Fx|Restoration", false },    { "Suno Chain", "Pouria Motabean", "Fx", false },
        { "Serum 2", "Xfer Records", "Instrument|Synth", true }, { "Vital", "Vital Audio", "Instrument|Synth", true },
        { "Omnisphere", "Spectrasonics", "Instrument|Synth", true }, { "Kontakt 8", "Native Instruments", "Instrument|Sampler", true },
        { "Keyscape", "Spectrasonics", "Instrument|Piano", true }, { "Groove Agent SE", "Steinberg", "Instrument|Drum", true },
        { "Ozone 11", "iZotope", "Fx|Mastering", false },       { "Neutron 4", "iZotope", "Fx", false },
    };

    items.clear();
    int k = 0;
    for (auto& d : demo)
    {
        PluginEntry e;
        e.name = d.n; e.maker = d.m; e.format = "VST3"; e.vstCategory = d.c; e.isInstrument = d.inst;
        e.id = "VST3|demo|" + e.name;
        e.autoCategory = Categories::guess (e.name, e.maker, e.vstCategory, e.isInstrument);
        e.favorite = (k % 6 == 0) || e.name == "CLA-76" || e.name == "Auto-Tune Pro";
        e.lastUsed = (k % 4 == 1) ? juce::Time::currentTimeMillis() - k * 3600000LL : 0;
        e.useCount = (k * 7) % 11;
        items.push_back (e);
        ++k;
    }

    deckList = { { "Vocal", { "VST3|demo|Auto-Tune Pro", "VST3|demo|MicroShift", "VST3|demo|CLA-76", "VST3|demo|Pro-Q 3", "VST3|demo|Soothe2", "VST3|demo|Suno Chain" } },
                 { "Vocal Special FX", { "VST3|demo|Little AlterBoy", "VST3|demo|EchoBoy", "VST3|demo|Decapitator", "VST3|demo|Blackhole" } },
                 { "Saturation", { "VST3|demo|Decapitator", "VST3|demo|Saturn 2", "VST3|demo|Kramer Master Tape" } },
                 { "Mix Bus", { "VST3|demo|Bus Compressor", "VST3|demo|Pultec EQP-1A", "VST3|demo|Ozone 11" } },
                 { "Pianos", { "VST3|demo|Keyscape", "VST3|demo|Kontakt 8" } } };
    sendChangeMessage();
}
