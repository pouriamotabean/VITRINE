#include "Grid.h"
#include "Theme.h"
#include "Thumbs.h"
#include "Filter.h"
#include "Categories.h"

namespace
{
    constexpr int pad = 24, gap = 16, headerH = 46, textH = 50, recentMax = 12;
    constexpr float corner = 10.0f;
}

Grid::Grid (Library& lib) : library (lib) {}

void Grid::setView (const ViewSel& v, const juce::String& search)
{
    view = v;
    searchText = search.trim();
    rebuild();
}

void Grid::setCardWidth (int w)
{
    cardW = juce::jlimit (140, 360, w);
    layoutFor (getWidth());
}

void Grid::rebuild()
{
    sections.clear();
    hover = -1;

    if (searchText.isNotEmpty())
    {
        const auto words = juce::StringArray::fromTokens (searchText.toLowerCase(), " ", "");
        Section s { "RESULTS", false, {} };
        for (auto* e : Filter::pluginsFor (library, {}))
            if (Filter::matchesSearch (*e, words))
                s.items.push_back (e);
        s.title = juce::String ((int) s.items.size()) + " RESULTS FOR \"" + searchText.toUpperCase() + "\"";
        sections.push_back (s);
    }
    else if (view.kind == ViewSel::all)
    {
        auto favs = Filter::pluginsFor (library, { ViewSel::favorites });
        if (! favs.empty())
            sections.push_back ({ "FAVORITES", true, favs });

        auto recent = Filter::pluginsFor (library, { ViewSel::recent });
        if (! recent.empty())
        {
            if ((int) recent.size() > recentMax) recent.resize ((size_t) recentMax);
            sections.push_back ({ "RECENT", false, recent });
        }

        for (auto& c : Categories::all())
        {
            ViewSel v; v.kind = ViewSel::category; v.name = c;
            auto items = Filter::pluginsFor (library, v);
            if (! items.empty())
                sections.push_back ({ c.toUpperCase(), false, items });
        }
    }
    else
    {
        juce::String title;
        switch (view.kind)
        {
            case ViewSel::favorites: title = "FAVORITES"; break;
            case ViewSel::recent:    title = "RECENT"; break;
            case ViewSel::forgotten: title = "NOT USED IN 30 DAYS"; break;
            case ViewSel::hidden:    title = "HIDDEN"; break;
            case ViewSel::deck:      title = juce::isPositiveAndBelow (view.deckIndex, (int) library.decks().size())
                                                 ? library.decks()[(size_t) view.deckIndex].name : juce::String(); break;
            case ViewSel::category:
            case ViewSel::maker:     title = view.name.toUpperCase(); break;
            case ViewSel::all:       break;
        }
        sections.push_back ({ title, view.kind == ViewSel::favorites, Filter::pluginsFor (library, view) });
    }

    layoutFor (getWidth());
}

void Grid::layoutFor (int width)
{
    cards.clear();
    if (width <= 0)
        return;

    const int avail = width - 2 * pad;
    const int cols = juce::jmax (1, (avail + gap) / (cardW + gap));
    const int cw = (avail - (cols - 1) * gap) / cols;
    const int ch = cw * 10 / 16 + textH;

    int y = 14;
    for (int si = 0; si < (int) sections.size(); ++si)
    {
        auto& s = sections[(size_t) si];
        s.y = y;
        y += headerH;

        if (s.items.empty())
            y += 90;   // room for the empty hint

        for (int i = 0; i < (int) s.items.size(); ++i)
        {
            const int c = i % cols, r = i / cols;
            cards.push_back ({ s.items[(size_t) i], { pad + c * (cw + gap), y + r * (ch + gap), cw, ch }, si, i });
        }

        if (! s.items.empty())
            y += (((int) s.items.size() + cols - 1) / cols) * (ch + gap);
        y += 10;
    }

    setSize (width, juce::jmax (y + pad, getParentHeight()));
    repaint();
}

//==============================================================================
juce::Rectangle<int> Grid::starArea (const Card& c) const
{
    return { c.bounds.getRight() - 34, c.bounds.getY() + 6, 28, 28 };
}

