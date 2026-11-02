#include "Scanner.h"

namespace
{
    constexpr int maxParallel = 4;
    constexpr int childTimeoutMs = 30000;

    juce::String makeId (const juce::String& format, const juce::String& path, const juce::String& name)
    {
        return format + "|" + path.toLowerCase() + "|" + name;
    }
}

Scanner::Scanner() : juce::Thread ("Vitrine scanner") {}

Scanner::~Scanner()
{
    stopThread (8000);
}

void Scanner::start (std::map<juce::String, Known> c, juce::StringArray extraFolders)
{
    if (isThreadRunning())
        return;

    cache = std::move (c);
    extra = std::move (extraFolders);
    {
        std::lock_guard<std::mutex> g (lock);
        progress = {};
        pending.clear();
        finalVst3.clear();
        finalVst2.clear();
        finalReady = false;
    }
    startThread (juce::Thread::Priority::low);
}

Scanner::Progress Scanner::getProgress() const
{
    std::lock_guard<std::mutex> g (lock);
    return progress;
}

std::vector<PluginEntry> Scanner::takeResults()
{
    std::lock_guard<std::mutex> g (lock);
    std::vector<PluginEntry> r;
    r.swap (pending);
    return r;
}

bool Scanner::takeFinalIds (juce::StringArray& v3, juce::StringArray& v2)
{
    std::lock_guard<std::mutex> g (lock);
    if (! finalReady)
        return false;
    v3 = finalVst3;
    v2 = finalVst2;
    finalReady = false;
    return true;
}

//==============================================================================
juce::StringArray Scanner::findVst3() const
{
    juce::VST3PluginFormat fmt;
    auto paths = fmt.getDefaultLocationsToSearch();
    for (auto& f : extra)
        if (juce::File (f).isDirectory())
            paths.addIfNotAlreadyThere (juce::File (f));

    auto found = fmt.searchPathsForPlugins (paths, true, false);
    found.removeDuplicates (true);
    found.sortNatural();
    return found;
}

juce::StringArray Scanner::findVst2() const
{
    juce::StringArray folders;
   #if JUCE_WINDOWS
    const auto pf = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory);   // C:\Program Files
    folders.add (pf.getChildFile ("VSTPlugins").getFullPathName());
    folders.add (pf.getChildFile ("Steinberg").getChildFile ("VSTPlugins").getFullPathName());
    folders.add (pf.getChildFile ("Common Files").getChildFile ("VST2").getFullPathName());
    folders.add (pf.getChildFile ("Common Files").getChildFile ("Steinberg").getChildFile ("VST2").getFullPathName());

    for (auto key : { "HKEY_LOCAL_MACHINE\\SOFTWARE\\VST\\VSTPluginsPath",
                      "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\VST\\VSTPluginsPath" })
    {
        const auto v = juce::WindowsRegistry::getValue (key);
        if (v.isNotEmpty())
            folders.addIfNotAlreadyThere (v);
    }
   #endif

    juce::StringArray dlls;
    for (auto& f : folders)
    {
        const juce::File dir (f);
        if (! dir.isDirectory())
            continue;

        for (const auto& e : juce::RangedDirectoryIterator (dir, true, "*.dll", juce::File::findFiles))
            dlls.addIfNotAlreadyThere (e.getFile().getFullPathName());
    }
    dlls.sortNatural();
    return dlls;
}

PluginEntry Scanner::fallbackEntry (const juce::String& path, const juce::String& format)
{
    const juce::File f (path);
    PluginEntry e;
    e.format = format;
    e.path = path;
    e.name = f.getFileNameWithoutExtension();
    e.fileTime = f.getLastModificationTime().toMilliseconds();

    if (format == "VST2")
    {
        // most installers put VST2 dlls in a folder named after the maker
        const auto parent = f.getParentDirectory().getFileName();
        if (! parent.equalsIgnoreCase ("VSTPlugins") && ! parent.equalsIgnoreCase ("VST2")
            && ! parent.equalsIgnoreCase ("Steinberg") && ! parent.equalsIgnoreCase ("x64"))
            e.maker = parent;
    }

    e.id = makeId (format, path, e.name);
    return e;
}

std::vector<PluginEntry> Scanner::scanVst3InChild (const juce::String& path, juce::ChildProcess& p, const juce::File& out)
{
    juce::ignoreUnused (p);
    std::vector<PluginEntry> r;
    const auto fileTime = juce::File (path).getLastModificationTime().toMilliseconds();

    if (auto xml = juce::parseXML (out))
    {
        for (auto* child : xml->getChildIterator())
        {
            juce::PluginDescription d;
            if (! d.loadFromXml (*child))
                continue;

            PluginEntry e;
            e.format = "VST3";
            e.path = path;
            e.name = d.name.isNotEmpty() ? d.name : juce::File (path).getFileNameWithoutExtension();
            e.maker = d.manufacturerName;
            e.vstCategory = d.category;
            e.isInstrument = d.isInstrument;
            e.uid = d.createIdentifierString();
            e.fileTime = fileTime;
            e.id = makeId ("VST3", path, e.name);
            r.push_back (e);
        }
    }

    out.deleteFile();
    return r;
}

