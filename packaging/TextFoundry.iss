#define MyAppName "TextFoundry"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef MySourceDir
  #define MySourceDir "..\\stage"
#endif
#ifndef MyOutputDir
  #define MyOutputDir "..\\dist"
#endif

[Setup]
AppId={{0E5D3F36-8E92-4F55-A0A0-3C2C2A0EE5F2}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=TextFoundry
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\bin\tf-gui.exe
Compression=lzma
SolidCompression=yes
WizardStyle=modern
OutputDir={#MyOutputDir}
OutputBaseFilename=textfoundry-windows-setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
Source: "{#MySourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\bin\tf-gui.exe"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\bin\tf-gui.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\tf-gui.exe"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
