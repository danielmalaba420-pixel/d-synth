// Offline renderer: plays phrases through the D-SYNTH engine and writes WAV files.
// Build: g++ -O2 -std=c++17 -I../Source/Engine render_demo.cpp -o render_demo
#include "DSynthEngine.h"
#include "Presets.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace dsynth;

struct Ev
{
    double beat;
    enum Type { On, Off, Mod, Bend, Pedal } type;
    int note = 0;
    float value = 0;
};

struct Song
{
    std::vector<Ev> ev;
    void note (double start, double len, int n, float vel)
    {
        ev.push_back ({ start, Ev::On, n, vel });
        ev.push_back ({ start + len, Ev::Off, n, 0 });
    }
    void ramp (Ev::Type t, double b0, double b1, float v0, float v1, int steps = 48)
    {
        for (int i = 0; i <= steps; ++i)
        {
            const double f = (double) i / steps;
            ev.push_back ({ b0 + (b1 - b0) * f, t, 0, (float) (v0 + (v1 - v0) * f) });
        }
    }
};

static const int kScale[] = { 51, 53, 55, 58, 60, 63, 65, 67, 70, 72, 75, 77, 79, 82, 84, 87 }; // Eb pentatonic

static void render (const Preset& preset, const Song& song, double bpm, double totalBeats,
                    std::vector<float>& L, std::vector<float>& R, double sr)
{
    Engine e;
    e.prepare (sr);
    e.setAll (preset.values());
    e.setTempo (bpm);

    auto evs = song.ev;
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b)
    {
        if (a.beat != b.beat) return a.beat < b.beat;
        return a.type == Ev::Off && b.type != Ev::Off; // note-offs first at equal times
    });

    const size_t total = (size_t) (totalBeats * 60.0 / bpm * sr);
    const size_t start = L.size();
    L.resize (start + total, 0.0f);
    R.resize (start + total, 0.0f);

    size_t pos = 0, ei = 0;
    while (pos < total)
    {
        while (ei < evs.size() && (size_t) (evs[ei].beat * 60.0 / bpm * sr) <= pos)
        {
            const auto& x = evs[ei++];
            switch (x.type)
            {
                case Ev::On:   e.noteOn (x.note, x.value); break;
                case Ev::Off:  e.noteOff (x.note); break;
                case Ev::Mod:  e.modWheel (x.value); break;
                case Ev::Bend: e.pitchBend (x.value); break;
                case Ev::Pedal: e.sustainPedal (x.value > 0.5f); break;
            }
        }
        size_t next = total;
        if (ei < evs.size()) next = std::min (next, (size_t) (evs[ei].beat * 60.0 / bpm * sr));
        size_t n = std::min<size_t> (next - pos, 256);
        if (n == 0) n = 1;
        e.process (L.data() + start + pos, R.data() + start + pos, (int) n);
        pos += n;
    }
}

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) L.size();
    const uint32_t dataBytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t l = (int16_t) std::lround (clampf (L[i], -1.0f, 1.0f) * 32767.0f);
        int16_t r = (int16_t) std::lround (clampf (R[i], -1.0f, 1.0f) * 32767.0f);
        std::fwrite (&l, 2, 1, f); std::fwrite (&r, 2, 1, f);
    }
    std::fclose (f);
}

