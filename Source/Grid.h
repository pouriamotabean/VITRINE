#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Library.h"
#include "View.h"

// The cards. Lives inside a Viewport; its height follows the content.
class Grid : public juce::Component,
             public juce::DragAndDropTarget
{
public:
    explicit Grid (Library& lib);

    std::function<void (const juce::String&)> onToast;
    std::function<void (PluginEntry&, bool alternate)> onActivate;   // click on a card (alternate = Ctrl+click)
    std::function<void (const juce::String& id)> onTakePicture, onChoosePicture, onSetShortcut;

    void setView (const ViewSel& v, const juce::String& search);
    void setCardWidth (int w);
    void setColumns (int n);          // > 0: fixed number of columns (Sidecar), 0: from card width
    void setStatusText (const juce::String& s) { status = s; repaint(); }
    void rebuild();                                       // library changed
    void layoutFor (int width);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

private:
    struct Card { PluginEntry* entry; juce::Rectangle<int> bounds; int section; int indexInSection; };
    struct Section { juce::String title; bool star; std::vector<PluginEntry*> items; int y = 0; };

    int cardAt (juce::Point<int> p) const;
    juce::Rectangle<int> starArea (const Card& c) const;
    void cardMenu (Card& c);
    void paintCard (juce::Graphics& g, const Card& c, bool hot) const;
    juce::Image cardImage (const Card& c) const;

    Library& library;
    ViewSel view;
    juce::String searchText, status;
    int cardW = 210;
    int fixedCols = 0;
    int pad = 24;                  // side margin; smaller in narrow windows
    std::vector<Section> sections;
    std::vector<Card> cards;
    int hover = -1, dropBefore = -1, pressed = -1;
    bool dragStarted = false;
};