void Grid::paintCard (juce::Graphics& g, const Card& c, bool hot) const
{
    auto& e = *c.entry;
    auto b = c.bounds.toFloat();
    if (hot) b = b.translated (0.0f, -2.0f);

    g.setColour (hot ? Theme::panelHi : Theme::panel);
    g.fillRoundedRectangle (b, corner);

    auto thumb = b.reduced (7.0f).withHeight ((b.getWidth() - 14.0f) * 10.0f / 16.0f);
    Thumbs::draw (g, thumb, e, corner - 4.0f);

    // favourite star (hover shows an outline you can click)
    if (e.favorite || hot)
    {
        auto sa = starArea (c).toFloat().translated (0.0f, hot ? -2.0f : 0.0f);
        juce::Path star;
        star.addStar (sa.getCentre(), 5, 4.2f, 9.5f, 0.0f);
        if (e.favorite) { g.setColour (Theme::accent); g.fillPath (star); }
        else            { g.setColour (Theme::text.withAlpha (0.55f)); g.strokePath (star, juce::PathStrokeType (1.4f)); }
    }

    // keyboard shortcut badge
    if (e.shortcut.isNotEmpty())
    {
        auto kb = juce::Rectangle<float> (thumb.getX() + 6.0f, thumb.getY() + 6.0f, 24.0f, 22.0f);
        g.setColour (Theme::bg.withAlpha (0.85f));
        g.fillRoundedRectangle (kb, 5.0f);
        g.setColour (Theme::accent);
        g.drawRoundedRectangle (kb, 5.0f, 1.2f);
        g.setFont (Theme::font (12.5f, true));
        g.drawText (e.shortcut, kb, juce::Justification::centred);
    }

    // text
    auto tb = b.withTrimmedTop (thumb.getBottom() - b.getY() + 6.0f).reduced (11.0f, 0.0f).withHeight (textH - 10.0f);
    auto tag = tb.removeFromRight (40.0f).withSizeKeepingCentre (38.0f, 20.0f);
    g.setColour (Theme::line);
    g.fillRoundedRectangle (tag, 4.0f);
    g.setColour (Theme::dim);
    g.setFont (Theme::font (10.5f, true));
    g.drawText (e.format, tag, juce::Justification::centred);

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText (e.name, tb.removeFromTop (20.0f), juce::Justification::centredLeft, true);
    g.setColour (Theme::dim);
    g.setFont (Theme::font (12.5f));
    g.drawText (e.maker.isNotEmpty() ? e.maker : e.category(), tb.removeFromTop (18.0f), juce::Justification::centredLeft, true);

    if (hot)
    {
        g.setColour (Theme::accent.withAlpha (0.85f));
        g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.2f);
    }
}

