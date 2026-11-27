// D-SYNTH by Danova — sound engine
// Plain C++17, no framework dependency: used by the plugin and by the offline demo renderer.
#pragma once

#include "Params.h"
#include "Samples.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dsynth
{

constexpr double kPi = 3.14159265358979323846;
constexpr int kMaxVoices = 16;
constexpr int kMaxUnison = 7;

inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float clampf (float x, float lo, float hi) { return std::min (hi, std::max (lo, x)); }

// Fast, deterministic noise (xorshift)
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) / 8388608.0f - 1.0f; }
    float unit() { return 0.5f * (next() + 1.0f); }
};

//==============================================================================
// Band-limited oscillator (PolyBLEP)
struct Osc
{
    double phase = 0.0;

    static double blep (double t, double dt)
    {
        if (t < dt)       { t /= dt;             return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt;  return t * t + t + t + 1.0; }
        return 0.0;
    }

    float tick (int wave, double dt)
    {
        double out = 0.0;
        const double t = phase;
        switch (wave)
        {
            case 0: out = 2.0 * t - 1.0 - blep (t, dt); break;                       // saw
            case 1:                                                                 // square
            {
                out = t < 0.5 ? 1.0 : -1.0;
                out += blep (t, dt);
                double t2 = t + 0.5; if (t2 >= 1.0) t2 -= 1.0;
                out -= blep (t2, dt);
                break;
            }
            case 2: out = std::sin (2.0 * kPi * t); break;                           // sine
            default: out = 1.0 - 4.0 * std::abs (t - 0.5); break;                    // triangle
        }
        phase += dt;
        if (phase >= 1.0) phase -= std::floor (phase);
        return (float) out;
    }
};

//==============================================================================
// ADSR with exponential segments
struct Env
{
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0.0f;
    float aCoef = 0, aBase = 0, dCoef = 0, dBase = 0, rCoef = 0, rBase = 0, sustain = 1.0f;

    static float coef (float ms, float sr, float ratio)
    {
        const float n = std::max (1.0f, ms * 0.001f * sr);
        return std::exp (-std::log ((1.0f + ratio) / ratio) / n);
    }

    void set (float aMs, float dMs, float s, float rMs, float sr)
    {
        sustain = s;
        const float ra = 0.3f, rd = 0.0001f;
        aCoef = coef (aMs, sr, ra); aBase = (1.0f + ra) * (1.0f - aCoef);
        dCoef = coef (dMs, sr, rd); dBase = (sustain - rd) * (1.0f - dCoef);
        rCoef = coef (rMs, sr, rd); rBase = -rd * (1.0f - rCoef);
    }

    void gate (bool on) { stage = on ? Attack : (stage == Idle ? Idle : Release); }

    float tick()
    {
        switch (stage)
        {
            case Attack:
                value = aBase + value * aCoef;
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value = dBase + value * dCoef;
                if (value <= sustain) { value = sustain; stage = Sustain; }
                break;
            case Sustain: value = sustain; break;
            case Release:
                value = rBase + value * rCoef;
                if (value <= 0.0f) { value = 0.0f; stage = Idle; }
                break;
            case Idle: break;
        }
        return value;
    }
};

//==============================================================================
// Zero-delay-feedback state-variable filter (lowpass output)
struct SVF
{
    float ic1 = 0, ic2 = 0;
    float process (float v0, float a1, float a2, float a3)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v2;
    }
    void reset() { ic1 = ic2 = 0; }
};

struct OnePole
{
    float z = 0;
    float lp (float x, float c) { z += c * (x - z); return z; }
    float hp (float x, float c) { return x - lp (x, c); }
    static float coefFor (float hz, float sr) { return 1.0f - std::exp (-2.0f * (float) kPi * hz / sr); }
};

// State-variable filter with lowpass and bandpass outputs (used by the FX kit)
struct SVF2
{
    float ic1 = 0, ic2 = 0;
    void tick (float v0, float g, float k, float& lp, float& bp)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1;
    }
};

// sine lookup (phase in cycles, 0..1)
inline float sinTab (double ph)
{
    static const std::vector<float> t = []
    {
        std::vector<float> v (4097);
        for (int i = 0; i <= 4096; ++i) v[(size_t) i] = (float) std::sin (2.0 * kPi * i / 4096.0);
        return v;
    }();
    const double x = (ph - std::floor (ph)) * 4096.0;
    const int i = (int) x; const float f = (float) (x - i);
    return t[(size_t) i] + f * (t[(size_t) i + 1] - t[(size_t) i]);
}

static constexpr const char* kFxNames[20] = {
    "MONTEE 4 MESURES", "MONTEE 2 MESURES", "DESCENTE", "IMPACT BOUM", "SUB DROP",
    "WHOOSH COURT", "WHOOSH LONG", "CYMBALE INVERSEE", "TAPE STOP", "SWELL INVERSE",
    "SOUFFLE QUI MONTE", "HIT", "ROULEMENT DE TOMS", "LASER", "ROULEMENT DE CAISSE",
    "BOUM LONG", "BALAYAGE FILTRE", "GLITCH", "VENT", "PULSATION SUB" };
constexpr int kFxFirstNote = 48;   // C3: the 20 effects sit on C3..G4
static constexpr double kOrgRatio[9] = { 0.5, 1.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0 };   // 16' 5 1/3' 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1'

//==============================================================================
struct Voice
{
    bool active = false;
    int note = 60;
    float velocity = 1.0f;
    double pitch = 60.0;        // current (gliding) pitch in semitones
    double target = 60.0;
    uint64_t age = 0;
    float timeSinceOn = 0.0f;   // seconds, for vibrato delay

    std::array<Osc, kMaxUnison> uni;
    Osc oscB, sub;
    Env amp, filt;
    SVF fL1, fL2, fR1, fR2;

    // electric piano layer (FM tine model)
    double epCar = 0, epMod = 0, epTine = 0, epLow = 0, epCar2 = 0, epMod2 = 0;
    Osc shim1, shim2;                // shimmer layer of the pad (+1 and +2 octaves)
    OnePole epToneL, epToneR, rhTone;
    float epToneCoef = 0.3f;
    float shimLfo = 0.0f, shimRate = 0.4f;
    float epEnv = 0, epBarkEnv = 0, epTineEnv = 0, epAtk = 0;
    float epCoefHeld = 0.9999f, epCoefRel = 0.999f;
    // finger bass: plucked string (Karplus-Strong style waveguide)
    std::vector<float> ks;
    int ksW = 0;
    float ksLp = 0.0f, ksDc = 0.0f, ksDcIn = 0.0f, ksAmp = 0.0f, ksPrev = 0.0f, muteRamp = 0.0f;
    int muteSamples = 0;
    float ksG = 0.0f, ksDamp = 0.0f, ksD = 0.0f;
    int excPos = 1 << 30, excLen = 0;
    float excPeak = 0.25f, excLpState = 0.0f, excLpCoef = 0.3f, excGain = 1.0f;
    float thumpPhase = 1.0f, thumpGain = 0.0f, bassBright = 0.0f, bassBrightCoef = 0.99f;
    OnePole bassTone1, bassTone2;
    // organ layer (tonewheels)
    std::array<double, 9> orgPh {};
    float clickEnv = 0.0f;
    OnePole clickLp;
    // sampled layer: up to 8 players (one per instrument / articulation)
    struct SmpPlay { const SampleZone* z = nullptr; double pos = 0; double rootMul = 1; float gain = 0; float pan = 0; };
    std::array<SmpPlay, 8> smp;
    int nSmp = 0;
    float smpTrans = 0.0f;   // glockenspiel sounds two octaves up
    uint8_t mask = 3;
    double glideMid = 60.0;  // first stage of the S-shaped (string-like) glide
    float padFade = 1.0f;
    float arpPan = 0.0f;     // arpeggio notes travel from left (low) to right (high)
    OnePole smpTiltL, smpTiltR;
    // FX kit
    int fxType = -1;
    float fxT = 0, fxNext = 0, fxHitT = 10, fxRel = 1, fxHitF = 0, fxPan = 0;
    int fxHits = 0;
    double fxPh[4] = { 0, 0, 0, 0 };
    SVF2 fxA, fxB;
    bool released = false;      // key up (or pedal up)
    bool keyDown = false;       // finger still on the key
    bool sustained = false;     // key up but held by the sustain pedal

    void start (int n, float vel, bool retriggerEnvs, bool glideFromCurrent, Rng& rng)
    {
        note = n; velocity = vel; target = n;
        if (! glideFromCurrent) pitch = n;
        if (retriggerEnvs)
        {
            if (! active)
            {
                for (auto& o : uni) o.phase = rng.unit();
                oscB.phase = rng.unit();
                sub.phase = 0.0;
                fL1.reset(); fL2.reset(); fR1.reset(); fR2.reset();
            }
            amp.gate (true); filt.gate (true);
            timeSinceOn = 0.0f;
            epCar = epMod = epTine = epLow = epCar2 = epMod2 = 0.0;
            shim1.phase = rng.unit(); shim2.phase = rng.unit();
            shimLfo = rng.unit(); shimRate = 0.25f + 0.35f * rng.unit();
            epEnv = 1.0f; epBarkEnv = 1.0f; epTineEnv = 1.0f; epAtk = 0.0f;
            clickEnv = 1.0f;
            if (! active) for (auto& o : orgPh) o = rng.unit();
        }
        released = false; keyDown = true; sustained = false;
        active = true;
    }

    void release() { amp.gate (false); filt.gate (false); released = true; keyDown = false; sustained = false; }
};

//==============================================================================
// Freeverb-style stereo reverb
struct Reverb
{
    struct Comb
    {
        std::vector<float> buf; int idx = 0; float store = 0;
        float process (float in, float fb, float damp)
        {
            float out = buf[(size_t) idx];
            store = out * (1.0f - damp) + store * damp;
            buf[(size_t) idx] = in + store * fb;
            if (++idx >= (int) buf.size()) idx = 0;
            return out;
        }
    };
    struct AllPass
    {
        std::vector<float> buf; int idx = 0;
        float process (float in)
        {
            float bufout = buf[(size_t) idx];
            float out = -in + bufout;
            buf[(size_t) idx] = in + bufout * 0.5f;
            if (++idx >= (int) buf.size()) idx = 0;
            return out;
        }
    };

    std::array<Comb, 8> cL, cR;
    std::array<AllPass, 4> aL, aR;

    void prepare (double sr)
    {
        const int combT[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        const int apT[4] = { 556, 441, 341, 225 };
        const double k = sr / 44100.0;
        for (int i = 0; i < 8; ++i)
        {
            cL[(size_t) i].buf.assign ((size_t) std::max (1, (int) (combT[i] * k)), 0.0f);
            cR[(size_t) i].buf.assign ((size_t) std::max (1, (int) ((combT[i] + 23) * k)), 0.0f);
            cL[(size_t) i].idx = cR[(size_t) i].idx = 0;
            cL[(size_t) i].store = cR[(size_t) i].store = 0;
        }
        for (int i = 0; i < 4; ++i)
        {
            aL[(size_t) i].buf.assign ((size_t) std::max (1, (int) (apT[i] * k)), 0.0f);
            aR[(size_t) i].buf.assign ((size_t) std::max (1, (int) ((apT[i] + 23) * k)), 0.0f);
            aL[(size_t) i].idx = aR[(size_t) i].idx = 0;
        }
    }

    void process (float inL, float inR, float size, float damp, float& outL, float& outR)
    {
        const float fb = size * 0.28f + 0.7f;
        const float d = damp * 0.4f;
        const float in = (inL + inR) * 0.015f;
        float l = 0, r = 0;
        for (size_t i = 0; i < 8; ++i) { l += cL[i].process (in, fb, d); r += cR[i].process (in, fb, d); }
        for (size_t i = 0; i < 4; ++i) { l = aL[i].process (l); r = aR[i].process (r); }
        outL = l; outR = r;
    }
};

//==============================================================================
// Smooth hall: input diffusion + 8-line feedback delay network (Hadamard mixing),
// damped and slowly modulated, as used by modern studio reverbs to avoid metallic ringing.
struct HallReverb
{
    static constexpr int N = 8;
    std::array<std::vector<float>, N> line;
    std::array<int, N> w {}, mask {};
    std::array<float, N> base {}, lpz {}, gain {}, modPh {}, modRate {};
    std::array<std::vector<float>, 4> apL, apR;
    std::array<int, 4> apI {};
    float sr = 44100.0f, lastSize = -1.0f, lastDamp = -1.0f, lpc = 0.5f;

