#include "MainComponent.h"
#include "Categories.h"
#include "Filter.h"
#include "WinHelpers.h"

namespace
{
    constexpr int topH = 64, compactTopH = 110, sideW = 236, pillW = 210, pillRightGap = 436;
}

MainComponent::MainComponent (Library& lib, bool scanNow)
    : library (lib), sidebar (lib), grid (lib)
{
    library.addChangeListener (this);

    sideView.setViewedComponent (&sidebar, false);
    sideView.setScrollBarsShown (true, false);
    sideView.setScrollBarThickness (10);
    addAndMakeVisible (sideView);

    gridView.setViewedComponent (&grid, false);
    gridView.setScrollBarsShown (true, false);
    gridView.setScrollBarThickness (12);
    gridView.setSingleStepSizes (40, 40);
    addAndMakeVisible (gridView);

    search.setTextToShowWhenEmpty ("Search plugins...   (a maker or part of a name)", Theme::faint);
    search.setFont (Theme::font (15.5f));
    search.setIndents (40, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.onTextChange = [this] { applyView(); };
    search.onEscapeKey  = [this] { search.clear(); applyView(); };
    addAndMakeVisible (search);

    rescanButton.setTooltip ("Look for new or updated plugins");
    rescanButton.onClick = [this] { startScan(); };
    addAndMakeVisible (rescanButton);

    foldersButton.setTooltip ("Add a folder where you keep VST3 plugins");
    foldersButton.onClick = [this] { chooseFolder(); };
    addAndMakeVisible (foldersButton);

    // One mode button. Its label is where it takes you: "Sidecar" in the big window, "Full" in the narrow one.
    sidecarButton.onClick = [this] { toggleSidecar(); };
    sidecarButton.setColour (juce::TextButton::textColourOffId, Theme::accent);
    addAndMakeVisible (sidecarButton);

    moreButton.setTooltip ("Rescan, folders, wide window");
    moreButton.onClick = [this] { showMoreMenu(); };
    addChildComponent (moreButton);

    viewBox.onChange = [this]
    {
        const int i = viewBox.getSelectedId() - 1;
        if (juce::isPositiveAndBelow (i, (int) viewChoices.size()))
        {
            view = viewChoices[(size_t) i];
            search.clear();
            applyView();
            gridView.setViewPosition (0, 0);
        }
    };
    addChildComponent (viewBox);

    sizeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    sizeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    sizeSlider.setRange (100, 340, 1);
    sizeSlider.setValue (library.cardWidth, juce::dontSendNotification);
    sizeSlider.setTooltip ("Card size");
    sizeSlider.onValueChange = [this]
    {
        library.cardWidth = (int) sizeSlider.getValue();
        library.saveSoon();
        grid.setCardWidth (library.cardWidth);
    };
    addAndMakeVisible (sizeSlider);

    sidebar.onSelect = [this] (const ViewSel& v)
    {
        view = v;
        search.clear();
        applyView();
        gridView.setViewPosition (0, 0);
    };
    sidebar.onToast = [this] (const juce::String& s) { showToast (s); };

    grid.onToast = [this] (const juce::String& s) { showToast (s); };
    grid.onActivate = [this] (PluginEntry& e, bool alt) { loadIntoCubase (e, alt); };

    link.onStateChange = [this] { repaint(); };
    link.onResult = [this] (bool ok, const juce::String& t) { showToast ((ok ? "" : "Cubase: ") + t, ok ? 2600 : 5200); };
    link.onDump = [this] (const juce::String& t)
    {
        const auto f = Library::dataFolder().getChildFile ("cubase-diagnostics.txt");
        f.replaceWithText (t);
        juce::SystemClipboard::copyTextToClipboard (t);
        showToast ("Cubase diagnostics copied to the clipboard (also saved in cubase-diagnostics.txt).", 5000);
    };

    if (! CubaseLink::scriptIsCurrent())
    {
        juce::String err;
        if (! CubaseLink::installScript (err))
            showToast ("Could not install the Cubase script: " + err, 6000);
    }

    setWantsKeyboardFocus (true);
    rebuildViewBox();
    applyView();
    startTimerHz (10);

    if (scanNow)
        startScan();
}

MainComponent::~MainComponent()
{
    library.removeChangeListener (this);
}

void MainComponent::startScan()
{
    if (scanner.isScanning())
        return;

    std::map<juce::String, Scanner::Known> cache;
    for (auto& e : library.plugins())
        if (e.format == "VST3")
        {
            auto& k = cache[e.path.toLowerCase()];
            k.fileTime = e.fileTime;
            k.ids.add (e.id);
        }

    scanner.start (std::move (cache), library.extraFolders);
    scanText = "Looking for plugins...";
    repaint();
}

void MainComponent::chooseFolder()
{
    chooser = std::make_unique<juce::FileChooser> ("Folder with VST3 plugins", juce::File(), "*");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (f.isDirectory())
        {
            library.extraFolders.addIfNotAlreadyThere (f.getFullPathName());
            library.saveSoon();
            showToast ("Added " + f.getFullPathName() + ". Scanning it now.");
            startScan();
        }
    });
}

