// Renders the plugin editor to a PNG (no DAW needed) for visual checks.
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    DSynthProcessor proc;
    if (argc > 2) proc.setCurrentProgram (std::atoi (argv[2]));
    proc.uiModWheel = 0.6f; proc.uiPitchBend = 0.3f;
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (SynthPanel::W, SynthPanel::H);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (argv[1]);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
