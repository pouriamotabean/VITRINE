#include "Thumbs.h"
#include "Theme.h"

namespace Thumbs
{
    juce::File fileFor (const PluginEntry& e)
    {
        return Library::dataFolder().getChildFile ("thumbs")
                   .getChildFile (juce::String::toHexString (e.id.hashCode64()) + ".png");
    }

    void draw (juce::Graphics& g, juce::Rectangle<float> r, const PluginEntry& e, float corner)
    {
        juce::Path clip;
        clip.addRoundedRectangle (r, corner);

        const auto f = fileFor (e);
        if (f.existsAsFile())
        {
            const auto img = juce::ImageCache::getFromFile (f);
            if (img.isValid())
            {
                juce::Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (clip);
                g.drawImage (img, r, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
                return;
            }
        }

        const auto mc = Theme::makerColour (e.maker.isNotEmpty() ? e.maker : e.name);

        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (clip);
        g.setColour (Theme::bg.interpolatedWith (mc, 0.06f));
        g.fillRect (r);

        // faint grid, like a plugin display
        g.setColour (mc.withAlpha (0.06f));
        const float step = juce::jmax (12.0f, r.getHeight() / 6.0f);
        for (float x = r.getX() + step; x < r.getRight(); x += step) g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        for (float y = r.getY() + step; y < r.getBottom(); y += step) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());

        Theme::drawCategoryIcon (g, e.category(), r.reduced (r.getWidth() * 0.22f, r.getHeight() * 0.18f), mc.withAlpha (0.92f));

        // maker initials, bottom-left, very quiet
        juce::String initials;
        for (auto& w : juce::StringArray::fromTokens (e.maker, " ", ""))
            if (w.isNotEmpty() && initials.length() < 2)
                initials << w.substring (0, 1).toUpperCase();

        g.setColour (mc.withAlpha (0.45f));
        g.setFont (Theme::font (juce::jmax (10.0f, r.getHeight() * 0.11f), true));
        g.drawText (initials, r.reduced (8.0f, 6.0f), juce::Justification::bottomLeft, false);
    }
}