void MainComponent::applyView()
{
    sidebar.setSelection (search.isEmpty() ? view : ViewSel { ViewSel::Kind (-1) });
    grid.setView (view, search.getText());

    for (int i = 0; i < (int) viewChoices.size(); ++i)
        if (viewChoices[(size_t) i] == view)
            viewBox.setSelectedId (i + 1, juce::dontSendNotification);
}

void MainComponent::rebuildViewBox()
{
    viewBox.clear (juce::dontSendNotification);
    viewChoices.clear();

    auto add = [this] (const juce::String& label, const ViewSel& v)
    {
        viewChoices.push_back (v);
        viewBox.addItem (label, (int) viewChoices.size());
    };

    add ("All plugins", {});
    add ("Favorites", { ViewSel::favorites });
    add ("Recent", { ViewSel::recent });
    add ("Not used lately", { ViewSel::forgotten });

    if (! library.decks().empty())
    {
        viewBox.addSectionHeading ("My decks");
        for (int i = 0; i < (int) library.decks().size(); ++i)
        {
            ViewSel v; v.kind = ViewSel::deck; v.deckIndex = i;
            add (library.decks()[(size_t) i].name, v);
        }
    }

    viewBox.addSectionHeading ("Auto");
    for (auto& c : Categories::all())
    {
        ViewSel v; v.kind = ViewSel::category; v.name = c;
        if (! Filter::pluginsFor (library, v).empty())
            add (c, v);
    }

    for (int i = 0; i < (int) viewChoices.size(); ++i)
        if (viewChoices[(size_t) i] == view)
            viewBox.setSelectedId (i + 1, juce::dontSendNotification);
}

void MainComponent::toggleSidecar()
{
    auto* w = getTopLevelComponent();
    if (w == nullptr)
        return;

    if (! isCompact())
    {
        // -> Sidecar: about 310 real pixels wide, on the right edge of Cubase's window
        boundsBeforeSidecar = w->getBounds();
        auto target = WinHelpers::findCubaseWindow();
        auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (target.isEmpty() ? boundsBeforeSidecar : target);
        auto area = display->userArea;
        const int sidecarW = juce::roundToInt (310.0 / juce::jmax (1.0, display->scale));

        if (target.isEmpty() || target.getWidth() < 600)
            target = area;
        target = target.getIntersection (area);

        w->setBounds (target.getRight() - sidecarW, target.getY(), sidecarW, target.getHeight());
        w->setAlwaysOnTop (true);
    }
    else
    {
        // -> Full
        w->setAlwaysOnTop (false);
        auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (w->getBounds());
        auto area = display->userArea;
        auto b = boundsBeforeSidecar;
        if (b.isEmpty() || b.getWidth() < 900)
            b = area.withSizeKeepingCentre (juce::jmin (1500, area.getWidth() - 40), juce::jmin (940, area.getHeight() - 40));
        w->setBounds (b);
    }
}

juce::String MainComponent::linkText() const
{
    switch (link.getState())
    {
        case CubaseLink::State::noPorts:   return "Cubase: loopMIDI ports missing";
        case CubaseLink::State::waiting:   return "Cubase: waiting...";
        case CubaseLink::State::connected:
        {
            const auto t = link.getTrack();
            return t.isNotEmpty() ? "Cubase - " + t : "Cubase connected";
        }
    }
    return {};
}

juce::Colour MainComponent::linkColour() const
{
    switch (link.getState())
    {
        case CubaseLink::State::noPorts:   return juce::Colour (0xffe0503a);
        case CubaseLink::State::waiting:   return Theme::faint;
        case CubaseLink::State::connected: return Theme::ok;
    }
    return Theme::faint;
}

