#include "Categories.h"

namespace Categories
{
    const juce::StringArray& effects()
    {
        static const juce::StringArray e { "EQ", "Compressor", "Limiter", "Reverb", "Delay", "Saturation",
                                           "Modulation", "Pitch & Tune", "Mastering", "Utility" };
        return e;
    }

    const juce::StringArray& instruments()
    {
        static const juce::StringArray i { "Synth", "Sampler", "Keys", "Drums" };
        return i;
    }

    juce::StringArray all()
    {
        juce::StringArray a (effects());
        a.addArray (instruments());
        a.add ("Other");
        return a;
    }

    bool isInstrumentCategory (const juce::String& c)
    {
        return instruments().contains (c);
    }

    namespace
    {
        bool hasWord (const juce::String& hay, std::initializer_list<const char*> words)
        {
            for (auto* w : words)
                if (hay.contains (w))
                    return true;
            return false;
        }
    }

    juce::String guess (const juce::String& name, const juce::String& maker,
                        const juce::String& vstCategory, bool isInstrument)
    {
        const auto n = " " + name.toLowerCase() + " ";
        const auto vc = vstCategory.toLowerCase();

        // ---- name words are the most specific (a "Tape" plugin tagged "Fx" is saturation)
        if (isInstrument || vc.contains ("instrument"))
        {
            if (hasWord (n, { "drum", "groove", "beat", "kit", "perc", "808", "battery", "addictive" }) || vc.contains ("drum"))
                return "Drums";
            if (hasWord (n, { "piano", "keys", "keyscape", "rhodes", "wurli", "organ", "grand", "upright", "ivory", "noire" }) || vc.contains ("piano"))
                return "Keys";
            if (hasWord (n, { "kontakt", "sampler", "halion", "sforzando", "decent", "spitfire", "labs", "padshop" }) || vc.contains ("sampler"))
                return "Sampler";
            return "Synth";
        }

        if (hasWord (n, { "limit", "pro-l", "maxim", " l1", " l2", " l3", "clipper", "clip " }))
            return "Limiter";
        if (hasWord (n, { "ozone", "master", "elevate" }) || vc.contains ("mastering"))
            return "Mastering";
        if (hasWord (n, { "verb", "reverb", "room", "hall", "plate", "spring", "raum", "blackhole", "heaven", "space", "chamber", "cathedral" }))
            return "Reverb";
        if (hasWord (n, { "delay", "echo", "timeless", "tape echo", "dub" }))
            return "Delay";
        if (hasWord (n, { "eq", "pro-q", "pultec", "equal", "filter", "nova", "frequency", "curve" }))
            return "EQ";
        if (hasWord (n, { "comp", "1176", "-76", "la-2a", "la2a", "fairchild", "dyn", "pro-c", "squash", "gate", "expander", "de-ess", "deess", "rider" }))
            return "Compressor";
        if (hasWord (n, { "satur", "tape", "tube", "distort", "decapitat", "saturn", "drive", "crush", "fuzz", "warm", "kramer", "j37", "radiator" }))
            return "Saturation";
        if (hasWord (n, { "tune", "pitch", "alterboy", "melodyne", "harmon", "voice", "vocalsynth", "doubler", "throat" }))
            return "Pitch & Tune";
        if (hasWord (n, { "chorus", "flang", "phase", "tremolo", "vibrato", "shift", "rotary", "ensemble", "modul" }))
            return "Modulation";
        if (hasWord (n, { "meter", "analy", "scope", "gain", "util", "tuner", "loud", "imager", "width", "mono", "stereo", "rec" }))
            return "Utility";

        // ---- then the plugin's own sub-categories
        if (vc.contains ("eq"))                                   return "EQ";
        if (vc.contains ("reverb"))                               return "Reverb";
        if (vc.contains ("delay"))                                return "Delay";
        if (vc.contains ("distortion"))                           return "Saturation";
        if (vc.contains ("pitch"))                                return "Pitch & Tune";
        if (vc.contains ("modulation"))                           return "Modulation";
        if (vc.contains ("dynamics"))                             return "Compressor";
        if (vc.contains ("analyzer") || vc.contains ("tools")
            || vc.contains ("spatial") || vc.contains ("restoration")) return "Utility";

        juce::ignoreUnused (maker);
        return "Other";
    }
}
