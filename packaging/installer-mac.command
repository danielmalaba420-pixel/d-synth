#!/bin/bash
# Installe D-SYNTH (AU pour Logic Pro, VST3 pour les autres logiciels) et débloque les fichiers téléchargés.
cd "$(dirname "$0")"
echo ""
echo "  D-SYNTH — by Danova"
echo "  Installation des plugins..."
echo ""
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Library/Audio/Plug-Ins/VST3"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/D-SYNTH.component" "$HOME/Library/Audio/Plug-Ins/VST3/D-SYNTH.vst3"
cp -R "D-SYNTH.component" "$HOME/Library/Audio/Plug-Ins/Components/"
cp -R "D-SYNTH.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/D-SYNTH.component" 2>/dev/null
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/D-SYNTH.vst3" 2>/dev/null
if [ -d "D-SYNTH.app" ]; then
  rm -rf "/Applications/D-SYNTH.app"
  cp -R "D-SYNTH.app" "/Applications/" 2>/dev/null && xattr -dr com.apple.quarantine "/Applications/D-SYNTH.app" 2>/dev/null
fi
killall -9 AudioComponentRegistrar 2>/dev/null
echo "  Terminé ! Relancez Logic Pro (ou votre logiciel) : D-SYNTH est dans"
echo "  Instruments logiciels > AU Instruments > Danova > D-SYNTH"
echo ""
read -n 1 -s -r -p "  Appuyez sur une touche pour fermer..."