void MainComponent::loadIntoCubase (PluginEntry& e, bool alternate)
{
    const auto state = link.getState();
    if (state == CubaseLink::State::noPorts)
    {
        showToast ("loopMIDI ports not found: open loopMIDI and press + twice (or click the Cubase status for help).", 5600);
        return;
    }
    if (state == CubaseLink::State::waiting)
    {
        showToast ("Cubase is not answering. Is Cubase open with a project? Click the Cubase status for help.", 5200);
        return;
    }
    const bool instrument = e.isInstrument || Categories::isInstrumentCategory (e.category());
    // instruments get their own new track; Ctrl+click swaps the instrument on the selected track
    const auto target = ! instrument ? CubaseLink::Target::insert
                                     : (alternate ? CubaseLink::Target::instrumentSlot : CubaseLink::Target::newInstrumentTrack);
    link.load (e.name, e.maker, target);
    library.markUsed (e.id);
    showToast ("Loading " + e.name + "...", 1500);
}

void MainComponent::showLinkMenu()
{
    juce::PopupMenu m;
    const auto state = link.getState();

    if (state == CubaseLink::State::noPorts)
        m.addSectionHeader ("loopMIDI ports not found (" + link.missingPorts() + " port missing)");
    else if (state == CubaseLink::State::waiting)
        m.addSectionHeader ("Ports OK. Waiting for the script in Cubase.");
    else
        m.addSectionHeader ("Connected to Cubase");

    m.addItem (1, "How to connect (step by step)");
    m.addItem (2, "Get loopMIDI (free)");
    m.addItem (3, CubaseLink::scriptIsCurrent() ? "Re-install the Cubase script" : "Install the Cubase script");
    m.addItem (4, "Show the script folder");
    m.addSeparator();
    m.addItem (5, "Copy diagnostics for Claude", state == CubaseLink::State::connected);

    juce::Component::SafePointer<MainComponent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (pillBounds)), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == 1)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "Connect Vitrine to Cubase",
                "1.  Install loopMIDI, open it and press the + button TWICE.\n"
                "     You get \"loopMIDI Port\" and \"loopMIDI Port 1\". The names do not matter,\n"
                "     Vitrine uses these two. (Ports named \"Vitrine\" and \"Vitrine Back\" work too.)\n"
                "     Tip: in loopMIDI's tray menu, tick \"Autostart loopMIDI\".\n\n"
                "2.  Vitrine has put its script in Cubase's MIDI Remote folder.\n"
                "     Restart Cubase, or in Cubase: Studio > MIDI Remote Manager > Scripts > Reload Scripts.\n\n"
                "3.  Cubase finds the \"Vitrine\" remote by itself. If not: MIDI Remote tab (lower zone) > +,\n"
                "     choose PouriaMotabean / Vitrine, input \"loopMIDI Port\", output \"loopMIDI Port 1\".\n\n"
                "4.  The status at the top turns green and shows the selected track.\n"
                "     Select a track in Cubase, click a card: the plugin goes into the first free insert.\n"
                "     Instruments get a new instrument track of their own.\n"
                "     Ctrl+click an instrument to swap it into the selected instrument track instead.\n\n"
                "Cubase 15.0.20 or newer is needed.");
        }
        if (r == 2) juce::URL ("https://www.tobias-erichsen.de/software/loopmidi.html").launchInDefaultBrowser();
        if (r == 3)
        {
            juce::String err;
            if (CubaseLink::installScript (err)) safe->showToast ("Script installed. In Cubase: MIDI Remote Manager > Scripts > Reload Scripts.", 5200);
            else safe->showToast ("Could not install: " + err, 6000);
        }
        if (r == 4) CubaseLink::scriptFolder().getChildFile ("PouriaMotabean_Vitrine.js").revealToUser();
        if (r == 5) safe->link.requestDump();
    });
}

void MainComponent::mouseUp (const juce::MouseEvent& e)
{
    if (pillBounds.contains (e.getPosition()))
        showLinkMenu();
}

