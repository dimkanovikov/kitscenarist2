;NSIS Modern User Interface
;Welcome/Finish Page Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  
  ; ������ ��� ����������� ���������� ������
  ; ���� ������ http://nsis.sourceforge.net/File_Association , �������� ���� "Program Files\NSIS\Include"
  !include "FileAssociation.nsh"

;--------------------------------
;General

  ;Show all languages, despite user's codepage
  !define MUI_LANGDLL_ALLLANGUAGES

  !define pkgdir "files_64"

  ;Name and file
  Name "Story Architect"
  Caption "Installing application for creating stories"
  OutFile "starc-setup-64.exe"
  BrandingText "Story Apps LLC"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Starc"
  InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "UninstallString"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_FINISHPAGE_RUN "$INSTDIR\starcapp.exe"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "Russian"

;--------------------------------
;Additional includes


!include FileAssociation.nsh

;--------------------------------
;Installer Sections

Section "App files section" SecFiles

  ; ���������� ��������� � ������ ������������� �������� �������
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "DisplayName" "Story Architect"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "Publisher" "Story Apps LLC"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "DisplayIcon" "$INSTDIR\starcapp.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc" "UrlInfoAbout" "https://starc.app"
  
  SetOutPath "$INSTDIR"
  File /r "${pkgdir}\"

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; ������������ ���������� 
  ${registerExtension} "$INSTDIR\starcapp.exe" ".starc" "Story Architect project"
  
  ; ��������� ���������
  System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Starc"
  CreateShortCut "$SMPROGRAMS\Starc\Story Architect.lnk" "$INSTDIR\starcapp.exe"
  CreateShortCut "$SMPROGRAMS\Starc\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

Section "Desctop Shortcut"

  CreateShortcut "$DESKTOP\Story Architect.lnk" "$INSTDIR\starcapp.exe" "" "$INSTDIR\starcapp.exe" 0
  
SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; ������ ��������� �� ������ �������������
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Starc"
  
  ; ������� ��� ����� ����� ���� ������
  Delete "$INSTDIR\audio\*.*"
  Delete "$INSTDIR\imageformats\*.*"
  Delete "$INSTDIR\platforms\*.*"
  Delete "$INSTDIR\plugins\*.*"
  Delete "$INSTDIR\printsupport\*.*"
  Delete "$INSTDIR\sqldrivers\*.*"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.ico"
  Delete "$INSTDIR\*.dat"
  Delete "$INSTDIR\*.pak"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Starc\*.*"
  Delete "$DESKTOP\Story Architect.lnk"

  ; Remove directories used
  RMDir /r "$INSTDIR\audio\*.*"
  RMDir /r "$INSTDIR\imageformats\*.*"
  RMDir /r "$INSTDIR\platforms\*.*"
  RMDir /r "$INSTDIR\plugins\*.*"
  RMDir /r "$INSTDIR\printsupport\*.*"
  RMDir /r "$INSTDIR\sqldrivers\*.*"
  RMDir "$INSTDIR"
  RMDir "$SMPROGRAMS\Starc"
  
  ; �������� ������������������ ���������� ������
  ${unregisterExtension} ".starc" "Story Architect project"

SectionEnd

Function .onInit

	;Language selection dialog

	InitPluginsDir
	!insertmacro MUI_LANGDLL_DISPLAY
	
FunctionEnd