    void prepare (double rate)
    {
        sr = (float) rate;
        const float ms[N] = { 31.7f, 37.3f, 41.9f, 47.3f, 53.1f, 59.3f, 67.1f, 73.9f };
        for (int i = 0; i < N; ++i)
        {
            base[(size_t) i] = ms[i] * 0.001f * sr;
            int sz = 1; while (sz < (int) (base[(size_t) i] * 1.7f) + 64) sz <<= 1;
            line[(size_t) i].assign ((size_t) sz, 0.0f); mask[(size_t) i] = sz - 1; w[(size_t) i] = 0; lpz[(size_t) i] = 0.0f;
            modPh[(size_t) i] = 0.13f * (float) i; modRate[(size_t) i] = 0.21f + 0.11f * (float) i;
        }
        const int ap[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i)
        {
            apL[(size_t) i].assign ((size_t) std::max (1, (int) (ap[i] * rate / 44100.0)), 0.0f);
            apR[(size_t) i].assign ((size_t) std::max (1, (int) ((ap[i] + 31) * rate / 44100.0)), 0.0f);
            apI[(size_t) i] = 0;
        }
        lastSize = -1.0f;
    }

    static float allpass (std::vector<float>& b, int idx, float x, float g)
    {
        const float d = b[(size_t) idx];
        const float y = -g * x + d;
        b[(size_t) idx] = x + g * y;
        return y;
    }

    void process (float inL, float inR, float size, float damp, float& outL, float& outR)
    {
        if (size != lastSize || damp != lastDamp)
        {
            lastSize = size; lastDamp = damp;
            const float scale = 0.7f + 0.9f * size, rt60 = 1.2f + 4.2f * size;
            for (int i = 0; i < N; ++i)
                gain[(size_t) i] = std::pow (10.0f, -3.0f * base[(size_t) i] * scale / (rt60 * sr));
            lpc = 1.0f - std::exp (-2.0f * (float) kPi * (1200.0f + 9000.0f * (1.0f - damp)) / sr);
        }
        const float scale = 0.7f + 0.9f * size;
        float dl = inL, dr = inR;
        for (int i = 0; i < 4; ++i)
        {
            dl = allpass (apL[(size_t) i], apI[(size_t) i] % (int) apL[(size_t) i].size(), dl, 0.6f);
            dr = allpass (apR[(size_t) i], apI[(size_t) i] % (int) apR[(size_t) i].size(), dr, 0.6f);
        }
        for (int i = 0; i < 4; ++i) { ++apI[(size_t) i]; if (apI[(size_t) i] >= 1 << 20) apI[(size_t) i] = 0; }

        float y[N];
        for (int i = 0; i < N; ++i)
        {
            modPh[(size_t) i] += modRate[(size_t) i] / sr; if (modPh[(size_t) i] >= 1.0f) modPh[(size_t) i] -= 1.0f;
            const float d = base[(size_t) i] * scale + 12.0f * std::sin (2.0f * (float) kPi * modPh[(size_t) i]);
            const auto& b = line[(size_t) i];
            float pos = (float) w[(size_t) i] - d; while (pos < 0.0f) pos += (float) b.size();
            const int i0 = (int) pos & mask[(size_t) i]; const float f = pos - std::floor (pos);
            const float v = b[(size_t) i0] + f * (b[(size_t) ((i0 + 1) & mask[(size_t) i])] - b[(size_t) i0]);
            lpz[(size_t) i] += lpc * (v - lpz[(size_t) i]);
            y[i] = lpz[(size_t) i] * gain[(size_t) i];
        }
        outL = (y[0] + y[2] + y[4] + y[6]) * 0.5f;
        outR = (y[1] + y[3] + y[5] + y[7]) * 0.5f;
        // 8x8 Hadamard mixing (fast Walsh-Hadamard), energy preserving
        for (int h = 1; h < N; h <<= 1)
            for (int i = 0; i < N; i += h << 1)
                for (int j = i; j < i + h; ++j) { const float a = y[j], c = y[j + h]; y[j] = a + c; y[j + h] = a - c; }
        for (int i = 0; i < N; ++i)
        {
            line[(size_t) i][(size_t) w[(size_t) i]] = y[i] * 0.35355339f + ((i & 1) ? dr : dl) * 0.5f;
            w[(size_t) i] = (w[(size_t) i] + 1) & mask[(size_t) i];
        }
    }
};

//==============================================================================
// Octave-up pitch shifter (two crossfaded taps), for the shimmer reverb
struct OctaveUp
{
    std::vector<float> buf = std::vector<float> (4096, 0.0f);
    int w = 0; float ph = 0.0f;
    float process (float x)
    {
        const int N = 2048;
        buf[(size_t) w] = x;
        ph += 1.0f / (float) N; if (ph >= 1.0f) ph -= 1.0f;
        float out = 0.0f;
        for (int t = 0; t < 2; ++t)
        {
            float p = ph + 0.5f * (float) t; if (p >= 1.0f) p -= 1.0f;
            const float d = (1.0f - p) * (float) N + 2.0f;
            float pos = (float) w - d; while (pos < 0.0f) pos += 4096.0f;
            const int i0 = (int) pos; const float f = pos - (float) i0;
            const float smp = buf[(size_t) i0] + f * (buf[(size_t) ((i0 + 1) & 4095)] - buf[(size_t) i0]);
            out += smp * std::sin ((float) kPi * p);
        }
        w = (w + 1) & 4095;
        return out * 0.7071f;
    }
};

//==============================================================================
class Engine
{
public:
    Engine() { values = defaultValues(); }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        dlyL.assign ((size_t) (sr * 4.0f) + 4, 0.0f);
        dlyR.assign ((size_t) (sr * 4.0f) + 4, 0.0f);
        dlyW = 0;
        reverb.prepare (sampleRate);
        hall.prepare (sampleRate);
        for (auto& v : voices)
        {
            v = Voice();
            v.ks.assign ((size_t) (sr / 20.0f) + 8, 0.0f);    // down to 20 Hz
        }
        held.clear();
        buildSm = values[pBuild];
        dlySamplesSm = -1.0f;
        compEnv = 0; duckEnv = 0;
        updateEnvelopes();
    }

    // ---- parameters
    void setParam (int id, float v) { if (id >= 0 && id < kNumParams) values[(size_t) id] = v; }
    float getParam (int id) const { return values[(size_t) id]; }
    void setAll (const ParamValues& v) { values = v; updateEnvelopes(); }
    const ParamValues& all() const { return values; }
    void setTempo (double bpm) { if (bpm > 20.0 && bpm < 400.0) tempo = (float) bpm; }
    // host position, called once per block before process(): keeps the arpeggiator on the DAW grid
    void setTransport (double ppqPosition, bool isPlaying) { hostPpq = ppqPosition; hostPlaying = isPlaying; transportValid = true; }
    // the sample bank is loaded on a background thread and handed over when ready
    void setSampleBank (const SampleBank* b) { bank.store (b); }
    bool samplesReady() const { const auto* b = bank.load(); return b != nullptr && ! b->empty(); }
    int lastFx() const { return lastFxIdx.load(); }
    float getTempo() const { return tempo; }
    bool leslieFast() const { return lesFast; }

    // ---- performance input
    void pitchBend (float normalised) { bend = clampf (normalised, -1.0f, 1.0f); }   // -1..1
    void modWheel (float normalised)  { mod = clampf (normalised, 0.0f, 1.0f); }     // 0..1

    void noteOn (int note, float vel)
    {
        if (keysDown == 0) { riserSamples = 0; percEnv = 1.0f; }   // a new phrase: riser restarts, organ percussion fires
        ++keysDown;
        if (values[pArp] > 0.5f && std::lround (values[pEngine]) != 3) { arpAdd (note, vel); return; }
        playOn (note, vel);
    }

    void noteOff (int note)
    {
        keysDown = std::max (0, keysDown - 1);
        if (values[pArp] > 0.5f && std::lround (values[pEngine]) != 3) { arpRemove (note); return; }
        playOff (note);
    }

    void playOn (int note, float vel)
    {
        updateEnvelopes();
        if (std::lround (values[pEngine]) == 3)
        {
            const int fx = note - kFxFirstNote;
            if (fx < 0 || fx >= 20) return;
            lastFxIdx.store (fx);
            polyNoteOn (note, vel);
            return;
        }
        if (values[pPoly] < 0.5f) monoNoteOn (note, vel);
        else polyNoteOn (note, vel);
    }

    void playOff (int note)
    {
        if (values[pPoly] < 0.5f) { monoNoteOff (note); return; }

        const bool keysMode = std::lround (values[pEngine]) == 1;
        for (auto& v : voices)
        {
            if (! v.active || v.note != note || ! v.keyDown || v.mask != noteMask) continue;
            if (pedal)
            {
                // pedal holds the sound; in Keys mode the pad still stops when you lift your fingers
                v.keyDown = false; v.sustained = true;
                if (keysMode) { v.amp.gate (false); v.filt.gate (false); }
            }
            else
                v.release();
        }
    }

    void sustainPedal (bool down)
    {
        pedal = down;
        if (! down)
            for (auto& v : voices)
                if (v.active && v.sustained) v.release();
    }

    void allNotesOff()
    {
        held.clear();
        arpKeys.clear(); arpRunning = false; arpSounding.clear(); keysDown = 0;
        pedal = false;
        for (auto& v : voices) if (v.active) v.release();
    }

