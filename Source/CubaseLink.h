#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <memory>
#include <mutex>

// Talks to the Vitrine MIDI Remote script inside Cubase through two loopMIDI
// ports: "Vitrine" (app -> Cubase) and "Vitrine Back" (Cubase -> app).
class CubaseLink : private juce::MidiInputCallback,
                   private juce::Timer
{
public:
    enum class State { noPorts, waiting, connected };

    CubaseLink();
    ~CubaseLink() override;

    State getState() const;
    juce::String getTrack() const;
    juce::String missingPorts() const;      // human-readable, empty when both exist

    // kind: true = instrument
    void load (const juce::String& name, const juce::String& vendor, bool instrument);
    void requestDump();

    // replies from Cubase, delivered on the message thread
    std::function<void (bool ok, const juce::String& text)> onResult;
    std::function<void (const juce::String& text)> onDump;
    std::function<void()> onStateChange;

    // script file management
    static juce::File scriptFolder();
    static juce::String scriptText();
    static bool scriptIsCurrent();
    static bool installScript (juce::String& error);

private:
    void timerCallback() override;
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;
    void openPorts();
    void send (char cmd, const juce::String& text);
    void handle (char cmd, const juce::String& text);

    std::unique_ptr<juce::MidiOutput> out;
    std::unique_ptr<juce::MidiInput> in;
    mutable std::mutex lock;
    juce::String track, missing;
    juce::uint32 lastHello = 0;
    State lastState = State::noPorts;
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);
};
