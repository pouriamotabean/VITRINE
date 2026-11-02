#include "CubaseLink.h"
#include "BinaryData.h"

namespace
{
    constexpr juce::uint8 manufacturer = 0x7D, tag = 0x56;   // non-commercial id, 'V'
    constexpr juce::uint32 helloTimeoutMs = 7000;

    // Our own names first; loopMIDI's default names work too (just press + twice in loopMIDI).
    juce::String deviceIdFor (const juce::Array<juce::MidiDeviceInfo>& list, bool toCubaseSide)
    {
        const juce::StringArray names = toCubaseSide ? juce::StringArray { "Vitrine", "loopMIDI Port" }
                                                     : juce::StringArray { "Vitrine Back", "loopMIDI Port 1" };
        for (auto& want : names)
            for (auto& d : list)
                if (d.name.trim().equalsIgnoreCase (want))
                    return d.identifier;
        return {};
    }
}

CubaseLink::CubaseLink()
{
    openPorts();
    startTimer (2000);
}

CubaseLink::~CubaseLink()
{
    alive->store (false);
    stopTimer();
    if (in) in->stop();
}

void CubaseLink::openPorts()
{
    if (out == nullptr)
    {
        const auto id = deviceIdFor (juce::MidiOutput::getAvailableDevices(), true);
        if (id.isNotEmpty())
            out = juce::MidiOutput::openDevice (id);
    }

    if (in == nullptr)
    {
        const auto id = deviceIdFor (juce::MidiInput::getAvailableDevices(), false);
        if (id.isNotEmpty())
        {
            in = juce::MidiInput::openDevice (id, this);
            if (in) in->start();
        }
    }

    juce::StringArray m;
    if (out == nullptr) m.add ("first");
    if (in == nullptr)  m.add ("second");
    std::lock_guard<std::mutex> g (lock);
    missing = m.joinIntoString (" and ");
}

CubaseLink::State CubaseLink::getState() const
{
    std::lock_guard<std::mutex> g (lock);
    if (missing.isNotEmpty()) return State::noPorts;
    if (lastHello != 0 && juce::Time::getMillisecondCounter() - lastHello < helloTimeoutMs) return State::connected;
    return State::waiting;
}

juce::String CubaseLink::getTrack() const
{
    std::lock_guard<std::mutex> g (lock);
    return track;
}

juce::String CubaseLink::missingPorts() const
{
    std::lock_guard<std::mutex> g (lock);
    return missing;
}

void CubaseLink::timerCallback()
{
    if (out == nullptr || in == nullptr)
        openPorts();

    send ('P', {});

    const auto s = getState();
    if (s != lastState)
    {
        lastState = s;
        if (onStateChange) onStateChange();
    }
}

void CubaseLink::send (char cmd, const juce::String& text)
{
    if (out == nullptr)
        return;

    juce::MemoryBlock utf8 (text.toRawUTF8(), text.getNumBytesAsUTF8());
    const auto hex = juce::String::toHexString (utf8.getData(), (int) utf8.getSize(), 0).toUpperCase();

    juce::Array<juce::uint8> body;
    body.add (manufacturer);
    body.add (tag);
    body.add ((juce::uint8) cmd);
    for (auto c : hex)
        body.add ((juce::uint8) c);

    out->sendMessageNow (juce::MidiMessage::createSysExMessage (body.getRawDataPointer(), body.size()));
}

void CubaseLink::load (const juce::String& name, const juce::String& vendor, bool instrument)
{
    send ('L', juce::String (instrument ? "I" : "F") + "\t" + name + "\t" + vendor);
}

void CubaseLink::requestDump()
{
    send ('D', {});
}

void CubaseLink::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m)
{
    if (! m.isSysEx())
        return;

    const auto* d = m.getSysExData();
    const int n = m.getSysExDataSize();
    if (n < 3 || d[0] != manufacturer || d[1] != tag)
        return;

    const char cmd = (char) d[2];
    juce::MemoryBlock bytes;
    for (int i = 3; i + 1 < n; i += 2)
    {
        auto nib = [] (juce::uint8 c) { return (int) (c >= 'A' ? c - 'A' + 10 : c - '0') & 15; };
        const juce::uint8 b = (juce::uint8) ((nib (d[i]) << 4) | nib (d[i + 1]));
        bytes.append (&b, 1);
    }
    const auto text = juce::String::fromUTF8 ((const char*) bytes.getData(), (int) bytes.getSize());

    auto flag = alive;
    juce::MessageManager::callAsync ([this, flag, cmd, text] { if (flag->load()) handle (cmd, text); });
}

void CubaseLink::handle (char cmd, const juce::String& text)
{
    if (cmd == 'H' || cmd == 'T')
    {
        {
            std::lock_guard<std::mutex> g (lock);
            lastHello = juce::Time::getMillisecondCounter();
            if (cmd == 'T') track = text;
            else if (text.contains ("track="))
            {
                const auto t = text.fromFirstOccurrenceOf ("track=", false, false);
                if (t.isNotEmpty()) track = t;
            }
        }
        if (lastState != State::connected || cmd == 'T')
        {
            lastState = getState();
            if (onStateChange) onStateChange();
        }
    }
    else if (cmd == 'O' || cmd == 'E')
    {
        if (onResult) onResult (cmd == 'O', text);
    }
    else if (cmd == 'D')
    {
        if (onDump) onDump (text);
    }
}

//==============================================================================
juce::File CubaseLink::scriptFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Steinberg").getChildFile ("Cubase").getChildFile ("MIDI Remote")
               .getChildFile ("Driver Scripts").getChildFile ("Local")
               .getChildFile ("PouriaMotabean").getChildFile ("Vitrine");
}

juce::String CubaseLink::scriptText()
{
    return juce::String::fromUTF8 (BinaryData::PouriaMotabean_Vitrine_js, BinaryData::PouriaMotabean_Vitrine_jsSize)
               .replace ("__VERSION__", JUCE_APPLICATION_VERSION_STRING);
}

bool CubaseLink::scriptIsCurrent()
{
    const auto f = scriptFolder().getChildFile ("PouriaMotabean_Vitrine.js");
    return f.existsAsFile() && f.loadFileAsString() == scriptText();
}

bool CubaseLink::installScript (juce::String& error)
{
    const auto dir = scriptFolder();
    if (! dir.createDirectory())
    {
        error = "Cannot create " + dir.getFullPathName();
        return false;
    }
    const auto f = dir.getChildFile ("PouriaMotabean_Vitrine.js");
    if (! f.replaceWithText (scriptText(), false, false, "\n"))
    {
        error = "Cannot write " + f.getFullPathName();
        return false;
    }
    return true;
}
