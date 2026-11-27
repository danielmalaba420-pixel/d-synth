// D-SYNTH engine checks: mod wheel effect per sound, and switching sounds while notes are held / the arp runs.
// Build: g++ -O2 -std=c++17 -I../../Source/Engine stress.cpp -o stress ; ./stress SAMPLES_DIR
#include "DSynthEngine.h"
#include "Presets.h"
#include <cstdio>
#include <cmath>
using namespace dsynth;
static double rms (const std::vector<float>& a, size_t from, size_t to) { double s = 0; for (size_t i = from; i < to; ++i) s += a[i] * a[i]; return std::sqrt (s / (double) (to - from) + 1e-20); }
static double hiRms (const std::vector<float>& a, size_t from, size_t to) { double s = 0; for (size_t i = from + 1; i < to; ++i) { double d = a[i] - a[i - 1]; s += d * d; } return std::sqrt (s / (double) (to - from) + 1e-20); }
int main (int argc, char** argv)
{
    SampleBank bank; if (argc > 1) bank.load (argv[1]);
    const double sr = 44100; const auto& P = factoryPresets(); int problems = 0;
    // 1. mod wheel: same chord, wheel at 0 then 1 — level, brightness, or movement must change
    std::printf ("== molette (CC1) par son ==\n");
    for (size_t k = 0; k < P.size(); ++k)
    {
        double res[2][2];
        for (int m = 0; m < 2; ++m)
        {
            Engine e; e.prepare (sr); e.setSampleBank (&bank); e.setAll (P[k].values()); e.setTempo (100);
            e.modWheel (m ? 1.0f : 0.0f);
            const int notes[3] = { 60, 64, 67 }; for (int n : notes) e.noteOn (n, 0.8f);
            std::vector<float> L (sr * 3), R (sr * 3);
            for (size_t p = 0; p < L.size(); p += 256) e.process (L.data() + p, R.data() + p, (int) std::min<size_t> (256, L.size() - p));
            res[m][0] = rms (L, sr * 1, sr * 3); res[m][1] = hiRms (L, sr * 1, sr * 3) / (res[m][0] + 1e-12);
        }
        const double dLev = 20 * std::log10 (res[1][0] / res[0][0]), dBr = 20 * std::log10 (res[1][1] / res[0][1]);
        const bool none = std::abs (dLev) < 0.5 && std::abs (dBr) < 0.5;
        std::printf ("%-14s niveau %+5.1f dB  brillance %+5.1f dB %s\n", P[k].name, dLev, dBr, none ? "<- la molette ne change presque rien" : "");
    }
    // 2. switching sounds while keys are held, then releasing: everything must stop
    std::printf ("== changement de son pendant le jeu ==\n");
    for (size_t a = 0; a < P.size(); ++a)
        for (size_t b = 0; b < P.size(); ++b)
        {
            if (a == b) continue;
            Engine e; e.prepare (sr); e.setSampleBank (&bank); e.setAll (P[a].values()); e.setTempo (100);
            std::vector<float> L (256), R (256);
            auto run = [&] (double sec) { double peak = 0; for (int i = 0; i < (int) (sec * sr / 256); ++i) { e.process (L.data(), R.data(), 256); for (int j = 0; j < 256; ++j) { if (! std::isfinite (L[(size_t) j])) return -1.0; peak = std::max (peak, (double) std::abs (L[(size_t) j])); } } return peak; };
            e.noteOn (48, 0.8f); e.noteOn (55, 0.8f); e.noteOn (64, 0.8f);
            run (0.7);
            e.setAll (P[b].values());          // the user picks another sound with the keys still down
            run (0.5);
            e.noteOff (48); e.noteOff (55); e.noteOff (64);
            run (6.0);                          // longest release / reverb tail
            const double tail = run (1.0);
            // then play the new sound normally: must sound
            e.noteOn (60, 0.8f); const double play = run (0.6); e.noteOff (60);
            if (tail < 0 || play < 0) { std::printf ("%s -> %s : valeurs invalides (NaN)\n", P[a].name, P[b].name); ++problems; }
            else if (tail > 0.003) { std::printf ("%s -> %s : le son continue après avoir lâché les touches (%.3f)\n", P[a].name, P[b].name, tail); ++problems; }
            else if (play < 0.002 && std::string (P[b].name) != "RISER") { std::printf ("%s -> %s : le nouveau son ne sort plus (%.4f)\n", P[a].name, P[b].name, play); ++problems; }
        }
    std::printf ("problèmes : %d\n", problems);
    return 0;
}