// The Rising Run showcase: climbing runs, power building with velocity and the mod wheel
static Song risingRunSong()
{
    Song s;
    double t = 0.0;
    // 1. low note, then a fast climb (16th-note triplets), velocity rising
    s.note (t, 0.9, 51, 0.6f); t += 1.0;
    for (int i = 1; i < 16; ++i)
    {
        const float vel = 0.5f + 0.5f * (float) i / 15.0f;
        s.note (t, 1.0 / 6.0 + 0.03, kScale[i], vel);
        t += 1.0 / 6.0;
    }
    // land on the top note, hold, bend up a whole step and back
    s.note (t, 2.3, 87, 1.0f);
    s.ramp (Ev::Bend, t + 0.8, t + 1.2, 0.0f, 2.0f / 12.0f, 16);
    s.ramp (Ev::Bend, t + 1.6, t + 2.0, 2.0f / 12.0f, 0.0f, 16);
    t += 2.3 + 1.7; // let the delay ring

    // 2. same climb, this time pushing the mod wheel: Build opens everything up
    s.ramp (Ev::Mod, t, t + 2.6, 0.0f, 1.0f, 64);
    s.note (t, 0.45, 63, 0.7f); t += 0.5;
    for (int i = 6; i < 16; ++i)
    {
        s.note (t, 1.0 / 8.0 + 0.02, kScale[i], 0.65f + 0.35f * (float) (i - 6) / 9.0f);
        t += 1.0 / 8.0;
    }
    // quick turn: down three notes and back up
    const int turn[] = { 84, 82, 84, 87, 89, 91 };
    for (int n : turn) { s.note (t, 1.0 / 8.0 + 0.02, n, 1.0f); t += 1.0 / 8.0; }
    s.note (t, 2.5, 94, 1.0f);
    s.ramp (Ev::Bend, t + 1.0, t + 1.3, 0.0f, 1.0f / 12.0f, 12);
    t += 2.5;
    s.ramp (Ev::Mod, t, t + 1.0, 1.0f, 0.0f, 16);
    return s;
}

// The Glide showcase: a gospel progression where each chord glides into the next
static Song glideSong()
{
    Song s;
    const std::vector<std::vector<int>> chords = {
        { 39, 46, 55, 58, 62, 65 },   // Ebmaj9
        { 36, 43, 58, 63, 65, 67 },   // Cm11
        { 44, 51, 55, 60, 63, 70 },   // Abmaj9
        { 46, 53, 56, 60, 63, 67 },   // Bb13sus
        { 39, 46, 55, 58, 62, 65 },   // Ebmaj9
        { 41, 48, 57, 60, 63, 67 },   // F13
        { 44, 51, 55, 60, 63, 70 },   // Abmaj9
        { 46, 53, 56, 61, 63, 68 },   // Bb7sus(b9)
    };
    double t = 0.0;
    for (size_t c = 0; c < chords.size(); ++c)
    {
        const double len = (c % 4 == 3) ? 3.9 : 1.95;   // legato: each new chord glides out of the previous one
        int k = 0;
        for (int n : chords[c]) { s.note (t + 0.012 * k, len, n, 0.62f + 0.05f * (float) (k % 3)); ++k; }
        t += (c % 4 == 3) ? 4.0 : 2.0;
    }
    // final chord with the sustain pedal: lift your hands, the pad leaves, the Rhodes keeps ringing
    s.ev.push_back ({ t, Ev::Pedal, 0, 1.0f });
    int k = 0;
    for (int n : { 39, 46, 55, 58, 62, 65, 70 }) s.note (t + 0.015 * k++, 1.9, n, 0.75f);
    s.ev.push_back ({ t + 6.5, Ev::Pedal, 0, 0.0f });
    return s;
}

// Finger bass groove: gospel/R&B line in Eb, with ghost notes and an octave pop
static Song bassSong()
{
    Song s;
    struct N { double b, len; int n; float v; };
    const std::vector<N> bar = {
        { 0.0, 0.45, 39, 0.95f }, { 0.75, 0.2, 39, 0.45f }, { 1.0, 0.45, 46, 0.8f }, { 1.5, 0.2, 51, 0.7f },
        { 2.0, 0.7, 44, 0.9f }, { 2.75, 0.2, 44, 0.4f }, { 3.0, 0.3, 46, 0.8f }, { 3.5, 0.4, 41, 0.75f } };
    const int roots[] = { 0, -3, -7, -2 };   // Eb, C, Ab, Bb
    for (int bar_i = 0; bar_i < 8; ++bar_i)
        for (auto x : bar)
            s.note (bar_i * 4.0 + x.b, x.len, x.n + roots[bar_i % 4], x.v);
    s.note (32.0, 1.6, 39, 1.0f);   // final low Eb rings
    return s;
}

