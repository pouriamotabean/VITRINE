#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
    const juce::Colour bg       { 0xff111214 };
    const juce::Colour panel    { 0xff1a1b1e };
    const juce::Colour panelHi  { 0xff222327 };
    const juce::Colour line     { 0xff2a2b30 };
    const juce::Colour text     { 0xffd9d6cf };
    const juce::Colour dim      { 0xff8b8983 };
    const juce::Colour faint    { 0xff5d5c58 };
    const juce::Colour accent   { 0xffff8a2a };
    const juce::Colour ok       { 0xff3ecf6e };

    juce::Font font (float size, bool bold = false);
    juce::Colour makerColour (const juce::String& maker);   // stable hue per maker

    // Draws a small vector icon for an automatic category.
    void drawCategoryIcon (juce::Graphics& g, const juce::String& category, juce::Rectangle<float> r, juce::Colour c);

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        juce::Font getPopupMenuFont() override;
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
        void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
        void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                            int thumbStart, int thumbSize, bool over, bool down) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        juce::Font getTextButtonFont (juce::TextButton&, int) override;
        juce::Font getAlertWindowMessageFont() override;
        juce::Font getAlertWindowTitleFont() override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
    };
}
