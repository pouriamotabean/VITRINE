#include "Theme.h"

namespace Theme
{
    juce::Font font (float size, bool bold)
    {
        auto opts = juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain);
       #if JUCE_WINDOWS
        opts = opts.withName ("Segoe UI");
       #endif
        return juce::Font (opts);
    }

    juce::Colour makerColour (const juce::String& maker)
    {
        const auto h = (juce::uint32) maker.toLowerCase().hashCode();
        const float hue = (float) (h % 360u) / 360.0f;
        return juce::Colour::fromHSV (hue, 0.45f, 0.78f, 1.0f);
    }

    void drawCategoryIcon (juce::Graphics& g, const juce::String& c, juce::Rectangle<float> r, juce::Colour col)
    {
        const float s = juce::jmin (r.getWidth(), r.getHeight());
        const auto b = r.withSizeKeepingCentre (s, s);
        const float st = juce::jmax (1.5f, s * 0.045f);
        g.setColour (col);
        juce::Path p;

        auto px = [&] (float fx) { return b.getX() + fx * b.getWidth(); };
        auto py = [&] (float fy) { return b.getY() + fy * b.getHeight(); };

        if (c == "EQ")
        {
            p.startNewSubPath (px (0.05f), py (0.62f));
            p.cubicTo (px (0.25f), py (0.62f), px (0.25f), py (0.25f), px (0.40f), py (0.30f));
            p.cubicTo (px (0.55f), py (0.35f), px (0.55f), py (0.80f), px (0.70f), py (0.72f));
            p.cubicTo (px (0.82f), py (0.66f), px (0.85f), py (0.45f), px (0.95f), py (0.45f));
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            for (auto pt : { std::pair<float, float> { 0.40f, 0.30f }, { 0.70f, 0.72f } })
                g.fillEllipse (px (pt.first) - st * 1.6f, py (pt.second) - st * 1.6f, st * 3.2f, st * 3.2f);
        }
        else if (c == "Compressor" || c == "Limiter")
        {
            // transfer curve with a knee
            p.startNewSubPath (px (0.1f), py (0.9f));
            p.lineTo (px (0.5f), py (0.5f));
            p.quadraticTo (px (0.6f), py (0.42f), px (0.9f), py (c == "Limiter" ? 0.40f : 0.30f));
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col.withAlpha (0.35f));
            g.drawLine (px (0.1f), py (0.9f), px (0.9f), py (0.1f), st * 0.6f);
            if (c == "Limiter")
                g.drawLine (px (0.1f), py (0.40f), px (0.9f), py (0.40f), st * 0.6f);
        }
        else if (c == "Reverb")
        {
            for (int i = 0; i < 9; ++i)
            {
                const float x = 0.12f + i * 0.095f;
                const float hgt = 0.75f * std::exp (-0.33f * (float) i);
                g.setColour (col.withAlpha (1.0f - i * 0.08f));
                g.fillRoundedRectangle (px (x), py (0.88f - hgt), b.getWidth() * 0.05f, b.getHeight() * hgt, st * 0.5f);
            }
        }
        else if (c == "Delay")
        {
            for (int i = 0; i < 5; ++i)
            {
                const float rad = 0.10f + i * 0.08f;
                g.setColour (col.withAlpha (1.0f - i * 0.18f));
                g.drawEllipse (px (0.5f - rad), py (0.5f - rad), b.getWidth() * rad * 2, b.getHeight() * rad * 2, st);
            }
        }
        else if (c == "Saturation")
        {
            p.startNewSubPath (px (0.05f), py (0.5f));
            for (int i = 0; i <= 60; ++i)
            {
                const float t = (float) i / 60.0f;
                const float v = std::tanh (3.0f * std::sin (t * juce::MathConstants<float>::twoPi * 2.0f)) * 0.32f;
                p.lineTo (px (0.05f + 0.9f * t), py (0.5f - v));
            }
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (c == "Modulation")
        {
            for (int k = 0; k < 2; ++k)
            {
                juce::Path w;
                for (int i = 0; i <= 60; ++i)
                {
                    const float t = (float) i / 60.0f;
                    const float v = std::sin (t * juce::MathConstants<float>::twoPi * 1.5f + k * 1.2f) * 0.25f;
                    if (i == 0) w.startNewSubPath (px (0.05f), py (0.5f - v));
                    else        w.lineTo (px (0.05f + 0.9f * t), py (0.5f - v));
                }
                g.setColour (col.withAlpha (k == 0 ? 1.0f : 0.45f));
                g.strokePath (w, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
        else if (c == "Pitch & Tune")
        {
            for (int i = 0; i < 4; ++i)
            {
                const float y = 0.28f + i * 0.15f;
                g.setColour (col.withAlpha (0.3f));
                g.drawLine (px (0.08f), py (y), px (0.92f), py (y), st * 0.5f);
            }
            g.setColour (col);
            const float xs[] = { 0.18f, 0.40f, 0.62f, 0.82f };
            const float ys[] = { 0.63f, 0.43f, 0.50f, 0.28f };
            for (int i = 0; i < 4; ++i)
                g.fillRoundedRectangle (px (xs[i]) - st * 3, py (ys[i]) - st * 1.2f, st * 6, st * 2.4f, st);
        }
        else if (c == "Mastering")
        {
            for (int i = 0; i < 7; ++i)
            {
                const float hgt = 0.2f + 0.6f * std::abs (std::sin (i * 1.3f + 0.4f));
                g.setColour (col.withAlpha (0.55f + 0.06f * i));
                g.fillRoundedRectangle (px (0.12f + i * 0.11f), py (0.85f - hgt), b.getWidth() * 0.07f, b.getHeight() * hgt, st * 0.5f);
            }
        }
        else if (c == "Utility")
        {
            g.drawEllipse (b.reduced (s * 0.18f), st);
            g.drawLine (px (0.5f), py (0.5f), px (0.68f), py (0.32f), st);
            g.fillEllipse (px (0.5f) - st * 1.5f, py (0.5f) - st * 1.5f, st * 3, st * 3);
        }
        else if (c == "Synth")
        {
            p.startNewSubPath (px (0.05f), py (0.7f));
            p.lineTo (px (0.20f), py (0.3f)); p.lineTo (px (0.20f), py (0.7f));
            p.lineTo (px (0.35f), py (0.3f)); p.lineTo (px (0.35f), py (0.7f));
            p.lineTo (px (0.50f), py (0.3f)); p.lineTo (px (0.50f), py (0.7f));
            p.lineTo (px (0.65f), py (0.3f)); p.lineTo (px (0.65f), py (0.7f));
            p.lineTo (px (0.80f), py (0.3f)); p.lineTo (px (0.80f), py (0.7f));
            p.lineTo (px (0.95f), py (0.3f));
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        else if (c == "Keys" || c == "Sampler")
        {
            const auto kb = juce::Rectangle<float> (px (0.08f), py (0.25f), b.getWidth() * 0.84f, b.getHeight() * 0.5f);
            g.drawRoundedRectangle (kb, st, st);
            for (int i = 1; i < 7; ++i)
                g.drawLine (kb.getX() + kb.getWidth() * i / 7.0f, kb.getY(), kb.getX() + kb.getWidth() * i / 7.0f, kb.getBottom(), st * 0.6f);
            for (int i : { 1, 2, 4, 5, 6 })
                g.fillRect (kb.getX() + kb.getWidth() * i / 7.0f - kb.getWidth() * 0.035f, kb.getY(), kb.getWidth() * 0.07f, kb.getHeight() * 0.58f);
            if (c == "Sampler")
            {
                juce::Path w;
                for (int i = 0; i <= 40; ++i)
                {
                    const float t = (float) i / 40.0f;
                    const float v = std::sin (t * 25.0f) * (1.0f - t) * 0.08f;
                    if (i == 0) w.startNewSubPath (px (0.1f), py (0.14f - v));
                    else        w.lineTo (px (0.1f + 0.8f * t), py (0.14f - v));
                }
                g.strokePath (w, juce::PathStrokeType (st * 0.7f));
            }
        }
        else if (c == "Drums")
        {
            for (int r2 = 0; r2 < 2; ++r2)
                for (int c2 = 0; c2 < 3; ++c2)
                {
                    auto pad = juce::Rectangle<float> (px (0.12f + c2 * 0.27f), py (0.22f + r2 * 0.30f), b.getWidth() * 0.22f, b.getHeight() * 0.24f);
                    if ((r2 + c2) % 3 == 0) g.fillRoundedRectangle (pad, st);
                    else                    g.drawRoundedRectangle (pad, st, st * 0.8f);
                }
        }
        else
        {
            g.drawEllipse (b.reduced (s * 0.25f), st);
            g.fillEllipse (b.reduced (s * 0.44f));
        }
    }

    //==========================================================================
    LookAndFeel::LookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, bg);
        setColour (juce::PopupMenu::backgroundColourId, panelHi);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.18f));
        setColour (juce::PopupMenu::highlightedTextColourId, text);
        setColour (juce::PopupMenu::headerTextColourId, dim);
        setColour (juce::TextEditor::backgroundColourId, panel);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.3f));
        setColour (juce::TextEditor::outlineColourId, line);
        setColour (juce::TextEditor::focusedOutlineColourId, accent);
        setColour (juce::CaretComponent::caretColourId, accent);
        setColour (juce::TextButton::buttonColourId, panelHi);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, bg);
        setColour (juce::AlertWindow::backgroundColourId, panelHi);
        setColour (juce::AlertWindow::textColourId, text);
        setColour (juce::AlertWindow::outlineColourId, line);
        setColour (juce::ScrollBar::thumbColourId, faint);
        setColour (juce::TooltipWindow::backgroundColourId, panelHi);
        setColour (juce::TooltipWindow::textColourId, text);
        setColour (juce::TooltipWindow::outlineColourId, line);
        setColour (juce::Label::textColourId, text);
    }

    juce::Font LookAndFeel::getPopupMenuFont()                  { return font (15.0f); }
    juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int) { return font (14.0f); }
    juce::Font LookAndFeel::getAlertWindowMessageFont()         { return font (15.0f); }
    juce::Font LookAndFeel::getAlertWindowTitleFont()           { return font (17.0f, true); }

    void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
    {
        g.fillAll (panelHi);
        g.setColour (line);
        g.drawRect (0, 0, w, h);
    }

    void LookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, (float) h * 0.5f);
    }

    void LookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& ed)
    {
        g.setColour (ed.hasKeyboardFocus (true) ? accent.withAlpha (0.8f) : line);
        g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f, (float) h * 0.5f, 1.0f);
    }

    void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle, juce::Slider&)
    {
        const float cy = (float) y + (float) h * 0.5f;
        g.setColour (line);
        g.fillRoundedRectangle ((float) x, cy - 1.5f, (float) w, 3.0f, 1.5f);
        g.setColour (dim);
        g.fillRoundedRectangle ((float) x, cy - 1.5f, pos - (float) x, 3.0f, 1.5f);
        g.setColour (text);
        g.fillEllipse (pos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
    }

    void LookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                     int thumbStart, int thumbSize, bool over, bool down)
    {
        juce::Rectangle<float> t = vertical ? juce::Rectangle<float> ((float) x + 3.0f, (float) thumbStart, (float) w - 6.0f, (float) thumbSize)
                                            : juce::Rectangle<float> ((float) thumbStart, (float) y + 3.0f, (float) thumbSize, (float) h - 6.0f);
        g.setColour ((over || down) ? dim : faint.withAlpha (0.7f));
        g.fillRoundedRectangle (t, juce::jmin (t.getWidth(), t.getHeight()) * 0.5f);
    }

    void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (on ? accent.withAlpha (0.12f) : (down ? panelHi.brighter (0.1f) : (over ? panelHi : panel)));
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (on ? accent : line);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
    }
}
