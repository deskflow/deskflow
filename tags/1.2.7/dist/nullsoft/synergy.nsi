; Synergy.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install makensisw.exe into a directory that the user selects,

;--------------------------------

; Path to root of tree
!define DEPTH "..\.."

; The name of the installer
Name "Synergy"

; The file to write
OutFile "${DEPTH}\build\SynergyInstaller.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Synergy

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Synergy" "Install_Dir"

;--------------------------------

; Pages

Page components
Page license
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; Text
ComponentText "This will install Synergy on your computer.  Select the optional components you want to install."
DirText "Choose a directory to install Synergy to:"
UninstallText "This will uninstall Synergy from your computer."
LicenseText "Synergy is distributed under the GNU GPL:"
LicenseData "COPYING.txt"

;--------------------------------

; The stuff to install
Section "Synergy (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put files there
  File "${DEPTH}\build\synergy.exe"
  File "${DEPTH}\build\synergyc.exe"
  File "${DEPTH}\build\synergys.exe"
  File "${DEPTH}\build\*.dll"
  File COPYING.txt
  File ChangeLog.txt
  File ${DEPTH}\doc\PORTING
  File ${DEPTH}\doc\authors.html
  File ${DEPTH}\doc\autostart.html
  File ${DEPTH}\doc\compiling.html
  File ${DEPTH}\doc\configuration.html
  File ${DEPTH}\doc\developer.html
  File ${DEPTH}\doc\faq.html
  File ${DEPTH}\doc\history.html
  File ${DEPTH}\doc\index.html
  File ${DEPTH}\doc\license.html
  File ${DEPTH}\doc\news.html
  File ${DEPTH}\doc\running.html
  File ${DEPTH}\doc\security.html
  File ${DEPTH}\doc\synergy.css
  File ${DEPTH}\doc\tips.html
  File ${DEPTH}\doc\todo.html

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
  CreateShortCut "$SMPROGRAMS\Synergy\FAQ.lnk" "$INSTDIR\faq.html"
  CreateShortCut "$SMPROGRAMS\Synergy\Synergy Folder.lnk" "$INSTDIR"
  CreateShortCut "$SMPROGRAMS\Synergy\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

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
