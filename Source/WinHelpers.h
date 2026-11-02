#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace WinHelpers
{
    // Screen area (logical pixels) of Cubase's main window, or empty if not found.
    juce::Rectangle<int> findCubaseWindow();
}