    // ---- audio
    void process (float* outL, float* outR, int numSamples)
    {
        const auto& p = values;
        updateEnvelopes();

        const int waveA = (int) std::lround (p[pOscAWave]);
        const int waveB = (int) std::lround (p[pOscBWave]);
        const int nUni = std::clamp ((int) std::lround (p[pUnison]), 1, kMaxUnison);
        const float uniGain = 1.0f / std::sqrt ((float) nUni);
        const float bLevel = p[pOscBLevel], subLevel = p[pSubLevel], noiseLevel = p[pNoise];
        const float subSemis = p[pSubOct] < 0.5f ? -12.0f : -24.0f;
        const float glideMs = p[pGlide];
        const double glideCoef = glideMs < 0.5f ? 0.0 : std::exp (-1.0 / (glideMs * 0.001 * sr * 0.33));
        const double glideCoef2 = glideMs < 0.5f ? 0.0 : std::exp (-1.0 / (glideMs * 0.001 * sr * 0.2));
        const bool sCurve = p[pSmpSet] > 0.5f && std::lround (p[pSmpSet]) == 1;   // sampled strings slide like a real section
        const float k = 2.0f - 1.96f * p[pReso];  // SVF damping
        const float pbSemis = bend * p[pPbRange];

        // unison detune / pan tables
        float detune[kMaxUnison], panL[kMaxUnison], panR[kMaxUnison];
        for (int u = 0; u < nUni; ++u)
        {
            const float pos = nUni == 1 ? 0.0f : (2.0f * u / (nUni - 1) - 1.0f);
            detune[u] = pos * p[pDetune] / 100.0f;
            const float pan = pos * p[pSpread];
            const float ang = (pan + 1.0f) * 0.25f * (float) kPi;
            panL[u] = std::cos (ang) * 1.4142f;
            panR[u] = std::sin (ang) * 1.4142f;
        }

        const float buildTarget = clampf (p[pBuild] + mod * p[pMwBuild], 0.0f, 1.0f);
        const float smC = OnePole::coefFor (8.0f, sr);
        const float vibInc = p[pVibRate] / sr;

        const float dlyHPc = OnePole::coefFor (p[pDlyHP], sr);
        const float dlyLPc = OnePole::coefFor (p[pDlyLP], sr);
        const float revHPc = OnePole::coefFor (220.0f, sr);
        const int divIdx = std::clamp ((int) std::lround (p[pDlyTime]), 0, 6);
        const float dlyTarget = std::min ((float) dlyL.size() - 4.0f, kDelayBeats[divIdx] * 60.0f / tempo * sr);
        if (dlySamplesSm < 0.0f) dlySamplesSm = dlyTarget;
        const bool pingPong = p[pDlyPingPong] > 0.5f;

        const float compAtk = OnePole::coefFor (1.0f / 0.004f / (2.0f * (float) kPi), sr);
        const float compRel = OnePole::coefFor (1.0f / 0.150f / (2.0f * (float) kPi), sr);
        const float duckAtk = OnePole::coefFor (1.0f / 0.003f / (2.0f * (float) kPi), sr);
        const float duckRel = OnePole::coefFor (1.0f / 0.300f / (2.0f * (float) kPi), sr);
        const float compThreshDb = -4.0f - p[pComp] * 18.0f;
        const float compRatio = 1.0f + p[pComp] * 5.0f;
        const float compMakeup = dbToGain (p[pComp] * 7.0f);
        const float outGain = dbToGain (p[pOutput]);
        const float dt = 1.0f / sr;

        const int engineMode = (int) std::lround (p[pEngine]);
        const bool keysMode = engineMode == 1;
        const bool bassMode = engineMode == 2;
        const bool cleanPath = p[pClean] > 0.5f;
        const bool epModelEP = p[pEpModel] > 0.5f;
        const float shimmer = p[pShimmer];
        const float rhodesMix = p[pRhodesMix];
        const float epLevel = p[pEpLevel], epBark = p[pEpBark], padLevel = p[pPadLevel];
        const float barkCoef = std::exp (-dt / 0.16f);
        const float tineCoef = std::exp (-dt / 0.045f);
        const float tremInc = 4.6f / sr;
        const float tremDepth = clampf (p[pEpTrem] + mod * p[pMwTrem], 0.0f, 1.0f);

        // ---- v2 layers and modules
        const bool fxMode = engineMode == 3;
        const float smpLevel = (p[pSmpSet] > 0.5f && samplesReady()) ? p[pSmpLevel] : 0.0f;
        const bool organSmp = smpLevel > 0.0f && std::lround (p[pSmpSet]) == 4;
        const float orgLevel = p[pOrgLevel];
        const float padGain = clampf (padLevel + mod * p[pMwPad], 0.0f, 1.5f);
        const bool riserOn = p[pRiser] > 0.5f;
        static constexpr float kRiserBars[] = { 1.0f, 2.0f, 4.0f, 8.0f };
        const float riserLen = std::max (1.0f, kRiserBars[std::clamp ((int) std::lround (p[pRiserBars]), 0, 3)] * 4.0f * 60.0f / tempo * sr);
        const float riserPitch = p[pRiserPitch];
        const float lfoInc = p[pLfoRate] / sr, lfoCut = p[pLfoCut];
        const float pitchEnv = p[pPitchEnv], pitchEnvTau = std::max (0.002f, p[pPitchEnvTime] * 0.001f);
        const float bloom = p[pDlyBloom], mwDelay = p[pMwDelay], chorusAmt = p[pChorus];
        static constexpr int kRegs[5][9] = { { 8,8,8,0,0,0,0,0,0 }, { 8,8,8,8,0,0,0,0,0 }, { 8,8,6,0,0,0,0,0,0 },
                                             { 8,0,0,0,0,0,0,0,8 }, { 8,8,8,8,8,8,8,8,8 } };
        float orgBar[9]; float orgNorm = 0.0f;
        {
            const int reg = std::clamp ((int) std::lround (p[pOrgReg]), 0, 4);
            float sumG = 0.0f;
            for (int d = 0; d < 9; ++d)
            {
                const int lv = kRegs[reg][d];
                orgBar[d] = lv > 0 ? std::pow (10.0f, -(float) (8 - lv) * 3.0f / 20.0f) : 0.0f;
                sumG += orgBar[d];
            }
            orgNorm = 0.5f / std::pow (std::max (1.0f, sumG), 0.6f);
        }
        const int orgVib = std::clamp ((int) std::lround (p[pOrgVib]), 0, 3);
        const float percLevel = p[pOrgPerc], clickLevel = p[pOrgClick] * 0.3f;
        const float percCoef = std::exp (-dt / 0.2f), clickCoef = std::exp (-dt / 0.0012f);
        const float clickC = OnePole::coefFor (5000.0f, sr);
        const int arpRateIdx = std::clamp ((int) std::lround (p[pArpRate]), 0, 4);
        const double arpStepBeats = kArpRateBeats[arpRateIdx];
        const double arpStep = arpStepBeats * 60.0 / tempo * sr;
        const double arpGate = arpStep * clampf (p[pArpGate], 0.05f, 1.0f);
        if (transportValid && hostPlaying && arpRunning) arpSync (arpStep, arpStepBeats);
        transportValid = false;

        for (int i = 0; i < numSamples; ++i)
        {
            buildSm += smC * (buildTarget - buildSm);
            const float b = buildSm;

            vibPhase += vibInc; if (vibPhase >= 1.0f) vibPhase -= 1.0f;
            const float vibSin = std::sin (2.0f * (float) kPi * vibPhase);
            tremPhase += tremInc; if (tremPhase >= 1.0f) tremPhase -= 1.0f;
            const float tremSin = std::sin (2.0f * (float) kPi * tremPhase);
            ++sampleClock;

            if (arpRunning) arpTick (arpStep, arpGate);
            float riserR = 0.0f;
            if (riserOn && keysDown > 0) { riserSamples += 1.0f; }
            if (riserOn) riserR = std::min (1.0f, riserSamples / riserLen);
            const float riserGain = riserOn ? 0.06f + 0.94f * std::pow (riserR, 1.4f) : 1.0f;
            lfoPhase += lfoInc; if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float lfoSin = lfoCut > 0.0f ? sinTab (lfoPhase) : 0.0f;
            percEnv *= percCoef;
            float orgBus = 0.0f;

            // Build macro: more drive, brighter, wider, more space
            const float drive = clampf (p[pDrive] + b * 0.45f + riserR * 0.3f, 0.0f, 1.0f);
            const float cutoffBuildOct = b * 2.0f + (smpLevel > 0.0f ? 0.0f : mod * p[pMwFilter] * 3.0f)
                                       + riserR * 4.5f + lfoSin * lfoCut;
            const float noiseNow = noiseLevel + (riserOn ? riserR * 0.4f : 0.0f);

            float sumL = 0.0f, sumR = 0.0f;
            float padBus = 0.0f;

            for (auto& v : voices)
            {
                if (! v.active) continue;

                // glide
                if (glideCoef > 0.0)
                {
                    if (sCurve) { v.glideMid = v.target + (v.glideMid - v.target) * glideCoef2; v.pitch = v.glideMid + (v.pitch - v.glideMid) * glideCoef2; }
                    else v.pitch = v.target + (v.pitch - v.target) * glideCoef;
                }
                else { v.pitch = v.target; v.glideMid = v.target; }

                v.timeSinceOn += dt;
                const float vibRamp = clampf ((v.timeSinceOn * 1000.0f - p[pVibDelay]) / 400.0f, 0.0f, 1.0f);
                const float vibDepth = p[pVibDepth] * vibRamp + mod * p[pMwVibrato] * 0.5f;
                double pitch = v.pitch + pbSemis + vibSin * vibDepth;
                if (riserOn) pitch += riserPitch * std::pow (riserR, 1.6f);
                if (pitchEnv > 0.0f) pitch += pitchEnv * std::exp (-v.timeSinceOn / pitchEnvTau);

                const float aenv = v.amp.tick();
                const float fenv = v.filt.tick();
                const bool padIdle = v.amp.stage == Env::Idle;
                if (fxMode)
                {
                    float fl = 0.0f, fr = 0.0f;
                    fxTick (v, fl, fr);
                    sumL += fl; sumR += fr;
                    continue;
                }
                if (padIdle && v.mask != 1 && ! bassMode && (! keysMode || v.epEnv < 1.0e-4f)) { v.active = false; continue; }

                const double baseHz = 440.0 * std::pow (2.0, (pitch - 69.0) / 12.0);
                const float velGain = 1.0f - p[pVelSens] + p[pVelSens] * v.velocity;

                // ---- finger bass: a plucked string read through a pickup
                if (bassMode)
                {
                    // the string model handles its own dynamics (velocity in the pluck, damping on release),
                    // so no envelope or gain jumps are applied on top: re-plucks and releases stay click-free
                    const float out = bassTick (v, baseHz);
                    sumL += out; sumR += out;
                    if (v.released && v.ksAmp < 1.0e-5f) v.active = false;
                    continue;
                }

                // ---- electric piano layer: rings on its own, damped when the key (or pedal) is released
                if (keysMode)
                {
                    v.epEnv *= v.released ? v.epCoefRel : v.epCoefHeld;
                    v.epBarkEnv *= barkCoef;
                    v.epTineEnv *= tineCoef;
                    v.epAtk = std::min (1.0f, v.epAtk + dt / 0.0015f);

                    const double inc = baseHz / sr;
                    v.epCar += inc;       if (v.epCar >= 1.0) v.epCar -= 1.0;
                    v.epMod += inc;       if (v.epMod >= 1.0) v.epMod -= 1.0;
                    v.epTine += inc * (epModelEP ? 14.0 : 7.0); if (v.epTine >= 1.0) v.epTine -= std::floor (v.epTine);
                    v.epLow += inc * 0.5; if (v.epLow >= 1.0) v.epLow -= 1.0;

                    const float vel = v.velocity;
                    float epL, epR;
                    if (epModelEP)
                    {
                        // DX-style EP: warm FM body, glassy bell on the attack, two detuned carriers = stereo chorus
                        v.epCar2 += inc * 1.0021; if (v.epCar2 >= 1.0) v.epCar2 -= 1.0;
                        v.epMod2 += inc * 0.9985; if (v.epMod2 >= 1.0) v.epMod2 -= 1.0;
                        // warm body (low FM index), only a touch of crystal on the attack
                        const float bodyIdx = 0.28f + epBark * (0.2f + 0.8f * vel * vel) * v.epBarkEnv;
                        const float bellIdx = (0.12f + 0.45f * vel) * v.epTineEnv * (0.3f + 0.5f * epBark);
                        const float tw = 2.0f * (float) kPi;
                        const float bell = std::sin (tw * (float) v.epTine);                     // ratio 7 modulator
                        const float b1 = std::sin (tw * (float) v.epCar + bodyIdx * std::sin (tw * (float) v.epMod) + bellIdx * bell);
                        const float b2 = std::sin (tw * (float) v.epCar2 + bodyIdx * std::sin (tw * (float) v.epMod2) + bellIdx * bell * 0.8f);
                        const float low = std::sin (tw * (float) v.epLow) * 0.42f                 // heavy: strong octave below
                                        + std::sin (tw * (float) v.epCar) * 0.25f;                // and pure fundamental
                        epL = (b1 * 0.8f + b2 * 0.35f + low) * v.epEnv * v.epAtk;
                        epR = (b2 * 0.8f + b1 * 0.35f + low) * v.epEnv * v.epAtk;
                        epL = std::tanh (epL * 1.9f) * 0.62f;
                        epR = std::tanh (epR * 1.9f) * 0.62f;
                        // soft tone control: rounds off the top so it never gets harsh
                        epL = v.epToneL.lp (epL, v.epToneCoef);
                        epR = v.epToneR.lp (epR, v.epToneCoef);

                        // optional Rhodes layer underneath: warm tine with a little bark, kept low
                        if (rhodesMix > 0.001f)
                        {
                            const float rIdx = 0.45f + (0.3f + 1.4f * vel * vel) * v.epBarkEnv;
                            float rh = std::sin (tw * (float) v.epCar + rIdx * std::sin (tw * (float) v.epMod))
                                     + std::sin (tw * (float) v.epLow) * 0.3f;
                            rh = std::tanh (rh * v.epEnv * v.epAtk * 1.4f) * 0.6f;
                            rh = v.rhTone.lp (rh, v.epToneCoef * 0.7f);
                            epL += rh * rhodesMix;
                            epR += rh * rhodesMix;
                        }
                    }
                    else
                    {
                        const float idx = 0.35f + epBark * (0.4f + 2.6f * vel * vel) * v.epBarkEnv;   // harder = more bark
                        const float modS = std::sin (2.0f * (float) kPi * (float) v.epMod);
                        const float car = std::sin (2.0f * (float) kPi * (float) v.epCar + idx * modS);
                        const float tine = std::sin (2.0f * (float) kPi * (float) v.epTine) * v.epTineEnv * 0.10f * (0.3f + vel) * epBark;
                        const float low = std::sin (2.0f * (float) kPi * (float) v.epLow) * 0.22f;
                        float ep = (car + tine + low) * v.epEnv * v.epAtk;
                        ep = std::tanh (ep * 1.6f) * 0.75f;
                        epL = epR = ep;
                    }

                    // low notes a bit left, high notes a bit right, plus tremolo (suitcase style)
                    const float pan = clampf ((float) (v.note - 60) / 48.0f, -0.35f, 0.35f) + tremSin * tremDepth * 0.5f;
                    const float ang = (clampf (pan, -1.0f, 1.0f) + 1.0f) * 0.25f * (float) kPi;
                    sumL += epL * std::cos (ang) * 1.4142f * epLevel * velGain;
                    sumR += epR * std::sin (ang) * 1.4142f * epLevel * velGain;
                }

                if (padIdle) continue;

                if (v.mask == 1)
                {
                    // the pad behind the arpeggio: three detuned saws + a soft sub, dark and wide, fades in and out
                    if (v.keyDown || v.sustained) v.padFade = std::min (1.0f, v.padFade + dt / 0.7f);
                    else { v.padFade -= dt / 0.45f; if (v.padFade <= 0.0f) { v.padFade = 0.0f; v.active = false; continue; } }
                    const double hz = 440.0 * std::pow (2.0, (v.pitch + v.smpTrans + pbSemis - 69.0) / 12.0);
                    const float a = v.uni[0].tick (0, std::min (0.49, hz * 1.0046 / sr));
                    const float c = v.uni[1].tick (0, std::min (0.49, hz * 0.9954 / sr));
                    const float m = v.uni[2].tick (0, std::min (0.49, hz / sr));
                    const float sub = 0.0f;
                    const float fc = clampf (650.0f * std::pow (2.0f, (float) (v.note + v.smpTrans - 60) / 12.0f * 0.5f), 200.0f, 2200.0f);
                    const float g = std::tan ((float) kPi * fc / sr);
                    const float a1 = 1.0f / (1.0f + g * (g + 1.4f)), a2 = g * a1, a3 = g * a2;
                    const float pl = v.fL2.process (v.fL1.process (a * 0.7f + m * 0.5f + sub * 0.5f, a1, a2, a3), a1, a2, a3);
                    const float pr = v.fR2.process (v.fR1.process (c * 0.7f + m * 0.5f + sub * 0.5f, a1, a2, a3), a1, a2, a3);
                    const float lvl = p[pArpPad] * v.padFade * v.padFade * 0.8f;
                    sumL += pl * lvl; sumR += pr * lvl;
                    continue;
                }
                const float voicePad = padGain;
                const float balL = std::min (1.0f, 1.0f - v.arpPan), balR = std::min (1.0f, 1.0f + v.arpPan);
                if (voicePad > 0.0001f)
                {
                    // oscillators
                    float vl = 0.0f, vr = 0.0f;
                    for (int u = 0; u < nUni; ++u)
                    {
                        const double hz = baseHz * std::pow (2.0, detune[u] / 12.0);
                        const float s = v.uni[(size_t) u].tick (waveA, std::min (0.49, hz / sr));
                        vl += s * panL[u]; vr += s * panR[u];
                    }
                    vl *= uniGain; vr *= uniGain;

                    float mono = 0.0f;
                    if (bLevel > 0.0001f)
                    {
                        const double hzB = baseHz * std::pow (2.0, (p[pOscBSemi] + p[pOscBDetune] / 100.0) / 12.0);
                        mono += bLevel * v.oscB.tick (waveB, std::min (0.49, hzB / sr));
                    }
                    if (subLevel > 0.0001f)
                    {
                        const double hzS = baseHz * std::pow (2.0, subSemis / 12.0);
                        mono += subLevel * v.sub.tick (2, hzS / sr);
                    }
                    if (noiseNow > 0.0001f) mono += noiseNow * 0.5f * rng.next();
                    vl += mono; vr += mono;

                    // drive before the filter: heavier with velocity
                    const float drv = clampf (drive + v.velocity * p[pVelDrive] * 0.5f, 0.0f, 1.0f);
                    const float pre = 1.0f + drv * 5.0f;
                    const float norm = 1.0f / std::tanh (pre * 0.7f);
                    if (cleanPath) { vl *= 0.7f; vr *= 0.7f; }          // pure tone: no saturation at all
                    else
                    {
                        vl = std::tanh (vl * pre * 0.7f) * norm * 0.7f;
                        vr = std::tanh (vr * pre * 0.7f) * norm * 0.7f;
                    }

                    // filter: opens with key (higher notes brighter), velocity, envelope, build and mod wheel
                    const float octaves = (float) ((v.note - 60) / 12.0) * p[pKeyTrack]
                                        + v.velocity * p[pVelCutoff]
                                        + fenv * p[pFiltEnv]
                                        + cutoffBuildOct;
                    const float fc = clampf (p[pCutoff] * std::pow (2.0f, octaves), 30.0f, sr * 0.45f);
                    const float g = std::tan ((float) kPi * fc / sr);
                    const float a1 = 1.0f / (1.0f + g * (g + k));
                    const float a2 = g * a1, a3 = g * a2;
                    const float k2 = 1.9f;                                 // second stage: no resonance
                    const float b1 = 1.0f / (1.0f + g * (g + k2));
                    const float b2 = g * b1, b3 = g * b2;
                    vl = v.fL2.process (v.fL1.process (vl, a1, a2, a3), b1, b2, b3);
                    vr = v.fR2.process (v.fR1.process (vr, a1, a2, a3), b1, b2, b3);

                    // post-filter grain
                    const float grain = 1.0f + drv * 1.5f;
                    if (! cleanPath)
                    {
                        vl = std::tanh (vl * grain) / grain * (1.0f + drv * 0.6f);
                        vr = std::tanh (vr * grain) / grain * (1.0f + drv * 0.6f);
                    }

                    // shimmer: soft high octaves that sparkle slowly, only while the pad sounds
                    if (shimmer > 0.001f)
                    {
                        v.shimLfo += v.shimRate * dt; if (v.shimLfo >= 1.0f) v.shimLfo -= 1.0f;
                        const float mv = 0.55f + 0.45f * std::sin (2.0f * (float) kPi * v.shimLfo);
                        const float s1 = v.shim1.tick (3, std::min (0.49, baseHz * 2.0 * 1.0035 / sr));
                        const float s2 = v.shim2.tick (2, std::min (0.49, baseHz * 4.0 * 0.9968 / sr));
                        const float sh = (s1 * 0.6f + s2 * 0.4f) * shimmer * 0.35f;
                        vl += sh * mv; vr += sh * (1.55f - mv);
                    }

                    const float gl = balL * vl * aenv * velGain * voicePad * riserGain;
                    const float gr = balR * vr * aenv * velGain * voicePad * riserGain;
                    sumL += gl; sumR += gr;
                    padBus += 0.5f * (gl + gr);
                }

                // ---- sampled layer (strings / brass / piano), follows the gliding pitch
                if (smpLevel > 0.0001f && v.nSmp > 0 && (v.mask & 2))
                {
                    float sl = 0.0f, sr2 = 0.0f;
                    const double base = std::pow (2.0, (pitch + v.smpTrans - 60.0) / 12.0) / sr;
                    for (int s = 0; s < v.nSmp; ++s)
                    {
                        auto& pl = v.smp[(size_t) s];
                        if (pl.z == nullptr) continue;
                        const auto& z = *pl.z;
                        float a, c; z.read (pl.pos, a, c);
                        sl += a * pl.gain * (1.0f - pl.pan); sr2 += c * pl.gain * (1.0f + pl.pan);
                        pl.pos += base * pl.rootMul;
                        if (z.looped()) { while (pl.pos >= (double) z.loopEnd) pl.pos -= (double) (z.loopEnd - z.loopStart); }
                        else if (pl.pos >= (double) (z.frames - 2)) pl.z = nullptr;
                    }
                    // mod wheel = crescendo: louder and brighter
                    const float mw = p[pMwFilter];
                    if (mw > 0.001f)
                    {
                        const float tc = OnePole::coefFor (1100.0f, sr);
                        const float lowL = v.smpTiltL.lp (sl, tc), lowR = v.smpTiltR.lp (sr2, tc);
                        const float hi = 1.0f - mw + mw * (0.3f + 1.0f * mod);
                        const float lvl = 1.0f - mw * 0.45f * (1.0f - mod);
                        sl = (lowL + (sl - lowL) * hi) * lvl;
                        sr2 = (lowR + (sr2 - lowR) * hi) * lvl;
                    }
                    if (organSmp)
                    {
                        // organ recordings go through the organ chain (scanner, drive, rotary); organs ignore velocity
                        float o = 0.5f * (sl + sr2);
                        v.orgPh[4] += baseHz * 3.0 / sr; if (v.orgPh[4] >= 1.0) v.orgPh[4] -= 1.0;
                        if (percLevel > 0.001f) o += sinTab (v.orgPh[4]) * percEnv * percLevel * 0.12f;
                        v.clickEnv *= clickCoef;
                        if (clickLevel > 0.0f && v.clickEnv > 1.0e-4f) o += v.clickLp.lp (rng.next(), clickC) * v.clickEnv * clickLevel * 0.15f;
                        orgBus += o * aenv * smpLevel * 0.9f;
                    }
                    else
                    {
                        const float sg = aenv * velGain * smpLevel * riserGain;
                        sumL += sl * sg * balL; sumR += sr2 * sg * balR;
                    }
                }

                // ---- tonewheel organ layer (drawbars, percussion, key click); rotary is applied on the bus
                if (orgLevel > 0.0001f)
                {
                    float o = 0.0f;
                    for (int d = 0; d < 9; ++d)
                    {
                        double f = baseHz * kOrgRatio[d];
                        while (f > 5900.0) f *= 0.5;          // the top tonewheels fold back an octave
                        v.orgPh[(size_t) d] += f / sr;
                        if (v.orgPh[(size_t) d] >= 1.0) v.orgPh[(size_t) d] -= 1.0;
                        o += (orgBar[d] + 0.004f) * sinTab (v.orgPh[(size_t) d]);   // + faint tonewheel leakage
                    }
                    if (percLevel > 0.001f) o += sinTab (v.orgPh[4]) * percEnv * percLevel * 0.9f;   // 3rd harmonic, fast decay
                    v.clickEnv *= clickCoef;
                    if (clickLevel > 0.0f && v.clickEnv > 1.0e-4f) o += v.clickLp.lp (rng.next(), clickC) * v.clickEnv * clickLevel * 1.2f;
                    orgBus += o * orgNorm * aenv * orgLevel;
                }
            }

            if (p[pArpDust] > 0.0f && arpRunning) dustTick (sumL, sumR, p[pArpDust]);

            if (orgLevel > 0.0001f || organSmp)
            {
                float oL, oR;
                organFx (orgBus, oL, oR, orgVib, p[pOrgDrive], p[pLeslie]);
                sumL += oL; sumR += oR;
            }

            if (p[pPoly] > 0.5f) { sumL *= 0.5f; sumR *= 0.5f; }   // chords stack up: keep headroom

            // width (M/S), widened by Build
            {
                const float w = clampf (p[pWidth] + b * 0.5f, 0.0f, 2.0f);
                const float m = 0.5f * (sumL + sumR), s = 0.5f * (sumL - sumR) * w;
                sumL = m + s; sumR = m - s;
            }
            if (chorusAmt > 0.001f) chorusTick (sumL, sumR, chorusAmt);

            // compressor (stereo linked)
            {
                const float lvl = std::max (std::abs (sumL), std::abs (sumR));
                compEnv += (lvl > compEnv ? compAtk : compRel) * (lvl - compEnv);
                const float envDb = 20.0f * std::log10 (compEnv + 1e-6f);
                const float over = envDb - compThreshDb;
                const float gr = over > 0.0f ? -over * (1.0f - 1.0f / compRatio) : 0.0f;
                const float gain = dbToGain (gr) * compMakeup;
                sumL *= gain; sumR *= gain;
            }

            // delay (ping-pong, tempo synced, ducked while you play)
            float wetL = 0.0f, wetR = 0.0f;
            {
                dlySamplesSm += 0.0005f * (dlyTarget - dlySamplesSm);
                const float dMix = clampf (p[pDlyMix] + b * 0.25f + mod * mwDelay * 0.8f, 0.0f, 1.5f);
                const float fb = clampf (p[pDlyFb] + b * 0.15f + mod * mwDelay * 0.12f, 0.0f, 0.95f);
                // bloom: a slow tape wobble on the repeats
                float dRead = dlySamplesSm;
                if (bloom > 0.001f) { wowPhase += 0.45f * dt; if (wowPhase >= 1.0f) wowPhase -= 1.0f; dRead += bloom * 0.0012f * sr * sinTab (wowPhase); }
                const float yL = readDelay (dlyL, dRead);
                const float yR = readDelay (dlyR, dRead);
                float fL = dlyHPL.hp (dlyLPL.lp (yL, dlyLPc), dlyHPc);
                float fR = dlyHPR.hp (dlyLPR.lp (yR, dlyLPc), dlyHPc);
                fL = std::tanh (fL); fR = std::tanh (fR);
                const float in = 0.5f * (sumL + sumR);
                const float duo = p[pDlyDuo];
                if (pingPong && duo > 0.001f)
                {
                    // "duo": the first answer comes from the right, as loud as you, while you sit a bit left
                    dlyR[(size_t) dlyW] = in + fL * fb;
                    dlyL[(size_t) dlyW] = fR * fb;
                    sumR *= 1.0f - 0.55f * duo;
                    sumL *= 1.0f + 0.15f * duo;
                }
                else if (pingPong)
                {
                    dlyL[(size_t) dlyW] = in + fR * fb;
                    dlyR[(size_t) dlyW] = fL * fb;
                }
                else
                {
                    dlyL[(size_t) dlyW] = sumL + fL * fb;
                    dlyR[(size_t) dlyW] = sumR + fR * fb;
                }
                if (++dlyW >= (int) dlyL.size()) dlyW = 0;

                const float lvl = std::abs (in);
                duckEnv += (lvl > duckEnv ? duckAtk : duckRel) * (lvl - duckEnv);
                const float duck = 1.0f - p[pDlyDuck] * clampf (duckEnv * 3.0f, 0.0f, 0.85f);
                wetL = yL * dMix * duck;
                wetR = yR * dMix * duck;
                if (bloom > 0.001f)
                {
                    // bloom: a second, crossed tap on the quarter note: repeats pile up into a cloud
                    const float q = std::min ((float) dlyL.size() - 4.0f, 60.0f / tempo * sr);
                    wetL += readDelay (dlyR, q) * dMix * bloom * 0.55f;
                    wetR += readDelay (dlyL, q) * dMix * bloom * 0.55f;
                }
            }

            // reverb (fed by dry + a bit of delay)
            float rL = 0.0f, rR = 0.0f;
            {
                const float rMix = clampf (p[pRevMix] + b * 0.2f + riserR * 0.25f, 0.0f, 1.0f);
                float shim = 0.0f;
                if (shimmer > 0.001f)
                    shim = octaveUp.process (padBus * 1.5f + 0.35f * (lastRevL + lastRevR)) * shimmer * 1.6f;
                const float wetSend = 0.5f + bloom * 1.3f;
                const float inL = revHPL.hp (sumL + wetL * wetSend + shim, revHPc);
                const float inR = revHPR.hp (sumR + wetR * wetSend + shim, revHPc);
                if (p[pRevHall] > 0.5f) { hall.process (inL * 0.25f, inR * 0.25f, p[pRevSize], p[pRevDamp], rL, rR); }
                else reverb.process (inL, inR, p[pRevSize], p[pRevDamp], rL, rR);
                lastRevL = rL; lastRevR = rR;
                rL *= rMix * 3.0f; rR *= rMix * 3.0f;
            }

            outL[i] = softLimit ((sumL + wetL + rL) * outGain);
            outR[i] = softLimit ((sumR + wetR + rR) * outGain);
        }
    }

private:
    ParamValues values;
    float sr = 44100.0f, tempo = 90.0f;
    float bend = 0.0f, mod = 0.0f;
    float buildSm = 0.0f, vibPhase = 0.0f;
    std::array<Voice, kMaxVoices> voices;
    std::vector<int> held;
    uint64_t ageCounter = 0;
    Rng rng;

