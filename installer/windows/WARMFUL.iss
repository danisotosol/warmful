; WARMFUL — Windows 10/11 installer (Inno Setup 6). Saved as UTF-8 with BOM so ISCC reads the accents.
; SPDX-License-Identifier: CC-BY-NC-SA-4.0
;
; Build (after `cmake --build build --config Release`):
;   ISCC.exe /DAppVersion=0.1.0 installer\windows\WARMFUL.iss
; Silent install (CI / power users):
;   WARMFUL-<ver>-Windows-Setup.exe /VERYSILENT /COMPONENTS=vst3,standalone /DIR="C:\WARMFUL" /VST3DIR="D:\Plugins\VST3"

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef ArtefactsDir
  #define ArtefactsDir "..\..\build\WARMFUL_artefacts\Release"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\build\installer"
#endif

[Setup]
AppId={{6B0E3C1A-5E7B-4E43-9C2B-8F1D2A7E4C11}
AppName=WARMFUL
AppVersion={#AppVersion}
AppVerName=WARMFUL {#AppVersion}
AppPublisher=danisotosol
AppPublisherURL=https://github.com/danisotosol/WARMFUL
AppSupportURL=https://github.com/danisotosol/WARMFUL/issues
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\WARMFUL
DefaultGroupName=WARMFUL
AllowNoIcons=yes
LicenseFile=..\..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=WARMFUL-{#AppVersion}-Windows-Setup
SetupIconFile=..\..\resources\warmful.ico
UninstallDisplayIcon={app}\WARMFUL.ico
UninstallDisplayName=WARMFUL {#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
; Administrator install by default (Program Files + Common Files\VST3); the user may choose
; a per-user install instead, which needs no admin rights.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog commandline
CloseApplications=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
spanish.TypeFull=Plug-in VST3 + aplicación standalone
english.TypeFull=VST3 plug-in + standalone application
spanish.TypeVst3=Solo plug-in VST3
english.TypeVst3=VST3 plug-in only
spanish.TypeCustom=Personalizada
english.TypeCustom=Custom
spanish.CompVst3=Plug-in VST3 (para tu DAW)
english.CompVst3=VST3 plug-in (for your DAW)
spanish.CompStandalone=Aplicación standalone (sin DAW, con tu interfaz de audio)
english.CompStandalone=Standalone application (no DAW, uses your audio interface)
spanish.CompDocs=Documentación, licencia y kit de medición contra el pedal
english.CompDocs=Documentation, licence and pedal measurement kit
spanish.Vst3PageTitle=Carpeta del plug-in VST3
english.Vst3PageTitle=VST3 plug-in folder
spanish.Vst3PageSubtitle=¿Dónde quieres instalar el plug-in VST3?
english.Vst3PageSubtitle=Where should the VST3 plug-in be installed?
spanish.Vst3PageText=Elige la carpeta que tu DAW escanea para plug-ins VST3 (la estándar es Common Files\VST3) y pulsa Siguiente.
english.Vst3PageText=Pick the folder your DAW scans for VST3 plug-ins (the standard one is Common Files\VST3), then click Next.

[Types]
Name: "full"; Description: "{cm:TypeFull}"
Name: "vst3"; Description: "{cm:TypeVst3}"
Name: "custom"; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
Name: "vst3"; Description: "{cm:CompVst3}"; Types: full vst3
Name: "standalone"; Description: "{cm:CompStandalone}"; Types: full
Name: "docs"; Description: "{cm:CompDocs}"; Types: full vst3

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone; Flags: unchecked

[Files]
Source: "{#ArtefactsDir}\VST3\WARMFUL.vst3\*"; DestDir: "{code:GetVst3Dir}\WARMFUL.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtefactsDir}\Standalone\WARMFUL.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\..\resources\warmful.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Components: docs; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Components: docs; Flags: ignoreversion
Source: "..\..\tools\measure\*"; DestDir: "{app}\measure"; Components: docs; Flags: ignoreversion

[Icons]
Name: "{group}\WARMFUL"; Filename: "{app}\WARMFUL.exe"; Components: standalone
Name: "{group}\README"; Filename: "{app}\README.md"; Components: docs
Name: "{group}\{cm:UninstallProgram,WARMFUL}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\WARMFUL"; Filename: "{app}\WARMFUL.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\WARMFUL.exe"; Description: "{cm:LaunchProgram,WARMFUL}"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[Code]
var
  Vst3Page: TInputDirWizardPage;

function DefaultVst3Dir(): String;
begin
  { VST3 spec locations: machine-wide for admin installs, per-user otherwise. }
  if IsAdminInstallMode then
    Result := ExpandConstant('{commoncf64}\VST3')
  else
    Result := ExpandConstant('{localappdata}\Programs\Common\VST3');
end;

procedure InitializeWizard();
begin
  Vst3Page := CreateInputDirPage(wpSelectComponents, CustomMessage('Vst3PageTitle'),
    CustomMessage('Vst3PageSubtitle'), CustomMessage('Vst3PageText'), False, '');
  Vst3Page.Add('');
  { /VST3DIR=... wins, then the folder chosen last time, then the standard location. }
  Vst3Page.Values[0] := ExpandConstant('{param:VST3DIR|' + GetPreviousData('Vst3Dir', DefaultVst3Dir()) + '}');
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = Vst3Page.ID) and not WizardIsComponentSelected('vst3');
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'Vst3Dir', Vst3Page.Values[0]);
end;

function GetVst3Dir(Param: String): String;
begin
  Result := Vst3Page.Values[0];
end;
