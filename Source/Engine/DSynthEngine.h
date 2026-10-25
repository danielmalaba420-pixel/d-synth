// D-SYNTH by Danova — sound engine
// Plain C++17, no framework dependency: used by the plugin and by the offline demo renderer.
#pragma once

#include "Params.h"

#include <algorithm>
#include <array>
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

    // ---- performance input
    void pitchBend (float normalised) { bend = clampf (normalised, -1.0f, 1.0f); }   // -1..1
    void modWheel (float normalised)  { mod = clampf (normalised, 0.0f, 1.0f); }     // 0..1

    void noteOn (int note, float vel)
    {
        updateEnvelopes();
        if (values[pPoly] < 0.5f) monoNoteOn (note, vel);
        else polyNoteOn (note, vel);
    }

    void noteOff (int note)
    {
        if (values[pPoly] < 0.5f) { monoNoteOff (note); return; }

        const bool keysMode = std::lround (values[pEngine]) == 1;
        for (auto& v : voices)
        {
            if (! v.active || v.note != note || ! v.keyDown) continue;
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

        for (int i = 0; i < numSamples; ++i)
        {
            buildSm += smC * (buildTarget - buildSm);
            const float b = buildSm;

            vibPhase += vibInc; if (vibPhase >= 1.0f) vibPhase -= 1.0f;
            const float vibSin = std::sin (2.0f * (float) kPi * vibPhase);
            tremPhase += tremInc; if (tremPhase >= 1.0f) tremPhase -= 1.0f;
            const float tremSin = std::sin (2.0f * (float) kPi * tremPhase);
            ++sampleClock;

            // Build macro: more drive, brighter, wider, more space
            const float drive = clampf (p[pDrive] + b * 0.45f, 0.0f, 1.0f);
            const float cutoffBuildOct = b * 2.0f + mod * p[pMwFilter] * 3.0f;

            float sumL = 0.0f, sumR = 0.0f;
            float padBus = 0.0f;

            for (auto& v : voices)
            {
                if (! v.active) continue;

                // glide
                if (glideCoef > 0.0) v.pitch = v.target + (v.pitch - v.target) * glideCoef;
                else v.pitch = v.target;

                v.timeSinceOn += dt;
                const float vibRamp = clampf ((v.timeSinceOn * 1000.0f - p[pVibDelay]) / 400.0f, 0.0f, 1.0f);
                const float vibDepth = p[pVibDepth] * vibRamp + mod * p[pMwVibrato] * 0.5f;
                const double pitch = v.pitch + pbSemis + vibSin * vibDepth;

                const float aenv = v.amp.tick();
                const float fenv = v.filt.tick();
                const bool padIdle = v.amp.stage == Env::Idle;
                if (padIdle && ! bassMode && (! keysMode || v.epEnv < 1.0e-4f)) { v.active = false; continue; }

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
                if (noiseLevel > 0.0001f) mono += noiseLevel * 0.5f * rng.next();
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

                const float gl = vl * aenv * velGain * padLevel;
                const float gr = vr * aenv * velGain * padLevel;
                sumL += gl; sumR += gr;
                padBus += 0.5f * (gl + gr);
            }

            if (p[pPoly] > 0.5f) { sumL *= 0.5f; sumR *= 0.5f; }   // chords stack up: keep headroom

            // width (M/S), widened by Build
            {
                const float w = clampf (p[pWidth] + b * 0.5f, 0.0f, 2.0f);
                const float m = 0.5f * (sumL + sumR), s = 0.5f * (sumL - sumR) * w;
                sumL = m + s; sumR = m - s;
            }

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
                const float dMix = clampf (p[pDlyMix] + b * 0.25f, 0.0f, 1.5f);
                const float fb = clampf (p[pDlyFb] + b * 0.15f, 0.0f, 0.95f);
                const float yL = readDelay (dlyL, dlySamplesSm);
                const float yR = readDelay (dlyR, dlySamplesSm);
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
            }

            // reverb (fed by dry + a bit of delay)
            float rL = 0.0f, rR = 0.0f;
            {
                const float rMix = clampf (p[pRevMix] + b * 0.2f, 0.0f, 1.0f);
                float shim = 0.0f;
                if (shimmer > 0.001f)
                    shim = octaveUp.process (padBus * 1.5f + 0.35f * (lastRevL + lastRevR)) * shimmer * 1.6f;
                const float inL = revHPL.hp (sumL + wetL * 0.5f + shim, revHPc);
                const float inR = revHPR.hp (sumR + wetR * 0.5f + shim, revHPc);
                reverb.process (inL, inR, p[pRevSize], p[pRevDamp], rL, rR);
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
    float compEnv = 0.0f, duckEnv = 0.0f;
    float tremPhase = 0.0f;
    OctaveUp octaveUp;
    float lastRevL = 0.0f, lastRevR = 0.0f;
    uint64_t sampleClock = 0, lastNoteOnClock = 0;
    std::vector<double> prevPitches;
    std::vector<int> currentChord;
    std::vector<bool> prevUsed;
    bool pedal = false;

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
            prevPitches.clear();
            for (int n : currentChord)
            {
                double from = (double) n;
                for (auto& v : voices)                 // still sounding (maybe mid-glide)? start from where it is now
                    if (v.active && v.note == n) { from = v.pitch; break; }
                prevPitches.push_back (from);
            }
            prevUsed.assign (prevPitches.size(), false);
            currentChord.clear();
        }
        currentChord.push_back (note);
        lastNoteOnClock = sampleClock;

        Voice* chosen = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) { chosen = &v; break; }
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
        epSetup (*chosen);
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
        }
    }
};

} // namespace dsynth
