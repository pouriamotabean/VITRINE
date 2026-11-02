#include "Sidebar.h"
#include "Theme.h"
#include "Filter.h"
#include "Categories.h"

namespace
{
    constexpr int rowH = 30, headerH = 36, padX = 18;
    constexpr int makersShown = 8;
}

Sidebar::Sidebar (Library& lib) : library (lib)
{
    rebuild();
}

void Sidebar::refresh()
{
    rebuild();
    repaint();
}

void Sidebar::rebuild()
{
    rows.clear();
    auto add = [this] (Row r) { rows.push_back (r); };

    auto count = [this] (const ViewSel& v) { return (int) Filter::pluginsFor (library, v).size(); };

    add ({ Row::header, "LIBRARY" });
    add ({ Row::item, "All plugins", count ({}), {} });
    { Row r { Row::item, "Favorites", count ({ ViewSel::favorites }), { ViewSel::favorites } }; r.star = true; add (r); }
    add ({ Row::item, "Recent", count ({ ViewSel::recent }), { ViewSel::recent } });
    add ({ Row::item, "Not used lately", count ({ ViewSel::forgotten }), { ViewSel::forgotten } });
    if (const int nh = count ({ ViewSel::hidden }); nh > 0)
        add ({ Row::item, "Hidden", nh, { ViewSel::hidden } });

    add ({ Row::header, "MY DECKS" });
    for (int i = 0; i < (int) library.decks().size(); ++i)
    {
        ViewSel v; v.kind = ViewSel::deck; v.deckIndex = i;
        add ({ Row::item, library.decks()[(size_t) i].name, library.decks()[(size_t) i].ids.size(), v });
    }
    add ({ Row::addDeck, "New deck" });

    add ({ Row::header, "AUTO" });
    for (auto& c : Categories::all())
    {
        ViewSel v; v.kind = ViewSel::category; v.name = c;
        const int n = count (v);
        if (n > 0)
            add ({ Row::item, c, n, v });
    }

    // makers, most plugins first
    std::vector<std::pair<juce::String, int>> makers;
    for (auto& m : library.allMakers())
    {
        ViewSel v; v.kind = ViewSel::maker; v.name = m;
        if (const int n = count (v); n > 0)
            makers.push_back ({ m, n });
    }
    std::stable_sort (makers.begin(), makers.end(), [] (auto& a, auto& b) { return a.second > b.second; });

    if (! makers.empty())
    {
        add ({ Row::header, "MAKERS" });
        const int shown = makersExpanded ? (int) makers.size() : juce::jmin ((int) makers.size(), makersShown);
        for (int i = 0; i < shown; ++i)
        {
            ViewSel v; v.kind = ViewSel::maker; v.name = makers[(size_t) i].first;
            add ({ Row::item, makers[(size_t) i].first, makers[(size_t) i].second, v });
        }
        if ((int) makers.size() > makersShown)
        {
            ViewSel v; v.kind = ViewSel::maker; v.name = "\x01toggle";
            add ({ Row::item, makersExpanded ? "Show fewer" : "Show all " + juce::String ((int) makers.size()), -1, v });
        }
    }

    int y = 10;
    for (auto& r : rows)
    {
        const int h = r.type == Row::header ? headerH : rowH;
        r.bounds = { 0, y, getWidth(), h };
        y += h;
    }
    idealHeight = y + 16;
}

int Sidebar::rowAt (juce::Point<int> p) const
{
    for (int i = 0; i < (int) rows.size(); ++i)
        if (rows[(size_t) i].type != Row::header && rows[(size_t) i].bounds.withWidth (getWidth()).contains (p))
            return i;
    return -1;
}

