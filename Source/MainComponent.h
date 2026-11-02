#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Library.h"
#include "Scanner.h"
#include "Sidebar.h"
#include "Grid.h"
#include "Theme.h"

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

    void showToast (const juce::String& s, int ms = 3200);
    void startScan();

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void applyView();
    void chooseFolder();

    Library& library;
    Scanner scanner;

    Sidebar sidebar;
    juce::Viewport sideView;
    Grid grid;
    juce::Viewport gridView;

    juce::TextEditor search;
    juce::TextButton rescanButton { "Rescan" }, foldersButton { "Folders" };
    juce::Slider sizeSlider;

    ViewSel view;
    juce::String toastText;
    juce::uint32 toastUntil = 0;
    juce::String scanText;
    bool shownLoadHint = false;
    std::unique_ptr<juce::FileChooser> chooser;
};
