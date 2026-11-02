#include "MainComponent.h"
#include "Categories.h"
#include "Filter.h"

namespace
{
    constexpr int topH = 64, compactTopH = 108, sideW = 236, pillW = 196, pillRightGap = 392;
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

    sidecarButton.setTooltip ("Narrow window on the right edge of the screen, always on top of Cubase");
    sidecarButton.setClickingTogglesState (false);
    sidecarButton.onClick = [this] { toggleSidecar(); };
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
    sizeSlider.setRange (150, 340, 1);
    sizeSlider.setValue (210, juce::dontSendNotification);
    sizeSlider.setTooltip ("Card size");
    sizeSlider.onValueChange = [this] { if (! isCompact()) grid.setCardWidth ((int) sizeSlider.getValue()); };
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
    grid.onActivate = [this] (PluginEntry& e)
    {
        showToast (shownLoadHint ? e.name
                                 : e.name + "  -  loading into Cubase comes with the Cubase link (next version)", 3600);
        shownLoadHint = true;
    };

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

    if (! sidecar)
    {
        boundsBeforeSidecar = w->getBounds();
        auto area = juce::Desktop::getInstance().getDisplays().getDisplayForRect (boundsBeforeSidecar)->userArea;
        w->setBounds (area.getRight() - 440, area.getY(), 440, area.getHeight());
        w->setAlwaysOnTop (true);
    }
    else
    {
        w->setAlwaysOnTop (false);
        if (! boundsBeforeSidecar.isEmpty())
            w->setBounds (boundsBeforeSidecar);
    }

    sidecar = ! sidecar;
    sidecarButton.setToggleState (sidecar, juce::dontSendNotification);
    sidecarButton.setButtonText (sidecar ? "Wide" : "Sidecar");
}

void MainComponent::showMoreMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Rescan plugins");
    m.addItem (2, "Add a plugin folder...");
    m.addItem (3, "New deck");
    juce::Component::SafePointer<MainComponent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&moreButton), [safe] (int r)
    {
        if (safe == nullptr) return;
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
        if (scanText.isNotEmpty())
        {
            g.setColour (Theme::accent.withAlpha (0.9f));
            g.setFont (Theme::font (11.0f));
            g.drawText (scanText, 16, compactTopH - 12, getWidth() - 32, 12, juce::Justification::centredLeft, true);
        }
        return;
    }

    // top bar
    g.setColour (Theme::text);
    g.setFont (Theme::font (19.0f, true).withExtraKerningFactor (0.22f));
    g.drawText ("VITRINE", 26, 0, sideW - 26, topH, juce::Justification::centredLeft);


    // Cubase link pill (session 2)
    auto pill = juce::Rectangle<float> ((float) (getWidth() - pillRightGap - pillW), 16.0f, (float) pillW, 32.0f);
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (pill, 16.0f);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (pill.reduced (0.5f), 16.0f, 1.0f);
    g.setColour (Theme::faint);
    g.fillEllipse (pill.getX() + 14.0f, pill.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour (Theme::dim);
    g.setFont (Theme::font (13.0f));
    g.drawText ("Cubase link: next version", pill.withTrimmedLeft (30.0f), juce::Justification::centredLeft);

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

    if (compact)
    {
        // narrow: search + buttons on one row, a view picker below, no sidebar
        sidecarButton.setBounds (w - 16 - 72, 14, 72, 34);
        moreButton.setBounds (w - 16 - 72 - 8 - 40, 14, 40, 34);
        search.setBounds (16, 13, moreButton.getX() - 8 - 16, 36);
        viewBox.setBounds (16, 60, w - 32, 34);

        gridView.setBounds (0, compactTopH + 1, w, h - compactTopH - 1);
        grid.setCardWidth (150);
        grid.layoutFor (gridView.getMaximumVisibleWidth());
        return;
    }

    sizeSlider.setBounds    (w - 74,  16, 60, 32);   // "SIZE" caption sits left of it
    rescanButton.setBounds  (w - 196, 16, 76, 32);
    foldersButton.setBounds (w - 278, 16, 76, 32);
    sidecarButton.setBounds (w - 370, 16, 84, 32);

    const int searchX = sideW + 24;
    search.setBounds (searchX, 14, juce::jmax (180, w - pillRightGap - pillW - 16 - searchX), 36);

    sideView.setBounds (0, topH + 1, sideW, h - topH - 1);
    sidebar.setSize (sideW, juce::jmax (sidebar.getIdealHeight(), sideView.getHeight()));
    sidebar.refresh();

    gridView.setBounds (sideW + 1, topH + 1, w - sideW - 1, h - topH - 1);
    grid.setCardWidth ((int) sizeSlider.getValue());
    grid.layoutFor (gridView.getMaximumVisibleWidth());
}
