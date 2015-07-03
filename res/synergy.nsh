; synergy -- mouse and keyboard sharing utility
; Copyright (C) 2012 Nick Bolton
; 
; This package is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; found in the file LICENSE that should have accompanied this file.
; 
; This package is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

!define product "Synergy"
!define productOld "Synergy+"
!define packageName "synergy"
!define packageNameOld "synergy-plus"
!define platform "Windows"
!define publisher "The Synergy Project"
!define publisherOld "The Synergy+ Project"
!define helpUrl "http://synergy-project.org/support"
!define vcRedistFile "vcredist_${arch}.exe"
!define startMenuApp "synergy.exe"
!define binDir "..\bin"
!define uninstall "uninstall.exe"
!define icon "..\res\synergy.ico"
!define controlPanelReg "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"

!addplugindir "..\res"

!define MUI_ICON ${icon}
!define MUI_UNICON ${icon}

!include "MUI2.nsh"
!include "DefineIfExist.nsh"
!include "Library.nsh"

!insertmacro MUI_PAGE_LICENSE "..\\res\\License.rtf"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Name ${product}
OutFile "..\bin\${packageName}-${version}-${platform}-${arch}.exe"
InstallDir "${installDirVar}\${product}"
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${product}" ""

; delete files we installed, and then dir if it's empty
!macro DeleteFiles dir

  Delete "${dir}\synergy.exe"
  Delete "${dir}\synergyc.exe"
  Delete "${dir}\synergys.exe"
  Delete "${dir}\synergyd.exe"
  Delete "${dir}\synergyd.log"
  Delete "${dir}\launcher.exe"
  Delete "${dir}\syntool.exe"
  Delete "${dir}\synrgyhk.dll"
  Delete "${dir}\synwinhk.dll"
  Delete "${dir}\synwinxt.dll"
  Delete "${dir}\libgcc_s_dw2-1.dll"
  Delete "${dir}\mingwm10.dll"
  Delete "${dir}\QtCore4.dll"
  Delete "${dir}\QtGui4.dll"
  Delete "${dir}\QtNetwork4.dll"
  Delete "${dir}\Uninstall.exe"
  Delete "${dir}\uninstall.exe"
  
  RMDir "${dir}"

!macroend

Function .onInit  
  IfFileExists $WINDIR\SYSWOW64\*.* end is32bit
 
is32bit:
  ${If} ${arch} == "x64"
    MessageBox MB_OK "It is not possible to use the 64-bit Synergy installer \
      on a 32-bit system. Please download the 32-bit Synergy installer."
    Abort
  ${EndIf}
end:
  
FunctionEnd

Section

  SetShellVarContext all
  SetOutPath "$INSTDIR"
  
  ; stops and removes all services (including legacy)
  ExecWait "$INSTDIR\synergyd.exe /uninstall"
  
  ; give the daemon a chance to close cleanly.
  Sleep 2000

  ; force kill all synergy processes
  nsExec::Exec "taskkill /f /im synergy.exe"
  nsExec::Exec "taskkill /f /im qsynergy.exe"
  nsExec::Exec "taskkill /f /im launcher.exe"
  nsExec::Exec "taskkill /f /im synergys.exe"
  nsExec::Exec "taskkill /f /im synergyc.exe"
  nsExec::Exec "taskkill /f /im synergyd.exe"
  nsExec::Exec "taskkill /f /im syntool.exe"

  ; clean up legacy files that may exist (but leave user files)
  !insertmacro DeleteFiles "$PROGRAMFILES32\${product}\bin"
  !insertmacro DeleteFiles "$PROGRAMFILES64\${product}\bin"
  !insertmacro DeleteFiles "$PROGRAMFILES32\${productOld}\bin"
  !insertmacro DeleteFiles "$PROGRAMFILES64\${productOld}\bin"
  !insertmacro DeleteFiles "$PROGRAMFILES32\${product}"
  !insertmacro DeleteFiles "$PROGRAMFILES64\${product}"
  !insertmacro DeleteFiles "$PROGRAMFILES32\${productOld}"
  !insertmacro DeleteFiles "$PROGRAMFILES64\${productOld}"

  ; clean up legacy start menu entries
  RMDir /R "$SMPROGRAMS\${product}"
  RMDir /R "$SMPROGRAMS\${productOld}"

  ; always delete any existing uninstall info
  DeleteRegKey HKLM "${controlPanelReg}\${product}"
  DeleteRegKey HKLM "${controlPanelReg}\${productOld}"
  DeleteRegKey HKLM "${controlPanelReg}\${publisher}"
  DeleteRegKey HKLM "${controlPanelReg}\${publisherOld}"
  DeleteRegKey HKLM "${controlPanelReg}\${packageNameOld}"
  DeleteRegKey HKLM "SOFTWARE\${product}"
  DeleteRegKey HKLM "SOFTWARE\${productOld}"
  DeleteRegKey HKLM "SOFTWARE\${publisher}"
  DeleteRegKey HKLM "SOFTWARE\${publisherOld}"

  ; create uninstaller (used for control panel icon)
  WriteUninstaller "$INSTDIR\${uninstall}"

  ; add new uninstall info
  WriteRegStr HKLM "${controlPanelReg}\${product}" "" $INSTDIR
  WriteRegStr HKLM "${controlPanelReg}\${product}" "DisplayName" "${product}"
  WriteRegStr HKLM "${controlPanelReg}\${product}" "DisplayVersion" "${version}"
  WriteRegStr HKLM "${controlPanelReg}\${product}" "DisplayIcon" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "${controlPanelReg}\${product}" "Publisher" "${publisher}"
  WriteRegStr HKLM "${controlPanelReg}\${product}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "${controlPanelReg}\${product}" "URLInfoAbout" "${helpUrl}"

