// D-SYNTH by Danova — the 12 factory leads
#pragma once

#include "Params.h"

#include <initializer_list>
#include <utility>
#include <vector>

namespace dsynth
{

struct Preset
{
    const char* name;
    const char* description;
    std::vector<std::pair<int, float>> overrides; // on top of the defaults

    ParamValues values() const
    {
        auto v = defaultValues();
        for (auto& [id, val] : overrides) v[(size_t) id] = val;
        return v;
    }
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = {

        // 01 — the flagship: heavy lead for runs, delay trails, power builds as you climb
        { "RISING RUN", "Lead lourd · runs · delay · montée en puissance", {
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 16 }, { pSpread, 0.85f },
            { pOscBWave, 1 }, { pOscBLevel, 0.35f }, { pOscBSemi, 0 }, { pOscBDetune, 6 },
            { pSubLevel, 0.55f }, { pSubOct, 0 },
            { pPoly, 0 }, { pGlide, 28 }, { pLegatoGlide, 0 },
            { pAmpA, 2 }, { pAmpD, 500 }, { pAmpS, 0.85f }, { pAmpR, 260 }, { pVelSens, 0.35f },
            { pCutoff, 1100 }, { pReso, 0.28f }, { pKeyTrack, 1.15f }, { pFiltEnv, 1.6f },
            { pFiltA, 1 }, { pFiltD, 320 }, { pFiltS, 0.35f }, { pFiltR, 300 }, { pVelCutoff, 1.3f },
            { pDrive, 0.55f }, { pVelDrive, 0.45f },
            { pVibRate, 5.6f }, { pVibDepth, 0.08f }, { pVibDelay, 380 },
            { pMwVibrato, 0.0f }, { pMwBuild, 1.0f }, { pMwFilter, 0.0f }, { pPbRange, 12 },
            { pDlyTime, 3 }, { pDlyFb, 0.42f }, { pDlyMix, 0.32f }, { pDlyPingPong, 1 },
            { pDlyHP, 380 }, { pDlyLP, 6500 }, { pDlyDuck, 0.55f },
            { pRevSize, 0.8f }, { pRevDamp, 0.4f }, { pRevMix, 0.16f },
            { pWidth, 1.05f }, { pComp, 0.6f }, { pOutput, -5.8f } } },

        // 02 — the duo lead: a loud tempo-synced echo answers every note from the other side
        { "THE RUN", "Gros delay · effet duo sur les runs", {
            { pOscAWave, 0 }, { pOscBWave, 1 }, { pOscBLevel, 0.45f }, { pOscBDetune, 5 },
            { pUnison, 3 }, { pDetune, 9 }, { pSpread, 0.35f }, { pSubLevel, 0.3f },
            { pPoly, 0 }, { pGlide, 18 }, { pLegatoGlide, 0 },
            { pAmpA, 1 }, { pAmpD, 350 }, { pAmpS, 0.75f }, { pAmpR, 180 }, { pVelSens, 0.35f },
            { pCutoff, 1700 }, { pReso, 0.22f }, { pKeyTrack, 0.9f }, { pFiltEnv, 1.8f },
            { pFiltD, 220 }, { pFiltS, 0.4f }, { pVelCutoff, 1.0f },
            { pDrive, 0.35f }, { pVelDrive, 0.3f },
            { pVibDepth, 0.06f }, { pMwVibrato, 0.6f }, { pMwBuild, 0.4f }, { pPbRange, 12 },
            { pDlyTime, 2 }, { pDlyFb, 0.32f }, { pDlyMix, 0.88f }, { pDlyPingPong, 1 },
            { pDlyHP, 220 }, { pDlyLP, 8000 }, { pDlyDuck, 0.0f }, { pDlyDuo, 1.0f },
            { pRevSize, 0.7f }, { pRevMix, 0.12f }, { pWidth, 1.0f }, { pComp, 0.5f }, { pOutput, -5.9f } } },

        // 03 — classic gospel fill lead
        { "GOSPEL SQUARE", "Carré doux · glide · fills", {
            { pOscAWave, 1 }, { pUnison, 2 }, { pDetune, 6 }, { pSpread, 0.4f },
            { pSubLevel, 0.25f }, { pPoly, 0 }, { pGlide, 45 }, { pLegatoGlide, 1 },
            { pCutoff, 1600 }, { pReso, 0.15f }, { pKeyTrack, 0.9f }, { pFiltEnv, 0.8f }, { pVelCutoff, 0.8f },
            { pDrive, 0.15f }, { pVibDepth, 0.1f }, { pMwVibrato, 1.0f }, { pPbRange, 2 },
            { pDlyTime, 2 }, { pDlyFb, 0.25f }, { pDlyMix, 0.15f },
            { pRevMix, 0.18f }, { pComp, 0.35f }, { pOutput, -7.8f } } },

