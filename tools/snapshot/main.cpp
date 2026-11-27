// Renders the plugin editor to a PNG (no DAW needed) for visual checks.
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
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
