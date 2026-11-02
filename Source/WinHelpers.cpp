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

// ============================================================================
namespace WinHelpers
{
    juce::String hotkeyText (Hotkey h)
    {
        if (h.key == 0) return {};
        juce::String s;
        if (h.mods & 1) s << "Ctrl+";
        if (h.mods & 2) s << "Alt+";
        if (h.mods & 4) s << "Shift+";
        if (h.key >= 0x70 && h.key <= 0x87) s << "F" << (h.key - 0x6F);
        else if ((h.key >= '0' && h.key <= '9') || (h.key >= 'A' && h.key <= 'Z')) s << juce::String::charToString ((juce::juce_wchar) h.key);
        else if (h.key >= 0x60 && h.key <= 0x69) s << "Num" << (h.key - 0x60);
        else s << "#" << h.key;
        return s;
    }

    bool hotkeyFromKeyPress (const juce::KeyPress& k, Hotkey& out)
    {
        const auto m = k.getModifiers();
        int mods = (m.isCtrlDown() ? 1 : 0) | (m.isAltDown() ? 2 : 0) | (m.isShiftDown() ? 4 : 0);
        int code = k.getKeyCode() & 0xffff;
        if (code >= 'a' && code <= 'z') code -= 32;
        const bool fkey = code >= 0x70 && code <= 0x87;
        const bool plain = (code >= '0' && code <= '9') || (code >= 'A' && code <= 'Z') || (code >= 0x60 && code <= 0x69);
        if (! fkey && ! plain) return false;
        if (! fkey && (mods & 3) == 0) return false;      // letters/digits need Ctrl or Alt (they are for typing)
        out = { mods, code };
        return true;
    }
}

#if JUCE_WINDOWS
namespace
{
    std::function<void (int)> hotkeyCallback;
    HWND hotkeyWindow = nullptr;
    juce::Array<int> registeredIds;

    LRESULT CALLBACK hotkeyProc (HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        if (msg == WM_HOTKEY && hotkeyCallback)
        {
            const int id = (int) wp;
            juce::MessageManager::callAsync ([id] { if (hotkeyCallback) hotkeyCallback (id); });
            return 0;
        }
        return DefWindowProcW (h, msg, wp, lp);
    }

    HWND getHotkeyWindow()
    {
        if (hotkeyWindow == nullptr)
        {
            WNDCLASSW wc {};
            wc.lpfnWndProc = hotkeyProc;
            wc.hInstance = GetModuleHandleW (nullptr);
            wc.lpszClassName = L"VitrineHotkeys";
            RegisterClassW (&wc);
            hotkeyWindow = CreateWindowExW (0, L"VitrineHotkeys", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
        }
        return hotkeyWindow;
    }
}

namespace WinHelpers
{
    void setHotkeyCallback (std::function<void (int)> cb) { hotkeyCallback = std::move (cb); }

    bool registerHotkey (int id, Hotkey h)
    {
        if (h.key == 0) return false;
        UINT mods = 0x4000; // MOD_NOREPEAT
        if (h.mods & 1) mods |= MOD_CONTROL;
        if (h.mods & 2) mods |= MOD_ALT;
        if (h.mods & 4) mods |= MOD_SHIFT;
        if (! RegisterHotKey (getHotkeyWindow(), id, mods, (UINT) h.key))
            return false;
        registeredIds.add (id);
        return true;
    }

    void unregisterAllHotkeys()
    {
        for (auto id : registeredIds)
            UnregisterHotKey (getHotkeyWindow(), id);
        registeredIds.clear();
    }

    juce::Image captureComponentOnScreen (juce::Component& c)
    {
        auto* peer = c.getPeer();
        if (peer == nullptr) return {};
        HWND h = (HWND) peer->getNativeHandle();
        RECT rc {};
        GetClientRect (h, &rc);
        POINT tl { rc.left, rc.top };
        ClientToScreen (h, &tl);
        const int w = rc.right - rc.left, ht = rc.bottom - rc.top;
        if (w <= 0 || ht <= 0) return {};

        HDC screen = GetDC (nullptr);
        HDC mem = CreateCompatibleDC (screen);
        BITMAPINFO bi {};
        bi.bmiHeader.biSize = sizeof (bi.bmiHeader);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -ht;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        void* bits = nullptr;
        HBITMAP bmp = CreateDIBSection (screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        auto old = SelectObject (mem, bmp);
        BitBlt (mem, 0, 0, w, ht, screen, tl.x, tl.y, SRCCOPY | CAPTUREBLT);

        juce::Image img (juce::Image::RGB, w, ht, false);
        {
            juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
            auto* src = static_cast<const juce::uint8*> (bits);
            for (int y = 0; y < ht; ++y)
                for (int x = 0; x < w; ++x)
                {
                    auto* p = src + ((size_t) y * (size_t) w + (size_t) x) * 4;
                    bd.setPixelColour (x, y, juce::Colour (p[2], p[1], p[0]));
                }
        }

        SelectObject (mem, old);
        DeleteObject (bmp);
        DeleteDC (mem);
        ReleaseDC (nullptr, screen);
        return img;
    }
}
#else
namespace WinHelpers
{
    void setHotkeyCallback (std::function<void (int)>) {}
    bool registerHotkey (int, Hotkey) { return false; }
    void unregisterAllHotkeys() {}
    juce::Image captureComponentOnScreen (juce::Component& c) { return c.createComponentSnapshot (c.getLocalBounds()); }
}
#endif