void Sidebar::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);
    const int w = getWidth();

    for (int i = 0; i < (int) rows.size(); ++i)
    {
        auto& r = rows[(size_t) i];
        auto b = r.bounds.withWidth (w);

        if (r.type == Row::header)
        {
            if (i > 0)
            {
                g.setColour (Theme::line);
                g.drawHorizontalLine (b.getY() + 4, (float) padX, (float) (w - padX));
            }
            g.setColour (Theme::faint);
            g.setFont (Theme::font (11.5f, true));
            g.drawText (r.label, b.withTrimmedLeft (padX).withTrimmedTop (10), juce::Justification::centredLeft);
            continue;
        }

        const bool selected = r.type == Row::item && r.target == sel && r.target.name != "\x01toggle";
        const bool hot = i == hover || i == dropRow;

        if (selected || hot)
        {
            g.setColour (i == dropRow ? Theme::accent.withAlpha (0.16f) : (selected ? Theme::panelHi : Theme::panel));
            g.fillRoundedRectangle (b.reduced (8, 2).toFloat(), 6.0f);
        }
        if (selected)
        {
            g.setColour (Theme::accent);
            g.fillRoundedRectangle ((float) 8, (float) b.getY() + 7.0f, 3.0f, (float) b.getHeight() - 14.0f, 1.5f);
        }

        auto tb = b.withTrimmedLeft (padX + 4).withTrimmedRight (padX);

        if (r.type == Row::addDeck)
        {
            g.setColour (hot ? Theme::text : Theme::dim);
            g.setFont (Theme::font (14.5f));
            g.drawText ("+  " + r.label, tb, juce::Justification::centredLeft);
            continue;
        }

        if (r.star)
        {
            juce::Path star;
            star.addStar (juce::Point<float> ((float) tb.getX() + 6.0f, (float) tb.getCentreY()), 5, 3.0f, 7.0f, 0.0f);
            g.setColour (Theme::accent);
            g.fillPath (star);
            tb.removeFromLeft (20);
        }

        const bool toggle = r.target.name == "\x01toggle";
        g.setColour (selected ? Theme::text : (toggle ? Theme::faint : Theme::text.withAlpha (0.82f)));
        g.setFont (Theme::font (14.5f, selected));

        if (r.count >= 0)
        {
            g.setColour (Theme::faint);
            g.setFont (Theme::font (12.5f));
            g.drawText (juce::String (r.count), tb, juce::Justification::centredRight);
            tb.removeFromRight (30);
            g.setColour (selected ? Theme::text : Theme::text.withAlpha (0.82f));
            g.setFont (Theme::font (14.5f, selected));
        }
        g.drawText (r.label, tb, juce::Justification::centredLeft, true);
    }
}

void Sidebar::mouseMove (const juce::MouseEvent& e)
{
    const int h = rowAt (e.getPosition());
    if (h != hover) { hover = h; repaint(); }
}

void Sidebar::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

void Sidebar::mouseDown (const juce::MouseEvent& e)
{
    downPos = e.getPosition();
    dragStarted = false;
    const int i = rowAt (e.getPosition());
    if (i < 0)
        return;

    auto& r = rows[(size_t) i];

    if (e.mods.isPopupMenu())
    {
        if (r.type == Row::item && r.target.kind == ViewSel::deck)
            deckMenu (r.target.deckIndex);
        return;
    }
}

void Sidebar::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 6)
        return;

    const int i = rowAt (downPos);
    if (i < 0 || rows[(size_t) i].target.kind != ViewSel::deck || rows[(size_t) i].type != Row::item)
        return;

    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
    {
        dragStarted = true;
        auto b = rows[(size_t) i].bounds.withWidth (getWidth());
        juce::Image img (juce::Image::ARGB, b.getWidth(), b.getHeight(), true);
        {
            juce::Graphics g (img);
            g.setColour (Theme::panelHi.withAlpha (0.9f));
            g.fillRoundedRectangle (img.getBounds().toFloat().reduced (8, 2), 6.0f);
            g.setColour (Theme::text);
            g.setFont (Theme::font (14.5f));
            g.drawText (rows[(size_t) i].label, img.getBounds().withTrimmedLeft (padX + 4), juce::Justification::centredLeft);
        }
        c->startDragging (dragDeckPrefix + juce::String (rows[(size_t) i].target.deckIndex), this, juce::ScaledImage (img));
    }
}

void Sidebar::mouseUp (const juce::MouseEvent& e)
{
    if (dragStarted || e.mods.isPopupMenu() || e.getDistanceFromDragStart() > 5)
        return;

    const int i = rowAt (e.getPosition());
    if (i < 0)
        return;

    auto& r = rows[(size_t) i];
    if (r.type == Row::addDeck)
    {
        newDeck();
        return;
    }

    if (r.target.name == "\x01toggle")
    {
        makersExpanded = ! makersExpanded;
        refresh();
        if (auto* p = getParentComponent()) p->resized();
        return;
    }

    sel = r.target;
    repaint();
    if (onSelect) onSelect (sel);
}

void Sidebar::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int i = rowAt (e.getPosition());
    if (i >= 0 && rows[(size_t) i].target.kind == ViewSel::deck && rows[(size_t) i].type == Row::item)
        renameDeck (rows[(size_t) i].target.deckIndex);
}

