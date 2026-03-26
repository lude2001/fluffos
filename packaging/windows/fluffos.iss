#define AppName "FluffOS"
#ifndef AppVersion
  #define AppVersion "0.0.0-dev"
#endif
#ifndef SourceDir
  #error SourceDir must be defined by the build script.
#endif
#ifndef OutputDir
  #define OutputDir SourceDir
#endif

[Setup]
AppId={{F0A4299D-9AB7-4E68-BF16-0F2E89E0AA91}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=FluffOS
DefaultDirName={autopf}\FluffOS
DefaultGroupName=FluffOS
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=fluffos-{#AppVersion}-windows-x86_64-installer
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\bin\lpcprj.exe

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\FluffOS\LPC Project Runner"; Filename: "{app}\bin\lpcprj.exe"
Name: "{autoprograms}\FluffOS\LPC Compiler"; Filename: "{app}\bin\lpccp.exe"

[Code]
var
  AddToPathCheckBox: TNewCheckBox;

function PathContainsEntry(const ExistingPath: string; const Entry: string): Boolean;
var
  NormalizedPath: string;
  NormalizedEntry: string;
begin
  NormalizedPath := ';' + Uppercase(ExistingPath) + ';';
  NormalizedEntry := ';' + Uppercase(Entry) + ';';
  Result := Pos(NormalizedEntry, NormalizedPath) > 0;
end;

function NeedsAddBinToPath(): Boolean;
var
  ExistingPath: string;
begin
  if not RegQueryStringValue(HKCU, 'Environment', 'Path', ExistingPath) then
    ExistingPath := '';
  Result := not PathContainsEntry(ExistingPath, ExpandConstant('{app}\bin'));
end;

procedure AddBinToUserPath();
var
  ExistingPath: string;
  BinDir: string;
begin
  BinDir := ExpandConstant('{app}\bin');
  if not RegQueryStringValue(HKCU, 'Environment', 'Path', ExistingPath) then
    ExistingPath := '';

  if PathContainsEntry(ExistingPath, BinDir) then
    exit;

  if (ExistingPath <> '') and (ExistingPath[Length(ExistingPath)] <> ';') then
    ExistingPath := ExistingPath + ';';

  ExistingPath := ExistingPath + BinDir;
  RegWriteExpandStringValue(HKCU, 'Environment', 'Path', ExistingPath);
end;

procedure InitializeWizard();
begin
  AddToPathCheckBox := TNewCheckBox.Create(WizardForm.FinishedPage);
  AddToPathCheckBox.Parent := WizardForm.FinishedPage;
  AddToPathCheckBox.Left := WizardForm.RunList.Left;
  AddToPathCheckBox.Top := WizardForm.RunList.Top;
  AddToPathCheckBox.Width := WizardForm.RunList.Width;
  AddToPathCheckBox.Caption := 'Add FluffOS commands to your user PATH';
  AddToPathCheckBox.Checked := False;
  AddToPathCheckBox.Visible := False;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpFinished then begin
    AddToPathCheckBox.Visible := NeedsAddBinToPath();
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and AddToPathCheckBox.Checked then begin
    AddBinToUserPath();
  end;
end;
