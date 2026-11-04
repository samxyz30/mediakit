[Setup]
AppName=mediakit
AppVersion=0.1
WizardStyle=modern
DefaultDirName={autopf}\mediakit
DefaultGroupName=mediakit
UninstallDisplayIcon={app}\mediakit.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\build\setup\
OutputBaseFilename=mediakit-setup
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
ArchitecturesAllowed=x64
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\mediakit"; Filename: "{app}\bin\mediakit.exe"
