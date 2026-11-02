#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace WinHelpers
{
    // Screen area (logical pixels) of Cubase's main window, or empty if not found.
    juce::Rectangle<int> findCubaseWindow();

    // Grabs what is on screen inside a component's native window (works for GPU-drawn plugin UIs).
    juce::Image captureComponentOnScreen (juce::Component& c);

    // System-wide shortcuts (work while Cubase is in front). Ctrl/Alt/Shift + a key.
    struct Hotkey { int mods = 0; int key = 0; };      // mods: 1 Ctrl, 2 Alt, 4 Shift; key: Windows virtual key
    juce::String hotkeyText (Hotkey h);
    bool hotkeyFromKeyPress (const juce::KeyPress& k, Hotkey& out);
    void setHotkeyCallback (std::function<void (int id)> cb);
    bool registerHotkey (int id, Hotkey h);
    void unregisterAllHotkeys();
}
