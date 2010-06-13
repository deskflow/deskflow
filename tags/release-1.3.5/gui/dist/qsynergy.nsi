!include "MUI.nsh"

!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_SMALLDESC

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"

!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"

!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"

!define VERSION "0.9.0"
!define PRODUCT_VERSION "0.9.0.0"

Name "QSynergy ${VERSION}"
OutFile "qsynergy-${VERSION}-setup.exe"
InstallDir "$PROGRAMFILES\QSynergy\"

BrandingText " "

SetCompressor /FINAL bzip2

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Fidra\QSynergy" "Install_Dir"

; ---------------------------
; Pages
; ---------------------------

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\COPYING"

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
  
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


  
; ---------------------------
; Languages
; ---------------------------
 
!insertmacro MUI_LANGUAGE "English"
 
LangString qsynergy ${LANG_ENGLISH} "QSynergy program files"
LangString DESC_qsynergy ${LANG_ENGLISH} "The program files required for QSynergy."

LangString qt_dlls ${LANG_ENGLISH} "Qt libraries QSynergy needs to run"
LangString DESC_qt_dlls ${LANG_ENGLISH} "If you have Qt 4.3 or newer installed, you don't need to install this."

LangString startmenu ${LANG_ENGLISH} "Start menu entry"
LangString DESC_startmenu ${LANG_ENGLISH} "Create a start menu entry for QSynergy."

LangString desktopicon ${LANG_ENGLISH} "Desktop icon"
LangString DESC_desktopicon ${LANG_ENGLISH} "Create an icon on the desktop for QSynergy."

; ---------------------------
; Environment PATH manipulation
; ---------------------------

; !include "path_manip.nsh"


; ---------------------------
; Version stuff
; ---------------------------

VIProductVersion "${PRODUCT_VERSION}"

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "QSynergy ${VERSION} Setup"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Installs QSynergy ${VERSION} on your computer."
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "Fidra Software Entwicklung, Volker Lanz"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "2008 Fidra Software Entwicklung, Volker Lanz"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "QSynergy ${VERSION} Setup"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION}"


; ---------------------------
; Callbacks
; ---------------------------

Function .onInit
	!insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Function un.onInit
	!insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd


; ---------------------------
; The stuff to install
; ---------------------------

Section $(qsynergy) section_qsynergy
	SectionIn RO
  
	SetOutPath "$INSTDIR"
	File "..\release\qsynergy.exe"
	File "..\README"
	File "..\COPYING"
  
	; Write the installation path into the registry
	WriteRegStr HKLM "Software\Fidra\QSynergy" "Install_Dir" "$INSTDIR"
  
	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QSynergy" "DisplayName" "QSynergy ${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QSynergy" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QSynergy" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QSynergy" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
SectionEnd

Section $(qt_dlls) section_qt_dlls
	SetOutPath "$INSTDIR"
	File "..\..\..\..\sw\qt\lib\qtcore4.dll"
	File "..\..\..\..\sw\qt\lib\qtgui4.dll"
	File "..\..\..\..\sw\qt\lib\qtnetwork4.dll"
SectionEnd

Section $(startmenu) section_startmenu
	CreateDirectory "$SMPROGRAMS\QSynergy"
	CreateShortCut "$SMPROGRAMS\QSynergy\QSynergy.lnk" "$INSTDIR\qsynergy.exe" "" "$INSTDIR\qsynergy.exe" 0
	CreateShortCut "$SMPROGRAMS\QSynergy\README.lnk" "$INSTDIR\README" "" "$INSTDIR\README" 0
	CreateShortCut "$SMPROGRAMS\QSynergy\COPYING.lnk" "$INSTDIR\COPYING" "" "$INSTDIR\COPYING" 0
	CreateShortCut "$SMPROGRAMS\QSynergy\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section $(desktopicon) section_desktopicon
	CreateShortCut "$DESKTOP\QSynergy.lnk" "$INSTDIR\qsynergy.exe" "" "$INSTDIR\qsynergy.exe" 0
SectionEnd

; ---------------------------
; Uninstaller
; ---------------------------

Section "Uninstall"
	; Remove registry keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QSynergy"
	DeleteRegKey HKLM SOFTWARE\Fidra\QSynergy

	; Remove files and uninstaller
	Delete "$INSTDIR\uninstall.exe"
  
	Delete "$INSTDIR\*.exe"
	Delete "$INSTDIR\README"
	Delete "$INSTDIR\COPYING"
	Delete "$INSTDIR\*.dll"
  
	; startmenu and desktop icon
	Delete "$SMPROGRAMS\QSynergy\*.*"
	Delete "$DESKTOP\QSynergy.lnk"
	
	; Remove directories used
	RMDir "$SMPROGRAMS\QSynergy"
	RMDir "$INSTDIR"
SectionEnd


; ---------------------------
; Section descriptions
; ---------------------------

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${section_qsynergy} $(DESC_qsynergy)
	!insertmacro MUI_DESCRIPTION_TEXT ${section_qt_dlls} $(DESC_qt_dlls)
	!insertmacro MUI_DESCRIPTION_TEXT ${section_startmenu} $(DESC_startmenu)
	!insertmacro MUI_DESCRIPTION_TEXT ${section_desktopicon} $(DESC_desktopicon)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