void Scanner::run()
{
    const auto vst3 = findVst3();
    const auto vst2 = findVst2();

    {
        std::lock_guard<std::mutex> g (lock);
        progress.total = vst3.size() + vst2.size();
    }

    juce::StringArray ids3, ids2;
    juce::StringArray todo;

    // unchanged files keep what we already know
    for (auto& path : vst3)
    {
        const auto key = path.toLowerCase();
        const auto t = juce::File (path).getLastModificationTime().toMilliseconds();
        auto it = cache.find (key);

        if (it != cache.end() && it->second.fileTime == t && ! it->second.ids.isEmpty())
        {
            ids3.addArray (it->second.ids);
            std::lock_guard<std::mutex> g (lock);
            ++progress.done;
        }
        else
        {
            todo.add (path);
        }
    }

    // changed / new VST3 files: a few child processes at a time
    const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    const auto tmpDir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("vitrine_scan");
    tmpDir.createDirectory();

    struct Job { std::unique_ptr<juce::ChildProcess> proc; juce::String path; juce::File out; juce::uint32 started = 0; };
    std::vector<Job> jobs;
    int next = 0, serial = 0;

    auto finish = [&] (Job& j, bool timedOut)
    {
        std::vector<PluginEntry> found;
        if (! timedOut)
            found = scanVst3InChild (j.path, *j.proc, j.out);

        if (found.empty())
            found.push_back (fallbackEntry (j.path, "VST3"));   // still show it, by file name

        std::lock_guard<std::mutex> g (lock);
        for (auto& e : found)
        {
            ids3.add (e.id);
            pending.push_back (e);
        }
        ++progress.done;
    };

    while (! threadShouldExit() && (next < todo.size() || ! jobs.empty()))
    {
        while ((int) jobs.size() < maxParallel && next < todo.size())
        {
            Job j;
            j.path = todo[next++];
            j.out = tmpDir.getChildFile ("scan_" + juce::String (++serial) + ".xml");
            j.out.deleteFile();
            j.proc = std::make_unique<juce::ChildProcess>();
            j.started = juce::Time::getMillisecondCounter();

            {
                std::lock_guard<std::mutex> g (lock);
                progress.current = juce::File (j.path).getFileNameWithoutExtension();
            }

            if (j.proc->start (juce::StringArray { exe.getFullPathName(), "--scan", j.path, j.out.getFullPathName() }, 0))
                jobs.push_back (std::move (j));
            else
                finish (j, true);
        }

        for (auto it = jobs.begin(); it != jobs.end();)
        {
            const bool running = it->proc->isRunning();
            const bool late = juce::Time::getMillisecondCounter() - it->started > (juce::uint32) childTimeoutMs;

            if (! running || late)
            {
                if (running)
                    it->proc->kill();
                finish (*it, running);
                it = jobs.erase (it);
            }
            else
            {
                ++it;
            }
        }

        wait (40);
    }

    for (auto& j : jobs)
        if (j.proc->isRunning())
            j.proc->kill();

    if (threadShouldExit())
        return;

    // VST2: by file name; skip plugins that also exist as VST3
    juce::StringArray vst3Names;
    {
        for (auto& id : ids3)
            vst3Names.add (id.fromLastOccurrenceOf ("|", false, false).toLowerCase());
    }

    for (auto& path : vst2)
    {
        auto e = fallbackEntry (path, "VST2");
        if (! vst3Names.contains (e.name.toLowerCase()))
        {
            ids2.add (e.id);
            std::lock_guard<std::mutex> g (lock);
            pending.push_back (e);
        }
        std::lock_guard<std::mutex> g (lock);
        ++progress.done;
    }

    std::lock_guard<std::mutex> g (lock);
    finalVst3 = ids3;
    finalVst2 = ids2;
    finalReady = true;
    progress.finished = true;
    progress.current = {};
}

//==============================================================================
int Scanner::runChildScan (const juce::String& path, const juce::File& out)
{
    juce::VST3PluginFormat fmt;
    juce::OwnedArray<juce::PluginDescription> found;
    fmt.findAllTypesForFile (found, path);

    juce::XmlElement root ("SCAN");
    for (auto* d : found)
        if (auto x = d->createXml())
            root.addChildElement (x.release());

    return root.writeTo (out) ? (found.isEmpty() ? 2 : 0) : 1;
}