void MainComponent::showMoreMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Rescan plugins");
    m.addItem (2, "Add a plugin folder...");
    m.addItem (3, "New deck");
    juce::PopupMenu sizes;
    for (auto [label, v] : { std::pair<const char*, int> { "Tiny", 110 }, { "Small", 150 }, { "Medium", 200 }, { "Large", 260 }, { "Huge", 340 } })
        sizes.addItem (1000 + v, label, true, std::abs (library.cardWidth - v) < 20);
    m.addSubMenu ("Card size", sizes);
    juce::Component::SafePointer<MainComponent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&moreButton), [safe] (int r)
    {
        if (safe == nullptr) return;
        if (r > 1000) safe->sizeSlider.setValue (r - 1000);
        if (r == 1) safe->startScan();
        if (r == 2) safe->chooseFolder();
        if (r == 3) safe->sidebar.newDeck();
    });
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Library changed (scan results, decks, favourites...)
    if (view.kind == ViewSel::deck && ! juce::isPositiveAndBelow (view.deckIndex, (int) library.decks().size()))
        view = {};

    const auto pos = gridView.getViewPosition();
    rebuildViewBox();
    sidebar.refresh();
    sidebar.setSize (sideW, juce::jmax (sidebar.getIdealHeight(), sideView.getHeight()));
    applyView();
    gridView.setViewPosition (pos);
}

void MainComponent::timerCallback()
{
    auto found = scanner.takeResults();
    for (auto& e : found)
        library.upsert (e);

    juce::StringArray v3, v2;
    if (scanner.takeFinalIds (v3, v2))
    {
        library.removeMissing (v3, "VST3");
        library.removeMissing (v2, "VST2");
        library.save();
        scanText = {};
        showToast (juce::String ((int) library.plugins().size()) + " plugins in your library.");
    }

    if (scanner.isScanning())
    {
        const auto p = scanner.getProgress();
        auto t = "Scanning " + juce::String (p.done) + " / " + juce::String (p.total);
        if (p.current.isNotEmpty()) t << "  -  " << p.current;
        if (t != scanText) { scanText = t; repaint (0, 0, getWidth(), compactTopH + 1); }
        grid.setStatusText (library.plugins().empty() ? "Scanning your plugins... the first time takes a few minutes." : juce::String());
    }

    if (toastText.isNotEmpty() && juce::Time::getMillisecondCounter() > toastUntil)
    {
        toastText = {};
        repaint();
    }
}

void MainComponent::showToast (const juce::String& s, int ms)
{
    toastText = s;
    toastUntil = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
    repaint();
}

bool MainComponent::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress ('f', juce::ModifierKeys::commandModifier, 0))
    {
        search.grabKeyboardFocus();
        return true;
    }
    if (k == juce::KeyPress ('n', juce::ModifierKeys::commandModifier, 0))
    {
        sidebar.newDeck();
        return true;
    }
    if (k.getTextCharacter() >= ' ' && ! k.getModifiers().isCommandDown() && ! search.hasKeyboardFocus (true))
    {
        // start typing anywhere to search
        search.grabKeyboardFocus();
        search.insertTextAtCaret (juce::String::charToString (k.getTextCharacter()));
        return true;
    }
    return false;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    if (isCompact())
    {
        g.setColour (Theme::line);
        g.drawHorizontalLine (compactTopH, 0.0f, (float) getWidth());
        g.setFont (Theme::font (11.0f));
        if (scanText.isNotEmpty())
        {
            g.setColour (Theme::accent.withAlpha (0.9f));
            g.drawText (scanText, 12, compactTopH - 14, getWidth() - 24, 12, juce::Justification::centredLeft, true);
            pillBounds = {};
        }
        else
        {
            pillBounds = { 12, compactTopH - 14, getWidth() - 24, 13 };
            g.setColour (linkColour());
            g.fillEllipse (12.0f, (float) compactTopH - 10.0f, 6.0f, 6.0f);
            g.setColour (Theme::dim);
            g.drawText (linkText(), 24, compactTopH - 14, getWidth() - 36, 12, juce::Justification::centredLeft, true);
        }
        return;
    }

    // top bar
    g.setColour (Theme::text);
    g.setFont (Theme::font (19.0f, true).withExtraKerningFactor (0.22f));
    g.drawText ("VITRINE", 26, 0, sideW - 26, topH, juce::Justification::centredLeft);


    // Cubase link status (click for help)
    auto pill = juce::Rectangle<float> ((float) (getWidth() - pillRightGap - pillW), 16.0f, (float) pillW, 32.0f);
    pillBounds = pill.toNearestInt();
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (pill, 16.0f);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (pill.reduced (0.5f), 16.0f, 1.0f);
    g.setColour (linkColour());
    g.fillEllipse (pill.getX() + 14.0f, pill.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour (link.getState() == CubaseLink::State::connected ? Theme::text : Theme::dim);
    g.setFont (Theme::font (13.0f));
    g.drawText (linkText(), pill.withTrimmedLeft (30.0f).withTrimmedRight (10.0f), juce::Justification::centredLeft, true);

    // size slider caption
    g.setColour (Theme::faint);
    g.setFont (Theme::font (11.0f, true));
    g.drawText ("SIZE", sizeSlider.getX() - 34, sizeSlider.getY(), 30, sizeSlider.getHeight(), juce::Justification::centredRight);

    g.setColour (Theme::line);
    g.drawHorizontalLine (topH, 0.0f, (float) getWidth());
    g.drawVerticalLine (sideW, (float) topH, (float) getHeight());

    // scan status under the logo
    if (scanText.isNotEmpty())
    {
        g.setColour (Theme::accent.withAlpha (0.9f));
        g.setFont (Theme::font (11.5f));
        g.drawText (scanText, 26, topH - 18, sideW - 30, 14, juce::Justification::centredLeft, true);
    }
}

