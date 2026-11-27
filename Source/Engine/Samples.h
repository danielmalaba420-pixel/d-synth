// D-SYNTH by Danova — sample bank (strings, brass, piano)
// Samples: VS Chamber Orchestra Community Edition (CC0 1.0) by Versilian Studios,
// prepared by tools/prep_samples.py into raw int16 files + samples.txt.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

namespace dsynth
{

enum SampleInst
{
    kViolins = 0, kViolas, kCellos, kBasses,
    kTrumpetSus, kTrumpetStac, kHornSus, kHornStac, kTromboneSus, kTromboneStac, kTubaSus, kTubaStac,
    kPiano,
    kOrgGospel, kOrg888862200, kOrgAll9, kOrgJazz,   // Daniel Malaba's organ recordings, four registrations
    kGlock,
    kDanStrings,                                      // Daniel Malaba's string ensemble
    kDanFilm,                                         // and his "film" strings layer (air on top)
    kNumSampleInsts
};

struct SampleZone
{
    int inst = 0, root = 60, velLo = 0, velHi = 127;
    int frames = 0, channels = 1;
    float rate = 44100.0f;
    int loopStart = 0, loopEnd = 0;   // loopEnd > loopStart: sustained (looped) sample
    float gain = 1.0f;
    std::vector<int16_t> pcm;         // interleaved

    bool looped() const { return loopEnd > loopStart + 16; }

    // 4-point Hermite interpolation (cleaner than linear while the pitch moves: no gritty imaging on slides),
    // wraps across the loop so the sustain stays seamless. Returns left/right.
    inline int idx (int i) const
    {
        if (looped() && i >= loopEnd) i -= (loopEnd - loopStart);
        return std::clamp (i, 0, frames - 1);
    }
    static inline float herm (float xm1, float x0, float x1, float x2, float t)
    {
        const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }
    inline void read (double pos, float& l, float& r) const
    {
        const int i0 = (int) pos;
        const float f = (float) (pos - (double) i0);
        const int im = idx (i0 - 1), ia = idx (i0), ib = idx (i0 + 1), ic = idx (i0 + 2);
        const float k = gain / 32768.0f;
        const int16_t* d = pcm.data();
        if (channels == 2)
        {
            l = herm (d[2 * im], d[2 * ia], d[2 * ib], d[2 * ic], f) * k;
            r = herm (d[2 * im + 1], d[2 * ia + 1], d[2 * ib + 1], d[2 * ic + 1], f) * k;
        }
        else l = r = herm (d[im], d[ia], d[ib], d[ic], f) * k;
    }
};

struct SampleBank
{
    std::vector<SampleZone> zones;
    std::vector<int> byInst[kNumSampleInsts];

    bool empty() const { return zones.empty(); }

    // Loads samples.txt and the raw files next to it. Returns false if nothing could be read.
    bool load (const std::string& dir)
    {
        zones.clear();
        for (auto& b : byInst) b.clear();
        const std::string sep = (! dir.empty() && (dir.back() == '/' || dir.back() == '\\')) ? "" : "/";
        FILE* m = std::fopen ((dir + sep + "samples.txt").c_str(), "r");
        if (! m) return false;
        char line[512];
        while (std::fgets (line, sizeof (line), m))
        {
            if (line[0] == '#' || line[0] == '\n') continue;
            SampleZone z; char fn[256] = { 0 };
            float rate = 44100.0f;
            if (std::sscanf (line, "%d %d %d %d %d %d %f %d %d %f %255s", &z.inst, &z.root, &z.velLo, &z.velHi,
                             &z.frames, &z.channels, &rate, &z.loopStart, &z.loopEnd, &z.gain, fn) != 11) continue;
            if (z.inst < 0 || z.inst >= kNumSampleInsts || z.frames < 2 || z.channels < 1 || z.channels > 2) continue;
            z.rate = rate;
            FILE* f = std::fopen ((dir + sep + fn).c_str(), "rb");
            if (! f) continue;
            z.pcm.resize ((size_t) z.frames * (size_t) z.channels);
            const size_t got = std::fread (z.pcm.data(), sizeof (int16_t), z.pcm.size(), f);
            std::fclose (f);
            if (got != z.pcm.size()) continue;
            zones.push_back (std::move (z));
        }
        std::fclose (m);
        for (int i = 0; i < (int) zones.size(); ++i) byInst[zones[(size_t) i].inst].push_back (i);
        return ! zones.empty();
    }

    // nearest zone for this note and velocity (0..127)
    const SampleZone* find (int inst, int note, int vel) const
    {
        if (inst < 0 || inst >= kNumSampleInsts) return nullptr;
        const SampleZone* best = nullptr; int bestD = 1 << 30;
        for (int idx : byInst[inst])
        {
            const auto& z = zones[(size_t) idx];
            if (vel < z.velLo || vel > z.velHi) continue;
            // prefer playing a sample slightly down rather than up (sounds more natural)
            const int d = std::abs (z.root - note) * 2 + (z.root < note ? 1 : 0);
            if (d < bestD) { bestD = d; best = &z; }
        }
        return best;
    }
};

} // namespace dsynth