// A short phrase for each lead in the tour
static Song tourPhrase (bool poly)
{
    Song s;
    if (poly)
    {
        const int chord1[] = { 63, 67, 70, 74 }, chord2[] = { 65, 68, 72, 75 };
        for (int n : chord1) s.note (0.0, 0.45, n, 0.85f);
        for (int n : chord1) s.note (0.75, 0.3, n, 0.75f);
        for (int n : chord2) s.note (1.5, 1.2, n, 0.9f);
        const int top[] = { 75, 77, 79, 82 };
        double t = 3.0;
        for (int n : top) { s.note (t, 0.22, n, 0.8f); t += 0.25; }
    }
    else
    {
        double t = 0.0;
        const int run[] = { 63, 65, 67, 70, 72, 75, 77, 79 };
        for (int n : run) { s.note (t, 0.2, n, 0.8f); t += 0.1667; }
        s.note (t, 1.3, 82, 0.9f); t += 1.5;
        s.note (t, 0.45, 79, 0.8f); t += 0.5;
        s.note (t, 0.9, 75, 0.8f);
    }
    return s;
}

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : ".";
    const double sr = 44100.0, bpm = 92.0;
    const auto& presets = factoryPresets();

    if (outDir == "--calibrate")
    {
        for (const auto& p : presets)
        {
            auto pr = p;
            float out = p.values()[pOutput];
            for (int it = 0; it < 4; ++it)
            {
                pr.overrides.push_back ({ pOutput, out });
                std::vector<float> L, R;
                render (pr, tourPhrase (p.values()[pPoly] > 0.5f), bpm, 6.0, L, R, sr);
                double acc = 0, pk = 0;
                for (size_t i = 0; i < L.size(); ++i)
                {
                    acc += 0.5 * (L[i] * L[i] + R[i] * R[i]);
                    pk = std::max (pk, (double) std::max (std::abs (L[i]), std::abs (R[i])));
                }
                const double rms = 10.0 * std::log10 (acc / (double) L.size() + 1e-12);
                const double pkDb = 20.0 * std::log10 (pk + 1e-12);
                out += (float) std::min (-16.0 - rms, -2.0 - pkDb);   // loudness match, but keep 2 dB of peak headroom
            }
            std::printf ("%s %.1f\n", p.name, out);
        }
        return 0;
    }

    {
        std::vector<float> L, R;
        render (presets[0], risingRunSong(), bpm, 23.0, L, R, sr);
        writeWav (outDir + "/D-SYNTH_Rising_Run.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        render (presets[1], risingRunSong(), bpm, 23.0, L, R, sr);
        writeWav (outDir + "/D-SYNTH_The_Run.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        render (presets[10], glideSong(), 72.0, 31.0, L, R, sr);
        writeWav (outDir + "/D-SYNTH_Elle_Glide.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        render (presets[11], glideSong(), 72.0, 31.0, L, R, sr);
        writeWav (outDir + "/D-SYNTH_Glide_Sine.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        render (presets[9], glideSong(), 72.0, 31.0, L, R, sr);
        writeWav (outDir + "/D-SYNTH_String_Glide.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        render (presets[7], bassSong(), 92.0, 34.5, L, R, sr);
        writeWav (outDir + "/D-SYNTH_Finger_Bass.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        render (presets[3], risingRunSong(), bpm, 23.0, L, R, sr);
        render (presets[4], risingRunSong(), bpm, 23.0, L, R, sr);
        writeWav (outDir + "/D-SYNTH_Soft_Sine_Flute_Duo.wav", L, R, (int) sr);
    }
    {
        std::vector<float> L, R;
        for (const auto& p : presets)
        {
            const bool poly = p.values()[pPoly] > 0.5f;
            render (p, tourPhrase (poly), bpm, 6.0, L, R, sr);
        }
        writeWav (outDir + "/D-SYNTH_12_Leads.wav", L, R, (int) sr);
    }
    std::printf ("done\n");
    return 0;
}
