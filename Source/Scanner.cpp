#include "Scanner.h"

namespace
{
    constexpr int maxParallel = 4;
    constexpr int childTimeoutMs = 180000;   // plugin "shells" (e.g. Waves) hold hundreds of plugins

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

// Modern VST3 bundles describe themselves in Contents/Resources/moduleinfo.json.
// Reading it needs no plugin code at all: faster, and companion apps such as
// UA Connect are not started.
std::vector<PluginEntry> Scanner::readModuleInfo (const juce::String& path)
{
    std::vector<PluginEntry> r;
    const juce::File bundle (path);
    const auto info = bundle.getChildFile ("Contents").getChildFile ("Resources").getChildFile ("moduleinfo.json");
    if (! info.existsAsFile())
        return r;

    // the file is JSON5: allow comments and trailing commas
    juce::String text;
    for (auto line : juce::StringArray::fromLines (info.loadFileAsString()))
    {
        const auto t = line.trimStart();
        if (! t.startsWith ("//"))
            text << line << "\n";
    }
    {
        // drop commas that are followed only by whitespace and a closing bracket
        juce::String out;
        for (int i = 0; i < text.length(); ++i)
        {
            if (text[i] == ',')
            {
                int k = i + 1;
                while (k < text.length() && juce::CharacterFunctions::isWhitespace (text[k])) ++k;
                if (k < text.length() && (text[k] == '}' || text[k] == ']'))
                    continue;
            }
            out << juce::String::charToString (text[i]);
        }
        text = out;
    }

    juce::var root;
    if (juce::JSON::parse (text, root).failed())
        return r;

    const auto factoryVendor = root["Factory Info"]["Vendor"].toString();
    const auto fileTime = bundle.getLastModificationTime().toMilliseconds();

    if (auto* classes = root["Classes"].getArray())
        for (auto& c : *classes)
        {
            if (c["Category"].toString() != "Audio Module Class")
                continue;

            juce::StringArray subs;
            if (auto* s = c["Sub Categories"].getArray())
                for (auto& x : *s) subs.add (x.toString());
            else
                subs.addTokens (c["Sub Categories"].toString(), "|", "");

            PluginEntry e;
            e.format = "VST3";
            e.path = path;
            e.name = c["Name"].toString();
            e.maker = c["Vendor"].toString().isNotEmpty() ? c["Vendor"].toString() : factoryVendor;
            e.vstCategory = subs.joinIntoString ("|");
            e.isInstrument = subs.contains ("Instrument", true);
            e.uid = c["CID"].toString();
            e.fileTime = fileTime;
            if (e.name.isEmpty())
                continue;
            e.id = makeId ("VST3", path, e.name);
            r.push_back (e);
        }

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
        else if (auto quick = readModuleInfo (path); ! quick.empty())
        {
            std::lock_guard<std::mutex> g (lock);
            for (auto& e : quick)
            {
                ids3.add (e.id);
                pending.push_back (e);
            }
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
//==============================================================================
// Fast VST3 listing straight from the plugin's factory: reads every class it
// offers (name, maker, category) without creating any plugin. Needed for
// "shell" bundles such as Waves, which hold hundreds of plugins in one file.
#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <cstring>

namespace
{
    #pragma pack(push, 8)
    struct PClassInfo2
    {
        unsigned char cid[16];
        juce::int32 cardinality;
        char category[32];
        char name[64];
        juce::uint32 classFlags;
        char subCategories[128];
        char vendor[64];
        char version[64];
        char sdkVersion[64];
    };
    struct PFactoryInfo { char vendor[64]; char url[256]; char email[128]; juce::int32 flags; };
    #pragma pack(pop)

    struct IFactory2   // COM layout of Steinberg::IPluginFactory2
    {
        virtual juce::int32 __stdcall queryInterface (const unsigned char*, void**) = 0;
        virtual juce::uint32 __stdcall addRef() = 0;
        virtual juce::uint32 __stdcall release() = 0;
        virtual juce::int32 __stdcall getFactoryInfo (PFactoryInfo*) = 0;
        virtual juce::int32 __stdcall countClasses() = 0;
        virtual juce::int32 __stdcall getClassInfo (juce::int32, void*) = 0;
        virtual juce::int32 __stdcall createInstance (const char*, const char*, void**) = 0;
        virtual juce::int32 __stdcall getClassInfo2 (juce::int32, PClassInfo2*) = 0;
    };

    void makeIid (unsigned char* out, juce::uint32 l1, juce::uint32 l2, juce::uint32 l3, juce::uint32 l4)
    {
        const unsigned char b[16] = {
            (unsigned char) (l1 & 0xff), (unsigned char) ((l1 >> 8) & 0xff), (unsigned char) ((l1 >> 16) & 0xff), (unsigned char) (l1 >> 24),
            (unsigned char) ((l2 >> 16) & 0xff), (unsigned char) (l2 >> 24), (unsigned char) (l2 & 0xff), (unsigned char) ((l2 >> 8) & 0xff),
            (unsigned char) (l3 >> 24), (unsigned char) ((l3 >> 16) & 0xff), (unsigned char) ((l3 >> 8) & 0xff), (unsigned char) (l3 & 0xff),
            (unsigned char) (l4 >> 24), (unsigned char) ((l4 >> 16) & 0xff), (unsigned char) ((l4 >> 8) & 0xff), (unsigned char) (l4 & 0xff) };
        std::memcpy (out, b, 16);
    }

    juce::File vst3Binary (const juce::File& bundle)
    {
        if (bundle.existsAsFile()) return bundle;    // old single-file VST3
        auto dir = bundle.getChildFile ("Contents").getChildFile ("x86_64-win");
        auto f = dir.getChildFile (bundle.getFileName());
        if (f.existsAsFile()) return f;
        for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*.vst3", juce::File::findFiles))
            return e.getFile();
        return {};
    }

    bool fastFactoryList (const juce::String& path, juce::OwnedArray<juce::PluginDescription>& found)
    {
        const auto bin = vst3Binary (juce::File (path));
        if (! bin.existsAsFile()) return false;

        HMODULE h = LoadLibraryW (bin.getFullPathName().toWideCharPointer());
        if (h == nullptr) return false;

        using InitFn = bool (*)();
        if (auto init = (InitFn) (void*) GetProcAddress (h, "InitDll")) init();

        using GetFactory = IFactory2* (__stdcall *)();
        auto getFactory = (GetFactory) (void*) GetProcAddress (h, "GetPluginFactory");
        if (getFactory == nullptr) return false;

        auto* f1 = getFactory();
        if (f1 == nullptr) return false;

        unsigned char iid2[16];
        makeIid (iid2, 0x0007B650, 0xF24B4C0B, 0xA464EDB9, 0xF00B2ABB);
        IFactory2* f2 = nullptr;
        if (f1->queryInterface (iid2, (void**) &f2) != 0 || f2 == nullptr)
            return false;

        PFactoryInfo fi {};
        f2->getFactoryInfo (&fi);
        const auto factoryVendor = juce::String (fi.vendor);

        const int n = f2->countClasses();
        for (int i = 0; i < n; ++i)
        {
            PClassInfo2 ci {};
            if (f2->getClassInfo2 (i, &ci) != 0) continue;
            if (juce::String (ci.category) != "Audio Module Class") continue;

            auto d = std::make_unique<juce::PluginDescription>();
            d->name = juce::String::fromUTF8 (ci.name);
            d->descriptiveName = d->name;
            d->manufacturerName = juce::String::fromUTF8 (ci.vendor).isNotEmpty() ? juce::String::fromUTF8 (ci.vendor) : factoryVendor;
            d->category = juce::String (ci.subCategories);
            d->isInstrument = d->category.containsIgnoreCase ("Instrument");
            d->pluginFormatName = "VST3";
            d->fileOrIdentifier = path;
            d->version = juce::String (ci.version);
            d->uniqueId = (int) juce::String::toHexString (ci.cid, 16, 0).hashCode();
            d->deprecatedUid = d->uniqueId;
            if (d->name.isNotEmpty())
                found.add (d.release());
        }
        f2->release();
        return ! found.isEmpty();
    }
}
#else
namespace { bool fastFactoryList (const juce::String&, juce::OwnedArray<juce::PluginDescription>&) { return false; } }
#endif

int Scanner::runChildScan (const juce::String& path, const juce::File& out)
{
    juce::OwnedArray<juce::PluginDescription> found;

    // 1) fast: ask the plugin's factory for its list (no plugin is created)
    if (! fastFactoryList (path, found))
    {
        // 2) JUCE's full scan
        found.clear();
        juce::VST3PluginFormat fmt;
        fmt.findAllTypesForFile (found, path);
    }

    juce::XmlElement root ("SCAN");
    for (auto* d : found)
        if (auto x = d->createXml())
            root.addChildElement (x.release());

    return root.writeTo (out) ? (found.isEmpty() ? 2 : 0) : 1;
}
