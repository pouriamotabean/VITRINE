#include "WinHelpers.h"

#if JUCE_WINDOWS
 #include <windows.h>

namespace
{
    struct Search { HWND best = nullptr; long bestArea = 0; };

    BOOL CALLBACK enumProc (HWND h, LPARAM lp)
    {
        if (! IsWindowVisible (h) || IsIconic (h))
            return TRUE;

        wchar_t title[256] = {};
        GetWindowTextW (h, title, 255);
        const juce::String t (title);
        if (! (t.startsWith ("Cubase") || t.startsWith ("Nuendo")))
            return TRUE;

        RECT r {};
        GetWindowRect (h, &r);
        const long area = (long) (r.right - r.left) * (long) (r.bottom - r.top);
        auto* s = reinterpret_cast<Search*> (lp);
        if (area > s->bestArea) { s->bestArea = area; s->best = h; }   // the project window is the big one
        return TRUE;
    }
}

namespace WinHelpers
{
    juce::Rectangle<int> findCubaseWindow()
    {
        Search s;
        EnumWindows (enumProc, reinterpret_cast<LPARAM> (&s));
        if (s.best == nullptr)
            return {};

        RECT r {};
        GetWindowRect (s.best, &r);
        const juce::Rectangle<int> physical (r.left, r.top, r.right - r.left, r.bottom - r.top);
        return juce::Desktop::getInstance().getDisplays().physicalToLogical (physical);
    }
}
#else
namespace WinHelpers
{
    juce::Rectangle<int> findCubaseWindow() { return {}; }
}
#endif