void Grid::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    for (int si = 0; si < (int) sections.size(); ++si)
    {
        auto& s = sections[(size_t) si];
        auto hb = juce::Rectangle<int> (pad, s.y, getWidth() - 2 * pad, headerH);

        int x = hb.getX();
        if (s.star)
        {
            juce::Path star;
            star.addStar ({ (float) x + 8.0f, (float) hb.getCentreY() }, 5, 4.0f, 9.0f, 0.0f);
            g.setColour (Theme::accent);
            g.fillPath (star);
            x += 26;
        }
        g.setColour (Theme::text);
        g.setFont (Theme::font (16.0f, true));
        g.drawText (s.title, hb.withLeft (x), juce::Justification::centredLeft, true);

        if (! s.items.empty() || sections.size() > 1)
        {
            g.setColour (Theme::faint);
            g.setFont (Theme::font (12.5f));
            g.drawText (juce::String ((int) s.items.size()), hb, juce::Justification::centredRight);
        }

        if (s.items.empty())
        {
            juce::String hint;
            if (searchText.isNotEmpty())               hint = "Nothing matches. Try a maker or part of a name.";
            else if (view.kind == ViewSel::deck)       hint = "Empty deck. Drag plugin cards here, or onto the deck's name on the left.";
            else if (view.kind == ViewSel::favorites)  hint = "No favorites yet. Hover a card and click its star.";
            else if (view.kind == ViewSel::recent)     hint = "Plugins you load through Vitrine will show up here.";
            else                                       hint = status.isNotEmpty() ? status : "Nothing here yet.";

            auto box = juce::Rectangle<int> (pad, s.y + headerH, getWidth() - 2 * pad, 76);
            g.setColour (Theme::line);
            g.drawRoundedRectangle (box.toFloat().reduced (0.5f), corner, 1.0f);
            g.setColour (Theme::dim);
            g.setFont (Theme::font (14.0f));
            g.drawText (hint, box, juce::Justification::centred, true);
        }
    }

    const auto clip = g.getClipBounds();
    for (int i = 0; i < (int) cards.size(); ++i)
        if (cards[(size_t) i].bounds.expanded (4).intersects (clip) && i != hover)
            paintCard (g, cards[(size_t) i], false);

    if (juce::isPositiveAndBelow (hover, (int) cards.size()))
        paintCard (g, cards[(size_t) hover], true);

    // insertion marker while re-ordering a deck
    if (juce::isPositiveAndBelow (dropBefore, (int) cards.size() + 1) && ! cards.empty())
    {
        juce::Rectangle<int> r;
        if (dropBefore < (int) cards.size())
            r = cards[(size_t) dropBefore].bounds.withWidth (3).translated (-gap / 2 - 1, 0);
        else
            r = cards.back().bounds.withX (cards.back().bounds.getRight() + gap / 2 - 1).withWidth (3);
        g.setColour (Theme::accent);
        g.fillRoundedRectangle (r.toFloat(), 1.5f);
    }
}

int Grid::cardAt (juce::Point<int> p) const
{
    for (int i = 0; i < (int) cards.size(); ++i)
        if (cards[(size_t) i].bounds.contains (p))
            return i;
    return -1;
}

void Grid::mouseMove (const juce::MouseEvent& e)
{
    const int h = cardAt (e.getPosition());
    if (h != hover)
    {
        if (juce::isPositiveAndBelow (hover, (int) cards.size())) repaint (cards[(size_t) hover].bounds.expanded (6));
        hover = h;
        if (juce::isPositiveAndBelow (hover, (int) cards.size())) repaint (cards[(size_t) hover].bounds.expanded (6));
        setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }
}

void Grid::mouseExit (const juce::MouseEvent&)
{
    if (hover >= 0) { hover = -1; repaint(); }
}

void Grid::mouseDown (const juce::MouseEvent& e)
{
    pressed = cardAt (e.getPosition());
    dragStarted = false;

    if (pressed >= 0 && e.mods.isPopupMenu())
    {
        cardMenu (cards[(size_t) pressed]);
        pressed = -1;
    }
}

void Grid::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed < 0 || dragStarted || e.getDistanceFromDragStart() < 6)
        return;

    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
    {
        dragStarted = true;
        auto& card = cards[(size_t) pressed];
        c->startDragging (dragPluginPrefix + card.entry->id, this, juce::ScaledImage (cardImage (card)), true);
    }
}

void Grid::mouseUp (const juce::MouseEvent& e)
{
    const int i = pressed;
    pressed = -1;
    if (dragStarted || i < 0 || e.mods.isPopupMenu() || e.getDistanceFromDragStart() > 5)
        return;

    auto& c = cards[(size_t) i];
    if (starArea (c).contains (e.getPosition()))
    {
        library.toggleFavorite (c.entry->id);
        return;
    }

    if (onActivate) onActivate (*c.entry);
}

juce::Image Grid::cardImage (const Card& c) const
{
    const float scale = 1.0f;
    juce::Image img (juce::Image::ARGB, c.bounds.getWidth(), c.bounds.getHeight(), true);
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::translation ((float) -c.bounds.getX(), (float) -c.bounds.getY()).scaled (scale));
    paintCard (g, c, false);
    img.multiplyAllAlphas (0.85f);
    return img;
}

