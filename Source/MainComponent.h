#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Library.h"
#include "Scanner.h"
#include "Sidebar.h"
#include "Grid.h"
#include "Theme.h"
#include "CubaseLink.h"

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      private juce::ChangeListener,
                      private juce::Timer
{
public:
    MainComponent (Library& lib, bool startScan);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void showToast (const juce::String& s, int ms = 3200);
    void startScan();

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void applyView();
    void chooseFolder();
    void toggleSidecar();
    void rebuildViewBox();
    void showMoreMenu();
    void showLinkMenu();
    void loadIntoCubase (PluginEntry& e, bool alternate);

    // pictures
    void startPictures (juce::StringArray ids, bool askFirst);
    void stopPictures();
    void pictureTick();
    void choosePicture (const juce::String& id);
    void showMainMenu();

    // shortcuts
    void editShortcut (const juce::String& id);
    void registerShortcuts();
    juce::String linkText() const;
    juce::Colour linkColour() const;
    bool isCompact() const { return getWidth() < 640; }

    Library& library;
    Scanner scanner;
    CubaseLink link;
    juce::Rectangle<int> pillBounds;

    Sidebar sidebar;
    juce::Viewport sideView;
    Grid grid;
    juce::Viewport gridView;

    juce::TextEditor search;
    juce::TextButton rescanButton { "Rescan" }, foldersButton { "Folders" };
    juce::TextButton sidecarButton { "Sidecar" }, moreButton { "..." };
    juce::ComboBox viewBox;
    std::vector<ViewSel> viewChoices;
    juce::Rectangle<int> boundsBeforeSidecar;
    juce::Slider sizeSlider;

    ViewSel view;
    juce::String toastText;
    juce::uint32 toastUntil = 0;
    juce::String scanText;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::StringArray picQueue;
    int picTotal = 0, picDone = 0, picFailed = 0;
    std::unique_ptr<juce::ChildProcess> picProc;
    juce::String picId;
    juce::File picOut;
    juce::uint32 picStarted = 0;
    juce::String picText;
    std::vector<juce::String> hotkeyIds;      // index = hotkey id - 1
    juce::StringArray hotkeyFailed;
};
