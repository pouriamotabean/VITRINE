#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Library.h"
#include <map>
#include <mutex>

// Finds installed plugins in the background.
//  - VST3: each new or changed bundle is opened in a separate Vitrine process
//    ("--scan"), so a crashing or hanging plugin can never take the app down.
//  - VST2: listed by file name only (the VST2 SDK is no longer available).
// Results are picked up by the UI with takeResults().
class Scanner : private juce::Thread
{
public:
    struct Known { juce::int64 fileTime; juce::StringArray ids; };

    Scanner();
    ~Scanner() override;

    // cache: path (lower case) -> what we already know about that file
    void start (std::map<juce::String, Known> cache, juce::StringArray extraFolders);
    bool isScanning() const { return isThreadRunning(); }

    struct Progress { int done = 0, total = 0; juce::String current; bool finished = false; };
    Progress getProgress() const;

    // New / changed entries since the last call, plus (once finished) the full id lists.
    std::vector<PluginEntry> takeResults();
    bool takeFinalIds (juce::StringArray& vst3Ids, juce::StringArray& vst2Ids);

    // child-process entry point: scan one VST3 file, write XML, return exit code
    static int runChildScan (const juce::String& path, const juce::File& out);

private:
    void run() override;
    juce::StringArray findVst3() const;
    juce::StringArray findVst2() const;
    std::vector<PluginEntry> scanVst3InChild (const juce::String& path, juce::ChildProcess& p, const juce::File& out);
    static PluginEntry fallbackEntry (const juce::String& path, const juce::String& format);

    std::map<juce::String, Known> cache;
    juce::StringArray extra;

    mutable std::mutex lock;
    Progress progress;
    std::vector<PluginEntry> pending;
    juce::StringArray finalVst3, finalVst2;
    bool finalReady = false;
};