    std::vector<float> dlyL, dlyR;
    int dlyW = 0;
    float dlySamplesSm = -1.0f;
    OnePole dlyHPL, dlyHPR, dlyLPL, dlyLPR, revHPL, revHPR;
    Reverb reverb;
    HallReverb hall;
    float compEnv = 0.0f, duckEnv = 0.0f;
    float tremPhase = 0.0f;
    OctaveUp octaveUp;
    float lastRevL = 0.0f, lastRevR = 0.0f;
    uint64_t sampleClock = 0, lastNoteOnClock = 0;
    std::vector<double> prevPitches;
    std::vector<int> currentChord;
    std::vector<bool> prevUsed;
    std::vector<int> prevVoice;
    bool pedal = false;

    // ---- v2 state
    std::atomic<const SampleBank*> bank { nullptr };
    std::atomic<int> lastFxIdx { -1 };
    double hostPpq = 0.0; bool hostPlaying = false, transportValid = false;
    int keysDown = 0;
    uint8_t noteMask = 3;
    Voice* lastStarted = nullptr;
    std::vector<int> arpKeys, arpSounding;
    float arpVel = 0.8f; bool arpRunning = false;
    double arpToNext = 0.0, arpGateLeft = 0.0; int arpIdx = 0;
    float riserSamples = 0.0f, lfoPhase = 0.0f, percEnv = 0.0f, wowPhase = 0.0f;
    std::vector<float> vibBuf = std::vector<float> (2048, 0.0f);
    std::vector<float> hornBuf = std::vector<float> (1024, 0.0f), drumBuf = std::vector<float> (1024, 0.0f);
    int vibW = 0, lesW = 0;
    float vibPh = 0.0f, hornAng = 0.0f, drumAng = 0.3f, hornSpd = 0.8f, drumSpd = 0.66f;
    bool lesFast = false;
    OnePole xo1, xo2, orgLow, orgSub;
    std::vector<float> chL = std::vector<float> (4096, 0.0f), chR = std::vector<float> (4096, 0.0f);
    int chW = 0; float chPh = 0.0f;

