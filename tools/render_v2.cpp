// D-SYNTH 2 demo renderer: one short phrase per new sound, written as WAV files.
// Build: g++ -O2 -std=c++17 -I../Source/Engine render_v2.cpp -o render_v2
// Usage: render_v2 SAMPLE_DIR OUT_DIR [--calibrate]
#include "DSynthEngine.h"
#include "Presets.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace dsynth;

struct Ev { double beat; int type; int note; float value; };   // 0 on, 1 off, 2 mod
struct Song
{
    std::vector<Ev> ev; double bpm = 90, beats = 16;
    void note (double t, double len, int n, float v) { ev.push_back ({ t, 0, n, v }); ev.push_back ({ t + len, 1, n, 0 }); }
    void chord (double t, double len, std::vector<int> ns, float v) { for (int n : ns) note (t, len, n, v); }
    void mod (double b0, double b1, float v0, float v1) { for (int i = 0; i <= 64; ++i) { double f = i / 64.0; ev.push_back ({ b0 + (b1 - b0) * f, 2, 0, (float) (v0 + (v1 - v0) * f) }); } }
};

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), bytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t l = (int16_t) std::lround (clampf (L[i], -1, 1) * 32767), r = (int16_t) std::lround (clampf (R[i], -1, 1) * 32767);
        std::fwrite (&l, 2, 1, f); std::fwrite (&r, 2, 1, f);
    }
    std::fclose (f);
}

static void render (const ParamValues& pv, const Song& s, const SampleBank* bank, std::vector<float>& L, std::vector<float>& R, double sr)
{
    Engine e; e.prepare (sr); e.setSampleBank (bank); e.setAll (pv); e.setTempo (s.bpm);
    auto evs = s.ev;
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.beat != b.beat ? a.beat < b.beat : (a.type == 1 && b.type != 1); });
    const size_t total = (size_t) (s.beats * 60.0 / s.bpm * sr);
    L.assign (total, 0); R.assign (total, 0);
    size_t pos = 0, ei = 0;
    while (pos < total)
    {
        while (ei < evs.size() && (size_t) (evs[ei].beat * 60.0 / s.bpm * sr) <= pos)
        {
            const auto& x = evs[ei++];
            if (x.type == 0) e.noteOn (x.note, x.value); else if (x.type == 1) e.noteOff (x.note); else e.modWheel (x.value);
        }
        size_t next = total; if (ei < evs.size()) next = std::min (next, (size_t) (evs[ei].beat * 60.0 / s.bpm * sr));
        size_t n = std::min<size_t> (std::max<size_t> (next - pos, 1), 256);
        e.process (L.data() + pos, R.data() + pos, (int) n); pos += n;
    }
}

static const std::vector<std::vector<int>> kProg = { { 39, 51, 58, 63, 67, 70 }, { 36, 48, 55, 63, 67, 70 }, { 44, 56, 60, 63, 67, 72 }, { 46, 58, 62, 65, 70, 74 } };  // Eb, Cm7, Ab, Bb (sus)

