// Checks for v0.3: renders Glide Sine on the reference chords, tests bass tuning.
#include "DSynthEngine.h"
#include "Presets.h"
#include <cstdio>
#include <string>
#include <vector>
using namespace dsynth;

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), bytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (uint32_t i = 0; i < n; ++i) { int16_t l = (int16_t) std::lround (clampf (L[i], -1, 1) * 32767), r = (int16_t) std::lround (clampf (R[i], -1, 1) * 32767); std::fwrite (&l, 2, 1, f); std::fwrite (&r, 2, 1, f); }
    std::fclose (f);
}

int presetIndex (const char* name)
{
    const auto& ps = factoryPresets();
    for (size_t i = 0; i < ps.size(); ++i) if (std::string (ps[i].name) == name) return (int) i;
    return -1;
}

int main (int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    const int sr = 44100;
    // 1) Glide Sine on the reference's chords (played legato: next chord pressed as the previous is lifted)
    {
        Engine e; e.prepare (sr); e.setAll (factoryPresets()[(size_t) presetIndex ("GLIDE SINE")].values());
        const std::vector<std::pair<double, std::vector<int>>> chords = {
            { 0.00, { 63, 67, 70, 72 } }, { 1.90, { 62, 65, 67, 70 } }, { 3.75, { 59, 62, 64, 67 } },
            { 5.55, { 58, 60, 63, 67 } }, { 7.35, { 63, 67, 70, 72 } }, { 8.05, { 65, 67, 70, 72 } },
            { 9.15, { 62, 65, 67, 70 } }, { 11.0, { 57, 59, 62, 64, 67 } }, { 12.8, { 58, 60, 63, 67 } } };
        const double total = 14.16;
        std::vector<float> L ((size_t) (total * sr)), R (L.size());
        size_t pos = 0; std::vector<int> held;
        for (size_t c = 0; c <= chords.size(); ++c)
        {
            const double t = c < chords.size() ? chords[c].first : total;
            const size_t upto = (size_t) (t * sr);
            if (upto > pos) { e.process (L.data() + pos, R.data() + pos, (int) (upto - pos)); pos = upto; }
            if (c == chords.size()) break;
            for (int n : held) e.noteOff (n);
            held = chords[c].second;
            for (int n : held) e.noteOn (n, 0.7f);
        }
        writeWav (out + "/glide_sine_on_reference_chords.wav", L, R, sr);
    }
    // 2) Finger bass tuning: pluck each note, measure frequency by autocorrelation
    {
        const int idx = presetIndex ("FINGER BASS");
        for (int note : { 28, 33, 38, 43, 48, 55 })
        {
            Engine e; e.prepare (sr); e.setAll (factoryPresets()[(size_t) idx].values());
            std::vector<float> L (sr), R (sr);
            e.noteOn (note, 0.8f); e.process (L.data(), R.data(), sr);
            const int a = sr / 4, len = sr / 2;
            const double want = 440.0 * std::pow (2.0, (note - 69) / 12.0);
            int best = 0; double bestV = -1;
            for (int lag = (int) (sr / want * 0.8); lag < (int) (sr / want * 1.25); ++lag)
            {
                double acc = 0; for (int i = 0; i < len - lag; ++i) acc += L[(size_t) (a + i)] * L[(size_t) (a + i + lag)];
                if (acc > bestV) { bestV = acc; best = lag; }
            }
            // parabolic refinement
            auto ac = [&] (int lag) { double acc = 0; for (int i = 0; i < len - lag; ++i) acc += L[(size_t) (a + i)] * L[(size_t) (a + i + lag)]; return acc; };
            const double y0 = ac (best - 1), y1 = ac (best), y2 = ac (best + 1);
            const double lagF = best + 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2);
            const double got = sr / lagF;
            double peak = 0; for (float x : L) peak = std::max (peak, (double) std::abs (x));
            std::printf ("bass note %d: want %.2f Hz got %.2f Hz (%+.1f cents) peak %.2f\n", note, want, got, 1200.0 * std::log2 (got / want), peak);
        }
    }
    std::printf ("done\n");
}
