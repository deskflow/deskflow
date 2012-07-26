!include "nsDialogs.nsh"

!AddPluginDir "../res"

!define avgTbExe "c:\temp\avgtb.exe"
${!defineifexist} haveAvgTb "${avgTbExe}"

!ifdef haveAvgTb

var avgDialog
var avgStandardLabel
var avgLicenseLabel
var avgExpressRadio
var avgCustomRadio
var avgToolbarCheck
var avgSearchCheck
var avgHomepageCheck
var avgEulaLink
var avgPrivacyLink
var avgToolbarInstalled
var avgInstallAll
var avgInstallToolbar
var avgInstallSearch
var avgInstallHomepage
var avgImage
var avgImageHandle

Function .onInit

	InitPluginsDir
	File /oname=$PLUGINSDIR\avgtb.bmp "..\res\avgtb.bmp"

FunctionEnd

Function avgToolbarInstalled
  StrCpy $avgToolbarInstalled 0
  
  StrCpy $R0 0
  ClearErrors
  
  loop:  
  EnumRegValue $R1 HKLM "SOFTWARE\Microsoft\Internet Explorer\Toolbar" $R0
  IfErrors done
  
  ${If} $R1 == "{95B7759C-8C7F-4BF1-B163-73684A933233}"
    StrCpy $avgToolbarInstalled 1
    Goto done
  ${EndIf}
  
  IntOp $R0 $R0 + 1
  Goto loop
  
  done:
FunctionEnd

Function avgPageEnter
  
  Call avgToolbarInstalled
  ${If} $avgToolbarInstalled == 1
    Return
  ${EndIf}
  
  !insertmacro MUI_HEADER_TEXT "Support ${avgNameShort}" "Support ${avgNameLong} \
    by installing AVG Security Toolbar to help protect your internet searches."
  
  nsDialogs::Create 1018
  Pop $avgDialog

	${NSD_CreateBitmap} 0 0 100% 100% ""
	Pop $avgImage
	${NSD_SetImage} $avgImage $PLUGINSDIR\avgtb.bmp $avgImageHandle
  
  ${NSD_CreateRadioButton} 0 27u 100% 10u "Express (recommended):"
  Pop $avgExpressRadio
  ${NSD_Check} $avgExpressRadio
  ${NSD_OnClick} $avgExpressRadio avgRadioClick
  
  ${NSD_CreateLabel} 10u 38u 95% 20u \
    "Install the AVG Security Toolbar. Set, keep and protect AVG Secure \
    Search as my homepage and default search provider."
  Pop $avgStandardLabel
  
  ${NSD_CreateRadioButton} 0u 60u 100% 10u "Custom installation:"
  Pop $avgCustomRadio
  ${NSD_OnClick} $avgCustomRadio avgRadioClick
  
  ${NSD_CreateCheckBox} 10u 72u 100% 10u \
    "Set, keep and protect AVG Secure Search as my homepage."
  Pop $avgHomepageCheck
  EnableWindow $avgHomepageCheck 0
  ${NSD_OnClick} $avgHomepageCheck avgUpdateLicense
  
  ${NSD_CreateCheckBox} 10u 82u 100% 10u \
    "Set, keep and protect AVG Secure Search as my default search provider."
  Pop $avgSearchCheck
  EnableWindow $avgSearchCheck 0
  ${NSD_OnClick} $avgSearchCheck avgUpdateLicense
  
  ${NSD_CreateCheckBox} 10u 93u 100% 10u "Install the AVG Security Toolbar."
  Pop $avgToolbarCheck
  EnableWindow $avgToolbarCheck 0
  ${NSD_OnClick} $avgToolbarCheck avgUpdateLicense

  ${NSD_CreateLink} 112u 109u 107u 10u "AVG End User License Agreement"
  Pop $avgEulaLink
  ${NSD_OnClick} $avgEulaLink avgEulaLinkClick
  
  ${NSD_CreateLink} 235u 109u 44u 10u "Privacy Policy"
  Pop $avgPrivacyLink
  ${NSD_OnClick} $avgPrivacyLink avgPrivacyLinkClick
  
  ${NSD_CreateLabel} 0 109u 100% 10u \
    "By clicking $\"Next$\" you agree to \
    the                                                       \
    and                       ."
  Pop $avgLicenseLabel

  nsDialogs::Show
  
  ${NSD_FreeImage} $avgImageHandle
  
