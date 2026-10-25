#!/bin/bash
# Builds the D-SYNTH Mac installer (.pkg): Audio Unit + VST3 + standalone app.
# Optional signing/notarization when the Apple Developer secrets are provided as environment variables:
#   APP_SIGN_ID       "Developer ID Application: Name (TEAMID)"
#   INSTALLER_SIGN_ID "Developer ID Installer: Name (TEAMID)"
#   NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD (app-specific password)
set -euo pipefail
set -x
VERSION="$1"; ART="$2"; OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
mkdir -p "$OUT"

stage () { mkdir -p "$WORK/$1"; cp -R "$2" "$WORK/$1/"; }
stage au   "$ART/AU/D-SYNTH.component"
stage vst3 "$ART/VST3/D-SYNTH.vst3"
stage app  "$ART/Standalone/D-SYNTH.app"

# Code signing: Developer ID when available, otherwise ad-hoc (works on the buyer's Mac but shows a Gatekeeper warning)
for b in "$WORK"/au/*.component "$WORK"/vst3/*.vst3 "$WORK"/app/*.app; do
  if [ -n "${APP_SIGN_ID:-}" ]; then
    codesign --force --deep --timestamp --options runtime --sign "$APP_SIGN_ID" "$b"
  else
    codesign --force --deep --sign - "$b"
  fi
done

# One component package per bundle, never relocated to an older copy elsewhere on disk
component_pkg () { # name source-dir install-location bundle-name
  local plist="$WORK/$1.plist"
  cat > "$plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><array><dict>
  <key>BundleHasStrictIdentifier</key><true/>
  <key>BundleIsRelocatable</key><false/>
  <key>BundleIsVersionChecked</key><false/>
  <key>BundleOverwriteAction</key><string>upgrade</string>
  <key>RootRelativeBundlePath</key><string>$4</string>
</dict></array></plist>
PLIST
  pkgbuild --root "$WORK/$2" --component-plist "$plist" --identifier "com.danova.dsynth.$1" \
           --version "$VERSION" --install-location "$3" "$WORK/$1.pkg"
}
component_pkg au   au   "/Library/Audio/Plug-Ins/Components" "D-SYNTH.component"
component_pkg vst3 vst3 "/Library/Audio/Plug-Ins/VST3" "D-SYNTH.vst3"
component_pkg app  app  "/Applications" "D-SYNTH.app"

sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
PKG="$OUT/D-SYNTH-$VERSION-Mac.pkg"
if [ -n "${INSTALLER_SIGN_ID:-}" ]; then
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" --resources "$HERE/resources" --sign "$INSTALLER_SIGN_ID" "$PKG"
else
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" --resources "$HERE/resources" "$PKG"
fi

if [ -n "${NOTARY_APPLE_ID:-}" ] && [ -n "${INSTALLER_SIGN_ID:-}" ]; then
  xcrun notarytool submit "$PKG" --apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_PASSWORD" --wait
  xcrun stapler staple "$PKG"
fi
echo "Built $PKG"
