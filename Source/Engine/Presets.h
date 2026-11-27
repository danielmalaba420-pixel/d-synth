// D-SYNTH by Danova — the 20 factory sounds (v2)
#pragma once

#include "Params.h"

#include <initializer_list>
#include <utility>
#include <vector>

namespace dsynth
{

// categories shown on the front panel
static constexpr const char* kCategories[] = { "LEAD", "ARP", "NAPPE", "CLAVIER", "CUIVRE", "BASSE", "FX" };

struct Preset
{
    const char* name;
    const char* description;
    const char* category;
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

        // 01
        { "RISING RUN", "Lead lourd · runs · delay · montée en puissance", "LEAD", {
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

        // 02
        { "THE RUN", "Gros delay · effet duo sur les runs", "LEAD", {
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

        // 03
        { "FLUTE SYNTH", "Doux · aérien · effet duo", "LEAD", {
            { pOscAWave, 3 }, { pOscBWave, 2 }, { pOscBLevel, 0.4f }, { pOscBSemi, 12 },
            { pNoise, 0.08f }, { pUnison, 1 }, { pPoly, 0 }, { pGlide, 40 },
            { pAmpA, 45 }, { pAmpR, 500 }, { pCutoff, 2600 }, { pReso, 0.05f }, { pKeyTrack, 0.7f },
            { pFiltEnv, 0.3f }, { pFiltA, 60 }, { pDrive, 0.0f }, { pVelDrive, 0.0f },
            { pVibDepth, 0.1f }, { pVibDelay, 250 }, { pMwVibrato, 1.0f },
            { pDlyTime, 2 }, { pDlyFb, 0.45f }, { pDlyMix, 1.35f }, { pDlyPingPong, 1 }, { pDlyHP, 180 }, { pDlyLP, 9500 }, { pDlyDuck, 0.0f }, { pDlyDuo, 1.0f }, { pRevSize, 0.85f }, { pRevMix, 0.18f }, { pOutput, -10.0f } } },

        // 04
        { "SUPERSAW", "Large · stéréo", "LEAD", {
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 28 }, { pSpread, 1.0f },
            { pPoly, 1 }, { pGlide, 0 }, { pAmpA, 4 }, { pAmpR, 450 },
            { pCutoff, 4000 }, { pReso, 0.1f }, { pKeyTrack, 0.5f }, { pFiltEnv, 0.6f },
            { pDrive, 0.15f }, { pMwBuild, 0.7f }, { pMwVibrato, 0.3f },
            { pDlyTime, 5 }, { pDlyFb, 0.3f }, { pDlyMix, 0.18f }, { pRevMix, 0.25f },
            { pWidth, 1.4f }, { pComp, 0.45f }, { pOutput, -5.3f } } },

        // 05
        { "AFRO PLUCK", "Court · rythmique", "LEAD", {
            { pOscAWave, 1 }, { pOscBWave, 0 }, { pOscBLevel, 0.4f }, { pOscBSemi, 12 },
            { pUnison, 2 }, { pDetune, 5 }, { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 1 }, { pAmpD, 260 }, { pAmpS, 0.0f }, { pAmpR, 200 },
            { pCutoff, 900 }, { pReso, 0.3f }, { pKeyTrack, 0.8f }, { pFiltEnv, 3.2f },
            { pFiltD, 160 }, { pFiltS, 0.0f }, { pDrive, 0.2f },
            { pDlyTime, 3 }, { pDlyFb, 0.35f }, { pDlyMix, 0.25f }, { pRevMix, 0.18f }, { pComp, 0.4f }, { pOutput, -6.2f }, { pMwFilter, 0.7f }, { pMwDelay, 0.25f } } },

        // 06
        { "SIGNLY", "Lead sinus · delays qui s'empilent", "LEAD", {
            { pOscAWave, 2 }, { pOscBWave, 2 }, { pOscBLevel, 0.12f }, { pOscBSemi, 12 }, { pUnison, 1 }, { pSubLevel, 0.0f }, { pNoise, 0.0f },
            { pPoly, 0 }, { pGlide, 35 }, { pLegatoGlide, 1 },
            { pAmpA, 8 }, { pAmpD, 600 }, { pAmpS, 0.9f }, { pAmpR, 260 }, { pVelSens, 0.3f },
            { pCutoff, 4200 }, { pReso, 0.0f }, { pKeyTrack, 0.4f }, { pFiltEnv, 0.3f }, { pVelCutoff, 0.2f },
            { pDrive, 0.08f }, { pVelDrive, 0.1f },
            { pVibRate, 5.4f }, { pVibDepth, 0.12f }, { pVibDelay, 320 }, { pMwVibrato, 0.0f }, { pMwDelay, 1.0f }, { pPbRange, 2 },
            { pDlyTime, 3 }, { pDlyFb, 0.55f }, { pDlyMix, 0.55f }, { pDlyPingPong, 1 }, { pDlyHP, 250 }, { pDlyLP, 4200 }, { pDlyDuck, 0.0f }, { pDlyBloom, 1.0f },
            { pRevSize, 0.9f }, { pRevDamp, 0.55f }, { pRevMix, 0.2f }, { pWidth, 1.1f }, { pComp, 0.35f }, { pOutput, -11.4f } } },

        // 07
        { "ARP PULSE", "Arpège rapide qui monte et descend · nappe", "ARP", {
            { pArp, 1 }, { pArpRate, 2 }, { pArpMode, 2 }, { pArpOct, 1 }, { pArpGate, 0.45f }, { pArpPad, 0.3f }, { pArpPan, 1.0f }, { pArpDust, 0.7f }, { pChorus, 0.25f },
            { pOscAWave, 0 }, { pOscBWave, 1 }, { pOscBLevel, 0.3f }, { pOscBSemi, 12 }, { pUnison, 3 }, { pDetune, 8 }, { pSpread, 0.6f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 1 }, { pAmpD, 220 }, { pAmpS, 0.0f }, { pAmpR, 180 }, { pVelSens, 0.4f },
            { pCutoff, 1100 }, { pReso, 0.22f }, { pKeyTrack, 0.8f }, { pFiltEnv, 2.8f }, { pFiltD, 160 }, { pFiltS, 0.0f }, { pVelCutoff, 0.8f },
            { pDrive, 0.1f }, { pVelDrive, 0.1f }, { pMwVibrato, 0.0f }, { pMwFilter, 0.6f }, { pVibDepth, 0.0f },
            { pDlyTime, 2 }, { pDlyFb, 0.3f }, { pDlyMix, 0.22f }, { pDlyPingPong, 1 }, { pDlyDuck, 0.5f },
            { pRevHall, 1 }, { pRevSize, 0.85f }, { pRevMix, 0.28f }, { pWidth, 1.2f }, { pComp, 0.4f }, { pOutput, 0.0f } } },

        // 08
        { "ARP CRYSTAL", "Glockenspiel en arpège · nappe derrière", "ARP", {
            { pArp, 1 }, { pArpRate, 2 }, { pArpMode, 2 }, { pArpOct, 1 }, { pArpGate, 0.45f }, { pArpPan, 1.0f }, { pArpDust, 0.7f },
            { pSmpSet, 5 }, { pSmpLevel, 1.0f }, { pPadLevel, 0.0f }, { pArpPad, 0.24f },
            { pOscAWave, 0 }, { pUnison, 5 }, { pDetune, 10 }, { pSpread, 0.9f }, { pOscBWave, 3 }, { pOscBLevel, 0.5f }, { pOscBSemi, -12 }, { pSubLevel, 0.0f },
            { pCutoff, 750 }, { pReso, 0.0f }, { pKeyTrack, 0.3f }, { pFiltEnv, 0.0f }, { pVelCutoff, 0.0f }, { pDrive, 0.0f }, { pVelDrive, 0.0f }, { pChorus, 0.3f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 0.5f }, { pAmpD, 400 }, { pAmpS, 1.0f }, { pAmpR, 1500 }, { pVelSens, 0.3f },
            { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pMwBuild, 0.0f }, { pMwFilter, 0.6f }, { pMwDelay, 0.6f },
            { pDlyTime, 2 }, { pDlyFb, 0.3f }, { pDlyMix, 0.25f }, { pDlyPingPong, 1 }, { pDlyHP, 600 }, { pDlyLP, 11000 }, { pDlyDuck, 0.5f },
            { pRevHall, 1 }, { pRevSize, 0.8f }, { pRevDamp, 0.25f }, { pRevMix, 0.3f }, { pWidth, 1.2f }, { pComp, 0.3f }, { pOutput, -7.9f } } },

        // 09
        { "ARP DRIVE", "Arpège en doubles croches · saw lourd · monte avec BUILD", "ARP", {
            { pArp, 1 }, { pArpRate, 2 }, { pArpMode, 2 }, { pArpOct, 1 }, { pArpGate, 0.5f }, { pArpPad, 0.3f }, { pArpPan, 1.0f }, { pArpDust, 0.6f },
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 18 }, { pSpread, 0.9f },
            { pOscBWave, 1 }, { pOscBLevel, 0.4f }, { pOscBSemi, -12 }, { pSubLevel, 0.4f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 1 }, { pAmpD, 180 }, { pAmpS, 0.35f }, { pAmpR, 120 }, { pVelSens, 0.45f },
            { pCutoff, 1400 }, { pReso, 0.2f }, { pKeyTrack, 0.6f }, { pFiltEnv, 1.8f }, { pFiltD, 140 }, { pFiltS, 0.3f },
            { pDrive, 0.45f }, { pVelDrive, 0.3f }, { pMwBuild, 1.0f }, { pMwVibrato, 0.0f }, { pVibDepth, 0.0f },
            { pDlyTime, 2 }, { pDlyFb, 0.25f }, { pDlyMix, 0.15f }, { pDlyDuck, 0.5f },
            { pRevHall, 1 }, { pRevSize, 0.8f }, { pRevMix, 0.2f }, { pWidth, 1.2f }, { pComp, 0.55f }, { pOutput, -0.1f } } },

        // 10
        { "STRING GLIDE", "Cordes + pad dark lourd · glide d'accords", "NAPPE", {
            { pEngine, 0 }, { pPoly, 1 }, { pGlide, 380 }, { pScoop, -2.0f },
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 14 }, { pSpread, 0.9f },
            { pOscBWave, 0 }, { pOscBLevel, 0.6f }, { pOscBSemi, -12 }, { pOscBDetune, -4 },
            { pSubLevel, 0.35f }, { pSubOct, 0 }, { pNoise, 0.02f },
            { pAmpA, 450 }, { pAmpD, 2000 }, { pAmpS, 1.0f }, { pAmpR, 900 }, { pVelSens, 0.35f },
            { pCutoff, 950 }, { pReso, 0.1f }, { pKeyTrack, 0.5f }, { pFiltEnv, 0.6f },
            { pFiltA, 800 }, { pFiltD, 1500 }, { pFiltS, 0.6f }, { pFiltR, 900 }, { pVelCutoff, 0.4f },
            { pDrive, 0.12f }, { pVelDrive, 0.0f },
            { pVibRate, 5.2f }, { pVibDepth, 0.04f }, { pVibDelay, 600 }, { pMwVibrato, 0.4f }, { pMwBuild, 0.0f }, { pPbRange, 2 },
            { pDlyTime, 5 }, { pDlyFb, 0.2f }, { pDlyMix, 0.05f }, { pDlyDuck, 0.5f },
            { pRevSize, 0.88f }, { pRevDamp, 0.55f }, { pRevMix, 0.32f }, { pWidth, 1.3f }, { pComp, 0.45f }, { pOutput, -3.9f }, { pMwFilter, 0.6f } } },

        // 11
        { "STRING DINSEI", "Cordes Disney · legato et glissandos", "NAPPE", {
            { pSmpSet, 6 }, { pSmpLevel, 1.0f }, { pPadLevel, 0.0f }, { pPoly, 1 }, { pGlide, 100 }, { pScoop, 0.0f },
            { pAmpA, 80 }, { pAmpD, 2000 }, { pAmpS, 1.0f }, { pAmpR, 550 }, { pVelSens, 0.55f },
            { pVibRate, 5.6f }, { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pMwFilter, 0.6f }, { pMwBuild, 0.0f }, { pPbRange, 2 },
            { pChorus, 0.0f }, { pDlyMix, 0.0f }, { pWidth, 1.25f },
            { pRevHall, 1 }, { pRevPre, 30 }, { pRevSize, 0.92f }, { pRevDamp, 0.45f }, { pRevMix, 0.34f }, { pComp, 0.25f }, { pOutput, 0.5f } } },

        // 12
        { "THE PRAYER", "Pad glorieux · W-A by Daniel Malaba · atmosphère de prière", "NAPPE", {
            { pSmpSet, 7 }, { pSmpLevel, 1.0f }, { pPadLevel, 0.25f }, { pMwPad, 1.0f },
            { pOscAWave, 0 }, { pUnison, 5 }, { pDetune, 11 }, { pSpread, 0.9f },
            { pOscBWave, 3 }, { pOscBLevel, 0.25f }, { pOscBSemi, 12 }, { pOscBDetune, -5 }, { pSubLevel, 0.15f }, { pSubOct, 0 }, { pNoise, 0.0f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 700 }, { pAmpD, 2500 }, { pAmpS, 1.0f }, { pAmpR, 2200 }, { pVelSens, 0.25f },
            { pCutoff, 1600 }, { pReso, 0.0f }, { pKeyTrack, 0.3f }, { pFiltEnv, 0.4f }, { pFiltA, 900 }, { pFiltD, 3000 }, { pFiltS, 0.6f }, { pFiltR, 2200 }, { pVelCutoff, 0.2f },
            { pLfoRate, 0.08f }, { pLfoCut, 0.08f }, { pDrive, 0.0f }, { pVelDrive, 0.0f }, { pShimmer, 0.0f }, { pChorus, 0.35f },
            { pMwFilter, 0.7f }, { pMwVibrato, 0.0f }, { pVibDepth, 0.0f },
            { pDlyMix, 0.0f },
            { pRevHall, 1 }, { pRevPre, 25 }, { pRevSize, 0.95f }, { pRevDamp, 0.55f }, { pRevMix, 0.38f }, { pWidth, 1.1f }, { pComp, 0.3f }, { pOutput, -3.4f } } },

        // 13
        { "HOLY G", "Orgue enregistré · Leslie à la molette", "CLAVIER", {
            { pSmpSet, 4 }, { pSmpLevel, 1.0f }, { pOrgLevel, 0.0f }, { pPadLevel, 0.0f },
            { pOrgReg, 0 }, { pOrgPerc, 0.3f }, { pOrgClick, 0.15f }, { pOrgVib, 3 }, { pOrgDrive, 0.1f }, { pLeslie, 1.0f },
            { pPoly, 1 }, { pGlide, 0 }, { pAmpA, 1.5f }, { pAmpD, 100 }, { pAmpS, 1.0f }, { pAmpR, 20 }, { pVelSens, 0.0f },
            { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pMwBuild, 0.0f }, { pMwFilter, 0.0f }, { pPbRange, 2 },
            { pDlyMix, 0.0f }, { pRevHall, 1 }, { pRevSize, 0.7f }, { pRevDamp, 0.55f }, { pRevPre, 18 }, { pRevMix, 0.22f }, { pWidth, 1.05f }, { pComp, 0.3f }, { pOutput, 1.1f } } },

        // 14
        { "FIRE KEYS", "Piano + super saw + orgue · louanges rapides", "CLAVIER", {
            { pSmpSet, 3 }, { pSmpLevel, 1.0f }, { pPadLevel, 0.22f }, { pMwPad, 0.5f },
            { pOrgLevel, 0.28f }, { pOrgReg, 0 }, { pOrgPerc, 0.0f }, { pOrgClick, 0.2f }, { pOrgVib, 3 }, { pOrgDrive, 0.15f }, { pLeslie, 0.8f },
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 16 }, { pSpread, 0.9f }, { pOscBLevel, 0.0f }, { pSubLevel, 0.0f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 2 }, { pAmpD, 1500 }, { pAmpS, 1.0f }, { pAmpR, 220 }, { pVelSens, 0.6f },
            { pCutoff, 2600 }, { pReso, 0.05f }, { pKeyTrack, 0.6f }, { pFiltEnv, 0.8f }, { pFiltD, 400 }, { pFiltS, 0.6f }, { pDrive, 0.1f },
            { pMwFilter, 0.0f }, { pMwBuild, 0.0f }, { pMwVibrato, 0.0f }, { pVibDepth, 0.0f }, { pPbRange, 2 },
            { pDlyTime, 2 }, { pDlyFb, 0.2f }, { pDlyMix, 0.07f }, { pDlyDuck, 0.6f },
            { pRevSize, 0.7f }, { pRevMix, 0.14f }, { pWidth, 1.15f }, { pComp, 0.45f }, { pOutput, -4.6f } } },