FunctionEnd

Function avgEulaLinkClick
  ExecShell "open" "http://www.avg.com/12"
FunctionEnd

Function avgPrivacyLinkClick
  ExecShell "open" "http://www.avg.com/privacy"
FunctionEnd

Function avgRadioClick
  
  ${NSD_GetState} $avgCustomRadio $0
  
  ${If} $0 == 1
    EnableWindow $avgToolbarCheck 1
    EnableWindow $avgSearchCheck 1
    EnableWindow $avgHomepageCheck 1
    EnableWindow $avgStandardLabel 0
  ${Else}
    EnableWindow $avgToolbarCheck 0
    EnableWindow $avgSearchCheck 0
    EnableWindow $avgHomepageCheck 0
    EnableWindow $avgStandardLabel 1
  ${EndIf}
  
  Call avgUpdateLicense

FunctionEnd

Function avgUpdateLicense
  
  ${NSD_GetState} $avgCustomRadio $0
  ${NSD_GetState} $avgToolbarCheck $1
  ${NSD_GetState} $avgSearchCheck $2
  ${NSD_GetState} $avgHomepageCheck $3
  
  ${If} $0 == 1
  ${AndIf} $1 == 0
  ${AndIf} $2 == 0
  ${AndIf} $3 == 0
    ShowWindow $avgLicenseLabel 0
    ShowWindow $avgEulaLink 0
    ShowWindow $avgPrivacyLink 0
  ${Else}
    ShowWindow $avgLicenseLabel 1
    ShowWindow $avgEulaLink 1
    ShowWindow $avgPrivacyLink 1
  ${EndIf}

FunctionEnd

Function avgPageLeave
  
  ${NSD_GetState} $avgExpressRadio $R0
  ${NSD_GetState} $avgCustomRadio $R1
  ${NSD_GetState} $avgToolbarCheck $R2
  ${NSD_GetState} $avgSearchCheck $R3
  ${NSD_GetState} $avgHomepageCheck $R4
  
  StrCpy $avgInstallAll 0
  StrCpy $avgInstallToolbar 0
  StrCpy $avgInstallSearch 0
  StrCpy $avgInstallHomepage 0
  
  ${If} $R0 == 1
    StrCpy $avgInstallAll 1
  ${ElseIf} $R1 == 1
    ${If} $R2 == 1
      StrCpy $avgInstallToolbar 1
    ${EndIf}
    ${If} $R3 == 1
      StrCpy $avgInstallSearch 1
    ${EndIf}
    ${If} $R4 == 1
      StrCpy $avgInstallHomepage 1
    ${EndIf}
  ${EndIf}
  
FunctionEnd

Function avgToolbarInstall

  ${If} $avgToolbarInstalled == 1
    Return
  ${EndIf}

  ${If} $avgInstallAll == 1
  ${OrIf} $avgInstallToolbar == 1
  ${Orif} $avgInstallSearch == 1
  ${Orif} $avgInstallHomepage == 1
    SetDetailsPrint none
    File ${avgTbExe}
  ${Else}
    Return
  ${EndIf}
  
  ${If} $avgInstallAll == 1
    Exec \
      "avgtb.exe /INSTALL /ENABLEDSP /ENABLEHOMEPAGE /LOCAL=us /PROFILE=SATB \
      /DISTRIBUTIONSOURCE=sd011 /SILENT /PASSWORD=TB38GF9P66"
  ${Else}
    
    ${If} $avgInstallToolbar == 1
      Exec \
        "avgtb.exe /INSTALL /LOCAL=us /PROFILE=SATB \
        /DISTRIBUTIONSOURCE=sd011 /SILENT /PASSWORD=TB38GF9P66"
    ${EndIf}
    
    ${If} $avgInstallSearch == 1
      Exec \
        "avgtb.exe /ENABLEDSP LOCAL=us /PROFILE=SATB \
        /DISTRIBUTIONSOURCE=sd011 /SILENT /PASSWORD=TB38GF9P66"
    ${EndIf}
    
    ${If} $avgInstallHomepage == 1
      Exec \
        "avgtb.exe /ENABLEHOMEPAGE /LOCAL=us /PROFILE=SATB \
        /DISTRIBUTIONSOURCE=sd011 /SILENT /PASSWORD=TB38GF9P66"
    ${EndIf}
    
  ${EndIf}
  
  SetDetailsPrint both
  
FunctionEnd

!endif
