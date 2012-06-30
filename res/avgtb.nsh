var avgDialog
var avgLabel
var avgStandardLabel
var avgLicenseLabel
var avgStandardRadio
var avgCustomRadio
var avgToolbarCheck
var avgSearchCheck
var avgEulaLink
var avgPrivacyLink
var avgToolbarInstalled
var avgInstallAll
var avgInstallToolbar
var avgInstallSearch

!include "nsDialogs.nsh"

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

  !insertmacro MUI_HEADER_TEXT "AVG" "Customize AVG Security Toolbar options."
  
  nsDialogs::Create 1018
  Pop $avgDialog

  ${NSD_CreateLabel} 0 0 100% 30u \
    "Thanks for choosing Synergy. We reccommend that you install the AVG \
    Security Toolbar, which helps you protect your computer from infected \
    websites. The toolbar is easy to uninstall later if you change your mind."
  Pop $avgLabel
  
  ${NSD_CreateRadioButton} 0 35u 80u 10u "&Standard"
  Pop $avgStandardRadio
  ${NSD_Check} $avgStandardRadio
  ${NSD_OnClick} $avgStandardRadio avgRadioClick
  
  ${NSD_CreateLabel} 10u 45u 95% 20u \
    "Install the AVG Security Toolbar. Set and protect AVG Secure Search \
    as my homepage and default search provider."
  Pop $avgStandardLabel
  
  ${NSD_CreateRadioButton} 0u 65u 100% 10u "&Custom"
  Pop $avgCustomRadio
  ${NSD_OnClick} $avgCustomRadio avgRadioClick
  
  ${NSD_CreateCheckBox} 10u 75u 100% 10u "Install the AVG Security &Toolbar."
  Pop $avgToolbarCheck
  ${NSD_Check} $avgToolbarCheck
  EnableWindow $avgToolbarCheck 0
  ${NSD_OnClick} $avgToolbarCheck avgCheckboxClick
  
  ${NSD_CreateCheckBox} 10u 85u 100% 10u \
    "Set and protect AVG Secure Search as my &homepage and default search \
    provider."
  Pop $avgSearchCheck
  ${NSD_Check} $avgSearchCheck
  EnableWindow $avgSearchCheck 0
  ${NSD_OnClick} $avgSearchCheck avgCheckboxClick

  ${NSD_CreateLabel} 0 105u 100% 10u \
    "By clicking $\"Next$\" you agree to the AVG End User License Agreement and \
    Privacy Policy."
  Pop $avgLicenseLabel
  
  ${NSD_CreateLink} 10u 115u 100% 10u "AVG End User License Agreement"
  Pop $avgEulaLink
  ${NSD_OnClick} $avgEulaLink avgEulaLinkClick
  
  ${NSD_CreateLink} 10u 125u 100% 10u "AVG Privacy Policy"
  Pop $avgPrivacyLink
  ${NSD_OnClick} $avgPrivacyLink avgPrivacyLinkClick

  nsDialogs::Show
  
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
    EnableWindow $avgStandardLabel 0
  ${Else}
    EnableWindow $avgToolbarCheck 0
    EnableWindow $avgSearchCheck 0
    EnableWindow $avgStandardLabel 1
  ${EndIf}

FunctionEnd

Function avgCheckboxClick
  
  ${NSD_GetState} $avgToolbarCheck $0
  ${NSD_GetState} $avgSearchCheck $1
  
  ${If} $0 == 0
  ${AndIf} $1 == 0
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
  
  ${NSD_GetState} $avgStandardRadio $R0
  ${NSD_GetState} $avgCustomRadio $R1
  ${NSD_GetState} $avgToolbarCheck $R2
  ${NSD_GetState} $avgSearchCheck $R3
  
  StrCpy $avgInstallAll 0
  StrCpy $avgInstallToolbar 0
  StrCpy $avgInstallSearch 0
  
  ${If} $R0 == 1
    StrCpy $avgInstallAll 1
  ${ElseIf} $R1 == 1
    ${If} $R2 == 1
      StrCpy $avgInstallToolbar 1
    ${EndIf}
    ${If} $R3 == 1
      StrCpy $avgInstallSearch 1
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
    File "..\res\avgtb.exe"
  ${Else}
    Return
  ${EndIf}
  
  ${If} $avgInstallAll == 1
    Exec \
      "avgtb.exe /INSTALL /ENABLEDSP /ENABLEHOMEPAGE /LOCAL=us /PROFILE=SATB \
      /DISTRIBUTIONSOURCE=TBD /SILENT /PASSWORD=TB38GF9P66"
  ${Else}
    
    ${If} $avgInstallToolbar == 1
      Exec \
        "avgtb.exe /INSTALL /LOCAL=us /PROFILE=SATB \
        /DISTRIBUTIONSOURCE=TBD /SILENT /PASSWORD=TB38GF9P66"
    ${EndIf}
    
    ${If} $avgInstallSearch == 1
      Exec \
        "avgtb.exe /ENABLEDSP /ENABLEHOMEPAGE /LOCAL=us /PROFILE=SATB \
        /DISTRIBUTIONSOURCE=TBD /SILENT /PASSWORD=TB38GF9P66"
    ${EndIf}
    
  ${EndIf}
  
FunctionEnd
