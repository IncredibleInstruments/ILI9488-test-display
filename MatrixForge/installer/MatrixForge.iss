; Inno Setup script for the one-click Windows installer.
; Build after the Release build with:
;   iscc /DAppVersion=1.1.0 installer\MatrixForge.iss
; Produces dist\MatrixForge-Setup.exe

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define BuildDir "..\build\MatrixForge_artefacts\Release"

[Setup]
AppId={{7D6C3F2E-4B1A-4E8B-9C2D-5A1F3E6B8C90}
AppName=MatrixForge
AppVersion={#AppVersion}
AppVerName=MatrixForge {#AppVersion}
AppPublisher=MatrixForge Audio
DefaultDirName={autopf}\MatrixForge
DefaultGroupName=MatrixForge
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=..\dist
OutputBaseFilename=MatrixForge-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
CloseApplications=yes
UninstallDisplayName=MatrixForge {#AppVersion}
UninstallDisplayIcon={app}\MatrixForge.exe

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Flags: unchecked

[Files]
; The VST3 bundle goes into the standard system folder every Windows DAW scans
Source: "{#BuildDir}\VST3\MatrixForge.vst3\*"; DestDir: "{commoncf64}\VST3\MatrixForge.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\MatrixForge.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[InstallDelete]
; Remove files from an older version of the bundle before copying the new one
Type: filesandordirs; Name: "{commoncf64}\VST3\MatrixForge.vst3"

[Icons]
Name: "{autoprograms}\MatrixForge"; Filename: "{app}\MatrixForge.exe"
Name: "{autodesktop}\MatrixForge"; Filename: "{app}\MatrixForge.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\MatrixForge.exe"; Description: "Open the MatrixForge standalone app now"; Flags: nowait postinstall skipifsilent unchecked