static Song songFor (const std::string& name)
{
    Song s;
    if (name == "SIGNLY")
    {
        s.bpm = 72; s.beats = 22;
        const int sc[] = { 63, 65, 67, 70, 72, 75, 77, 79, 82, 84 };
        double t = 0; s.note (t, 1.4, 63, 0.7f); t = 1.5;
        for (int i = 0; i < 10; ++i) { s.note (t, 0.24, sc[i], 0.75f); t += 0.25; }
        s.note (t, 1.6, 87, 0.8f); t += 2.0;
        for (int i = 9; i >= 0; --i) { s.note (t, 0.24, sc[i], 0.75f); t += 0.25; }
        s.note (t, 2.0, 58, 0.7f); t += 2.5;
        for (int i = 2; i < 8; ++i) { s.note (t, 0.16, sc[i], 0.8f); t += 1.0 / 6.0; }
        s.note (t, 0.9, 82, 0.8f);
        s.mod (9, 12, 0, 0.8f);
    }
    else if (name.rfind ("ARP", 0) == 0)
    {
        s.bpm = 100; s.beats = 34;
        const std::vector<std::vector<int>> v = { { 51, 55, 58, 62 }, { 48, 51, 55, 58 }, { 44, 48, 51, 55 }, { 46, 50, 53, 58 } };   // Ebmaj7, Cm7, Abmaj7, Bb
        for (int c = 0; c < 4; ++c) s.chord (c * 8.0, 7.95, v[(size_t) c], 0.8f);
        if (name == "ARP DRIVE") s.mod (16, 30, 0, 1);
    }
    else if (name == "STRING DINSEI")
    {
        // an original lyrical phrase: legato melody (overlapping notes = glissando), soft and strong,
        // over sustained chords that change by voice-leading; mod wheel swells the vibrato on long notes
        s.bpm = 72; s.beats = 34;
        auto legato = [&] (double t0, std::vector<int> ns, std::vector<double> ds, std::vector<float> vs)
        {
            double t = t0;
            for (size_t i = 0; i < ns.size(); ++i) { s.note (t, ds[i] + 0.12, ns[i], vs[i]); t += ds[i]; }
        };
        // chords (left hand), each held slightly into the next
        s.chord (0, 8.15, { 39, 51, 58, 63 }, 0.5f);
        s.chord (8, 8.15, { 36, 48, 55, 63 }, 0.5f);
        s.chord (16, 8.15, { 32, 44, 51, 60 }, 0.55f);
        s.chord (24, 9.5, { 34, 46, 53, 58 }, 0.6f);
        // melody: soft phrase (slow, deep slides), then a stronger heroic answer (quick slides)
        legato (1, { 70, 75, 74, 72, 70, 67 }, { 1.5, 1.5, 0.5, 0.5, 1.0, 3.0 }, { 0.35f, 0.4f, 0.35f, 0.35f, 0.4f, 0.35f });
        legato (9, { 67, 72, 79, 77, 75 }, { 1.0, 1.0, 2.0, 1.0, 3.0 }, { 0.4f, 0.45f, 0.5f, 0.45f, 0.4f });
        legato (17, { 72, 75, 80, 84, 82 }, { 1.0, 1.0, 1.0, 1.5, 2.5 }, { 0.85f, 0.85f, 0.9f, 0.95f, 0.9f });
        legato (25, { 82, 77, 79, 82 }, { 1.0, 1.0, 1.0, 5.5 }, { 0.6f, 0.55f, 0.55f, 0.5f });
        s.mod (3, 7, 0.0f, 0.7f); s.mod (7, 9, 0.7f, 0.1f); s.mod (12, 16, 0.1f, 0.8f); s.mod (16, 17, 0.8f, 0.3f); s.mod (27, 33, 0.2f, 0.9f);
    }
    else if (name == "THE PRAYER")
    {
        s.bpm = 66; s.beats = 26;
        const std::vector<std::vector<int>> v = { { 39, 51, 58, 63, 67 }, { 36, 48, 55, 63, 67 }, { 32, 44, 56, 60, 63 }, { 34, 46, 58, 62, 65 },
                                                  { 39, 51, 58, 63, 70 } };
        for (int c = 0; c < 5; ++c) s.chord (c * 4.0, c == 4 ? 7.0 : 4.02, v[(size_t) c], 0.6f);
        s.mod (4, 14, 0.15f, 0.7f); s.mod (14, 20, 0.7f, 0.3f);
    }
    else if (name == "HOLY G")
    {
        s.bpm = 92; s.beats = 26;
        const std::vector<std::vector<int>> v = { { 58, 63, 67, 70 }, { 58, 63, 67, 72 }, { 60, 63, 68, 72 }, { 58, 62, 65, 70 } };
        const int lh[] = { 51, 48, 44, 46 };
        double t = 0;
        for (int r = 0; r < 2; ++r) for (int c = 0; c < 4; ++c)
        {
            s.chord (t, 0.45, v[(size_t) c], 0.8f); s.chord (t + 0.75, 0.25, v[(size_t) c], 0.8f); s.chord (t + 1.5, 0.45, v[(size_t) c], 0.8f);
            s.note (t, 1.9, lh[c], 0.8f); t += 2.0;
        }
        s.chord (t, 9.5, { 58, 63, 67, 70, 75 }, 0.8f); s.note (t, 9.5, 51, 0.8f);
        s.mod (t + 1.5, t + 1.6, 0, 1); s.mod (t + 6.0, t + 6.1, 1, 0);
    }
    else if (name == "FIRE KEYS")
    {
        s.bpm = 132; s.beats = 32;
        for (int bar = 0; bar < 8; ++bar)
        {
            auto ch = kProg[(size_t) (bar / 2 % 4)]; std::vector<int> v (ch.begin() + 2, ch.end());
            const double b = bar * 4.0;
            const double hits[] = { 0, 0.75, 1.5, 2.5, 3.0, 3.5 };
            for (double h : hits) s.chord (b + h, 0.3, v, h == 0 ? 0.95f : 0.8f);
            s.note (b, 1.4, ch[0], 0.9f); s.note (b + 2.5, 1.0, ch[0] + 12, 0.85f);
        }
        s.mod (16, 17, 0, 1);
    }
    else if (name == "GLORY BRASS")
    {
        s.bpm = 110; s.beats = 26;
        double t = 0;
        for (int r = 0; r < 2; ++r)
        {
            s.chord (t, 0.25, { 51, 58, 63, 67 }, 1.0f); s.chord (t + 0.75, 0.25, { 51, 58, 63, 67 }, 0.95f);
            s.chord (t + 1.5, 0.4, { 53, 58, 62, 65 }, 1.0f); s.chord (t + 2.5, 1.2, { 55, 60, 63, 67 }, 0.9f); t += 4.0;
        }
        s.chord (t, 6.0, { 44, 51, 56, 60, 63 }, 0.55f); s.mod (t, t + 5.5, 0, 1); t += 7;
        s.chord (t, 0.3, { 46, 53, 58, 62, 65 }, 1.0f); s.chord (t + 1, 0.3, { 46, 53, 58, 62, 65 }, 1.0f);
        s.chord (t + 2, 3.5, { 39, 51, 58, 63, 67, 70 }, 1.0f);
    }
    else if (name == "SYNTH BASS")
    {
        s.bpm = 100; s.beats = 16;
        const int line[] = { 39, 39, 51, 39, 46, 48, 39, 0, 36, 36, 48, 36, 43, 44, 46, 47 };
        for (int i = 0; i < 16; ++i) if (line[i]) s.note (i * 0.5 + (i % 2 ? 0.05 : 0), i % 4 == 3 ? 0.5 : 0.35, line[i], i % 4 == 0 ? 1.0f : 0.75f);
        for (int i = 0; i < 16; ++i) if (line[i]) s.note (8 + i * 0.5, 0.4, line[i] - 4, 0.85f);
    }
    else if (name == "SUB BASS")
    {
        s.bpm = 75; s.beats = 16;
        s.note (0, 1.4, 27, 1); s.note (1.5, 0.4, 27, 0.8f); s.note (2, 1.5, 34, 0.9f); s.note (3.3, 0.6, 39, 0.9f);   // overlap = glide
        s.note (4, 1.2, 32, 1); s.note (5.0, 1.0, 36, 0.9f); s.note (6.0, 1.8, 34, 0.9f);
        s.note (8, 1.0, 27, 1); s.note (8.8, 0.5, 39, 0.9f); s.note (9.5, 2.5, 34, 1); s.note (12, 3.5, 32, 1); s.note (13.5, 2.0, 27, 0.9f);
    }
    else if (name == "RISER")
    {
        s.bpm = 100; s.beats = 19;
        s.chord (0, 16, { 51, 58, 63, 67 }, 0.8f);
    }
    else if (name == "FX KIT")
    {
        s.bpm = 100; s.beats = 52;
        s.note (0, 8, 49, 1);          // uplifter 2 bars
        s.note (8, 1, 51, 1);          // impact
        s.note (12, 1, 53, 1);         // short whoosh
        s.note (14, 1, 52, 1);         // sub drop
        s.note (18, 4, 55, 1);         // reverse cymbal 1 bar...
        s.note (22, 1, 63, 1);         // ...into a long boom
        s.note (28, 1, 56, 1);         // tape stop
        s.note (30, 4, 60, 1);         // tom fill
        s.note (34, 1, 59, 1);         // hit
        s.note (36, 4, 65, 1);         // glitch
        s.note (40, 8, 62, 1);         // snare roll riser 2 bars
        s.note (48, 1, 51, 1);         // impact
    }
    return s;
}