    static float ring (const std::vector<float>& b, int w, float d)
    {
        const int mask = (int) b.size() - 1;
        float pos = (float) w - d;
        while (pos < 0.0f) pos += (float) b.size();
        const int i0 = (int) pos & mask; const float f = pos - std::floor (pos);
        return b[(size_t) i0] + f * (b[(size_t) ((i0 + 1) & mask)] - b[(size_t) i0]);
    }

    // ---- arpeggiator: plays the held chord in time with the song, stops when you let go
    void arpAdd (int note, float vel)
    {
        if (std::find (arpKeys.begin(), arpKeys.end(), note) == arpKeys.end()) arpKeys.push_back (note);
        arpVel = vel;
        if (values[pArpPad] > 0.0f) { noteMask = 1; playOn (note, vel * 0.8f); noteMask = 3; }
        if (! arpRunning) { arpRunning = true; arpIdx = 0; arpToNext = 0.0; }
    }
    void arpRemove (int note)
    {
        arpKeys.erase (std::remove (arpKeys.begin(), arpKeys.end(), note), arpKeys.end());
        if (values[pArpPad] > 0.0f) { noteMask = 1; playOff (note); noteMask = 3; }
        if (arpKeys.empty()) { arpRelease(); arpRunning = false; }
    }
    uint8_t arpMask() const { return values[pArpPad] > 0.0f ? (uint8_t) 2 : (uint8_t) 3; }
    void arpRelease() { noteMask = arpMask(); for (int n : arpSounding) playOff (n); arpSounding.clear(); noteMask = 3; }
    void arpTick (double stepSamples, double gateSamples)
    {
        if (arpGateLeft > 0.0) { arpGateLeft -= 1.0; if (arpGateLeft <= 0.0) arpRelease(); }
        arpToNext -= 1.0;
        if (arpToNext > 0.0) return;
        arpToNext += stepSamples;
        arpRelease();
        if (arpKeys.empty()) return;
        const int mode = std::clamp ((int) std::lround (values[pArpMode]), 0, 5);
        const float accent = (arpIdx % 4 == 0) ? 1.0f : 0.84f;
        const float vel = clampf (arpVel * accent, 0.05f, 1.0f);
        noteMask = arpMask();
        if (mode == 3)
            for (int n : arpKeys) { playOn (n, vel); arpSounding.push_back (n); }
        else
        {
            std::vector<int> k = arpKeys; std::sort (k.begin(), k.end());
            const int oct = std::clamp ((int) std::lround (values[pArpOct]), 1, 3);
            std::vector<int> seq;
            for (int o = 0; o < oct; ++o) for (int n : k) seq.push_back (n + 12 * o);
            if (mode == 1) std::reverse (seq.begin(), seq.end());
            else if (mode == 2 && seq.size() > 2) for (int i = (int) seq.size() - 2; i >= 1; --i) seq.push_back (seq[(size_t) i]);
            else if (mode == 4 || mode == 5)
            {
                // worship patterns built on the chord: root, fifth, octave, tenth...
                const int r = k.front();
                auto nearest = [&] (int target) { int best = k.size() > 1 ? k[1] : r + 7; int d = 99;
                                                  for (int n : k) { int m = r + ((n - r) % 12); if (std::abs (m - target) < d && n != r) { d = std::abs (m - target); best = m; } } return best; };
                const int f = nearest (r + 7), t = nearest (r + 4);
                if (mode == 4) seq = { r, f, r + 12, t + 12, f + 12, t + 12, r + 12, f };          // 1-5-8-10-12-10-8-5
                else           seq = { r, f + 12, t + 12, f + 12, r + 12, f + 12, t + 24, f + 12 }; // broken chord, rolling
            }
            const int n = seq[(size_t) (arpIdx % (int) seq.size())];
            lastStarted = nullptr;
            playOn (n, vel); arpSounding.push_back (n);
            if (lastStarted != nullptr && values[pArpPan] > 0.0f)
            {
                const auto [lo, hi] = std::minmax_element (seq.begin(), seq.end());
                const float pos = *hi > *lo ? (float) (n - *lo) / (float) (*hi - *lo) : 0.5f;
                lastStarted->arpPan = (pos * 2.0f - 1.0f) * values[pArpPan];
            }
        }
        noteMask = 3;
        ++arpIdx;
        arpGateLeft = gateSamples;
    }
    void arpSync (double stepSamples, double stepBeats)
    {
        const double pos = hostPpq / stepBeats;
        const double toNext = (std::ceil (pos - 1.0e-9) - pos) * stepSamples;
        double diff = toNext - arpToNext;
        while (diff > 0.5 * stepSamples) diff -= stepSamples;
        while (diff <= -0.5 * stepSamples) diff += stepSamples;
        arpToNext += diff;
    }