void Sidebar::newDeck()
{
    auto* w = new juce::AlertWindow ("New deck", "Name it the way you think, e.g. \"Vocal Special FX\".", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", {}, {});
    w->addButton ("Create", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Sidebar> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (safe == nullptr || result != 1) return;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        const int idx = safe->library.addDeck (name);
        ViewSel v; v.kind = ViewSel::deck; v.deckIndex = idx;
        safe->sel = v;
        if (safe->onSelect) safe->onSelect (v);
        if (safe->onToast) safe->onToast ("Deck \"" + name + "\" created. Drag plugin cards onto it.");
    }), true);
}

void Sidebar::renameDeck (int d)
{
    if (! juce::isPositiveAndBelow (d, (int) library.decks().size()))
        return;

    auto* w = new juce::AlertWindow ("Rename deck", {}, juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", library.decks()[(size_t) d].name, {});
    w->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Sidebar> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, d] (int result)
    {
        if (safe != nullptr && result == 1)
            safe->library.renameDeck (d, w->getTextEditorContents ("name"));
    }), true);
}

void Sidebar::deckMenu (int d)
{
    const int n = (int) library.decks().size();
    juce::PopupMenu m;
    m.addItem (1, "Rename");
    m.addItem (2, "Move up", d > 0);
    m.addItem (3, "Move down", d < n - 1);
    m.addSeparator();
    m.addItem (4, "Delete deck (plugins stay installed)");

    juce::Component::SafePointer<Sidebar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, d] (int r)
    {
        if (safe == nullptr) return;
        auto& lib = safe->library;
        if (r == 1) safe->renameDeck (d);
        if (r == 2) lib.moveDeck (d, d - 1);
        if (r == 3) lib.moveDeck (d, d + 1);
        if (r == 4)
        {
            const auto name = lib.decks()[(size_t) d].name;
            lib.removeDeck (d);
            if (safe->sel.kind == ViewSel::deck)
            {
                safe->sel = {};
                if (safe->onSelect) safe->onSelect (safe->sel);
            }
            if (safe->onToast) safe->onToast ("Deck \"" + name + "\" deleted.");
        }
    });
}

//==============================================================================
bool Sidebar::isInterestedInDragSource (const SourceDetails& d)
{
    const auto s = d.description.toString();
    return s.startsWith (dragPluginPrefix) || s.startsWith (dragDeckPrefix);
}

void Sidebar::itemDragMove (const SourceDetails& d)
{
    int i = rowAt (d.localPosition.toInt());
    if (i >= 0 && ! (rows[(size_t) i].target.kind == ViewSel::deck && rows[(size_t) i].type == Row::item)
               && ! (rows[(size_t) i].type == Row::addDeck && d.description.toString().startsWith (dragPluginPrefix))
               && ! (rows[(size_t) i].target.kind == ViewSel::favorites && d.description.toString().startsWith (dragPluginPrefix)))
        i = -1;
    if (i != dropRow) { dropRow = i; repaint(); }
}

void Sidebar::itemDragExit (const SourceDetails&)
{
    dropRow = -1;
    repaint();
}

void Sidebar::itemDropped (const SourceDetails& d)
{
    const int i = dropRow;
    dropRow = -1;
    repaint();
    if (i < 0)
        return;

    const auto s = d.description.toString();
    auto& r = rows[(size_t) i];

    if (s.startsWith (dragDeckPrefix))
    {
        if (r.target.kind == ViewSel::deck)
        {
            const int from = s.fromFirstOccurrenceOf (":", false, false).getIntValue();
            library.moveDeck (from, r.target.deckIndex);
        }
        return;
    }

    const auto id = s.fromFirstOccurrenceOf (":", false, false);
    auto* e = library.find (id);
    if (e == nullptr)
        return;

    if (r.target.kind == ViewSel::favorites)
    {
        if (! e->favorite) library.toggleFavorite (id);
        if (onToast) onToast (e->name + " added to Favorites.");
    }
    else if (r.type == Row::addDeck)
    {
        const int idx = library.addDeck ("New deck");
        library.addToDeck (idx, id);
        renameDeck (idx);
    }
    else if (r.target.kind == ViewSel::deck)
    {
        const auto deckName = library.decks()[(size_t) r.target.deckIndex].name;
        if (library.addToDeck (r.target.deckIndex, id))
        {
            if (onToast) onToast (e->name + " added to " + deckName + ".");
        }
        else if (onToast) onToast (e->name + " is already in " + deckName + ".");
    }
}
