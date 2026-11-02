#include <juce_gui_extra/juce_gui_extra.h>
#include "MainComponent.h"
#include "Scanner.h"

class VitrineApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "Vitrine"; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return true; }   // scan helpers are extra instances

    void initialise (const juce::String& commandLine) override
    {
        auto args = juce::StringArray::fromTokens (commandLine, true);
        for (auto& a : args) a = a.unquoted();

        // helper process: scan one plugin and exit
        if (args[0] == "--scan" && args.size() >= 3)
        {
            setApplicationReturnValue (Scanner::runChildScan (args[1], juce::File (args[2])));
            quit();
            return;
        }

        // test helper: render the window with demo data to a PNG
        if (args[0] == "--render" && args.size() >= 2)
        {
            juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
            library.fillDemo();
            MainComponent mc (library, false);
            mc.setSize (args.size() >= 4 ? args[2].getIntValue() : 1600, args.size() >= 4 ? args[3].getIntValue() : 1000);
            const auto img = mc.createComponentSnapshot (mc.getLocalBounds(), true, 1.0f);
            juce::File out (args[1]);
            out.deleteFile();
            juce::FileOutputStream os (out);
            juce::PNGImageFormat().writeImageToStream (img, os);
            quit();
            return;
        }

        juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
        library.load();
        const bool demo = args.contains ("--demo");
        if (demo)
            library.fillDemo();

        window = std::make_unique<MainWindow> (library, ! demo);
    }

    void shutdown() override
    {
        window.reset();
        if (library.plugins().size() > 0 && ! dirtyDemo())
            library.save();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override { quit(); }

private:
    bool dirtyDemo() const { return getCommandLineParameters().contains ("--demo") || getCommandLineParameters().contains ("--render"); }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (Library& lib, bool scan)
            : DocumentWindow ("Vitrine", Theme::bg, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent (lib, scan), false);
            setResizable (true, true);
            setResizeLimits (280, 420, 10000, 10000);

            const auto state = Library::dataFolder().getChildFile ("window.txt").loadFileAsString();
            if (state.isNotEmpty()) restoreWindowStateFromString (state);
            else centreWithSize (1500, 940);

            setVisible (true);
        }

        ~MainWindow() override
        {
            Library::dataFolder().getChildFile ("window.txt").replaceWithText (getWindowStateAsString());
        }

        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

    Theme::LookAndFeel lnf;
    Library library;
    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION (VitrineApp)
