#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

#ifndef SourceRoot
  #error SourceRoot must be defined.
#endif

#ifndef StageDir
  #error StageDir must be defined.
#endif

#ifndef OutputDir
  #define OutputDir AddBackslash(SourceRoot) + "output\\windows-installer\\dist"
#endif

[Setup]
AppId={{2A4CC7B4-DA8F-4B33-96DD-4BAFFB69F8D7}
AppName=SAVT
AppVersion={#AppVersion}
AppVerName=SAVT {#AppVersion}
AppPublisher=SAVT
DefaultDirName={autopf}\SAVT
DefaultGroupName=SAVT
DisableProgramGroupPage=yes
AllowNoIcons=yes
OutputDir={#OutputDir}
OutputBaseFilename=SAVT-{#AppVersion}-zh-CN-setup
SetupIconFile={#SourceRoot}\apps\desktop\assets\app_icon.ico
UninstallDisplayIcon={app}\savt_desktop.exe
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ShowLanguageDialog=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\Chinese(Simplified).isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\SAVT"; Filename: "{app}\savt_desktop.exe"; IconFilename: "{app}\savt_desktop.exe"
Name: "{autodesktop}\SAVT"; Filename: "{app}\savt_desktop.exe"; Tasks: desktopicon; IconFilename: "{app}\savt_desktop.exe"

[Run]
Filename: "{app}\savt_desktop.exe"; Description: "启动 SAVT"; Flags: nowait postinstall skipifsilent