        // 04 — soft sine, talkbox-ish runs
        { "SOFT SINE", "Sinusoïde · gros duo et delay", {
            { pOscAWave, 2 }, { pOscBWave, 3 }, { pOscBLevel, 0.3f }, { pOscBSemi, 12 },
            { pUnison, 1 }, { pPoly, 0 }, { pGlide, 70 }, { pLegatoGlide, 0 },
            { pAmpA, 6 }, { pCutoff, 3500 }, { pKeyTrack, 0.5f }, { pFiltEnv, 0.3f },
            { pDrive, 0.25f }, { pVelDrive, 0.3f }, { pVibDepth, 0.12f }, { pVibRate, 5.0f },
            { pMwVibrato, 1.0f }, { pPbRange, 2 },
            { pDlyTime, 2 }, { pDlyFb, 0.45f }, { pDlyMix, 1.35f }, { pDlyPingPong, 1 }, { pDlyHP, 180 }, { pDlyLP, 9500 }, { pDlyDuck, 0.0f }, { pDlyDuo, 1.0f }, { pRevMix, 0.16f }, { pOutput, -11.2f } } },

        // 05 — airy flute synth
        { "FLUTE SYNTH", "Doux · aérien · effet duo", {
            { pOscAWave, 3 }, { pOscBWave, 2 }, { pOscBLevel, 0.4f }, { pOscBSemi, 12 },
            { pNoise, 0.08f }, { pUnison, 1 }, { pPoly, 0 }, { pGlide, 40 },
            { pAmpA, 45 }, { pAmpR, 500 }, { pCutoff, 2600 }, { pReso, 0.05f }, { pKeyTrack, 0.7f },
            { pFiltEnv, 0.3f }, { pFiltA, 60 }, { pDrive, 0.0f }, { pVelDrive, 0.0f },
            { pVibDepth, 0.1f }, { pVibDelay, 250 }, { pMwVibrato, 1.0f },
            { pDlyTime, 2 }, { pDlyFb, 0.45f }, { pDlyMix, 1.35f }, { pDlyPingPong, 1 }, { pDlyHP, 180 }, { pDlyLP, 9500 }, { pDlyDuck, 0.0f }, { pDlyDuo, 1.0f }, { pRevSize, 0.85f }, { pRevMix, 0.18f }, { pOutput, -10.0f } } },

        // 06 — whistle
        { "WHISTLE", "Sifflé · aigu", {
            { pOscAWave, 2 }, { pNoise, 0.05f }, { pUnison, 1 }, { pPoly, 0 }, { pGlide, 90 },
            { pLegatoGlide, 0 }, { pAmpA, 20 }, { pAmpR, 400 },
            { pCutoff, 5000 }, { pKeyTrack, 0.3f }, { pFiltEnv, 0.0f }, { pDrive, 0.0f }, { pVelDrive, 0.0f },
            { pVibDepth, 0.2f }, { pVibRate, 6.2f }, { pVibDelay, 200 }, { pMwVibrato, 1.0f },
            { pDlyTime, 3 }, { pDlyFb, 0.3f }, { pDlyMix, 0.2f }, { pRevMix, 0.28f }, { pOutput, -9.0f } } },

        // 07 — wide supersaw
        { "SUPERSAW", "Large · stéréo", {
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 28 }, { pSpread, 1.0f },
            { pPoly, 1 }, { pGlide, 0 }, { pAmpA, 4 }, { pAmpR, 450 },
            { pCutoff, 4000 }, { pReso, 0.1f }, { pKeyTrack, 0.5f }, { pFiltEnv, 0.6f },
            { pDrive, 0.15f }, { pMwBuild, 0.7f }, { pMwVibrato, 0.3f },
            { pDlyTime, 5 }, { pDlyFb, 0.3f }, { pDlyMix, 0.18f }, { pRevMix, 0.25f },
            { pWidth, 1.4f }, { pComp, 0.45f }, { pOutput, -5.3f } } },

        // 08 — synth brass stabs
        { "BRASS LEAD", "Cuivré · stabs", {
            { pOscAWave, 0 }, { pOscBWave, 0 }, { pOscBLevel, 0.7f }, { pOscBDetune, 9 },
            { pUnison, 2 }, { pDetune, 7 }, { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 12 }, { pAmpD, 600 }, { pAmpS, 0.7f }, { pAmpR, 180 },
            { pCutoff, 600 }, { pReso, 0.12f }, { pKeyTrack, 0.7f }, { pFiltEnv, 3.0f },
            { pFiltA, 40 }, { pFiltD, 380 }, { pFiltS, 0.35f }, { pVelCutoff, 1.5f },
            { pDrive, 0.3f }, { pMwFilter, 0.4f }, { pMwVibrato, 0.5f },
            { pDlyMix, 0.1f }, { pRevMix, 0.16f }, { pComp, 0.5f }, { pOutput, -3.7f } } },