    // ---- sampled layer: picks the instruments for this note (section layering by register)
    void smpSetup (Voice& v)
    {
        v.nSmp = 0;
        const SampleBank* b = bank.load();
        const int set = std::clamp ((int) std::lround (values[pSmpSet]), 0, 5);
        v.smpTrans = set == 5 ? 24.0f : 0.0f;
        if (b == nullptr || b->empty() || set == 0 || values[pSmpLevel] <= 0.0f) return;
        const int n = v.note;
        const float velScale = set == 1 ? 0.72f : 1.0f;   // strings: the soft (warm) layer unless you really dig in
        const int vel = std::clamp ((int) std::lround (v.velocity * 127.0f * velScale), 1, 127);
        const float x = (float) n;
        auto ramp = [] (float t, float a, float c) { return clampf ((t - a) / (c - a), 0.0f, 1.0f); };
        struct L { int inst; float w; float pan; };
        std::vector<L> ls;
        if (set == 1)
        {
            ls = { { kBasses, 0.9f * (1.0f - ramp (x, 43, 50)), 0.25f },
                   { kCellos, (x < 36 ? 0.7f : 1.0f) * (1.0f - ramp (x, 62, 69)), 0.15f },
                   { kViolas, ramp (x, 47, 53) * (1.0f - ramp (x, 72, 79)), 0.05f },
                   { kViolins, ramp (x, 54, 60), -0.2f } };
        }
        else if (set == 2)
        {
            const float st = values[pSmpStac] * std::pow (v.velocity, 1.5f) * 1.3f;
            const float wT = ramp (x, 57, 63), wH = ramp (x, 40, 45) * (1.0f - ramp (x, 70, 76));
            const float wB = (x >= 36 ? 1.0f : 0.7f) * (1.0f - ramp (x, 63, 69)), wU = 1.0f - ramp (x, 45, 52);
            ls = { { kTrumpetSus, wT, -0.25f }, { kHornSus, wH * 0.9f, -0.05f }, { kTromboneSus, wB, 0.2f }, { kTubaSus, wU * 0.9f, 0.3f },
                   { kTrumpetStac, wT * st, -0.25f }, { kHornStac, wH * st * 0.9f, -0.05f }, { kTromboneStac, wB * st, 0.2f }, { kTubaStac, wU * st * 0.9f, 0.3f } };
        }
        else if (set == 3) ls = { { kPiano, 1.0f, 0.0f } };
        else if (set == 4) ls = { { kOrgGospel + std::clamp ((int) std::lround (values[pOrgReg]), 0, 3), 1.0f, 0.0f } };
        else ls = { { kGlock, 1.0f, 0.0f } };
        float pw = 0.0f;
        for (size_t i = 0; i < ls.size() && i < 4; ++i) pw += ls[i].w * ls[i].w;
        const float norm = 1.0f / std::sqrt (std::max (0.6f, pw));
        for (auto& l : ls)
        {
            if (l.w <= 0.002f || v.nSmp >= 8) continue;
            const SampleZone* z = b->find (l.inst, n + (int) v.smpTrans, set == 4 ? 100 : vel);
            if (z == nullptr) continue;
            auto& pl = v.smp[(size_t) v.nSmp++];
            pl.z = z; pl.gain = l.w * norm; pl.pan = l.pan;
            pl.pos = (set == 1 && z->looped()) ? std::min (0.12 * z->rate, (double) z->loopStart * 0.5) : 0.0;
            pl.rootMul = std::pow (2.0, (60.0 - z->root) / 12.0) * z->rate;
        }
    }

    // ---- organ bus: vibrato/chorus scanner, tube drive, rotary speaker (horn + drum, own speeds)
    void organFx (float x, float& outL, float& outR, int vibMode, float drv, float les)
    {
        if (vibMode > 0)
        {
            vibBuf[(size_t) vibW] = x;
            vibPh += 6.8f / sr; if (vibPh >= 1.0f) vibPh -= 1.0f;
            const float depthMs = vibMode == 1 ? 0.25f : (vibMode == 2 ? 0.45f : 0.7f);
            const float d = (1.1f + depthMs * sinTab (vibPh)) * 0.001f * sr;
            const float wet = ring (vibBuf, vibW, d);
            vibW = (vibW + 1) & 2047;
            x = 0.62f * (x + wet);
        }
        const float pre = 1.0f + drv * 3.0f;
        x = std::tanh (x * pre * 0.5f) * 2.0f / pre;
        // the cabinet: low end tightened (below ~180 Hz pulled down, rumble under 50 Hz removed)
        {
            const float lowC = OnePole::coefFor (180.0f, sr), subC = OnePole::coefFor (50.0f, sr);
            const float low = orgLow.lp (x, lowC), sub = orgSub.lp (x, subC);
            x = x - 0.45f * low - 0.5f * sub;
        }
        if (les < 0.001f) { outL = outR = x; return; }
        if (mod > 0.55f) lesFast = true; else if (mod < 0.45f) lesFast = false;
        const float hT = lesFast ? 6.8f : 0.8f, dT = lesFast ? 5.9f : 0.66f;
        hornSpd += (hT - hornSpd) * (1.0f / ((hT > hornSpd ? 0.55f : 1.2f) * sr));
        drumSpd += (dT - drumSpd) * (1.0f / ((dT > drumSpd ? 3.2f : 4.5f) * sr));
        hornAng += hornSpd / sr; if (hornAng >= 1.0f) hornAng -= 1.0f;
        drumAng += drumSpd / sr; if (drumAng >= 1.0f) drumAng -= 1.0f;
        const float xoC = OnePole::coefFor (800.0f, sr);
        const float low = xo2.lp (xo1.lp (x, xoC), xoC), high = x - low;
        hornBuf[(size_t) lesW] = high; drumBuf[(size_t) lesW] = low;
        const float k = sr / 44100.0f;
        const float hs = sinTab (hornAng), hc = sinTab (hornAng + 0.25f);
        const float ds = sinTab (drumAng), dc = sinTab (drumAng + 0.25f);
        const float hL = ring (hornBuf, lesW, (26.0f + 18.0f * hs) * k) * (0.6f + 0.4f * hc);
        const float hR = ring (hornBuf, lesW, (26.0f - 18.0f * hs) * k) * (0.6f - 0.4f * hc);
        const float dL = ring (drumBuf, lesW, (10.0f + 6.0f * ds) * k) * (0.8f + 0.2f * dc);
        const float dR = ring (drumBuf, lesW, (10.0f - 6.0f * ds) * k) * (0.8f - 0.2f * dc);
        lesW = (lesW + 1) & 1023;
        outL = x * (1.0f - les) + (hL + dL) * les * 1.15f;
        outR = x * (1.0f - les) + (hR + dR) * les * 1.15f;
    }

    // ---- FX kit: one effect per key (C3..G4), synthesised, tempo-synced where it matters
    void fxSetup (Voice& v)
    {
        v.fxType = std::clamp (v.note - kFxFirstNote, 0, 19);
        v.fxT = 0.0f; v.fxNext = 0.0f; v.fxHitT = 10.0f; v.fxRel = 1.0f; v.fxHits = -1; v.fxHitF = 0.0f;
        for (auto& ph : v.fxPh) ph = 0.0;
        v.fxA = SVF2(); v.fxB = SVF2();
    }