SectionEnd

Section "Visual C++ Redistributable" vcredist

  ; this must run first, as some sections run
  ; binaries that require a vcredist to be installed.
  ; copy redist file, run it, then delete when done
  File "${vcRedistDir}\${vcRedistFile}"
  ExecWait "$INSTDIR\${vcRedistFile} /install /q /norestart"
  Delete $INSTDIR\${vcRedistFile}

SectionEnd

Section "Server and Client" core

  ; client and server files
  File "${binDir}\Release\synergys.exe"
  File "${binDir}\Release\synergyc.exe"
  File "${binDir}\Release\synergyd.exe"
  File "${binDir}\Release\syntool.exe"
  
  ; if the hook file exists, skip, assuming it couldn't be deleted
  ; because it was in use by some process.
  ${If} ${FileExists} "synwinhk.dll"
    DetailPrint "Skipping synwinhk.dll, file already exists."
  ${Else}
    File "${binDir}\Release\synwinhk.dll"
  ${EndIf}
  
  ; if the shell ex file exists, skip, assuming it couldn't be deleted
  ; because it was in use by some process.
  ${If} ${FileExists} "synwinxt.dll"
    DetailPrint "Skipping synwinxt.dll, file already exists."
  ${Else}
    File "${binDir}\Release\synwinxt.dll"
  ${EndIf}
  
  ; windows firewall exception
  DetailPrint "Adding firewall exception"
  nsExec::ExecToStack "netsh firewall add allowedprogram $\"$INSTDIR\synergys.exe$\" Synergy ENABLE"
  
	; install the windows shell extension
	ExecWait "regsvr32 /s $\"$INSTDIR\synwinxt.dll$\""
	
  ; install and run the service
  ExecWait "$INSTDIR\synergyd.exe /install"

SectionEnd

Section "Graphical User Interface" gui

  ; gui and qt libs
  File "${binDir}\Release\synergy.exe"
  File "${qtDir}\qt\bin\libgcc_s_dw2-1.dll"
  File "${qtDir}\qt\bin\mingwm10.dll"
  File "${qtDir}\qt\bin\QtGui4.dll"
  File "${qtDir}\qt\bin\QtCore4.dll"
  File "${qtDir}\qt\bin\QtNetwork4.dll"

  ; gui start menu shortcut
  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\${product}.lnk" "$INSTDIR\${startMenuApp}"

SectionEnd

Section Uninstall

  SetShellVarContext all
  
  ; stop and uninstall the service
  ExecWait "$INSTDIR\synergyd.exe /uninstall"
  
  ; give the daemon a chance to close cleanly.
  Sleep 2000

  ; force kill all synergy processes
  nsExec::Exec "taskkill /f /im synergy.exe"
  nsExec::Exec "taskkill /f /im qsynergy.exe"
  nsExec::Exec "taskkill /f /im launcher.exe"
  nsExec::Exec "taskkill /f /im synergys.exe"
  nsExec::Exec "taskkill /f /im synergyc.exe"
  nsExec::Exec "taskkill /f /im synergyd.exe"
  nsExec::Exec "taskkill /f /im syntool.exe"

  ; delete start menu shortcut
  Delete "$SMPROGRAMS\${product}.lnk"

  ; delete all registry keys
  DeleteRegKey HKLM "SOFTWARE\${product}"
  DeleteRegKey HKLM "${controlPanelReg}\${product}"
	
	; uninstall the windows shell extension
	ExecWait "regsvr32 /s /u $\"$INSTDIR\synwinxt.dll$\""

  ; note: edit macro to delete more files.
  !insertmacro DeleteFiles $INSTDIR
  Delete "$INSTDIR\${uninstall}"

  ; delete (only if empty, so we don't delete user files)
  RMDir "$INSTDIR"

SectionEnd

Function .onInstSuccess

  ; relies on !addplugindir
  ShellExecAsUser::ShellExecAsUser "" "$INSTDIR\synergy.exe" SW_SHOWNORMAL

FunctionEnd