        // 15
        { "GLORY BRASS", "Section de cuivres · stabs et notes tenues", "CUIVRE", {
            { pSmpSet, 2 }, { pSmpLevel, 1.0f }, { pSmpStac, 0.9f }, { pPadLevel, 0.08f },
            { pOscAWave, 0 }, { pUnison, 3 }, { pDetune, 8 }, { pSpread, 0.6f }, { pOscBLevel, 0.0f }, { pSubLevel, 0.0f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 3 }, { pAmpD, 800 }, { pAmpS, 1.0f }, { pAmpR, 180 }, { pVelSens, 0.55f },
            { pCutoff, 1500 }, { pReso, 0.05f }, { pKeyTrack, 0.7f }, { pFiltEnv, 2.0f }, { pFiltA, 30 }, { pFiltD, 400 }, { pFiltS, 0.4f }, { pDrive, 0.1f },
            { pMwFilter, 1.0f }, { pMwVibrato, 0.0f }, { pVibDepth, 0.0f }, { pPbRange, 2 },
            { pDlyMix, 0.0f }, { pRevSize, 0.75f }, { pRevMix, 0.18f }, { pWidth, 1.1f }, { pComp, 0.45f }, { pOutput, -5.1f } } },

        // 16
        { "FINGER BASS", "Vraie basse aux doigts · corde et micro", "BASSE", {
            { pEngine, 2 }, { pPoly, 0 }, { pGlide, 0 }, { pLegatoGlide, 1 },
            { pBassTone, 0.55f }, { pBassPluck, 0.24f }, { pBassSustain, 4.0f }, { pBassPickup, 0.55f }, { pBassThump, 0.55f },
            { pAmpA, 0.5f }, { pAmpD, 100 }, { pAmpS, 1.0f }, { pAmpR, 350 }, { pVelSens, 0.5f },
            { pVibDepth, 0.0f }, { pMwVibrato, 0.25f }, { pPbRange, 2 },
            { pDlyMix, 0.0f }, { pRevSize, 0.5f }, { pRevMix, 0.03f },
            { pWidth, 0.0f }, { pComp, 0.6f }, { pOutput, -4.5f } } },

