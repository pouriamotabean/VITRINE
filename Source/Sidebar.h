#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Library.h"
#include "View.h"

// Left column: library views, the user's decks, automatic categories, makers.
class Sidebar : public juce::Component,
                public juce::DragAndDropTarget
{
public:
    explicit Sidebar (Library& lib);

    std::function<void (const ViewSel&)> onSelect;
    std::function<void (const juce::String&)> onToast;

    void setSelection (const ViewSel& s) { sel = s; repaint(); }
    void refresh();                         // library changed
    int  getIdealHeight() const { return idealHeight; }

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

    void newDeck();

private:
    struct Row
    {
        enum Type { header, item, addDeck } type;
        juce::String label;
        int count = -1;
        ViewSel target;
        bool star = false;
        juce::Rectangle<int> bounds;
    };

    void rebuild();
    int rowAt (juce::Point<int> p) const;
    void deckMenu (int deckIndex);
    void renameDeck (int deckIndex);

    Library& library;
    std::vector<Row> rows;
    ViewSel sel;
    int hover = -1, dropRow = -1, idealHeight = 400;
    bool makersExpanded = false;
    juce::Point<int> downPos;
    bool dragStarted = false;
};
