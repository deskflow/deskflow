; Synergy.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install makensisw.exe into a directory that the user selects,

;--------------------------------
!include "MUI2.nsh"

!ifndef OUTPUTDIR
!define OUTPUTDIR "build\Release"
!endif

SetDatablockOptimize 	on

; The name of the installer
!define PRODUCT "Synergy+"
Name "${PRODUCT} 1.3.3"
BrandingText "${PRODUCT} 1.3.3"

; The file to write
OutFile "${OUTPUTFILE}"

; The default installation directory
InstallDir $PROGRAMFILES\Synergy

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Synergy" "Install_Dir"

;Request application privileges for Windows Vista
  RequestExecutionLevel admin

!define MUI_ICON cmd\launcher\synergy.ico
!define MUI_UNICON cmd\launcher\synergy.ico

;--------------------------------

; Pages

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "${OUTPUTDIR}\COPYING.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;Page components
;Page license
;Page directory
;Page instfiles

UninstPage uninstConfirm
UninstPage instfiles
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

; Text
ComponentText "This will install Synergy on your computer.  Select the optional components you want to install."
DirText "Choose a directory to install Synergy to:"
UninstallText "This will uninstall Synergy from your computer."
LicenseText "Synergy is distributed under the GNU GPL:"
LicenseData ${OUTPUTDIR}\COPYING.txt

;--------------------------------

; The stuff to install
Section "Synergy+ (required)"

  SectionIn RO

  ; Install VC Redist fiiles
  SetOutPath $TEMP
  File "dist\nullsoft\files\vcredist_x86_2005sp1.exe"
  File "dist\nullsoft\files\vcredist_x86_2008sp1.exe"
    
  Exec '$TEMP\vcredist_x86_2005sp1.exe /q:a /c:"VCREDI~1.EXE /q:a /c:""msiexec /i vcredist.msi /qn"" "'
  Exec '$TEMP\vcredist_x86_2008sp1.exe /q:a /c:"VCREDI~1.EXE /q:a /c:""msiexec /i vcredist.msi /qn"" "'


  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put files there
  File "${OUTPUTDIR}\synergy.exe"
  File "${OUTPUTDIR}\synergyc.exe"
  File "${OUTPUTDIR}\synergys.exe"
  File "${OUTPUTDIR}\*.dll"
  File "${OUTPUTDIR}\COPYING.txt"
  File "${OUTPUTDIR}\ChangeLog.txt"
  File doc\PORTING
  File doc\about.html
  File doc\authors.html
  File doc\autostart.html
  File doc\banner.html
  File doc\compiling.html
  File doc\configuration.html
  File doc\contact.html
  File doc\developer.html
  File doc\faq.html
  File doc\history.html
  File doc\home.html
  File doc\index.html
  File doc\license.html
  File doc\news.html
  File doc\roadmap.html
  File doc\running.html
  File doc\security.html
  File doc\synergy.css
  File doc\tips.html
  File doc\toc.html
  File doc\trouble.html

  SetOutPath $INSTDIR\images
  File doc\images\logo.gif
  File doc\images\warp.gif

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Synergy "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Synergy" "DisplayName" "Synergy"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Synergy" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Synergy" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Synergy" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Synergy"
  CreateShortCut "$SMPROGRAMS\Synergy\Synergy.lnk" "$INSTDIR\synergy.exe" "" "$INSTDIR\synergy.exe" 0
  CreateShortCut "$SMPROGRAMS\Synergy\README.lnk" "$INSTDIR\index.html"
  CreateShortCut "$SMPROGRAMS\Synergy\Synergy Folder.lnk" "$INSTDIR"
  CreateShortCut "$SMPROGRAMS\Synergy\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

SectionEnd

; Optional section (can be disabled by the user)
Section "Desktop Icon"

  CreateShortCut "$DESKTOP\Synergy.lnk" "$INSTDIR\synergy.exe" "" "$INSTDIR\synergy.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  ; Stop and uninstall the daemons
  ExecWait '"$INSTDIR\synergy.exe" /uninstall'

  ; Remove autorun registry keys for synergy
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\Synergy Server"
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\Synergy Client"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\RunServices" "Synergy Server"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\RunServices" "Synergy Client"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Synergy Server"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Synergy Client"
  
  ; not all keys will have existed, so errors WILL have happened
  ClearErrors

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Synergy"
  DeleteRegKey HKLM SOFTWARE\Synergy

  ClearErrors

  ; First try to remove files that might be locked (if synergy is running)
  Delete /REBOOTOK $INSTDIR\synergy.exe
  Delete /REBOOTOK $INSTDIR\synergyc.exe
  Delete /REBOOTOK $INSTDIR\synergys.exe
  Delete /REBOOTOK $INSTDIR\synrgyhk.dll

  ; Remove files and directory
  Delete $INSTDIR\*.*
  RMDir $INSTDIR

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Synergy\*.*"
  Delete "$DESKTOP\Synergy.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Synergy"
  RMDir "$INSTDIR"

  IfRebootFlag 0 EndOfAll
	MessageBox MB_OKCANCEL "Uninstaller needs to reboot to finish cleaning up. reboot now?" IDCANCEL NoReboot
	ClearErrors
	Reboot
	IfErrors 0 EndOfAll
		MessageBox MB_OK "Uninstaller could not reboot. Please reboot manually. Thank you."
		Abort "Uninstaller could not reboot. Please reboot manually. Thank you."
  NoReboot:
	DetailPrint ""
	DetailPrint "Uninstaller could not reboot. Please reboot manually. Thank you."
	DetailPrint ""
	SetDetailsView show
  EndOfAll:

SectionEnd
