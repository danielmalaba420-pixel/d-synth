# D-SYNTH by Danova — version 0.1

Plugin de 12 sons synthé pour Mac : Audio Unit (Logic Pro, Mainstage, GarageBand), VST3 (Ableton, Studio One, FL Studio, Reaper…) et une version autonome qui marche sans DAW.

## Installer sur ton Mac (une seule fois)

1. **Outils Apple** : ouvre le Terminal et tape `xcode-select --install`, puis accepte.
2. **CMake** : `brew install cmake` (si tu n'as pas Homebrew, installe-le d'abord depuis https://brew.sh).
3. **Compiler et installer D-SYNTH** : dézippe le dossier dans Téléchargements, puis dans le Terminal :
   ```
   cd ~/Downloads/D-SYNTH
   chmod +x build_mac.sh
   ./build_mac.sh
   ```
   La première fois, ça prend 5 à 10 minutes (le script télécharge JUCE puis compile). À la fin, D-SYNTH est copié dans tes dossiers de plugins et vérifié pour Logic.
4. **Dans Logic Pro** : nouvelle piste instrument › AU Instruments › Danova › D-SYNTH.

Si Logic ne le voit pas tout de suite : ferme Logic, rouvre-le, et regarde dans Logic Pro › Réglages › Plug-in Manager.

## Les 12 sons

1. Rising Run — lead lourd pour les runs, la molette pousse la macro Build
2. The Run — gros delay, effet duo : une réponse à droite après chaque note
3. Gospel Square
4. Worship Dotted
5. Soft Sine
6. Flute Synth
7. Whistle
8. Supersaw
9. Brass Lead
10. Afro Pluck
11. Vintage 80
12. The Glide — EP lourd avec pad shimmer, les accords glissent l'un dans l'autre

## Jouer

- **Pitch bend** : ±12 demi-tons sur Rising Run et The Run, ±2 sur la plupart des autres.
- **Molette** : Build sur Rising Run, tremolo sur The Glide, vibrato sur les autres. L'écran affiche ce qu'elle fait.
- **Pédale de sustain** : sur The Glide, elle fait résonner l'EP, mais le pad s'arrête quand tu lèves les doigts.
- **CHORD GLIDE** : sur The Glide, le premier potard règle la vitesse du glissement entre les accords (0 à 1 seconde).
- **Delay** synchronisé au tempo du DAW.
- La fenêtre se redimensionne en tirant le coin en bas à droite.

## Pour plus tard (vente)

- Signature et notarisation Apple (compte Apple Developer) pour que tes clients installent sans avertissement.
- Installateur .pkg, version Windows, MIDI Learn.
- Licence JUCE : la licence gratuite « Starter » couvre jusqu'à 20 000 $ de revenus par an.

## Dossiers

- `Source/Engine/` : le moteur sonore et les 12 sons
- `Source/` : le plugin et l'interface
- `Assets/` : texture bois et polices (licence SIL Open Font, utilisables dans un produit commercial)
- `tools/render_demo.cpp` : génère les extraits audio de démonstration