    void fxTick (Voice& v, float& outL, float& outR)
    {
        const float dt = 1.0f / sr, beat = 60.0f / tempo, t = v.fxT;
        v.fxT += dt;
        auto bp = [this] (SVF2& f, float x, float fc, float q) { float lp, b; f.tick (x, std::tan ((float) kPi * clampf (fc, 20.0f, sr * 0.45f) / sr), 1.0f / q, lp, b); return b; };
        auto lp = [this] (SVF2& f, float x, float fc, float q) { float l, b; f.tick (x, std::tan ((float) kPi * clampf (fc, 20.0f, sr * 0.45f) / sr), 1.0f / q, l, b); return l; };
        auto adv = [this] (double& ph, double hz) { ph += hz / sr; if (ph >= 1.0) ph -= std::floor (ph); return ph; };
        auto saw = [] (double ph) { return (float) (2.0 * ph - 1.0); };
        const float nz = rng.next();
        float s = 0.0f, pan = 0.0f, sR = 0.0f; bool stereo = false, done = false;
        switch (v.fxType)
        {
            case 0: case 1: // uplifter, 4 or 2 bars
            {
                const float D = (v.fxType == 0 ? 16.0f : 8.0f) * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float n = bp (v.fxA, nz, 250.0f * std::pow (2.0f, r * 5.5f), 2.5f) * 1.4f;
                const double f = 110.0 * std::pow (2.0, 2.0 * r);
                const float sw = 0.5f * (saw (adv (v.fxPh[0], f)) + saw (adv (v.fxPh[1], f * 1.007)));
                s = (n * 0.75f + lp (v.fxB, sw, 300.0f * std::pow (2.0f, r * 5.0f), 0.9f) * 0.35f) * r * r * 1.2f;
                pan = 0.3f * r * sinTab (t * 0.25);
                break;
            }
            case 2: // downlifter, 2 bars
            {
                const float D = 8.0f * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float n = bp (v.fxA, nz, 9000.0f * std::pow (2.0f, -6.0f * r), 2.0f);
                const float sw = saw (adv (v.fxPh[0], 880.0 * std::pow (2.0, -3.0 * r)));
                s = (n * 0.8f + lp (v.fxB, sw, 4000.0f * std::pow (2.0f, -4.0f * r), 0.8f) * 0.3f) * std::pow (1.0f - r, 1.5f);
                break;
            }
            case 3: case 15: // impact, long boom
            {
                const bool lng = v.fxType == 15;
                if (t > (lng ? 5.0f : 2.5f)) { done = true; break; }
                const float f = 42.0f + 150.0f * std::exp (-t / 0.035f);
                const float kick = sinTab (adv (v.fxPh[0], f)) * std::exp (-t / (lng ? 1.6f : 0.7f)) * std::min (1.0f, t / 0.0008f);
                const float nb = lp (v.fxA, nz, 2500.0f + 6000.0f * std::exp (-t / 0.05f), 0.7f) * std::exp (-t / 0.15f);
                const float rum = lng ? lp (v.fxB, nz, 140.0f, 0.7f) * std::exp (-t / 2.2f) * 2.2f : 0.0f;
                s = std::tanh ((kick * 1.1f + nb * 0.6f + rum) * 1.5f) * 0.8f;
                break;
            }
            case 4: // sub drop
            {
                if (t > 3.0f) { done = true; break; }
                const float f = 28.0f + 44.0f * std::exp (-t / 0.6f);
                s = std::tanh (sinTab (adv (v.fxPh[0], f)) * std::exp (-t / 1.6f) * std::min (1.0f, t / 0.005f) * 1.6f) * 0.85f;
                break;
            }
            case 5: case 6: // whoosh short / long
            {
                const float D = v.fxType == 5 ? 0.7f : 1.8f, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float e = std::sin ((float) kPi * r);
                s = bp (v.fxA, nz, 350.0f * std::pow (2.0f, 4.3f * e), 1.6f) * e * e * 1.7f;
                pan = v.fxType == 5 ? -0.8f + 1.6f * r : 0.8f - 1.6f * r;
                break;
            }
            case 7: // reverse cymbal, 1 bar
            {
                const float D = 4.0f * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float hi = nz - lp (v.fxA, nz, 5000.0f, 0.7f);
                s = (hi * 0.7f + bp (v.fxB, nz, 9000.0f, 4.0f) * 0.5f) * std::exp (-(1.0f - r) / 0.22f) * 0.9f;
                break;
            }
            case 8: // tape stop
            {
                const float D = 0.9f, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const double rate = std::pow (1.0 - r, 1.6);
                const float sum = saw (adv (v.fxPh[0], 220.0 * rate)) + saw (adv (v.fxPh[1], 277.18 * rate)) + saw (adv (v.fxPh[2], 329.63 * rate));
                s = lp (v.fxA, sum, 200.0f + 5000.0f * (float) rate, 0.8f) * std::pow (1.0f - r, 0.4f) * 0.33f;
                break;
            }
            case 9: // reverse swell, 1 bar
            {
                const float D = 4.0f * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float sum = saw (adv (v.fxPh[0], 130.81)) + saw (adv (v.fxPh[1], 196.0 * 1.003)) + saw (adv (v.fxPh[2], 261.63 * 0.997));
                s = lp (v.fxA, sum, 250.0f * std::pow (2.0f, 4.0f * r), 0.9f) * std::pow (r, 2.5f) * 0.42f;
                break;
            }
            case 10: // rising noise, 1 bar
            {
                const float D = 4.0f * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                s = lp (v.fxA, nz, 150.0f * std::pow (2.0f, 6.5f * r), 1.2f) * (0.25f + 0.75f * r) * 0.85f;
                pan = 0.5f * sinTab (r * 2.0f);
                break;
            }
            case 11: // hit
            {
                if (t > 1.6f) { done = true; break; }
                const float sum = saw (adv (v.fxPh[0], 130.81)) + saw (adv (v.fxPh[1], 155.56 * 1.004)) + saw (adv (v.fxPh[2], 196.0 * 0.996)) + saw (adv (v.fxPh[3], 261.63));
                const float body = lp (v.fxA, sum, 400.0f + 7000.0f * std::exp (-t / 0.18f), 0.8f) * std::exp (-t / 0.4f) * 0.35f;
                const float nb = lp (v.fxB, nz, 4000.0f, 0.7f) * std::exp (-t / 0.06f) * 0.4f;
                s = std::tanh ((body + nb) * 1.6f) * 0.75f;
                break;
            }
            case 12: // tom fill, 1 bar of 1/16
            {
                const float hd = beat * 0.25f;
                if (t > 16.0f * hd + 0.5f) { done = true; break; }
                const int idx = std::min (15, (int) (t / hd));
                if (idx != v.fxHits) { v.fxHits = idx; v.fxPh[0] = 0.0; }
                const float tt = t - (float) idx * hd;
                const float f = (210.0f - (float) idx * 8.0f) * (1.0f + 0.5f * std::exp (-tt / 0.015f));
                const float tom = sinTab (adv (v.fxPh[0], f)) * std::exp (-tt / 0.14f) + nz * std::exp (-tt / 0.012f) * 0.3f;
                s = std::tanh (tom * (0.45f + 0.55f * (float) idx / 15.0f) * 1.3f) * 0.8f;
                pan = 0.5f - (float) idx / 15.0f;
                break;
            }
            case 13: // laser
            {
                if (t > 0.6f) { done = true; break; }
                const float f = 120.0f + 3500.0f * std::exp (-t / 0.07f);
                s = std::tanh (5.0f * sinTab (adv (v.fxPh[0], f))) * std::exp (-t / 0.22f) * 0.33f;
                break;
            }
            case 14: // snare roll riser, 2 bars, gets faster
            {
                const float D = 8.0f * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float iv = beat * (r < 0.5f ? 0.5f : r < 0.75f ? 0.25f : r < 0.9f ? 0.125f : 0.0625f);
                if (t >= v.fxNext) { v.fxHitT = 0.0f; v.fxNext += iv; v.fxPh[0] = 0.0; }
                v.fxHitT += dt;
                const float sn = (bp (v.fxA, nz, 1800.0f, 0.8f) * 0.9f + sinTab (adv (v.fxPh[0], 190.0)) * 0.4f) * std::exp (-v.fxHitT / 0.09f);
                s = sn * (0.25f + 0.75f * r);
                break;
            }
            case 16: // filter sweep, 2 bars
            {
                const float D = 8.0f * beat, r = t / D;
                if (r >= 1.0f) { done = true; break; }
                const float e = std::sin ((float) kPi * r);
                const float x = nz * 0.5f + 0.5f * (saw (adv (v.fxPh[0], 65.41)) + saw (adv (v.fxPh[1], 65.41 * 1.006)));
                s = lp (v.fxA, x, 200.0f * std::pow (2.0f, 6.0f * e), 3.0f) * std::pow (e, 0.7f) * 0.55f;
                break;
            }
            case 17: // glitch stutter, 1 bar of 1/32
            {
                const float D = 4.0f * beat;
                if (t >= D) { done = true; break; }
                const float slot = beat / 8.0f;
                const int idx = (int) (t / slot);
                static constexpr float kG[8] = { 220, 330, 440, 660, 880, 1320, 165, 110 };
                if (idx != v.fxHits) { v.fxHits = idx; v.fxHitF = kG[(idx * 7 + 3) % 8]; }
                const float loc = (t - (float) idx * slot) / slot;
                if (loc < 0.55f)
                {
                    const float sq = sinTab (adv (v.fxPh[0], v.fxHitF)) > 0.0f ? 1.0f : -1.0f;
                    s = std::round (sq * std::exp (-loc * slot / 0.05f) * 4.0f) / 4.0f * 0.25f;
                }
                pan = (idx & 1) ? 0.6f : -0.6f;
                break;
            }
            case 18: // wind texture, held
            {
                if (v.released) { v.fxRel *= std::exp (-dt / 0.6f); if (v.fxRel < 1.0e-4f) { done = true; break; } }
                const float att = std::min (1.0f, t / 1.2f) * v.fxRel;
                const float fc = 700.0f + 450.0f * sinTab (0.13 * t) + 250.0f * sinTab (0.31 * t + 0.2);
                s = bp (v.fxA, nz, fc, 1.2f) * 1.1f * att;
                sR = bp (v.fxB, rng.next(), fc * 1.6f, 1.5f) * 1.0f * att;
                stereo = true;
                break;
            }
            default: // 19: sub pulse (heartbeat), held
            {
                if (v.released) { v.fxRel *= std::exp (-dt / 0.3f); if (v.fxRel < 1.0e-4f) { done = true; break; } }
                const int idx = (int) (t / beat);
                const float tt = t - (float) idx * beat;
                if (idx != v.fxHits) { v.fxHits = idx; v.fxPh[0] = 0.0; v.fxPh[1] = 0.0; }
                const float t2 = tt - 0.3f * beat;
                float x = sinTab (adv (v.fxPh[0], 48.0f + 60.0f * std::exp (-tt / 0.03f))) * std::exp (-tt / 0.35f);
                if (t2 > 0.0f) x += 0.6f * sinTab (adv (v.fxPh[1], 46.0f + 50.0f * std::exp (-t2 / 0.03f))) * std::exp (-t2 / 0.3f);
                s = std::tanh (x * 1.5f) * 0.8f * v.fxRel;
                break;
            }
        }
        if (done) { v.active = false; outL = outR = 0.0f; return; }
        {
            static constexpr float kEndBeats[20] = { 16, 8, 8, 0, 0, 0, 0, 4, 0, 4, 4, 0, 0, 0, 8, 0, 8, 4, 0, 0 };
            const float endB = kEndBeats[v.fxType];
            if (endB > 0.0f) s *= clampf ((endB * beat - t) / 0.004f, 0.0f, 1.0f), sR *= clampf ((endB * beat - t) / 0.004f, 0.0f, 1.0f);
            if (t < 0.002f && v.fxType != 3 && v.fxType != 15) { s *= t / 0.002f; sR *= t / 0.002f; }
        }
        const float g = 0.5f + 0.5f * v.velocity;
        if (stereo) { outL = s * g; outR = sR * g; return; }
        const float ang = (clampf (pan, -1.0f, 1.0f) + 1.0f) * 0.25f * (float) kPi;
        outL = s * std::cos (ang) * 1.4142f * g;
        outR = s * std::sin (ang) * 1.4142f * g;
    }

    struct Grain { bool on = false; double ph = 0, inc = 0; float t = 0, dur = 0.2f, amp = 0, pan = 0; };
    std::array<Grain, 24> grains;
    float dustTimer = 0.0f;

    void dustTick (float& l, float& r, float amount)
    {
        const float dt = 1.0f / sr;
        dustTimer -= dt;
        if (dustTimer <= 0.0f && ! arpKeys.empty())
        {
            dustTimer = 0.03f + 0.12f * rng.unit();                       // about ten specks per second, irregular
            const int base = arpKeys[(size_t) std::min ((int) arpKeys.size() - 1, (int) (rng.unit() * (float) arpKeys.size()))];
            float trans = 0.0f;
            for (auto& v : voices) if (v.active && v.mask == 1) { trans = v.smpTrans; break; }
            const int n = base + (int) trans + (rng.unit() < 0.5f ? 12 : 24);
            for (auto& gr : grains)
                if (! gr.on)
                {
                    gr.on = true; gr.ph = 0.0; gr.t = 0.0f;
                    gr.inc = 440.0 * std::pow (2.0, (std::min (n, 100) - 69.0) / 12.0) / sr;
                    gr.dur = 0.15f + 0.35f * rng.unit();
                    gr.amp = 0.085f * amount * (0.4f + 0.6f * rng.unit());
                    gr.pan = rng.next() * 0.9f;
                    break;
                }
        }
        for (auto& gr : grains)
        {
            if (! gr.on) continue;
            gr.t += dt;
            if (gr.t >= gr.dur) { gr.on = false; continue; }
            gr.ph += gr.inc; if (gr.ph >= 1.0) gr.ph -= 1.0;
            const float w = std::sin ((float) kPi * gr.t / gr.dur);
            const float s = sinTab (gr.ph) * w * w * gr.amp;               // pure sine: soft, never bright
            l += s * (1.0f - gr.pan); r += s * (1.0f + gr.pan);
        }
    }

    void chorusTick (float& l, float& r, float amt)
    {
        chL[(size_t) chW] = l; chR[(size_t) chW] = r;
        chPh += 0.5f / sr; if (chPh >= 1.0f) chPh -= 1.0f;
        const float base = 0.0045f * sr, dep = 0.0018f * sr;
        const float wl = ring (chL, chW, base + dep * sinTab (chPh));
        const float wr = ring (chR, chW, base + dep * sinTab (chPh + 0.25f));
        chW = (chW + 1) & 4095;
        l += (wl * 0.6f - wr * 0.15f) * amt; r += (wr * 0.6f - wl * 0.15f) * amt;
    }

