; D-SYNTH by Danova — Windows installer (Inno Setup 6)
; Built by GitHub Actions:  ISCC /DAppVersion=2.0.0 /DArtDir=<artefacts> /DSmpDir=<samples> /DOutDir=<dist> D-SYNTH.iss

#ifndef AppVersion
  #define AppVersion "2.0.0"
#endif

[Setup]
AppId={{678FB1F8-CD0A-5A03-8709-57ADC16FDAB1}
AppName=D-SYNTH
AppVersion={#AppVersion}
AppVerName=D-SYNTH {#AppVersion}
AppPublisher=Danova
AppPublisherURL=https://danova.cd
DefaultDirName={autopf}\Danova\D-SYNTH
DefaultGroupName=Danova
DisableProgramGroupPage=yes
DisableDirPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir={#OutDir}
OutputBaseFilename=D-SYNTH-{#AppVersion}-Windows-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=D-SYNTH by Danova
CloseApplications=yes

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "{cm:FullInstall}"
Name: "custom"; Description: "{cm:CustomInstall}"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 (FL Studio, Ableton, Studio One, Cubase, Reaper...)"; Types: full custom; Flags: fixed
Name: "app"; Description: "{cm:StandaloneApp}"; Types: full

[Files]
; VST3 bundle goes to the standard folder every DAW scans
Source: "{#ArtDir}\VST3\D-SYNTH.vst3\*"; DestDir: "{commoncf64}\VST3\D-SYNTH.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#ArtDir}\Standalone\D-SYNTH.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: app
; the sounds (lossless FLAC), shared by the VST3 and the app
Source: "{#SmpDir}\*"; DestDir: "{commonappdata}\Danova\D-SYNTH\Samples"; Flags: ignoreversion
Source: "LISEZ-MOI-Windows.txt"; DestDir: "{app}"; DestName: "LISEZ-MOI.txt"; Flags: ignoreversion isreadme

[Icons]
Name: "{autoprograms}\D-SYNTH"; Filename: "{app}\D-SYNTH.exe"; Components: app
Name: "{autodesktop}\D-SYNTH"; Filename: "{app}\D-SYNTH.exe"; Components: app; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Components: app; Flags: unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\D-SYNTH.vst3"

[CustomMessages]
french.FullInstall=Installation complète
english.FullInstall=Full installation
french.CustomInstall=Installation personnalisée
english.CustomInstall=Custom installation
french.StandaloneApp=Application D-SYNTH (sans logiciel)
english.StandaloneApp=D-SYNTH standalone app
