/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * THIS FILE IS GENERATED DYNAMICALLY FROM KeyTypes.h BY mkspecialkeynames.pl
 */

#ifndef SPECIALKEYNAMEMAP_H
#define SPECIALKEYNAMEMAP_H

#include "KeyTypes.h"
#include "CString.h"
#include "stdmap.h"

#define mkModifierStdMap(mapname) \
	mapname["AltGr"] = KeyModifierAltGr; \
	mapname["Super"] = KeyModifierSuper; \
	mapname["NumLock"] = KeyModifierNumLock; \
	mapname["Shift"] = KeyModifierShift; \
	mapname["Meta"] = KeyModifierMeta; \
	mapname["Control"] = KeyModifierControl; \
	mapname["ScrollLock"] = KeyModifierScrollLock; \
	mapname["Alt"] = KeyModifierAlt; \
	mapname["CapsLock"] = KeyModifierCapsLock;

#define mkSpecialKeyStdMap(mapname) \
	mapname["F16"] = kKeyF16; \
	mapname["Sleep"] = kKeySleep; \
	mapname["F33"] = kKeyF33; \
	mapname["F15"] = kKeyF15; \
	mapname["KP_F2"] = kKeyKP_F2; \
	mapname["F21"] = kKeyF21; \
	mapname["Cancel"] = kKeyCancel; \
	mapname["KP_Delete"] = kKeyKP_Delete; \
	mapname["KP_0"] = kKeyKP_0; \
	mapname["ShiftLock"] = kKeyShiftLock; \
	mapname["NumLock"] = kKeyNumLock; \
	mapname["F27"] = kKeyF27; \
	mapname["F3"] = kKeyF3; \
	mapname["F31"] = kKeyF31; \
	mapname["CapsLock"] = kKeyCapsLock; \
	mapname["KP_6"] = kKeyKP_6; \
	mapname["PageDown"] = kKeyPageDown; \
	mapname["KP_F4"] = kKeyKP_F4; \
	mapname["AudioNext"] = kKeyAudioNext; \
	mapname["Eject"] = kKeyEject; \
	mapname["WWWHome"] = kKeyWWWHome; \
	mapname["Print"] = kKeyPrint; \
	mapname["Clear"] = kKeyClear; \
	mapname["End"] = kKeyEnd; \
	mapname["AudioUp"] = kKeyAudioUp; \
	mapname["F35"] = kKeyF35; \
	mapname["Tab"] = kKeyTab; \
	mapname["Meta_L"] = kKeyMeta_L; \
	mapname["KP_Right"] = kKeyKP_Right; \
	mapname["F29"] = kKeyF29; \
	mapname["F24"] = kKeyF24; \
	mapname["KP_4"] = kKeyKP_4; \
	mapname["Super_R"] = kKeySuper_R; \
	mapname["Begin"] = kKeyBegin; \
	mapname["F6"] = kKeyF6; \
	mapname["SysReq"] = kKeySysReq; \
	mapname["KP_Left"] = kKeyKP_Left; \
	mapname["Hyper_R"] = kKeyHyper_R; \
	mapname["F17"] = kKeyF17; \
	mapname["KP_Subtract"] = kKeyKP_Subtract; \
	mapname["KP_3"] = kKeyKP_3; \
	mapname["Super_L"] = kKeySuper_L; \
	mapname["Home"] = kKeyHome; \
	mapname["KP_5"] = kKeyKP_5; \
	mapname["Pause"] = kKeyPause; \
	mapname["F7"] = kKeyF7; \
	mapname["AppMail"] = kKeyAppMail; \
	mapname["Menu"] = kKeyMenu; \
	mapname["AudioPlay"] = kKeyAudioPlay; \
	mapname["None"] = kKeyNone; \
	mapname["KP_Tab"] = kKeyKP_Tab; \
	mapname["AltGr"] = kKeyAltGr; \
	mapname["Linefeed"] = kKeyLinefeed; \
	mapname["F9"] = kKeyF9; \
	mapname["AudioPrev"] = kKeyAudioPrev; \
	mapname["Alt_R"] = kKeyAlt_R; \
	mapname["F10"] = kKeyF10; \
	mapname["KP_Add"] = kKeyKP_Add; \
	mapname["Zenkaku"] = kKeyZenkaku; \
	mapname["F13"] = kKeyF13; \
	mapname["Right"] = kKeyRight; \
	mapname["Find"] = kKeyFind; \
	mapname["WWWSearch"] = kKeyWWWSearch; \
	mapname["Control_L"] = kKeyControl_L; \
	mapname["Execute"] = kKeyExecute; \
	mapname["WWWForward"] = kKeyWWWForward; \
	mapname["Alt_L"] = kKeyAlt_L; \
	mapname["KP_Multiply"] = kKeyKP_Multiply; \
	mapname["Return"] = kKeyReturn; \
	mapname["F23"] = kKeyF23; \
	mapname["KP_9"] = kKeyKP_9; \
	mapname["F19"] = kKeyF19; \
	mapname["KP_8"] = kKeyKP_8; \
	mapname["F5"] = kKeyF5; \
	mapname["AppUser2"] = kKeyAppUser2; \
	mapname["KP_Space"] = kKeyKP_Space; \
	mapname["Henkan"] = kKeyHenkan; \
	mapname["KP_Divide"] = kKeyKP_Divide; \
	mapname["F32"] = kKeyF32; \
	mapname["Hyper_L"] = kKeyHyper_L; \
	mapname["F26"] = kKeyF26; \
	mapname["Delete"] = kKeyDelete; \
	mapname["AudioDown"] = kKeyAudioDown; \
	mapname["F8"] = kKeyF8; \
	mapname["KP_Enter"] = kKeyKP_Enter; \
	mapname["KP_Down"] = kKeyKP_Down; \
	mapname["KP_Insert"] = kKeyKP_Insert; \
	mapname["F30"] = kKeyF30; \
	mapname["BackSpace"] = kKeyBackSpace; \
	mapname["F25"] = kKeyF25; \
	mapname["WWWRefresh"] = kKeyWWWRefresh; \
	mapname["KP_PageUp"] = kKeyKP_PageUp; \
	mapname["F34"] = kKeyF34; \
	mapname["KP_Separator"] = kKeyKP_Separator; \
	mapname["KP_Equal"] = kKeyKP_Equal; \
	mapname["KP_F3"] = kKeyKP_F3; \
	mapname["Undo"] = kKeyUndo; \
	mapname["F2"] = kKeyF2; \
	mapname["ScrollLock"] = kKeyScrollLock; \
	mapname["Select"] = kKeySelect; \
	mapname["KP_2"] = kKeyKP_2; \
	mapname["KP_Decimal"] = kKeyKP_Decimal; \
	mapname["KP_Home"] = kKeyKP_Home; \
	mapname["AudioStop"] = kKeyAudioStop; \
	mapname["F28"] = kKeyF28; \
	mapname["LeftTab"] = kKeyLeftTab; \
	mapname["Shift_L"] = kKeyShift_L; \
	mapname["WWWStop"] = kKeyWWWStop; \
	mapname["Insert"] = kKeyInsert; \
	mapname["KP_Begin"] = kKeyKP_Begin; \
	mapname["AppUser1"] = kKeyAppUser1; \
	mapname["Break"] = kKeyBreak; \
	mapname["KP_End"] = kKeyKP_End; \
	mapname["Shift_R"] = kKeyShift_R; \
	mapname["Meta_R"] = kKeyMeta_R; \
	mapname["Redo"] = kKeyRedo; \
	mapname["Control_R"] = kKeyControl_R; \
	mapname["KP_7"] = kKeyKP_7; \
	mapname["Left"] = kKeyLeft; \
	mapname["F1"] = kKeyF1; \
	mapname["KP_Up"] = kKeyKP_Up; \
	mapname["WWWFavorites"] = kKeyWWWFavorites; \
	mapname["PageUp"] = kKeyPageUp; \
	mapname["F4"] = kKeyF4; \
	mapname["KP_PageDown"] = kKeyKP_PageDown; \
	mapname["Down"] = kKeyDown; \
	mapname["KP_1"] = kKeyKP_1; \
	mapname["KP_F1"] = kKeyKP_F1; \
	mapname["F22"] = kKeyF22; \
	mapname["F14"] = kKeyF14; \
	mapname["WWWBack"] = kKeyWWWBack; \
	mapname["F18"] = kKeyF18; \
	mapname["F20"] = kKeyF20; \
	mapname["Escape"] = kKeyEscape; \
	mapname["F11"] = kKeyF11; \
	mapname["AppMedia"] = kKeyAppMedia; \
	mapname["F12"] = kKeyF12; \
	mapname["Up"] = kKeyUp; \
	mapname["AudioMute"] = kKeyAudioMute; \
	mapname["Help"] = kKeyHelp;

