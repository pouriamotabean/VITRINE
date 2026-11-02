#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

// One plugin as Vitrine shows it.
struct PluginEntry
{
    juce::String id;            // stable key: format|file|name
    juce::String name;
    juce::String maker;
    juce::String format;        // "VST3" / "VST2"
    juce::String path;          // file or bundle
    juce::String vstCategory;   // the plugin's own sub-categories, e.g. "Fx|EQ"
    juce::String autoCategory;  // our guess (see Categories)
    juce::String userCategory;  // set by the user; wins over autoCategory
    juce::String uid;           // VST3 class id (for loading in Cubase later)
    bool isInstrument = false;
    bool favorite = false;
    bool hidden = false;        // hidden from Vitrine by the user (files untouched)
    juce::String shortcut;      // key badge text, e.g. "Ctrl+Alt+Q"
    int hotMods = 0, hotKey = 0; // system-wide shortcut
    bool noPicture = false;     // picture could not be taken (do not retry automatically)
    juce::int64 fileTime = 0;
    juce::int64 lastUsed = 0;   // ms since epoch, 0 = never
    juce::int64 added = 0;
    int useCount = 0;

    juce::StringArray extraCategories;   // also shown in these categories
    juce::StringArray hiddenFrom;        // hidden from these categories only

    juce::String category() const { return userCategory.isNotEmpty() ? userCategory : autoCategory; }   // main one
    juce::StringArray categories() const
    {
        juce::StringArray c;
        c.add (category());
        for (auto& x : extraCategories) c.addIfNotAlreadyThere (x);
        for (auto& h : hiddenFrom) c.removeString (h);
        return c;
    }
    bool inCategory (const juce::String& c) const { return categories().contains (c); }
};

struct Deck
{
    juce::String name;
    juce::StringArray ids;      // in the user's order
};

// All plugins + the user's decks. Lives on the message thread; saved as JSON.
class Library : public juce::ChangeBroadcaster
{
public:
    Library();

    static juce::File dataFolder();
    void load();
    void save();                         // debounced through saveSoon()
    void saveSoon();

    std::vector<PluginEntry>& plugins()  { return items; }
    const std::vector<PluginEntry>& plugins() const { return items; }
    std::vector<Deck>& decks()           { return deckList; }

    PluginEntry* find (const juce::String& id);
    void upsert (const PluginEntry& scanned);   // keeps the user's own fields
    void removeMissing (const juce::StringArray& keepIds, const juce::String& format);

    // decks
    int  addDeck (const juce::String& name);
    void renameDeck (int index, const juce::String& name);
    void removeDeck (int index);
    void moveDeck (int from, int to);
    bool addToDeck (int deck, const juce::String& id, int insertBefore = -1);
    void removeFromDeck (int deck, const juce::String& id);
    juce::StringArray decksContaining (const juce::String& id) const;

    // user state
    void toggleFavorite (const juce::String& id);
    void setHidden (const juce::String& id, bool hidden);
    void setUserCategory (const juce::String& id, const juce::String& cat);
    void moveToCategory (const juce::String& id, const juce::String& cat);     // only there
    void toggleExtraCategory (const juce::String& id, const juce::String& cat); // also there / not any more
    void hideFromCategory (const juce::String& id, const juce::String& cat);
    void markUsed (const juce::String& id);

    juce::StringArray allMakers() const;
    juce::StringArray extraFolders;      // extra VST3 search folders chosen by the user
    int cardWidth = 210;                 // card size in the big window
    int sidecarColumns = 2;              // columns in the narrow window
    bool cidPassDone = false;            // plugins were read once to get their class ids
    enum class Sort { name, leastUsed, mostUsed };
    Sort sort = Sort::name;

    // demo data for screenshots / first look without a scan
    void fillDemo();

private:
    struct Saver : juce::Timer { Library& l; explicit Saver (Library& o) : l (o) {} void timerCallback() override { stopTimer(); l.save(); } };

    std::vector<PluginEntry> items;
    std::vector<Deck> deckList;
    Saver saver { *this };
};