    static float softLimit (float x)
    {
        const float a = std::abs (x);
        if (a < 0.8f) return x;
        const float y = 0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f);
        return x < 0 ? -y : y;
    }

    float readDelay (const std::vector<float>& buf, float delaySamples) const
    {
        float pos = (float) dlyW - delaySamples;
        const float size = (float) buf.size();
        while (pos < 0.0f) pos += size;
        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % (int) buf.size();
        const float f = pos - (float) i0;
        return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
    }

    void updateEnvelopes()
    {
        const auto& p = values;
        for (auto& v : voices)
        {
            v.amp.set (p[pAmpA], p[pAmpD], p[pAmpS], p[pAmpR], sr);
            v.filt.set (p[pFiltA], p[pFiltD], p[pFiltS], p[pFiltR], sr);
        }
    }

    // ---- mono legato: runs glide from note to note without retriggering
    void monoNoteOn (int note, float vel)
    {
        auto& v = voices[0];
        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        const bool legato = ! held.empty() && v.active && v.amp.stage != Env::Release;
        held.push_back (note);

        const bool legatoOnly = values[pLegatoGlide] > 0.5f;
        const bool glide = v.active && (legato || ! legatoOnly);
        if (std::lround (values[pEngine]) == 2)
        {
            // bass: every note is a new pluck on the string (a slide only if Glide is turned up)
            v.start (note, vel, true, glide && values[pGlide] > 0.5f, rng);
            bassPluck (v);
            return;
        }
        if (legato)
        {
            v.target = note; v.note = note;
            v.velocity = 0.6f * v.velocity + 0.4f * vel;   // keep power building through a run
        }
        else
        {
            v.start (note, vel, true, glide, rng);
            epSetup (v);
            smpSetup (v);
        }
    }

    void monoNoteOff (int note)
    {
        auto& v = voices[0];
        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        if (held.empty()) { if (v.active) v.release(); }
        else if (v.note == note) { v.target = held.back(); v.note = held.back(); }
    }

    // ---- finger bass -----------------------------------------------------------------
    // A finger pluck: a soft, rounded displacement fed into the string over one period,
    // plus a tiny low "thump" of the fingertip hitting the string.
    void bassPluck (Voice& v)
    {
        const float hz = 440.0f * std::pow (2.0f, ((float) v.note - 69.0f) / 12.0f);
        const float period = sr / hz;
        const float vel = v.velocity;
        const bool ringing = v.ksAmp > 0.02f && v.active;
        if (ringing)                                       // re-pluck: the finger stops most of the old vibration
        {
            v.muteSamples = (int) period + 1;              // over one full period the whole string is damped...
            v.muteRamp = 0.0f;                             // ...with a short fade so there is no click
        }
        v.excLen = std::max (4, (int) period);
        v.excPos = 0;
        v.excPeak = clampf (values[pBassPluck], 0.05f, 0.5f);
        // harder = brighter attack; Tone knob opens it further
        const float excHz = (800.0f + 3500.0f * vel * vel) * (0.5f + values[pBassTone]);
        v.excLpCoef = OnePole::coefFor (excHz, sr);
        v.excLpState = 0.0f;
        v.excGain = 0.45f + 0.75f * vel;
        v.bassBright = 1.0f;
        v.bassBrightCoef = std::exp (-1.0f / (0.28f * sr));
        v.thumpPhase = 0.0f;
        v.thumpGain = values[pBassThump] * (0.3f + 0.7f * vel);
        v.ksAmp = 1.0f;
        if (! ringing) { v.ksLp = 0.0f; v.ksDc = v.ksDcIn = 0.0f; v.ksPrev = 0.0f; v.muteSamples = 0; v.ksG = 0.0f; v.ksD = 0.0f; std::fill (v.ks.begin(), v.ks.end(), 0.0f); }
    }

    float bassTick (Voice& v, double hz)
    {
        const int size = (int) v.ks.size();
        const float period = clampf ((float) (sr / hz), 2.0f, (float) size - 4.0f);
        const float tone = values[pBassTone];

        // string losses: low notes ring longer; a released key is muted by the fingers
        const float t60 = v.released ? 0.09f
                                     : values[pBassSustain] * std::pow (2.0f, -(float) (v.note - 28) / 18.0f);
        // on a re-pluck the finger damps the old vibration during one period (smoothly, via ksG)
        const float gT = v.muteSamples > 0 ? 0.3f
                                           : std::pow (10.0f, -3.0f * period / (sr * std::max (0.02f, t60)));
        const float dampT = clampf (0.58f - 0.4f * tone + (v.released ? 0.25f : 0.0f), 0.05f, 0.9f);  // one-pole loss filter: highs fade first
        // the finger lands on the string over a few ms: smooth the change so releases never click
        const float sm = 1.0f - std::exp (-1.0f / (0.006f * sr));
        if (v.ksG <= 0.0f) { v.ksG = gT; v.ksDamp = dampT; }
        v.ksG += sm * (gT - v.ksG);
        v.ksDamp += sm * (dampT - v.ksDamp);
        const float g = v.ksG, damp = v.ksDamp;

        // tuning: remove the loss filter's phase delay so the note stays in tune
        const float w = 2.0f * (float) kPi / period;
        const float lpDelay = std::atan2 (damp * std::sin (w), 1.0f - damp * std::cos (w)) / w;
        const float dT = clampf (period - lpDelay, 2.0f, (float) size - 4.0f);
        // a new fret position takes ~2 ms to settle, so the read point never jumps (no click)
        if (v.ksD <= 0.0f) v.ksD = dT;
        v.ksD += (1.0f - std::exp (-1.0f / (0.002f * sr))) * (dT - v.ksD);
        const float d = v.ksD;

        float rp = (float) v.ksW - d;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        const float y = v.ks[(size_t) i0] + fr * (v.ks[(size_t) ((i0 + 1) % size)] - v.ks[(size_t) i0]);

        // loss filter + decay
        v.ksLp = (1.0f - damp) * y + damp * v.ksLp;
        float in = v.ksLp * g;
        if (v.muteSamples > 0) --v.muteSamples;

        // excitation: rounded pluck shape (triangle peaking at the pluck point), softened by the finger
        if (v.excPos < v.excLen + 96)
        {
            const float x = std::min (1.0f, (float) v.excPos / (float) v.excLen);
            // rounded finger shape (no corner at the start, so the string starts moving smoothly)
            const float shape = x < v.excPeak ? 0.5f - 0.5f * std::cos ((float) kPi * x / v.excPeak)
                                              : 0.5f + 0.5f * std::cos ((float) kPi * (x - v.excPeak) / (1.0f - v.excPeak));
            v.excLpState += v.excLpCoef * (shape - v.excLpState);
            in += v.excLpState * v.excGain;
            ++v.excPos;
        }
        v.ks[(size_t) v.ksW] = in;
        v.ksAmp = 0.999f * v.ksAmp + 0.001f * std::abs (in);

        // pickup near the bridge: the string seen from a point (comb) gives the electric-bass "growl"
        const float pickupPos = 0.06f + 0.16f * (1.0f - values[pBassPickup]);
        float pp = (float) v.ksW - pickupPos * period;
        while (pp < 0.0f) pp += (float) size;
        const int p0 = (int) pp;
        const float yp = v.ks[(size_t) p0];
        if (++v.ksW >= size) v.ksW = 0;

        // a magnetic pickup senses string velocity as well as position: blend both
        const float velSig = (in - v.ksPrev) * period / (2.0f * (float) kPi);
        v.ksPrev = in;
        float out = 0.55f * (in - 0.85f * yp) + 0.6f * velSig;

        // remove DC (the pluck shape has an offset)
        v.ksDc = out - v.ksDcIn + 0.9995f * v.ksDc; v.ksDcIn = out;
        out = v.ksDc;

        // fingertip thump: a short low bump on the attack
        if (v.thumpPhase < 1.0f)
        {
            out += std::sin ((float) kPi * v.thumpPhase) * v.thumpGain * 0.35f;
            v.thumpPhase += 1.0f / (0.012f * sr);
        }

        // amp / cabinet tone: rounded top, a little warmth
        // the string's highs fade after the attack: tone opens on the pluck, then settles
        v.bassBright *= v.bassBrightCoef;
        const float cut = (500.0f + 2500.0f * tone * tone + 600.0f * v.velocity) * (1.0f + 2.5f * v.bassBright);
        const float c = OnePole::coefFor (cut, sr);
        out = v.bassTone2.lp (v.bassTone1.lp (out, c), c);
        return std::tanh (out * 1.6f) * 0.8f;
    }

    // Rhodes decay: longer in the bass, shorter in the treble; quick damper on release
    void epSetup (Voice& v)
    {
        const float tau = values[pEpDecay] * std::pow (2.0f, -(float) (v.note - 60) / 24.0f);
        v.epCoefHeld = std::exp (-1.0f / (std::max (0.05f, tau) * sr));
        v.epCoefRel = std::exp (-1.0f / (0.30f * sr));   // a short, soft damper: chords breathe a little after you lift
        const float toneHz = clampf (2600.0f * std::pow (2.0f, (float) (v.note - 60) / 24.0f) * (0.75f + 0.5f * v.velocity), 1200.0f, 9000.0f);
        v.epToneCoef = OnePole::coefFor (toneHz, sr);
    }

    void polyNoteOn (int note, float vel)
    {
        // A new chord starts when notes are more than ~60 ms apart. The chord you played before
        // is remembered however long you held it: the new chord's notes glide out of it.
        const uint64_t gap = sampleClock - lastNoteOnClock;
        if (gap > (uint64_t) (0.06f * sr))
        {
            prevPitches.clear(); prevVoice.clear();
            for (int n : currentChord)
            {
                double from = (double) n; int vi = -1;
                for (int k = 0; k < kMaxVoices; ++k)   // still sounding (maybe mid-glide)? start from where it is now
                    if (voices[(size_t) k].active && voices[(size_t) k].note == n) { from = voices[(size_t) k].pitch; vi = k; break; }
                prevPitches.push_back (from); prevVoice.push_back (vi);
            }
            prevUsed.assign (prevPitches.size(), false);
            currentChord.clear();
        }
        currentChord.push_back (note);
        lastNoteOnClock = sampleClock;

        Voice* chosen = nullptr;
        const bool sampled = values[pSmpSet] > 0.5f && values[pSmpLevel] > 0.0f;
        if (sampled && values[pGlide] > 0.5f)
        {
            // a common tone keeps sounding: the bow does not restart
            for (auto& v : voices)
                if (v.active && v.note == note && v.mask == noteMask)
                {
                    v.keyDown = true; v.released = false; v.sustained = false; v.age = ++ageCounter;
                    if (v.amp.stage == Env::Release || v.amp.stage == Env::Idle) { v.amp.gate (true); v.filt.gate (true); }
                    return;
                }
            // the nearest note of the previous chord slides to this one, on the same voice
            int best = -1; double bestDist = 12.5;
            for (size_t i = 0; i < prevPitches.size(); ++i)
            {
                const double d = std::abs (prevPitches[i] - note);
                if (! prevUsed[i] && d < bestDist && d > 0.01 && i < prevVoice.size() && prevVoice[i] >= 0) { bestDist = d; best = (int) i; }
            }
            if (best >= 0)
            {
                auto& v = voices[(size_t) prevVoice[(size_t) best]];
                if (v.active && v.mask == noteMask && std::abs (v.pitch - prevPitches[(size_t) best]) < 0.6)
                {
                    prevUsed[(size_t) best] = true;
                    v.note = note; v.target = note;
                    v.velocity = 0.5f * v.velocity + 0.5f * vel;
                    v.keyDown = true; v.released = false; v.sustained = false; v.age = ++ageCounter;
                    if (v.amp.stage == Env::Release || v.amp.stage == Env::Idle) { v.amp.gate (true); v.filt.gate (true); }
                    return;
                }
            }
        }
        // sampled sounds without glide, and effects: a repeated note gets a fresh voice, the old one fades out
        const bool freshVoice = std::lround (values[pEngine]) == 3 || (sampled && values[pGlide] <= 0.5f);
        for (auto& v : voices) if (v.active && v.note == note && v.mask == noteMask) { if (freshVoice) { if (v.keyDown || v.sustained) v.release(); } else { chosen = &v; break; } }
        if (! chosen) for (auto& v : voices) if (! v.active) { chosen = &v; break; }
        if (! chosen)
        {
            // steal: prefer released voices, then the quietest / oldest
            chosen = &voices[0];
            for (auto& v : voices)
            {
                const bool vRel = v.released, cRel = chosen->released;
                if ((vRel && ! cRel) || (vRel == cRel && v.age < chosen->age)) chosen = &v;
            }
        }
        chosen->age = ++ageCounter;
        chosen->start (note, vel, true, false, rng);
        chosen->mask = noteMask;
        chosen->arpPan = 0.0f;
        lastStarted = chosen;
        chosen->padFade = noteMask == 1 ? 0.0f : 1.0f;
        chosen->glideMid = chosen->pitch;
        epSetup (*chosen);
        smpSetup (*chosen);
        if (std::lround (values[pEngine]) == 3) fxSetup (*chosen);
        if (std::lround (values[pEngine]) == 2) bassPluck (*chosen);

        // chord glide: start from the nearest unused note of the previous chord, else scoop up from below
        if (values[pGlide] > 0.5f)
        {
            int best = -1; double bestDist = 12.5;   // every chord tone glides from a tone of the previous chord
            for (size_t i = 0; i < prevPitches.size(); ++i)
            {
                const double d = std::abs (prevPitches[i] - note);
                if (! prevUsed[i] && d < bestDist && d > 0.01) { bestDist = d; best = (int) i; }
            }
            if (best >= 0) { chosen->pitch = prevPitches[(size_t) best]; prevUsed[(size_t) best] = true; }
            else chosen->pitch = note + values[pScoop];
            chosen->glideMid = chosen->pitch;
        }
    }
};

} // namespace dsynth