#define mkModifierNameStdMap(mapname) \
	mapname[KeyModifierAltGr] = "AltGr"; \
	mapname[KeyModifierSuper] = "Super"; \
	mapname[KeyModifierNumLock] = "NumLock"; \
	mapname[KeyModifierShift] = "Shift"; \
	mapname[KeyModifierMeta] = "Meta"; \
	mapname[KeyModifierControl] = "Control"; \
	mapname[KeyModifierScrollLock] = "ScrollLock"; \
	mapname[KeyModifierAlt] = "Alt"; \
	mapname[KeyModifierCapsLock] = "CapsLock";

#define mkSpecialKeyNameStdMap(mapname) \
	mapname[kKeyF16] = "F16"; \
	mapname[kKeySleep] = "Sleep"; \
	mapname[kKeyF33] = "F33"; \
	mapname[kKeyF15] = "F15"; \
	mapname[kKeyKP_F2] = "KP_F2"; \
	mapname[kKeyF21] = "F21"; \
	mapname[kKeyCancel] = "Cancel"; \
	mapname[kKeyKP_Delete] = "KP_Delete"; \
	mapname[kKeyKP_0] = "KP_0"; \
	mapname[kKeyShiftLock] = "ShiftLock"; \
	mapname[kKeyNumLock] = "NumLock"; \
	mapname[kKeyF27] = "F27"; \
	mapname[kKeyF3] = "F3"; \
	mapname[kKeyF31] = "F31"; \
	mapname[kKeyCapsLock] = "CapsLock"; \
	mapname[kKeyKP_6] = "KP_6"; \
	mapname[kKeyPageDown] = "PageDown"; \
	mapname[kKeyKP_F4] = "KP_F4"; \
	mapname[kKeyAudioNext] = "AudioNext"; \
	mapname[kKeyEject] = "Eject"; \
	mapname[kKeyWWWHome] = "WWWHome"; \
	mapname[kKeyPrint] = "Print"; \
	mapname[kKeyClear] = "Clear"; \
	mapname[kKeyEnd] = "End"; \
	mapname[kKeyAudioUp] = "AudioUp"; \
	mapname[kKeyF35] = "F35"; \
	mapname[kKeyTab] = "Tab"; \
	mapname[kKeyMeta_L] = "Meta_L"; \
	mapname[kKeyKP_Right] = "KP_Right"; \
	mapname[kKeyF29] = "F29"; \
	mapname[kKeyF24] = "F24"; \
	mapname[kKeyKP_4] = "KP_4"; \
	mapname[kKeySuper_R] = "Super_R"; \
	mapname[kKeyBegin] = "Begin"; \
	mapname[kKeyF6] = "F6"; \
	mapname[kKeySysReq] = "SysReq"; \
	mapname[kKeyKP_Left] = "KP_Left"; \
	mapname[kKeyHyper_R] = "Hyper_R"; \
	mapname[kKeyF17] = "F17"; \
	mapname[kKeyKP_Subtract] = "KP_Subtract"; \
	mapname[kKeyKP_3] = "KP_3"; \
	mapname[kKeySuper_L] = "Super_L"; \
	mapname[kKeyHome] = "Home"; \
	mapname[kKeyKP_5] = "KP_5"; \
	mapname[kKeyPause] = "Pause"; \
	mapname[kKeyF7] = "F7"; \
	mapname[kKeyAppMail] = "AppMail"; \
	mapname[kKeyMenu] = "Menu"; \
	mapname[kKeyAudioPlay] = "AudioPlay"; \
	mapname[kKeyNone] = "None"; \
	mapname[kKeyKP_Tab] = "KP_Tab"; \
	mapname[kKeyAltGr] = "AltGr"; \
	mapname[kKeyLinefeed] = "Linefeed"; \
	mapname[kKeyF9] = "F9"; \
	mapname[kKeyAudioPrev] = "AudioPrev"; \
	mapname[kKeyAlt_R] = "Alt_R"; \
	mapname[kKeyF10] = "F10"; \
	mapname[kKeyKP_Add] = "KP_Add"; \
	mapname[kKeyZenkaku] = "Zenkaku"; \
	mapname[kKeyF13] = "F13"; \
	mapname[kKeyRight] = "Right"; \
	mapname[kKeyFind] = "Find"; \
	mapname[kKeyWWWSearch] = "WWWSearch"; \
	mapname[kKeyControl_L] = "Control_L"; \
	mapname[kKeyExecute] = "Execute"; \
	mapname[kKeyWWWForward] = "WWWForward"; \
	mapname[kKeyAlt_L] = "Alt_L"; \
	mapname[kKeyKP_Multiply] = "KP_Multiply"; \
	mapname[kKeyReturn] = "Return"; \
	mapname[kKeyF23] = "F23"; \
	mapname[kKeyKP_9] = "KP_9"; \
	mapname[kKeyF19] = "F19"; \
	mapname[kKeyKP_8] = "KP_8"; \
	mapname[kKeyF5] = "F5"; \
	mapname[kKeyAppUser2] = "AppUser2"; \
	mapname[kKeyKP_Space] = "KP_Space"; \
	mapname[kKeyHenkan] = "Henkan"; \
	mapname[kKeyKP_Divide] = "KP_Divide"; \
	mapname[kKeyF32] = "F32"; \
	mapname[kKeyHyper_L] = "Hyper_L"; \
	mapname[kKeyF26] = "F26"; \
	mapname[kKeyDelete] = "Delete"; \
	mapname[kKeyAudioDown] = "AudioDown"; \
	mapname[kKeyF8] = "F8"; \
	mapname[kKeyKP_Enter] = "KP_Enter"; \
	mapname[kKeyKP_Down] = "KP_Down"; \
	mapname[kKeyKP_Insert] = "KP_Insert"; \
	mapname[kKeyF30] = "F30"; \
	mapname[kKeyBackSpace] = "BackSpace"; \
	mapname[kKeyF25] = "F25"; \
	mapname[kKeyWWWRefresh] = "WWWRefresh"; \
	mapname[kKeyKP_PageUp] = "KP_PageUp"; \
	mapname[kKeyF34] = "F34"; \
	mapname[kKeyKP_Separator] = "KP_Separator"; \
	mapname[kKeyKP_Equal] = "KP_Equal"; \
	mapname[kKeyKP_F3] = "KP_F3"; \
	mapname[kKeyUndo] = "Undo"; \
	mapname[kKeyF2] = "F2"; \
	mapname[kKeyScrollLock] = "ScrollLock"; \
	mapname[kKeySelect] = "Select"; \
	mapname[kKeyKP_2] = "KP_2"; \
	mapname[kKeyKP_Decimal] = "KP_Decimal"; \
	mapname[kKeyKP_Home] = "KP_Home"; \
	mapname[kKeyAudioStop] = "AudioStop"; \
	mapname[kKeyF28] = "F28"; \
	mapname[kKeyLeftTab] = "LeftTab"; \
	mapname[kKeyShift_L] = "Shift_L"; \
	mapname[kKeyWWWStop] = "WWWStop"; \
	mapname[kKeyInsert] = "Insert"; \
	mapname[kKeyKP_Begin] = "KP_Begin"; \
	mapname[kKeyAppUser1] = "AppUser1"; \
	mapname[kKeyBreak] = "Break"; \
	mapname[kKeyKP_End] = "KP_End"; \
	mapname[kKeyShift_R] = "Shift_R"; \
	mapname[kKeyMeta_R] = "Meta_R"; \
	mapname[kKeyRedo] = "Redo"; \
	mapname[kKeyControl_R] = "Control_R"; \
	mapname[kKeyKP_7] = "KP_7"; \
	mapname[kKeyLeft] = "Left"; \
	mapname[kKeyF1] = "F1"; \
	mapname[kKeyKP_Up] = "KP_Up"; \
	mapname[kKeyWWWFavorites] = "WWWFavorites"; \
	mapname[kKeyPageUp] = "PageUp"; \
	mapname[kKeyF4] = "F4"; \
	mapname[kKeyKP_PageDown] = "KP_PageDown"; \
	mapname[kKeyDown] = "Down"; \
	mapname[kKeyKP_1] = "KP_1"; \
	mapname[kKeyKP_F1] = "KP_F1"; \
	mapname[kKeyF22] = "F22"; \
	mapname[kKeyF14] = "F14"; \
	mapname[kKeyWWWBack] = "WWWBack"; \
	mapname[kKeyF18] = "F18"; \
	mapname[kKeyF20] = "F20"; \
	mapname[kKeyEscape] = "Escape"; \
	mapname[kKeyF11] = "F11"; \
	mapname[kKeyAppMedia] = "AppMedia"; \
	mapname[kKeyF12] = "F12"; \
	mapname[kKeyUp] = "Up"; \
	mapname[kKeyAudioMute] = "AudioMute"; \
	mapname[kKeyHelp] = "Help";

#endif