        // 09 — short afro pluck
        { "AFRO PLUCK", "Court · rythmique", {
            { pOscAWave, 1 }, { pOscBWave, 0 }, { pOscBLevel, 0.4f }, { pOscBSemi, 12 },
            { pUnison, 2 }, { pDetune, 5 }, { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 1 }, { pAmpD, 260 }, { pAmpS, 0.0f }, { pAmpR, 200 },
            { pCutoff, 900 }, { pReso, 0.3f }, { pKeyTrack, 0.8f }, { pFiltEnv, 3.2f },
            { pFiltD, 160 }, { pFiltS, 0.0f }, { pDrive, 0.2f },
            { pDlyTime, 3 }, { pDlyFb, 0.35f }, { pDlyMix, 0.25f }, { pRevMix, 0.18f }, { pComp, 0.4f }, { pOutput, -6.2f } } },

        // 10 — eighties vintage lead
        { "VINTAGE 80", "Rétro années 80", {
            { pOscAWave, 1 }, { pOscBWave, 1 }, { pOscBLevel, 0.6f }, { pOscBDetune, 12 },
            { pUnison, 1 }, { pPoly, 0 }, { pGlide, 50 }, { pLegatoGlide, 1 },
            { pCutoff, 1500 }, { pReso, 0.35f }, { pKeyTrack, 0.7f }, { pFiltEnv, 1.5f },
            { pDrive, 0.2f }, { pVibDepth, 0.08f }, { pMwVibrato, 1.0f }, { pPbRange, 2 },
            { pDlyTime, 5 }, { pDlyFb, 0.4f }, { pDlyMix, 0.25f }, { pDlyPingPong, 0 },
            { pRevMix, 0.22f }, { pWidth, 0.9f }, { pOutput, -10.3f } } },

        // 11 — Elle Glide: EP on top, a low Rhodes underneath, and a low drone shimmer pad
        //      that only swells while you keep your hands on the keys (Glide knob = chord glide)
        { "ELLE GLIDE", "EP · Rhodes doux · pad drone shimmer", {
            { pEngine, 1 }, { pEpModel, 1 }, { pEpLevel, 1.0f }, { pEpBark, 0.3f }, { pEpDecay, 5.0f },
            { pRhodesMix, 0.35f }, { pEpTrem, 0.06f }, { pMwTrem, 0.6f },
            { pPadLevel, 0.17f }, { pShimmer, 0.5f }, { pScoop, -1.5f },
            { pOscAWave, 0 }, { pUnison, 5 }, { pDetune, 9 }, { pSpread, 0.95f },
            { pOscBWave, 2 }, { pOscBLevel, 0.6f }, { pOscBSemi, -12 },
            { pSubLevel, 0.0f }, { pNoise, 0.0f },
            { pPoly, 1 }, { pGlide, 180 },
            { pAmpA, 2600 }, { pAmpD, 2000 }, { pAmpS, 1.0f }, { pAmpR, 220 }, { pVelSens, 0.45f },
            { pCutoff, 520 }, { pReso, 0.05f }, { pKeyTrack, 0.4f }, { pFiltEnv, 0.0f }, { pVelCutoff, 0.0f },
            { pDrive, 0.08f }, { pVelDrive, 0.0f },
            { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pPbRange, 2 },
            { pDlyTime, 5 }, { pDlyFb, 0.18f }, { pDlyMix, 0.06f }, { pDlyDuck, 0.5f },
            { pRevSize, 0.88f }, { pRevDamp, 0.55f }, { pRevMix, 0.26f }, { pWidth, 1.2f }, { pComp, 0.4f }, { pOutput, -7.1f } } },

        // 12 — The Glide: a soft sine glide pad (in the spirit of a "sine one" glide pad):
        //      round, warm, no bright edge; chords glide into each other (Glide knob = chord glide)
        { "THE GLIDE", "Glide pad sinus lourd · doux et rond", {
            { pEngine, 0 }, { pShimmer, 0.0f }, { pPadLevel, 1.0f }, { pScoop, -2.0f },
            { pOscAWave, 2 }, { pUnison, 3 }, { pDetune, 6 }, { pSpread, 0.85f },
            { pOscBWave, 2 }, { pOscBLevel, 0.55f }, { pOscBSemi, -12 }, { pOscBDetune, 3 },
            { pSubLevel, 0.25f }, { pSubOct, 0 }, { pNoise, 0.0f },
            { pPoly, 1 }, { pGlide, 900 },
            { pAmpA, 120 }, { pAmpD, 2000 }, { pAmpS, 1.0f }, { pAmpR, 900 }, { pVelSens, 0.3f },
            { pCutoff, 1150 }, { pReso, 0.0f }, { pKeyTrack, 0.3f }, { pFiltEnv, 0.0f }, { pVelCutoff, 0.0f },
            { pDrive, 0.2f }, { pVelDrive, 0.0f },
            { pVibRate, 4.8f }, { pVibDepth, 0.0f }, { pMwVibrato, 0.3f }, { pMwBuild, 0.0f }, { pPbRange, 2 },
            { pDlyTime, 5 }, { pDlyFb, 0.2f }, { pDlyMix, 0.05f }, { pDlyDuck, 0.5f },
            { pRevSize, 0.85f }, { pRevDamp, 0.65f }, { pRevMix, 0.3f }, { pWidth, 1.3f }, { pComp, 0.5f }, { pOutput, -3.3f } } },
    };
    return presets;
}

} // namespace dsynth
