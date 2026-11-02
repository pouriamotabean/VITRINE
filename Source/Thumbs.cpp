#include "Thumbs.h"
#include "Theme.h"
#include "WinHelpers.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace Thumbs
{
    juce::File fileFor (const PluginEntry& e)
    {
        return Library::dataFolder().getChildFile ("thumbs")
                   .getChildFile (juce::String::toHexString (e.id.hashCode64()) + ".png");
    }

    bool hasPicture (const PluginEntry& e) { return fileFor (e).existsAsFile(); }

    namespace
    {
        // true if the picture is basically one flat colour (black window, blank panel)
        bool looksEmpty (const juce::Image& img)
        {
            if (! img.isValid()) return true;
            double sum = 0, sum2 = 0; int n = 0;
            for (int y = 0; y < img.getHeight(); y += 7)
                for (int x = 0; x < img.getWidth(); x += 7)
                {
                    const float b = img.getPixelAt (x, y).getPerceivedBrightness();
                    sum += b; sum2 += b * b; ++n;
                }
            if (n == 0) return true;
            const double mean = sum / n, var = sum2 / n - mean * mean;
            return var < 0.00002;     // practically one flat colour
        }

        struct ShotWindow : juce::DocumentWindow
        {
            explicit ShotWindow (juce::Component* content)
                : DocumentWindow ("Vitrine - taking a picture", juce::Colours::black, 0)
            {
                setUsingNativeTitleBar (false);
                setTitleBarHeight (0);
                setContentNonOwned (content, true);
                setAlwaysOnTop (true);
                auto area = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
                setTopLeftPosition (area.getX() + 40, area.getY() + 40);
            }
            void closeButtonPressed() override {}
        };
    }

    int runChildShot (const juce::String& path, const juce::String& name, const juce::File& out)
    {
        juce::VST3PluginFormat fmt;
        juce::OwnedArray<juce::PluginDescription> types;
        fmt.findAllTypesForFile (types, path);
        if (types.isEmpty()) return 2;

        auto* desc = types.getFirst();
        for (auto* t : types)
            if (t->name.equalsIgnoreCase (name)) desc = t;

        juce::String err;
        auto inst = fmt.createInstanceFromDescription (*desc, 44100.0, 512, err);
        if (inst == nullptr) return 3;
        inst->enableAllBuses();
        inst->prepareToPlay (44100.0, 512);

        std::unique_ptr<juce::AudioProcessorEditor> ed (inst->createEditorIfNeeded());
        if (ed == nullptr) return 4;

        juce::Image best;
        {
            ShotWindow w (ed.get());
            w.setVisible (true);
            w.toFront (true);

            // let the plugin draw itself; take a few tries for slow GPU UIs
            for (int attempt = 0; attempt < 4; ++attempt)
            {
                juce::MessageManager::getInstance()->runDispatchLoopUntil (attempt == 0 ? 1800 : 900);
                auto img = WinHelpers::captureComponentOnScreen (*ed);
                if (! looksEmpty (img)) { best = img; break; }
                if (juce::SystemStats::getEnvironmentVariable ("VITRINE_SHOT_DEBUG", {}).isNotEmpty())
                    std::fprintf (stderr, "attempt %d: %dx%d valid=%d\n", attempt, img.getWidth(), img.getHeight(), (int) img.isValid());
            }
            w.setVisible (false);
            w.clearContentComponent();
        }

        if (! best.isValid()) return 5;

        const int maxW = 640;
        if (best.getWidth() > maxW)
            best = best.rescaled (maxW, juce::roundToInt (best.getHeight() * (double) maxW / best.getWidth()), juce::Graphics::highResamplingQuality);

        out.getParentDirectory().createDirectory();
        out.deleteFile();
        juce::FileOutputStream os (out);
        if (! os.openedOk() || ! juce::PNGImageFormat().writeImageToStream (best, os)) return 6;
        os.flush();
        return 0;
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
                g.drawImage (img, r, juce::RectanglePlacement::xMid | juce::RectanglePlacement::yTop | juce::RectanglePlacement::fillDestination);
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