int main (int argc, char** argv)
{
    if (argc < 3) { std::printf ("usage: render_v2 SAMPLE_DIR OUT_DIR [--calibrate] [NAME]\n"); return 1; }
    const double sr = 44100.0;
    static SampleBank bank;
    std::printf ("samples: %s\n", bank.load (argv[1]) ? "ok" : "missing");
    if (argc > 5 && std::strcmp (argv[3], "--file") == 0)
    {
        // render a note list (BPM line, then "start_beats length_beats note velocity") with one sound
        Song s; FILE* f = std::fopen (argv[4], "r"); if (! f) return 1;
        char line[256]; double maxEnd = 0;
        while (std::fgets (line, sizeof line, f))
        {
            double a, b; int n; float v;
            if (std::sscanf (line, "BPM %lf", &a) == 1) { s.bpm = a; continue; }
            if (std::sscanf (line, "%lf %lf %d %f", &a, &b, &n, &v) == 4) { s.note (a, b, n, v); maxEnd = std::max (maxEnd, a + b); }
            else if (std::sscanf (line, "MOD %lf %lf %f", &a, &b, &v) == 3) s.mod (a, b, 0.0f, v);
        }
        std::fclose (f);
        s.beats = maxEnd + 6.0;
        for (const auto& p : factoryPresets())
            if (std::string (p.name) == argv[5])
            {
                std::vector<float> L, R; render (p.values(), s, &bank, L, R, sr);
                writeWav (argv[2], L, R, (int) sr); std::printf ("wrote %s\n", argv[2]);
            }
        return 0;
    }
    const bool cal = argc > 3 && std::strcmp (argv[3], "--calibrate") == 0;
    const std::string only = argc > 4 ? argv[4] : (argc > 3 && ! cal ? argv[3] : "");
    int idx = 0;
    for (const auto& p : factoryPresets())
    {
        ++idx;
        const std::string name = p.name;
        if (! only.empty() && name != only) continue;
        Song s = songFor (name);
        if (s.ev.empty()) continue;
        ParamValues pv = p.values();
        std::vector<float> L, R;
        if (cal)
        {
            float out = pv[pOutput];
            for (int it = 0; it < 3; ++it)
            {
                pv[pOutput] = out; render (pv, s, &bank, L, R, sr);
                double acc = 0, pk = 0;
                for (size_t i = 0; i < L.size(); ++i) { acc += 0.5 * (L[i] * L[i] + R[i] * R[i]); pk = std::max (pk, (double) std::max (std::abs (L[i]), std::abs (R[i]))); }
                const double rms = 10 * std::log10 (acc / (double) L.size() + 1e-12), pkDb = 20 * std::log10 (pk + 1e-12);
                out += (float) std::min (-16.0 - rms, -2.0 - pkDb);
            }
            std::printf ("%s %.1f\n", p.name, out);
            continue;
        }
        render (pv, s, &bank, L, R, sr);
        char fn[256]; std::snprintf (fn, sizeof fn, "%s/%02d_%s.wav", argv[2], idx, name.c_str());
        for (char* c = fn + std::strlen (argv[2]); *c; ++c) if (*c == ' ') *c = '_';
        writeWav (fn, L, R, (int) sr);
        std::printf ("wrote %s\n", fn);
    }
    return 0;
}