void Grid::cardMenu (Card& c)
{
    auto& e = *c.entry;
    const auto id = e.id;

    juce::PopupMenu m;
    m.addSectionHeader (e.name);
    m.addItem (1, e.favorite ? "Remove from Favorites" : "Add to Favorites");

    juce::PopupMenu decks;
    for (int i = 0; i < (int) library.decks().size(); ++i)
        decks.addItem (100 + i, library.decks()[(size_t) i].name, true, library.decks()[(size_t) i].ids.contains (id));
    if (! library.decks().empty()) decks.addSeparator();
    decks.addItem (99, "New deck...");
    m.addSubMenu ("Add to deck", decks);

    if (view.kind == ViewSel::deck && searchText.isEmpty())
        m.addItem (2, "Remove from this deck");

    juce::PopupMenu cats;
    const auto all = Categories::all();
    for (int i = 0; i < all.size(); ++i)
        cats.addItem (200 + i, all[i], true, e.category() == all[i]);
    if (e.userCategory.isNotEmpty())
    {
        cats.addSeparator();
        cats.addItem (3, "Back to automatic (" + e.autoCategory + ")");
    }
    m.addSubMenu ("Category", cats);

    m.addItem (4, "Show in folder", juce::File (e.path).exists());
    m.addItem (6, e.hidden ? "Show again" : "Hide from Vitrine");
    m.addSeparator();
    m.addItem (5, e.format + (e.vstCategory.isNotEmpty() ? "  -  " + e.vstCategory : juce::String()), false);

    juce::Component::SafePointer<Grid> safe (this);
    const int deckView = view.kind == ViewSel::deck ? view.deckIndex : -1;
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, id, all, deckView] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& lib = safe->library;
        auto* en = lib.find (id);
        if (en == nullptr) return;

        if (r == 1) lib.toggleFavorite (id);
        else if (r == 2 && deckView >= 0) lib.removeFromDeck (deckView, id);
        else if (r == 3) lib.setUserCategory (id, en->autoCategory);
        else if (r == 4) juce::File (en->path).revealToUser();
        else if (r == 6)
        {
            const bool hide = ! en->hidden;
            const auto name = en->name;
            lib.setHidden (id, hide);
            if (safe->onToast)
                safe->onToast (hide ? name + " hidden. Find it again under Hidden on the left."
                                    : name + " is back.");
        }
        else if (r == 99)
        {
            const int idx = lib.addDeck ("New deck");
            lib.addToDeck (idx, id);
            if (safe->onToast) safe->onToast ("New deck made with " + en->name + ". Double-click its name to rename it.");
        }
        else if (r >= 200 && r < 200 + all.size())
            lib.setUserCategory (id, all[r - 200]);
        else if (r >= 100 && r < 200)
        {
            const int d = r - 100;
            if (lib.decks()[(size_t) d].ids.contains (id)) lib.removeFromDeck (d, id);
            else lib.addToDeck (d, id);
        }
    });
}

//==============================================================================
bool Grid::isInterestedInDragSource (const SourceDetails& d)
{
    return view.kind == ViewSel::deck && searchText.isEmpty()
        && d.description.toString().startsWith (dragPluginPrefix);
}

void Grid::itemDragMove (const SourceDetails& d)
{
    const auto p = d.localPosition.toInt();
    int before = (int) cards.size();
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        auto b = cards[(size_t) i].bounds.expanded (gap / 2);
        if (b.contains (p))
        {
            before = p.x < b.getCentreX() ? i : i + 1;
            break;
        }
    }
    if (before != dropBefore) { dropBefore = before; repaint(); }
}

void Grid::itemDragExit (const SourceDetails&)
{
    dropBefore = -1;
    repaint();
}

void Grid::itemDropped (const SourceDetails& d)
{
    const int before = dropBefore;
    dropBefore = -1;
    const auto id = d.description.toString().fromFirstOccurrenceOf (":", false, false);
    if (view.kind == ViewSel::deck)
        library.addToDeck (view.deckIndex, id, before);
    repaint();
}