void MainComponent::paintOverChildren (juce::Graphics& g)
{
    // search icon (on top of the search field)
    {
        auto sb = search.getBounds().toFloat();
        const float cx = sb.getX() + 20.0f, cy = sb.getCentreY() - 1.0f;
        g.setColour (Theme::dim);
        g.drawEllipse (cx - 6.5f, cy - 6.5f, 13.0f, 13.0f, 1.6f);
        g.drawLine (cx + 4.5f, cy + 4.5f, cx + 9.0f, cy + 9.0f, 1.8f);
    }

    if (toastText.isEmpty())
        return;

    g.setFont (Theme::font (14.0f));
    const int w = juce::jmin (getWidth() - (isCompact() ? 32 : sideW + 60), juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), toastText) + 44);
    auto r = juce::Rectangle<float> ((float) (getWidth() - w - 28), (float) (getHeight() - 70), (float) w, 42.0f);
    g.setColour (Theme::panelHi);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
    g.setColour (Theme::accent);
    g.fillRoundedRectangle (r.getX() + 12.0f, r.getCentreY() - 7.0f, 3.0f, 14.0f, 1.5f);
    g.setColour (Theme::text);
    g.drawText (toastText, r.withTrimmedLeft (26.0f).withTrimmedRight (14.0f), juce::Justification::centredLeft, true);
}

void MainComponent::resized()
{
    const int w = getWidth(), h = getHeight();
    const bool compact = isCompact();

    sideView.setVisible (! compact);
    viewBox.setVisible (compact);
    moreButton.setVisible (compact);
    rescanButton.setVisible (! compact);
    foldersButton.setVisible (! compact);
    sizeSlider.setVisible (! compact);

    search.setTextToShowWhenEmpty (compact ? "Search plugins..." : "Search plugins...   (a maker or part of a name)", Theme::faint);
    sidecarButton.setButtonText (compact ? "Full" : "Sidecar");
    sidecarButton.setTooltip (compact ? "Back to the big window" : "Narrow window on the right edge of Cubase, always on top");
    if (! compact)
        if (auto* top = getTopLevelComponent())
            if (top->isAlwaysOnTop()) top->setAlwaysOnTop (false);

    if (compact)
    {
        // narrow: search on top, view picker + buttons below, no sidebar
        search.setBounds (12, 12, w - 24, 34);
        sidecarButton.setBounds (w - 12 - 52, 54, 52, 32);
        moreButton.setBounds (sidecarButton.getX() - 6 - 34, 54, 34, 32);
        viewBox.setBounds (12, 54, moreButton.getX() - 6 - 12, 32);

        gridView.setBounds (0, compactTopH + 1, w, h - compactTopH - 1);
        grid.setCardWidth (library.cardWidth);
        grid.layoutFor (gridView.getMaximumVisibleWidth());
        return;
    }

    sizeSlider.setBounds    (w - 74,  16, 60, 32);   // "SIZE" caption sits left of it
    rescanButton.setBounds  (w - 196, 16, 76, 32);
    foldersButton.setBounds (w - 278, 16, 76, 32);
    sidecarButton.setBounds (w - 362, 16, 76, 32);

    const int searchX = sideW + 24;
    search.setBounds (searchX, 14, juce::jmax (180, w - pillRightGap - pillW - 16 - searchX), 36);

    sideView.setBounds (0, topH + 1, sideW, h - topH - 1);
    sidebar.setSize (sideW, juce::jmax (sidebar.getIdealHeight(), sideView.getHeight()));
    sidebar.refresh();

    gridView.setBounds (sideW + 1, topH + 1, w - sideW - 1, h - topH - 1);
    grid.setCardWidth (library.cardWidth);
    grid.layoutFor (gridView.getMaximumVisibleWidth());
}
