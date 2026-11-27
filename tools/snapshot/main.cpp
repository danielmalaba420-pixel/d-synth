// Renders the plugin editor to a PNG (no DAW needed) for visual checks.
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 3 && juce::String (argv[1]) == "--bank")
    {
        // check: the FLAC bank decodes to exactly the raw bank
        dsynth::SampleBank a, b;
        auto decode = [] (const std::string& path, int16_t* dest, int frames, int channels) -> bool
        {
            juce::FlacAudioFormat flac;
            std::unique_ptr<juce::AudioFormatReader> r (flac.createReaderFor (new juce::FileInputStream (juce::File (path)), true));
            if (r == nullptr || (int) r->numChannels != channels || r->lengthInSamples < frames) return false;
            std::vector<int> x ((size_t) frames), y ((size_t) frames);
            int* chans[2] = { x.data(), channels == 2 ? y.data() : nullptr };
            if (! r->read (chans, channels, 0, frames, false)) return false;
            for (int i = 0; i < frames; ++i)
                for (int ch = 0; ch < channels; ++ch)
                    dest[(size_t) i * (size_t) channels + (size_t) ch] = (int16_t) ((ch == 0 ? x : y)[(size_t) i] >> 16);
            return true;
        };
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        a.load (argv[2], decode);
        const auto t1 = juce::Time::getMillisecondCounterHiRes();
        b.load (argv[3]);
        int same = 0;
        for (size_t i = 0; i < std::min (a.zones.size(), b.zones.size()); ++i) same += a.zones[i].pcm == b.zones[i].pcm ? 1 : 0;
        std::printf ("flac zones %d, raw zones %d, identical %d, flac load %.0f ms\n", (int) a.zones.size(), (int) b.zones.size(), same, t1 - t0);
        return 0;
    }
    DSynthProcessor proc;
    if (argc > 2) proc.setCurrentProgram (std::atoi (argv[2]));
    proc.uiModWheel = 0.45f; proc.uiPitchBend = 0.0f;
    if (argc > 3) for (auto& n : juce::StringArray::fromTokens (argv[3], ",", "")) if (n.getIntValue() > 0) proc.uiNotes[(size_t) n.getIntValue()] = 1;
    if (argc > 4) proc.uiCategory = argv[4];
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (SynthPanel::W, SynthPanel::H);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (argv[1]);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
