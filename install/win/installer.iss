; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "SpriteSheet Packer"
#define MyAppVersion "1.0.9"
#define MyAppPublisher "amakaseev"
#define MyAppURL "http://www.spicyminds-lab.com"
#define MyAppExeName "SpriteSheetPacker.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{12F16109-492B-42E6-BECE-401E887C5E7D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE.md
;InfoAfterFile=..\..\README.md
OutputDir=.\
OutputBaseFilename=SpriteSheetPacker-Installer
SetupIconFile=..\..\SpriteSheetPacker\SpritePacker.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "vcredist2013_x86.exe"; DestDir: {tmp}; Flags: deleteafterinstall
Source: "vcredist2015_2017_2019_x86.exe"; DestDir: {tmp}; Flags: deleteafterinstall
Source: "bin\SpriteSheetPacker.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "bin\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{commonprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{tmp}\vcredist2013_x86.exe"; \
  Parameters: "/install /quiet /norestart"; \
  Check: IsDllMissing('msvcr120.dll'); \
  WorkingDir: {tmp}; \
  StatusMsg: Installing the VC++ x86 Redistributable for VS 2013...
Filename: "{tmp}\vcredist2015_2017_2019_x86.exe"; \
  Parameters: "/install /quiet /norestart"; \
  Check: IsDllMissing('vcruntime140.dll'); \
  WorkingDir: {tmp}; \
  StatusMsg: Installing the VC++ x86 Redistributable for VS 2015, 2017, 2019...
Filename: "{app}\{#MyAppExeName}"; \
  Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; \
  Flags: nowait postinstall skipifsilent

[Code]
function IsDllMissing(const dllName: string): Boolean;
begin
  if IsWin64 then
    begin
      Result := not ( FileExists(ExpandConstant('{syswow64}')+ '\' + dllName) );
    end
  else
    begin
      Result := not ( FileExists(ExpandConstant('{sys}')+ '\' + dllName) );
    end;  
end;