        // 17
        { "SYNTH BASS", "Basse ronde et punchy · style Moog", "BASSE", {
            { pOscAWave, 0 }, { pOscBWave, 1 }, { pOscBLevel, 0.6f }, { pOscBSemi, 0 }, { pOscBDetune, 7 }, { pUnison, 1 }, { pSubLevel, 0.55f }, { pSubOct, 0 },
            { pPoly, 0 }, { pGlide, 45 }, { pLegatoGlide, 1 },
            { pAmpA, 1 }, { pAmpD, 600 }, { pAmpS, 0.75f }, { pAmpR, 90 }, { pVelSens, 0.4f },
            { pCutoff, 230 }, { pReso, 0.38f }, { pKeyTrack, 0.5f }, { pFiltEnv, 3.3f }, { pFiltA, 1 }, { pFiltD, 210 }, { pFiltS, 0.18f }, { pFiltR, 120 }, { pVelCutoff, 1.0f },
            { pDrive, 0.35f }, { pVelDrive, 0.3f }, { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pMwFilter, 0.6f }, { pPbRange, 2 },
            { pDlyMix, 0.0f }, { pRevMix, 0.02f }, { pWidth, 0.0f }, { pComp, 0.6f }, { pOutput, -4.7f } } },

        // 18
        { "SUB BASS", "Sub profonde style 808 · glide", "BASSE", {
            { pOscAWave, 2 }, { pOscBLevel, 0.0f }, { pUnison, 1 }, { pSubLevel, 0.0f }, { pNoise, 0.0f },
            { pPoly, 0 }, { pGlide, 70 }, { pLegatoGlide, 1 },
            { pAmpA, 1 }, { pAmpD, 2500 }, { pAmpS, 0.55f }, { pAmpR, 160 }, { pVelSens, 0.35f },
            { pCutoff, 9000 }, { pReso, 0.0f }, { pKeyTrack, 0.0f }, { pFiltEnv, 0.0f }, { pVelCutoff, 0.0f },
            { pDrive, 0.3f }, { pVelDrive, 0.2f }, { pPitchEnv, 7.0f }, { pPitchEnvTime, 25 },
            { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pMwBuild, 0.6f }, { pPbRange, 12 },
            { pDlyMix, 0.0f }, { pRevMix, 0.0f }, { pWidth, 0.0f }, { pComp, 0.5f }, { pOutput, -6.5f } } },

