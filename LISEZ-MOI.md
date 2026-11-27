# D-SYNTH by Danova — code source

Plugin de 20 sons pour claviéristes auxiliaires (Audio Unit, VST3 et application autonome)## Les 20 sons (version 2.0.0)

LEAD : 1. Rising Run · 2. The Run · 3. Flute Synth · 4. Supersaw · 5. Afro Pluck · 6. Signly
ARP : 7. Arp Pulse · 8. Arp Crystal · 9. Arp Drive
NAPPE : 10. String Glide · 11. String Dinsei · 12. The Prayer
CLAVIER : 13. Holy G · 14. Fire Keys · CUIVRE : 15. Glory Brass
BASSE : 16. Finger Bass · 17. Synth Bass · 18. Sub Bass · FX : 19. Riser · 20. FX Kit

lide · 12. Glide Sine

## Où est quoi

| Dossier / fichier | Contenu |
|---|---|
| `Source/Engine/Presets.h` | **Les 12 sons** : chaque son est une liste de réglages. C'est ici qu'on modifie ou remplace un son. |
| `Source/Engine/Params.h` | La liste de tous les réglages possibles (filtre, glide, delay, basse…). |
| `Source/Engine/DSynthEngine.h` | Le moteur sonore : oscillateurs, EP/Rhodes, modèle de corde pour la basse, glide d'accords, delay, reverb. |
| `Source/PluginEditor.cpp` | L'interface : la façade vient de la maquette, le code dessine ce qui bouge. |
| `Source/PluginProcessor.cpp` | Le lien avec Logic et les autres logiciels (MIDI, pitch bend, molette, pédale). |
| `Assets/` | Polices (licence SIL Open Font) et l'image de la façade (`skin_bg.png`, `skin_on.png`). |
| `tools/skin/` | Fabrique la façade à partir de la maquette validée (`python3 tools/skin/make_skin.py`). |
| `Samples/` | Les sons enregistrés en FLAC (sans perte) ; les installateurs les copient dans Application Support (Mac) ou ProgramData (Windows). |
| `packaging/mac/` | Fabrication de l'installateur Mac (.pkg). |
| `packaging/windows/` | Fabrication de l'installateur Windows (Setup.exe, Inno Setup). |
| `tools/render_demo.cpp` | Génère les démos audio sans ouvrir de logiciel. |
| `.github/workflows/build.yml` | La fabrication automatique par GitHub. |

## Faire une mise à jour

1. Modifier le code (par exemple un son dans `Presets.h`).
2. Changer le numéro de version dans `.github/workflows/build.yml` (ligne `VERSION:`) et dans `CMakeLists.txt` (`project(DSYNTH VERSION …)`).
3. Envoyer sur GitHub (branche `main`). GitHub compile alors D-SYNTH pour Mac (Apple Silicon + Intel) et Windows, fabrique les deux installateurs, les installe pour les tester, puis les dépose sur la branche **`builds`** du dépôt.
4. Pour une version officielle, créer une étiquette `v1.0.1` (par exemple) : GitHub publie aussi une « Release » avec les deux installateurs.

## Supprimer les avertissements à l'installation (signature)

La fabrication est prête à signer automatiquement. Il suffit d'ajouter ces secrets dans GitHub (Settings › Secrets and variables › Actions) :

- Mac (compte Apple Developer) : `MAC_CERTS_P12` (certificats Developer ID Application + Installer exportés en .p12, encodés en base64), `MAC_CERTS_PASSWORD`, `MAC_APP_SIGN_ID`, `MAC_INSTALLER_SIGN_ID`, `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD` (mot de passe d'application).
- Windows : un certificat de signature de code (à brancher dans l'étape « Build the installer »).

## Licence JUCE

D-SYNTH utilise JUCE 8. La licence gratuite « Starter » couvre jusqu'à 20 000 $ de revenus par an ; au-delà, passer à la licence Indie.
