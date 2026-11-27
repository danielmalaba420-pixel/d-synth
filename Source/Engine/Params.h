// D-SYNTH by Danova — parameter table
// Shared by the DSP engine, the presets, the plugin wrapper and the demo renderer.
#pragma once

#include <array>
#include <cstring>

namespace dsynth
{

enum ParamId : int
{
    // Oscillators
    pOscAWave = 0, pOscBWave, pOscBLevel, pOscBSemi, pOscBDetune,
    pUnison, pDetune, pSpread, pSubLevel, pSubOct, pNoise,
    // Voicing
    pPoly, pGlide, pLegatoGlide,
    // Amp envelope
    pAmpA, pAmpD, pAmpS, pAmpR, pVelSens,
    // Filter
    pCutoff, pReso, pKeyTrack, pFiltEnv, pFiltA, pFiltD, pFiltS, pFiltR, pVelCutoff,
    // Drive
    pDrive, pVelDrive,
    // Vibrato and performance
    pVibRate, pVibDepth, pVibDelay, pMwVibrato, pMwBuild, pMwFilter, pBuild, pPbRange,
    // Delay
    pDlyTime, pDlyFb, pDlyMix, pDlyPingPong, pDlyHP, pDlyLP, pDlyDuck, pDlyDuo,
    // Reverb
    pRevSize, pRevDamp, pRevMix,
    // Master
    pWidth, pComp, pOutput,
    // Keys engine (electric piano layer + pad layer + chord glide)
    pEngine, pEpLevel, pEpBark, pEpDecay, pEpTrem, pMwTrem, pPadLevel, pScoop, pEpModel, pShimmer, pRhodesMix,
    // Bass engine (plucked string model)
    pBassTone, pBassPluck, pBassSustain, pBassPickup, pBassThump,
    pClean,
    // v2: arpeggiator, bloom delay, chorus, slow LFO, pitch envelope
    pArp, pArpRate, pArpMode, pArpOct, pArpGate,
    pDlyBloom, pMwDelay, pChorus, pLfoRate, pLfoCut, pPitchEnv, pPitchEnvTime,
    // v2: tonewheel organ layer + rotary speaker
    pOrgLevel, pOrgReg, pOrgPerc, pOrgClick, pOrgVib, pOrgDrive, pLeslie,
    // v2: sampled layer (strings, brass, piano)
    pSmpSet, pSmpLevel, pSmpStac, pMwPad,
    // v2: tempo-synced riser
    pRiser, pRiserBars, pRiserPitch,
    pRevHall, pArpPad, pArpPan, pArpDust, pRevPre,
    kNumParams
};

enum class ParamKind { Float, Int, Choice, Bool };

struct ParamInfo
{
    const char* id;
    const char* name;
    float min, max, def;
    ParamKind kind;
    const char* unit;
    float skewCentre; // 0 = linear, otherwise value that sits at the middle of the knob
};

// Wave shapes for oscillator A and B
static constexpr const char* kWaveNames[] = { "Saw", "Square", "Sine", "Triangle" };
static constexpr const char* kEngineNames[] = { "Lead", "Keys", "Bass", "FX" };
static constexpr const char* kArpRateNames[] = { "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static constexpr float kArpRateBeats[] = { 0.5f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f, 0.125f };
static constexpr const char* kArpModeNames[] = { "Up", "Down", "Up/Down", "Repeat", "Worship 1-5-8", "Broken" };
static constexpr const char* kOrgRegNames[] = { "Gospel", "888862200", "All 9 Bars", "J's Jazz" };
static constexpr const char* kOrgVibNames[] = { "Off", "C1", "C2", "C3" };
static constexpr const char* kSmpSetNames[] = { "None", "Strings", "Brass", "Piano", "Organ", "Glockenspiel", "Disney Strings" };
static constexpr const char* kRiserBarNames[] = { "1 bar", "2 bars", "4 bars", "8 bars" };
static constexpr const char* kEpModelNames[] = { "Rhodes", "EP" };

// Delay divisions, in beats
static constexpr const char* kDelayNames[] = { "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4." };
static constexpr float kDelayBeats[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f };

inline const std::array<ParamInfo, kNumParams>& paramTable()
{
    static const std::array<ParamInfo, kNumParams> t = { {
        { "oscAWave",   "Osc A Wave",        0, 3, 0,      ParamKind::Choice, "",    0 },
        { "oscBWave",   "Osc B Wave",        0, 3, 1,      ParamKind::Choice, "",    0 },
        { "oscBLevel",  "Osc B Level",       0, 1, 0.0f,   ParamKind::Float,  "",    0 },
        { "oscBSemi",   "Osc B Semitones",  -24, 24, 0,    ParamKind::Int,    "st",  0 },
        { "oscBDetune", "Osc B Detune",     -50, 50, 0,    ParamKind::Float,  "ct",  0 },
        { "unison",     "Unison Voices",     1, 7, 1,      ParamKind::Int,    "",    0 },
        { "detune",     "Unison Detune",     0, 50, 10,    ParamKind::Float,  "ct",  0 },
        { "spread",     "Stereo Spread",     0, 1, 0.5f,   ParamKind::Float,  "",    0 },
        { "subLevel",   "Sub Level",         0, 1, 0.0f,   ParamKind::Float,  "",    0 },
        { "subOct",     "Sub Octave",        0, 1, 0,      ParamKind::Choice, "",    0 },
        { "noise",      "Noise",             0, 1, 0.0f,   ParamKind::Float,  "",    0 },

        { "poly",       "Poly Mode",         0, 1, 0,      ParamKind::Bool,   "",    0 },
        { "glide",      "Glide",             0, 2000, 30,  ParamKind::Float,  "ms",  120 },
        { "legatoGlide","Glide Legato Only", 0, 1, 1,      ParamKind::Bool,   "",    0 },

        { "ampA",       "Amp Attack",        0.5f, 2000, 3,    ParamKind::Float, "ms", 60 },
        { "ampD",       "Amp Decay",         1, 4000, 400,     ParamKind::Float, "ms", 400 },
        { "ampS",       "Amp Sustain",       0, 1, 0.85f,      ParamKind::Float, "",   0 },
        { "ampR",       "Amp Release",       1, 5000, 250,     ParamKind::Float, "ms", 400 },
        { "velSens",    "Velocity Sens",     0, 1, 0.4f,       ParamKind::Float, "",   0 },

        { "cutoff",     "Filter",            40, 18000, 3000,  ParamKind::Float, "Hz", 1200 },
        { "reso",       "Resonance",         0, 1, 0.2f,       ParamKind::Float, "",   0 },
        { "keyTrack",   "Key Tracking",      0, 1.5f, 0.8f,    ParamKind::Float, "",   0 },
        { "filtEnv",    "Filter Env Amount", -4, 6, 1.0f,      ParamKind::Float, "oct",0 },
        { "filtA",      "Filter Attack",     0.5f, 2000, 2,    ParamKind::Float, "ms", 60 },
        { "filtD",      "Filter Decay",      1, 4000, 300,     ParamKind::Float, "ms", 400 },
        { "filtS",      "Filter Sustain",    0, 1, 0.4f,       ParamKind::Float, "",   0 },
        { "filtR",      "Filter Release",    1, 5000, 300,     ParamKind::Float, "ms", 400 },
        { "velCutoff",  "Velocity > Filter", 0, 3, 0.8f,       ParamKind::Float, "oct",0 },

        { "drive",      "Drive",             0, 1, 0.2f,   ParamKind::Float, "",   0 },
        { "velDrive",   "Velocity > Drive",  0, 1, 0.2f,   ParamKind::Float, "",   0 },

        { "vibRate",    "Vibrato Rate",      0.5f, 10, 5.5f, ParamKind::Float, "Hz", 0 },
        { "vibDepth",   "Auto Vibrato",      0, 1, 0.05f,  ParamKind::Float, "st", 0 },
        { "vibDelay",   "Vibrato Delay",     0, 1500, 350, ParamKind::Float, "ms", 0 },
        { "mwVibrato",  "Mod > Vibrato",     0, 1, 1.0f,   ParamKind::Float, "",   0 },
        { "mwBuild",    "Mod > Build",       0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "mwFilter",   "Mod > Filter",      0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "build",      "Build",             0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "pbRange",    "Pitch Bend Range",  1, 24, 2,     ParamKind::Int,   "st", 0 },

        { "dlyTime",    "Delay Time",        0, 6, 3,      ParamKind::Choice, "",   0 },
        { "dlyFb",      "Delay Feedback",    0, 0.95f, 0.35f, ParamKind::Float, "", 0 },
        { "dlyMix",     "Delay",             0, 1.5f, 0.2f, ParamKind::Float, "",   0 },
        { "dlyPingPong","Ping-Pong",         0, 1, 1,      ParamKind::Bool,  "",   0 },
        { "dlyHP",      "Delay Low Cut",     20, 2000, 300,  ParamKind::Float, "Hz", 300 },
        { "dlyLP",      "Delay High Cut",    1000, 20000, 7000, ParamKind::Float, "Hz", 6000 },
        { "dlyDuck",    "Delay Ducking",     0, 1, 0.4f,   ParamKind::Float, "",   0 },
        { "dlyDuo",     "Delay Duo",         0, 1, 0.0f,   ParamKind::Float, "",   0 },

        { "revSize",    "Reverb Size",       0, 1, 0.7f,   ParamKind::Float, "",   0 },
        { "revDamp",    "Reverb Damping",    0, 1, 0.45f,  ParamKind::Float, "",   0 },
        { "revMix",     "Reverb",            0, 1, 0.15f,  ParamKind::Float, "",   0 },

        { "width",      "Width",             0, 2, 1.0f,   ParamKind::Float, "",   0 },
        { "comp",       "Compression",       0, 1, 0.3f,   ParamKind::Float, "",   0 },
        { "output",     "Output",           -24, 6, -3,    ParamKind::Float, "dB", 0 },

        { "engine",     "Engine",            0, 3, 0,      ParamKind::Choice, "",   0 },
        { "epLevel",    "Keys Level",        0, 1, 0.8f,   ParamKind::Float, "",   0 },
        { "epBark",     "Keys Bark",         0, 1, 0.5f,   ParamKind::Float, "",   0 },
        { "epDecay",    "Keys Decay",        0.5f, 12, 4,  ParamKind::Float, "s",  0 },
        { "epTrem",     "Keys Tremolo",      0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "mwTrem",     "Mod > Tremolo",     0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "padLevel",   "Pad / Synth Level", 0, 1, 1.0f,   ParamKind::Float, "",   0 },
        { "scoop",      "Glide Scoop",      -5, 0, 0.0f,   ParamKind::Float, "st", 0 },
        { "epModel",    "Keys Model",        0, 1, 0,      ParamKind::Choice, "",  0 },
        { "shimmer",    "Pad Shimmer",       0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "rhodesMix",  "Rhodes Layer",      0, 1, 0.0f,   ParamKind::Float, "",   0 },
        { "bassTone",   "Bass Tone",         0, 1, 0.45f,  ParamKind::Float, "",   0 },
        { "bassPluck",  "Bass Pluck Position", 0.05f, 0.5f, 0.22f, ParamKind::Float, "", 0 },
        { "bassSustain","Bass Sustain",      0.5f, 10, 4.0f, ParamKind::Float, "s",  0 },
        { "bassPickup", "Bass Pickup",       0, 1, 0.5f,   ParamKind::Float, "",   0 },
        { "bassThump",  "Bass Finger Thump", 0, 1, 0.5f,   ParamKind::Float, "",   0 },
        { "clean",      "Pure Tone",         0, 1, 0,      ParamKind::Bool,  "",   0 },
        { "arp",        "Arpeggiator",       0, 1, 0,      ParamKind::Bool,   "",   0 },
        { "arpRate",    "Arp Rate",          0, 4, 2,      ParamKind::Choice, "",   0 },
        { "arpMode",    "Arp Pattern",       0, 5, 0,      ParamKind::Choice, "",   0 },
        { "arpOct",     "Arp Octaves",       1, 3, 1,      ParamKind::Int,    "",   0 },
        { "arpGate",    "Arp Gate",          0.1f, 1, 0.6f, ParamKind::Float, "",   0 },
        { "dlyBloom",   "Delay Bloom",       0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "mwDelay",    "Mod > Delay",       0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "chorus",     "Chorus",            0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "lfoRate",    "Slow LFO Rate",     0.02f, 2, 0.12f, ParamKind::Float, "Hz", 0 },
        { "lfoCut",     "Slow LFO > Filter", 0, 2, 0.0f,   ParamKind::Float,  "oct",0 },
        { "pitchEnv",   "Pitch Drop",        0, 24, 0.0f,  ParamKind::Float,  "st", 0 },
        { "pitchEnvT",  "Pitch Drop Time",   2, 300, 40,   ParamKind::Float,  "ms", 0 },
        { "orgLevel",   "Organ Level",       0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "orgReg",     "Organ Drawbars",    0, 3, 0,      ParamKind::Choice, "",   0 },
        { "orgPerc",    "Organ Percussion",  0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "orgClick",   "Organ Key Click",   0, 1, 0.4f,   ParamKind::Float,  "",   0 },
        { "orgVib",     "Organ Vibrato",     0, 3, 3,      ParamKind::Choice, "",   0 },
        { "orgDrive",   "Organ Drive",       0, 1, 0.2f,   ParamKind::Float,  "",   0 },
        { "leslie",     "Rotary",            0, 1, 1.0f,   ParamKind::Float,  "",   0 },
        { "smpSet",     "Sample Set",        0, 6, 0,      ParamKind::Choice, "",   0 },
        { "smpLevel",   "Sample Level",      0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "smpStac",    "Sample Stabs",      0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "mwPad",      "Mod > Synth Level", 0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "riser",      "Riser",             0, 1, 0,      ParamKind::Bool,   "",   0 },
        { "riserBars",  "Riser Length",      0, 3, 2,      ParamKind::Choice, "",   0 },
        { "riserPitch", "Riser Pitch",       0, 24, 12,    ParamKind::Float,  "st", 0 },
        { "revHall",    "Smooth Hall Reverb", 0, 1, 0,     ParamKind::Bool,   "",   0 },
        { "arpPad",     "Pad Under Arp",     0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "arpPan",     "Arp Left-Right",    0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "arpDust",    "Arp Stardust",      0, 1, 0.0f,   ParamKind::Float,  "",   0 },
        { "revPre",     "Reverb Pre-delay",  0, 80, 0,     ParamKind::Float,  "ms", 0 },
    } };
    return t;
}

using ParamValues = std::array<float, kNumParams>;

inline ParamValues defaultValues()
{
    ParamValues v{};
    const auto& t = paramTable();
    for (int i = 0; i < kNumParams; ++i)
        v[(size_t) i] = t[(size_t) i].def;
    return v;
}

inline int findParam(const char* id)
{
    const auto& t = paramTable();
    for (int i = 0; i < kNumParams; ++i)
        if (std::strcmp(t[(size_t) i].id, id) == 0)
            return i;
    return -1;
}

} // namespace dsynth