        // 19
        { "RISER", "Montée synchronisée au tempo", "FX", {
            { pRiser, 1 }, { pRiserBars, 2 }, { pRiserPitch, 12 },
            { pOscAWave, 0 }, { pUnison, 7 }, { pDetune, 22 }, { pSpread, 1.0f }, { pOscBWave, 0 }, { pOscBLevel, 0.35f }, { pOscBSemi, 12 }, { pNoise, 0.1f },
            { pPoly, 1 }, { pGlide, 0 },
            { pAmpA, 30 }, { pAmpD, 1000 }, { pAmpS, 1.0f }, { pAmpR, 400 }, { pVelSens, 0.2f },
            { pCutoff, 350 }, { pReso, 0.3f }, { pKeyTrack, 0.3f }, { pFiltEnv, 0.0f }, { pVelCutoff, 0.0f },
            { pDrive, 0.2f }, { pVelDrive, 0.0f }, { pVibDepth, 0.0f }, { pMwVibrato, 0.0f }, { pMwBuild, 0.0f },
            { pDlyTime, 2 }, { pDlyFb, 0.3f }, { pDlyMix, 0.15f }, { pDlyDuck, 0.0f },
            { pRevSize, 0.9f }, { pRevMix, 0.25f }, { pWidth, 1.4f }, { pComp, 0.5f }, { pOutput, -7.7f } } },

        // 20
        { "FX KIT", "20 effets · un par touche (Do3 à Sol4)", "FX", {
            { pEngine, 3 }, { pPoly, 1 }, { pGlide, 0 }, { pVelSens, 0.0f }, { pVibDepth, 0.0f }, { pMwVibrato, 0.0f },
            { pDlyMix, 0.0f }, { pRevSize, 0.88f }, { pRevDamp, 0.45f }, { pRevMix, 0.25f }, { pWidth, 1.1f }, { pComp, 0.35f }, { pOutput, -4.6f } } },
    };
    return presets;
}

} // namespace dsynth
