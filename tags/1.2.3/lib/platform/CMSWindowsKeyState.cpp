/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#include "CMSWindowsKeyState.h"
#include "CMSWindowsDesks.h"
#include "CThread.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CStringUtil.h"
#include "CArchMiscWindows.h"

// extended mouse buttons
#if !defined(VK_XBUTTON1)
#define VK_XBUTTON1				0x05
#define VK_XBUTTON2				0x06
#endif

// multimedia keys
#if !defined(VK_BROWSER_BACK)
#define VK_BROWSER_BACK			0xA6
#define VK_BROWSER_FORWARD		0xA7
#define VK_BROWSER_REFRESH		0xA8
#define VK_BROWSER_STOP			0xA9
#define VK_BROWSER_SEARCH		0xAA
#define VK_BROWSER_FAVORITES	0xAB
#define VK_BROWSER_HOME			0xAC
#define VK_VOLUME_MUTE			0xAD
#define VK_VOLUME_DOWN			0xAE
#define VK_VOLUME_UP			0xAF
#define VK_MEDIA_NEXT_TRACK		0xB0
#define VK_MEDIA_PREV_TRACK		0xB1
#define VK_MEDIA_STOP			0xB2
#define VK_MEDIA_PLAY_PAUSE		0xB3
#define VK_LAUNCH_MAIL			0xB4
#define VK_LAUNCH_MEDIA_SELECT	0xB5
#define VK_LAUNCH_APP1			0xB6
#define VK_LAUNCH_APP2			0xB7
#endif
#if !defined(VK_SLEEP)
#define VK_SLEEP				0x5F
#endif

//
// CMSWindowsKeyState
//

const char*				CMSWindowsKeyState::s_vkToName[] =
{
	"vk 0x00",
	"Left Button",
	"Right Button",
	"VK_CANCEL",
	"Middle Button",
	"vk 0x05",
	"vk 0x06",
	"vk 0x07",
	"VK_BACK",
	"VK_TAB",
	"vk 0x0a",
	"vk 0x0b",
	"VK_CLEAR",
	"VK_RETURN",
	"vk 0x0e",
	"vk 0x0f",
	"VK_SHIFT",
	"VK_CONTROL",
	"VK_MENU",
	"VK_PAUSE",
	"VK_CAPITAL",
	"VK_KANA",
	"vk 0x16",
	"VK_JUNJA",
	"VK_FINAL",
	"VK_KANJI",
	"vk 0x1a",
	"VK_ESCAPE",
	"VK_CONVERT",
	"VK_NONCONVERT",
	"VK_ACCEPT",
	"VK_MODECHANGE",
	"VK_SPACE",
	"VK_PRIOR",
	"VK_NEXT",
	"VK_END",
	"VK_HOME",
	"VK_LEFT",
	"VK_UP",
	"VK_RIGHT",
	"VK_DOWN",
	"VK_SELECT",
	"VK_PRINT",
	"VK_EXECUTE",
	"VK_SNAPSHOT",
	"VK_INSERT",
	"VK_DELETE",
	"VK_HELP",
	"VK_0",
	"VK_1",
	"VK_2",
	"VK_3",
	"VK_4",
	"VK_5",
	"VK_6",
	"VK_7",
	"VK_8",
	"VK_9",
	"vk 0x3a",
	"vk 0x3b",
	"vk 0x3c",
	"vk 0x3d",
	"vk 0x3e",
	"vk 0x3f",
	"vk 0x40",
	"VK_A",
	"VK_B",
	"VK_C",
	"VK_D",
	"VK_E",
	"VK_F",
	"VK_G",
	"VK_H",
	"VK_I",
	"VK_J",
	"VK_K",
	"VK_L",
	"VK_M",
	"VK_N",
	"VK_O",
	"VK_P",
	"VK_Q",
	"VK_R",
	"VK_S",
	"VK_T",
	"VK_U",
	"VK_V",
	"VK_W",
	"VK_X",
	"VK_Y",
	"VK_Z",
	"VK_LWIN",
	"VK_RWIN",
	"VK_APPS",
	"vk 0x5e",
	"VK_SLEEP",
	"VK_NUMPAD0",
	"VK_NUMPAD1",
	"VK_NUMPAD2",
	"VK_NUMPAD3",
	"VK_NUMPAD4",
	"VK_NUMPAD5",
	"VK_NUMPAD6",
	"VK_NUMPAD7",
	"VK_NUMPAD8",
	"VK_NUMPAD9",
	"VK_MULTIPLY",
	"VK_ADD",
	"VK_SEPARATOR",
	"VK_SUBTRACT",
	"VK_DECIMAL",
	"VK_DIVIDE",
	"VK_F1",
	"VK_F2",
	"VK_F3",
	"VK_F4",
	"VK_F5",
	"VK_F6",
	"VK_F7",
	"VK_F8",
	"VK_F9",
	"VK_F10",
	"VK_F11",
	"VK_F12",
	"VK_F13",
	"VK_F14",
	"VK_F15",
	"VK_F16",
	"VK_F17",
	"VK_F18",
	"VK_F19",
	"VK_F20",
	"VK_F21",
	"VK_F22",
	"VK_F23",
	"VK_F24",
	"vk 0x88",
	"vk 0x89",
	"vk 0x8a",
	"vk 0x8b",
	"vk 0x8c",
	"vk 0x8d",
	"vk 0x8e",
	"vk 0x8f",
	"VK_NUMLOCK",
	"VK_SCROLL",
	"vk 0x92",
	"vk 0x93",
	"vk 0x94",
	"vk 0x95",
	"vk 0x96",
	"vk 0x97",
	"vk 0x98",
	"vk 0x99",
	"vk 0x9a",
	"vk 0x9b",
	"vk 0x9c",
	"vk 0x9d",
	"vk 0x9e",
	"vk 0x9f",
	"VK_LSHIFT",
	"VK_RSHIFT",
	"VK_LCONTROL",
	"VK_RCONTROL",
	"VK_LMENU",
	"VK_RMENU",
	"VK_BROWSER_BACK",
	"VK_BROWSER_FORWARD",
	"VK_BROWSER_REFRESH",
	"VK_BROWSER_STOP",
	"VK_BROWSER_SEARCH",
	"VK_BROWSER_FAVORITES",
	"VK_BROWSER_HOME",
	"VK_VOLUME_MUTE",
	"VK_VOLUME_DOWN",
	"VK_VOLUME_UP",
	"VK_MEDIA_NEXT_TRACK",
	"VK_MEDIA_PREV_TRACK",
	"VK_MEDIA_STOP",
	"VK_MEDIA_PLAY_PAUSE",
	"VK_LAUNCH_MAIL",
	"VK_LAUNCH_MEDIA_SELECT",
	"VK_LAUNCH_APP1",
	"VK_LAUNCH_APP2",
	"vk 0xb8",
	"vk 0xb9",
	"vk 0xba",
	"vk 0xbb",
	"vk 0xbc",
	"vk 0xbd",
	"vk 0xbe",
	"vk 0xbf",
	"vk 0xc0",
	"vk 0xc1",
	"vk 0xc2",
	"vk 0xc3",
	"vk 0xc4",
	"vk 0xc5",
	"vk 0xc6",
	"vk 0xc7",
	"vk 0xc8",
	"vk 0xc9",
	"vk 0xca",
	"vk 0xcb",
	"vk 0xcc",
	"vk 0xcd",
	"vk 0xce",
	"vk 0xcf",
	"vk 0xd0",
	"vk 0xd1",
	"vk 0xd2",
	"vk 0xd3",
	"vk 0xd4",
	"vk 0xd5",
	"vk 0xd6",
	"vk 0xd7",
	"vk 0xd8",
	"vk 0xd9",
	"vk 0xda",
	"vk 0xdb",
	"vk 0xdc",
	"vk 0xdd",
	"vk 0xde",
	"vk 0xdf",
	"vk 0xe0",
	"vk 0xe1",
	"vk 0xe2",
	"vk 0xe3",
	"vk 0xe4",
	"VK_PROCESSKEY",
	"vk 0xe6",
	"vk 0xe7",
	"vk 0xe8",
	"vk 0xe9",
	"vk 0xea",
	"vk 0xeb",
	"vk 0xec",
	"vk 0xed",
	"vk 0xee",
	"vk 0xef",
	"vk 0xf0",
	"vk 0xf1",
	"vk 0xf2",
	"vk 0xf3",
	"vk 0xf4",
	"vk 0xf5",
	"VK_ATTN",
	"VK_CRSEL",
	"VK_EXSEL",
	"VK_EREOF",
	"VK_PLAY",
	"VK_ZOOM",
	"VK_NONAME",
	"VK_PA1",
	"VK_OEM_CLEAR",
	"vk 0xff"
};

// map virtual keys to synergy key enumeration
const KeyID				CMSWindowsKeyState::s_virtualKey[][2] =
{
	/* 0x00 */ { kKeyNone,			kKeyNone },		// reserved
	/* 0x01 */ { kKeyNone,			kKeyNone },		// VK_LBUTTON
	/* 0x02 */ { kKeyNone,			kKeyNone },		// VK_RBUTTON
	/* 0x03 */ { kKeyNone,			kKeyBreak },	// VK_CANCEL
	/* 0x04 */ { kKeyNone,			kKeyNone },		// VK_MBUTTON
	/* 0x05 */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x06 */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x07 */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x08 */ { kKeyBackSpace,		kKeyNone },		// VK_BACK
	/* 0x09 */ { kKeyTab,			kKeyNone },		// VK_TAB
	/* 0x0a */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x0b */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x0c */ { kKeyClear,			kKeyClear },	// VK_CLEAR
	/* 0x0d */ { kKeyReturn,		kKeyKP_Enter },	// VK_RETURN
	/* 0x0e */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x0f */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x10 */ { kKeyShift_L,		kKeyShift_R },	// VK_SHIFT
	/* 0x11 */ { kKeyControl_L,		kKeyControl_R },// VK_CONTROL
	/* 0x12 */ { kKeyAlt_L,			kKeyAlt_R },	// VK_MENU
	/* 0x13 */ { kKeyPause,			kKeyNone },		// VK_PAUSE
	/* 0x14 */ { kKeyCapsLock,		kKeyNone },		// VK_CAPITAL
	/* 0x15 */ { kKeyNone,			kKeyNone },		// VK_KANA			
	/* 0x16 */ { kKeyNone,			kKeyNone },		// VK_HANGUL		
	/* 0x17 */ { kKeyNone,			kKeyNone },		// VK_JUNJA			
	/* 0x18 */ { kKeyNone,			kKeyNone },		// VK_FINAL			
	/* 0x19 */ { kKeyZenkaku,		kKeyNone },		// VK_KANJI			
	/* 0x1a */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x1b */ { kKeyEscape,		kKeyNone },		// VK_ESCAPE
	/* 0x1c */ { kKeyHenkan,		kKeyNone },		// VK_CONVERT		
	/* 0x1d */ { kKeyNone,			kKeyNone },		// VK_NONCONVERT	
	/* 0x1e */ { kKeyNone,			kKeyNone },		// VK_ACCEPT		
	/* 0x1f */ { kKeyNone,			kKeyNone },		// VK_MODECHANGE	
	/* 0x20 */ { kKeyNone,			kKeyNone },		// VK_SPACE
	/* 0x21 */ { kKeyKP_PageUp,		kKeyPageUp },	// VK_PRIOR
	/* 0x22 */ { kKeyKP_PageDown,	kKeyPageDown },	// VK_NEXT
	/* 0x23 */ { kKeyKP_End,		kKeyEnd },		// VK_END
	/* 0x24 */ { kKeyKP_Home,		kKeyHome },		// VK_HOME
	/* 0x25 */ { kKeyKP_Left,		kKeyLeft },		// VK_LEFT
	/* 0x26 */ { kKeyKP_Up,			kKeyUp },		// VK_UP
	/* 0x27 */ { kKeyKP_Right,		kKeyRight },	// VK_RIGHT
	/* 0x28 */ { kKeyKP_Down,		kKeyDown },		// VK_DOWN
	/* 0x29 */ { kKeySelect,		kKeySelect },	// VK_SELECT
	/* 0x2a */ { kKeyNone,			kKeyNone },		// VK_PRINT
	/* 0x2b */ { kKeyExecute,		kKeyExecute },	// VK_EXECUTE
	/* 0x2c */ { kKeyPrint,			kKeyPrint },	// VK_SNAPSHOT
	/* 0x2d */ { kKeyKP_Insert,		kKeyInsert },	// VK_INSERT
	/* 0x2e */ { kKeyKP_Delete,		kKeyDelete },	// VK_DELETE
	/* 0x2f */ { kKeyHelp,			kKeyHelp },		// VK_HELP
	/* 0x30 */ { kKeyNone,			kKeyNone },		// VK_0
	/* 0x31 */ { kKeyNone,			kKeyNone },		// VK_1
	/* 0x32 */ { kKeyNone,			kKeyNone },		// VK_2
	/* 0x33 */ { kKeyNone,			kKeyNone },		// VK_3
	/* 0x34 */ { kKeyNone,			kKeyNone },		// VK_4
	/* 0x35 */ { kKeyNone,			kKeyNone },		// VK_5
	/* 0x36 */ { kKeyNone,			kKeyNone },		// VK_6
	/* 0x37 */ { kKeyNone,			kKeyNone },		// VK_7
	/* 0x38 */ { kKeyNone,			kKeyNone },		// VK_8
	/* 0x39 */ { kKeyNone,			kKeyNone },		// VK_9
	/* 0x3a */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x3b */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x3c */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x3d */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x3e */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x3f */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x40 */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x41 */ { kKeyNone,			kKeyNone },		// VK_A
	/* 0x42 */ { kKeyNone,			kKeyNone },		// VK_B
	/* 0x43 */ { kKeyNone,			kKeyNone },		// VK_C
	/* 0x44 */ { kKeyNone,			kKeyNone },		// VK_D
	/* 0x45 */ { kKeyNone,			kKeyNone },		// VK_E
	/* 0x46 */ { kKeyNone,			kKeyNone },		// VK_F
	/* 0x47 */ { kKeyNone,			kKeyNone },		// VK_G
	/* 0x48 */ { kKeyNone,			kKeyNone },		// VK_H
	/* 0x49 */ { kKeyNone,			kKeyNone },		// VK_I
	/* 0x4a */ { kKeyNone,			kKeyNone },		// VK_J
	/* 0x4b */ { kKeyNone,			kKeyNone },		// VK_K
	/* 0x4c */ { kKeyNone,			kKeyNone },		// VK_L
	/* 0x4d */ { kKeyNone,			kKeyNone },		// VK_M
	/* 0x4e */ { kKeyNone,			kKeyNone },		// VK_N
	/* 0x4f */ { kKeyNone,			kKeyNone },		// VK_O
	/* 0x50 */ { kKeyNone,			kKeyNone },		// VK_P
	/* 0x51 */ { kKeyNone,			kKeyNone },		// VK_Q
	/* 0x52 */ { kKeyNone,			kKeyNone },		// VK_R
	/* 0x53 */ { kKeyNone,			kKeyNone },		// VK_S
	/* 0x54 */ { kKeyNone,			kKeyNone },		// VK_T
	/* 0x55 */ { kKeyNone,			kKeyNone },		// VK_U
	/* 0x56 */ { kKeyNone,			kKeyNone },		// VK_V
	/* 0x57 */ { kKeyNone,			kKeyNone },		// VK_W
	/* 0x58 */ { kKeyNone,			kKeyNone },		// VK_X
	/* 0x59 */ { kKeyNone,			kKeyNone },		// VK_Y
	/* 0x5a */ { kKeyNone,			kKeyNone },		// VK_Z
	/* 0x5b */ { kKeyNone,			kKeySuper_L },	// VK_LWIN
	/* 0x5c */ { kKeyNone,			kKeySuper_R },	// VK_RWIN
	/* 0x5d */ { kKeyMenu,			kKeyMenu },		// VK_APPS
	/* 0x5e */ { kKeyNone,			kKeyNone },		// undefined
	/* 0x5f */ { kKeySleep,			kKeyNone },		// VK_SLEEP
	/* 0x60 */ { kKeyKP_0,			kKeyNone },		// VK_NUMPAD0
	/* 0x61 */ { kKeyKP_1,			kKeyNone },		// VK_NUMPAD1
	/* 0x62 */ { kKeyKP_2,			kKeyNone },		// VK_NUMPAD2
	/* 0x63 */ { kKeyKP_3,			kKeyNone },		// VK_NUMPAD3
	/* 0x64 */ { kKeyKP_4,			kKeyNone },		// VK_NUMPAD4
	/* 0x65 */ { kKeyKP_5,			kKeyNone },		// VK_NUMPAD5
	/* 0x66 */ { kKeyKP_6,			kKeyNone },		// VK_NUMPAD6
	/* 0x67 */ { kKeyKP_7,			kKeyNone },		// VK_NUMPAD7
	/* 0x68 */ { kKeyKP_8,			kKeyNone },		// VK_NUMPAD8
	/* 0x69 */ { kKeyKP_9,			kKeyNone },		// VK_NUMPAD9
	/* 0x6a */ { kKeyKP_Multiply,	kKeyNone },		// VK_MULTIPLY
	/* 0x6b */ { kKeyKP_Add,		kKeyNone },		// VK_ADD
	/* 0x6c */ { kKeyKP_Separator,	kKeyKP_Separator },// VK_SEPARATOR
	/* 0x6d */ { kKeyKP_Subtract,	kKeyNone },		// VK_SUBTRACT
	/* 0x6e */ { kKeyKP_Decimal,	kKeyNone },		// VK_DECIMAL
	/* 0x6f */ { kKeyNone,			kKeyKP_Divide },// VK_DIVIDE
	/* 0x70 */ { kKeyF1,			kKeyNone },		// VK_F1
	/* 0x71 */ { kKeyF2,			kKeyNone },		// VK_F2
	/* 0x72 */ { kKeyF3,			kKeyNone },		// VK_F3
	/* 0x73 */ { kKeyF4,			kKeyNone },		// VK_F4
	/* 0x74 */ { kKeyF5,			kKeyNone },		// VK_F5
	/* 0x75 */ { kKeyF6,			kKeyNone },		// VK_F6
	/* 0x76 */ { kKeyF7,			kKeyNone },		// VK_F7
	/* 0x77 */ { kKeyF8,			kKeyNone },		// VK_F8
	/* 0x78 */ { kKeyF9,			kKeyNone },		// VK_F9
	/* 0x79 */ { kKeyF10,			kKeyNone },		// VK_F10
	/* 0x7a */ { kKeyF11,			kKeyNone },		// VK_F11
	/* 0x7b */ { kKeyF12,			kKeyNone },		// VK_F12
	/* 0x7c */ { kKeyF13,			kKeyF13 },		// VK_F13
	/* 0x7d */ { kKeyF14,			kKeyF14 },		// VK_F14
	/* 0x7e */ { kKeyF15,			kKeyF15 },		// VK_F15
	/* 0x7f */ { kKeyF16,			kKeyF16 },		// VK_F16
	/* 0x80 */ { kKeyF17,			kKeyF17 },		// VK_F17
	/* 0x81 */ { kKeyF18,			kKeyF18 },		// VK_F18
	/* 0x82 */ { kKeyF19,			kKeyF19 },		// VK_F19
	/* 0x83 */ { kKeyF20,			kKeyF20 },		// VK_F20
	/* 0x84 */ { kKeyF21,			kKeyF21 },		// VK_F21
	/* 0x85 */ { kKeyF22,			kKeyF22 },		// VK_F22
	/* 0x86 */ { kKeyF23,			kKeyF23 },		// VK_F23
	/* 0x87 */ { kKeyF24,			kKeyF24 },		// VK_F24
	/* 0x88 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x89 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x8a */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x8b */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x8c */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x8d */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x8e */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x8f */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x90 */ { kKeyNumLock,		kKeyNumLock },	// VK_NUMLOCK
	/* 0x91 */ { kKeyScrollLock,	kKeyNone },		// VK_SCROLL
	/* 0x92 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x93 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x94 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x95 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x96 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x97 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x98 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x99 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x9a */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x9b */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x9c */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x9d */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x9e */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0x9f */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xa0 */ { kKeyShift_L,		kKeyShift_L },	// VK_LSHIFT
	/* 0xa1 */ { kKeyShift_R,		kKeyShift_R },	// VK_RSHIFT
	/* 0xa2 */ { kKeyControl_L,		kKeyControl_L },// VK_LCONTROL
	/* 0xa3 */ { kKeyControl_R,		kKeyControl_R },// VK_RCONTROL
	/* 0xa4 */ { kKeyAlt_L,			kKeyAlt_L },	// VK_LMENU
	/* 0xa5 */ { kKeyAlt_R,			kKeyAlt_R },	// VK_RMENU
	/* 0xa6 */ { kKeyNone,			kKeyWWWBack },	// VK_BROWSER_BACK
	/* 0xa7 */ { kKeyNone,			kKeyWWWForward },// VK_BROWSER_FORWARD
	/* 0xa8 */ { kKeyNone,			kKeyWWWRefresh },// VK_BROWSER_REFRESH
	/* 0xa9 */ { kKeyNone,			kKeyWWWStop },	// VK_BROWSER_STOP
	/* 0xaa */ { kKeyNone,			kKeyWWWSearch },// VK_BROWSER_SEARCH
	/* 0xab */ { kKeyNone,			kKeyWWWFavorites },// VK_BROWSER_FAVORITES
	/* 0xac */ { kKeyNone,			kKeyWWWHome },	// VK_BROWSER_HOME
	/* 0xad */ { kKeyNone,			kKeyAudioMute },// VK_VOLUME_MUTE
	/* 0xae */ { kKeyNone,			kKeyAudioDown },// VK_VOLUME_DOWN
	/* 0xaf */ { kKeyNone,			kKeyAudioUp },	// VK_VOLUME_UP
	/* 0xb0 */ { kKeyNone,			kKeyAudioNext },// VK_MEDIA_NEXT_TRACK
	/* 0xb1 */ { kKeyNone,			kKeyAudioPrev },// VK_MEDIA_PREV_TRACK
	/* 0xb2 */ { kKeyNone,			kKeyAudioStop },// VK_MEDIA_STOP
	/* 0xb3 */ { kKeyNone,			kKeyAudioPlay },// VK_MEDIA_PLAY_PAUSE
	/* 0xb4 */ { kKeyNone,			kKeyAppMail },	// VK_LAUNCH_MAIL
	/* 0xb5 */ { kKeyNone,			kKeyAppMedia },	// VK_LAUNCH_MEDIA_SELECT
	/* 0xb6 */ { kKeyNone,			kKeyAppUser1 },	// VK_LAUNCH_APP1
	/* 0xb7 */ { kKeyNone,			kKeyAppUser2 },	// VK_LAUNCH_APP2
	/* 0xb8 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xb9 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xba */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xbb */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xbc */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xbd */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xbe */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xbf */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xc0 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xc1 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc2 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc3 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc4 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc5 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc6 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc7 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc8 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xc9 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xca */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xcb */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xcc */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xcd */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xce */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xcf */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd0 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd1 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd2 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd3 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd4 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd5 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd6 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd7 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd8 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xd9 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xda */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xdb */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xdc */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xdd */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xde */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xdf */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe0 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe1 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe2 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe3 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe4 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe5 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xe6 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xe7 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xe8 */ { kKeyNone,			kKeyNone },		// unassigned
	/* 0xe9 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xea */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xeb */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xec */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xed */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xee */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xef */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf0 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf1 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf2 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf3 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf4 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf5 */ { kKeyNone,			kKeyNone },		// OEM specific
	/* 0xf6 */ { kKeyNone,			kKeyNone },		// VK_ATTN			
	/* 0xf7 */ { kKeyNone,			kKeyNone },		// VK_CRSEL			
	/* 0xf8 */ { kKeyNone,			kKeyNone },		// VK_EXSEL			
	/* 0xf9 */ { kKeyNone,			kKeyNone },		// VK_EREOF			
	/* 0xfa */ { kKeyNone,			kKeyNone },		// VK_PLAY			
	/* 0xfb */ { kKeyNone,			kKeyNone },		// VK_ZOOM			
	/* 0xfc */ { kKeyNone,			kKeyNone },		// reserved
	/* 0xfd */ { kKeyNone,			kKeyNone },		// VK_PA1			
	/* 0xfe */ { kKeyNone,			kKeyNone },		// VK_OEM_CLEAR		
	/* 0xff */ { kKeyNone,			kKeyNone }		// reserved
};

// map special KeyID keys to virtual key codes plus whether or not
// the key maps to an extended scan code
const UINT				CMSWindowsKeyState::s_mapE000[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, VK_SLEEP | 0x100u,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0,
	/* 0xa4 */ 0, 0, VK_BROWSER_BACK | 0x100u, VK_BROWSER_FORWARD | 0x100u,
	/* 0xa8 */ VK_BROWSER_REFRESH | 0x100u, VK_BROWSER_STOP | 0x100u,
	/* 0xaa */ VK_BROWSER_SEARCH | 0x100u, VK_BROWSER_FAVORITES | 0x100u,
	/* 0xac */ VK_BROWSER_HOME | 0x100u, VK_VOLUME_MUTE | 0x100u,
	/* 0xae */ VK_VOLUME_DOWN | 0x100u, VK_VOLUME_UP | 0x100u,
	/* 0xb0 */ VK_MEDIA_NEXT_TRACK | 0x100u, VK_MEDIA_PREV_TRACK | 0x100u,
	/* 0xb2 */ VK_MEDIA_STOP | 0x100u, VK_MEDIA_PLAY_PAUSE | 0x100u,
	/* 0xb4 */ VK_LAUNCH_MAIL | 0x100u, VK_LAUNCH_MEDIA_SELECT | 0x100u,
	/* 0xb6 */ VK_LAUNCH_APP1 | 0x100u, VK_LAUNCH_APP2 | 0x100u,
	/* 0xb8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, 0
};
const UINT				CMSWindowsKeyState::s_mapEE00[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ VK_TAB, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xb0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xb8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, 0
};
/* in g_mapEF00, 0xac is VK_DECIMAL not VK_SEPARATOR because win32
 * doesn't seem to use VK_SEPARATOR but instead maps VK_DECIMAL to
 * the same meaning. */
const UINT				CMSWindowsKeyState::s_mapEF00[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,
	/* 0x10 */ 0, 0, 0, VK_PAUSE, VK_SCROLL, 0/*sys-req*/, 0, 0,
	/* 0x18 */ 0, 0, 0, VK_ESCAPE, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, VK_CONVERT, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, VK_KANJI, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ VK_HOME | 0x100u, VK_LEFT | 0x100u,
	/* 0x52 */ VK_UP | 0x100u, VK_RIGHT | 0x100u,
	/* 0x54 */ VK_DOWN | 0x100u, VK_PRIOR | 0x100u,
	/* 0x56 */ VK_NEXT | 0x100u, VK_END | 0x100u,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT | 0x100u,
	/* 0x64 */ 0, 0, 0, VK_APPS | 0x100u,
	/* 0x68 */ 0, 0, VK_HELP, VK_CANCEL | 0x100u, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, VK_NUMLOCK | 0x100u,
	/* 0x80 */ VK_SPACE, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, VK_TAB, 0, 0, 0, VK_RETURN | 0x100u, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,
	/* 0x98 */ VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT,
	/* 0x9c */ VK_END, 0, VK_INSERT, VK_DELETE,
	/* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa8 */ 0, 0, VK_MULTIPLY, VK_ADD,
	/* 0xac */ VK_DECIMAL, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE | 0x100u,
	/* 0xb0 */ VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,
	/* 0xb4 */ VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
	/* 0xb8 */ VK_NUMPAD8, VK_NUMPAD9, 0, 0, 0, 0, VK_F1, VK_F2,
	/* 0xc0 */ VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
	/* 0xc8 */ VK_F11, VK_F12, VK_F13 | 0x100u, VK_F14 | 0x100u,
	/* 0xcc */ VK_F15 | 0x100u, VK_F16 | 0x100u,
	/* 0xce */ VK_F17 | 0x100u, VK_F18 | 0x100u,
	/* 0xd0 */ VK_F19 | 0x100u, VK_F20 | 0x100u,
	/* 0xd2 */ VK_F21 | 0x100u, VK_F22 | 0x100u,
	/* 0xd4 */ VK_F23 | 0x100u, VK_F24 | 0x100u, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, VK_LSHIFT, VK_RSHIFT | 0x100u, VK_LCONTROL,
	/* 0xe4 */ VK_RCONTROL | 0x100u, VK_CAPITAL, 0, 0,
	/* 0xe8 */ 0, VK_LMENU, VK_RMENU | 0x100u, VK_LWIN | 0x100u,
	/* 0xec */ VK_RWIN | 0x100u, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, VK_DELETE | 0x100u
};

CMSWindowsKeyState::CMSWindowsKeyState(CMSWindowsDesks* desks) :
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_desks(desks),
	m_keyLayout(GetKeyboardLayout(0))
{
	// do nothing
}

CMSWindowsKeyState::~CMSWindowsKeyState()
{
	// do nothing
}

void
CMSWindowsKeyState::setKeyLayout(HKL keyLayout)
{
	m_keyLayout = keyLayout;
}

void
CMSWindowsKeyState::fixKey(void* target, UINT virtualKey)
{
	// check if virtualKey is up but we think it's down.  if so then
	// synthesize a key release for it.
	//
	// we use GetAsyncKeyState() to check the state of the keys even
	// though we might not be in sync with that yet.
	KeyButton button = m_virtKeyToScanCode[virtualKey];
	if (isKeyDown(button) && (GetAsyncKeyState(virtualKey) & 0x8000) == 0) {
		// compute appropriate parameters for fake event
		LPARAM lParam = 0xc0000000 | ((LPARAM)button << 16);

		// process as if it were a key up
		KeyModifierMask mask;
		KeyID key = mapKeyFromEvent(virtualKey, lParam, &mask);
		LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
		CKeyState::sendKeyEvent(target, false, false, key, mask, 1, button);
		setKeyDown(button, false);
	}
}

KeyID
CMSWindowsKeyState::mapKeyFromEvent(WPARAM charAndVirtKey,
				LPARAM info, KeyModifierMask* maskOut) const
{
	// note:  known microsoft bugs
	//  Q72583 -- MapVirtualKey() maps keypad keys incorrectly
	//    95,98: num pad vk code -> invalid scan code
	//    95,98,NT4: num pad scan code -> bad vk code except
	//      SEPARATOR, MULTIPLY, SUBTRACT, ADD

	// extract character and virtual key
	char c       = (char)((charAndVirtKey & 0xff00u) >> 8);
	UINT vkCode  = (charAndVirtKey & 0xffu);

	// handle some keys via table lookup
	int extended = ((info >> 24) & 1);
	KeyID id     = s_virtualKey[vkCode][extended];

	// check if not in table;  map character to key id
	if (id == kKeyNone && c != 0) {
		if ((c & 0x80u) == 0) {
			// ASCII
			id = static_cast<KeyID>(c) & 0xffu;
		}
		else {
			// character is not really ASCII.  instead it's some
			// character in the current ANSI code page.  try to
			// convert that to a Unicode character.  if we fail
			// then use the single byte character as is.
			char src = c;
			wchar_t unicode;
			if (MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED,
										&src, 1, &unicode, 1) > 0) {
				id = static_cast<KeyID>(unicode);
			}
			else {
				id = static_cast<KeyID>(c) & 0xffu;
			}
		}
	}

	// set mask
	KeyModifierMask activeMask = getActiveModifiers();
	bool needAltGr = false;
	if (id != kKeyNone && c != 0) {
		// note if key requires AltGr.  VkKeyScan() can have a problem
		// with some characters.  there are two problems in particular.
		// first, typing a dead key then pressing space will cause
		// VkKeyScan() to return 0xffff.  second, certain characters
		// may map to multiple virtual keys and we might get the wrong
		// one.  if that happens then we might not get the right
		// modifier mask.  AltGr+9 on the french keyboard layout (^)
		// has this problem.  in the first case, we'll assume AltGr is
		// required (only because it solves the problems we've seen
		// so far).  in the second, we'll use whatever the keyboard
		// state says.
		WORD virtualKeyAndModifierState = VkKeyScanEx(c, m_keyLayout);
		if (virtualKeyAndModifierState == 0xffff) {
			// there is no mapping.  assume AltGr.
			LOG((CLOG_DEBUG1 "no VkKeyScan() mapping"));
			needAltGr = true;
		}
		else if (LOBYTE(virtualKeyAndModifierState) != vkCode) {
			// we didn't get the key that was actually pressed
			LOG((CLOG_DEBUG1 "VkKeyScan() mismatch"));
			if ((activeMask & (KeyModifierControl | KeyModifierAlt)) ==
							(KeyModifierControl | KeyModifierAlt)) {
				needAltGr = true;
			}
		}
		else {
			BYTE modifierState = HIBYTE(virtualKeyAndModifierState);
			if ((modifierState & 6) == 6) {
				// key requires ctrl and alt == AltGr
				needAltGr = true;
			}
		}
	}

	// map modifier key
	if (maskOut != NULL) {
		if (needAltGr) {
			activeMask |=  KeyModifierModeSwitch;
			activeMask &= ~(KeyModifierControl | KeyModifierAlt);
		}
		else {
			activeMask &= ~KeyModifierModeSwitch;
		}
		*maskOut = activeMask;
	}

	return id;
}

KeyButton
CMSWindowsKeyState::virtualKeyToButton(UINT virtualKey) const
{
	return m_virtKeyToScanCode[virtualKey & 0xffu];
}

void
CMSWindowsKeyState::sendKeyEvent(void* target,
							bool press, bool isAutoRepeat,
							KeyID key, KeyModifierMask mask,
							SInt32 count, KeyButton button)
{
	if (press || isAutoRepeat) {
		// if AltGr is required for this key then make sure
		// the ctrl and alt keys are *not* down on the
		// client.  windows simulates AltGr with ctrl and
		// alt for some inexplicable reason and clients
		// will get confused if they see mode switch and
		// ctrl and alt.  we'll also need to put ctrl and
		// alt back the way they were after we simulate
		// the key.
		bool ctrlL = isKeyDown(m_virtKeyToScanCode[VK_LCONTROL]);
		bool ctrlR = isKeyDown(m_virtKeyToScanCode[VK_RCONTROL]);
		bool altL  = isKeyDown(m_virtKeyToScanCode[VK_LMENU]);
		bool altR  = isKeyDown(m_virtKeyToScanCode[VK_RMENU]);
		if ((mask & KeyModifierModeSwitch) != 0) {
			KeyModifierMask mask2 = (mask &
								~(KeyModifierControl |
								KeyModifierAlt |
								KeyModifierModeSwitch));
			if (ctrlL) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyControl_L, mask2, 1,
							m_virtKeyToScanCode[VK_LCONTROL]);
			}
			if (ctrlR) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyControl_R, mask2, 1,
							m_virtKeyToScanCode[VK_RCONTROL]);
			}
			if (altL) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyAlt_L, mask2, 1,
							m_virtKeyToScanCode[VK_LMENU]);
			}
			if (altR) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyAlt_R, mask2, 1,
							m_virtKeyToScanCode[VK_RMENU]);
			}
		}

		// send key
		if (press) {
			CKeyState::sendKeyEvent(target, true, false,
							key, mask, 1, button);
			if (count > 0) {
				--count;
			}
		}
		if (count >= 1) {
			CKeyState::sendKeyEvent(target, true, true,
							key, mask, count, button);
		}

		// restore ctrl and alt state
		if ((mask & KeyModifierModeSwitch) != 0) {
			KeyModifierMask mask2 = (mask &
								~(KeyModifierControl |
								KeyModifierAlt |
								KeyModifierModeSwitch));
			if (ctrlL) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyControl_L, mask2, 1,
							m_virtKeyToScanCode[VK_LCONTROL]);
				mask2 |= KeyModifierControl;
			}
			if (ctrlR) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyControl_R, mask2, 1,
							m_virtKeyToScanCode[VK_RCONTROL]);
				mask2 |= KeyModifierControl;
			}
			if (altL) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyAlt_L, mask2, 1,
							m_virtKeyToScanCode[VK_LMENU]);
				mask2 |= KeyModifierAlt;
			}
			if (altR) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyAlt_R, mask2, 1,
							m_virtKeyToScanCode[VK_RMENU]);
				mask2 |= KeyModifierAlt;
			}
		}
	}
	else {
		// do key up
		CKeyState::sendKeyEvent(target, false, false, key, mask, 1, button);
	}
}

bool
CMSWindowsKeyState::fakeCtrlAltDel()
{
	if (!m_is95Family) {
		// to fake ctrl+alt+del on the NT family we broadcast a suitable
		// hotkey to all windows on the winlogon desktop.  however, the
		// current thread must be on that desktop to do the broadcast
		// and we can't switch just any thread because some own windows
		// or hooks.  so start a new thread to do the real work.
		CThread cad(new CFunctionJob(&CMSWindowsKeyState::ctrlAltDelThread));
		cad.wait();
	}
	else {
		// simulate ctrl+alt+del
		fakeKeyDown(kKeyDelete, KeyModifierControl | KeyModifierAlt,
							m_virtKeyToScanCode[VK_DELETE]);
	}
	return true;
}

void
CMSWindowsKeyState::ctrlAltDelThread(void*)
{
	// get the Winlogon desktop at whatever privilege we can
	HDESK desk = OpenDesktop("Winlogon", 0, FALSE, MAXIMUM_ALLOWED);
	if (desk != NULL) {
		if (SetThreadDesktop(desk)) {
			PostMessage(HWND_BROADCAST, WM_HOTKEY, 0,
						MAKELPARAM(MOD_CONTROL | MOD_ALT, VK_DELETE));
		}
		else {
			LOG((CLOG_DEBUG "can't switch to Winlogon desk: %d", GetLastError()));
		}
		CloseDesktop(desk);
	}
	else {
		LOG((CLOG_DEBUG "can't open Winlogon desk: %d", GetLastError()));
	}
}

const char*
CMSWindowsKeyState::getKeyName(KeyButton button) const
{
	char keyName[100];
	char keyName2[100];
	CMSWindowsKeyState* self = const_cast<CMSWindowsKeyState*>(this);
	if (GetKeyNameText(button << 16, keyName, sizeof(keyName)) != 0) {
		// get the extended name of the key if button is not extended
		// or vice versa.  if the names are different then report both.
		button ^= 0x100u;
		if (GetKeyNameText(button << 16, keyName2, sizeof(keyName2)) != 0 &&
			strcmp(keyName, keyName2) != 0) {
			self->m_keyName = CStringUtil::print("%s or %s", keyName, keyName2);
		}
		else {
			self->m_keyName = keyName;
		}
	}
	else if (m_scanCodeToVirtKey[button] != 0) {
		self->m_keyName = s_vkToName[m_scanCodeToVirtKey[button]];
	}
	else {
		self->m_keyName = CStringUtil::print("scan code 0x%03x", button);
	}
	return m_keyName.c_str();
}

void
CMSWindowsKeyState::doUpdateKeys()
{
	// clear scan code to/from virtual key mapping
	memset(m_scanCodeToVirtKey, 0, sizeof(m_scanCodeToVirtKey));
	memset(m_scanCodeToVirtKeyNumLock, 0, sizeof(m_scanCodeToVirtKeyNumLock));
	memset(m_virtKeyToScanCode, 0, sizeof(m_virtKeyToScanCode));

	// add modifiers.  note that ModeSwitch is mapped to VK_RMENU and
	// that it's mapped *before* the Alt modifier.  we must map it so
	// KeyModifierModeSwitch mask can be converted to keystrokes.  it
	// must be mapped before the Alt modifier so that the Alt modifier
	// takes precedence when mapping keystrokes to modifier masks.
	KeyButtons keys;
	keys.push_back(mapVirtKeyToButton(VK_RMENU));
	addModifier(KeyModifierModeSwitch, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_LSHIFT));
	keys.push_back(mapVirtKeyToButton(VK_RSHIFT));
	addModifier(KeyModifierShift, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_LCONTROL));
	keys.push_back(mapVirtKeyToButton(VK_RCONTROL));
	addModifier(KeyModifierControl, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_LMENU));
	keys.push_back(mapVirtKeyToButton(VK_RMENU));
	addModifier(KeyModifierAlt, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_LWIN));
	keys.push_back(mapVirtKeyToButton(VK_RWIN));
	addModifier(KeyModifierSuper, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_CAPITAL));
	addModifier(KeyModifierCapsLock, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_NUMLOCK));
	addModifier(KeyModifierNumLock, keys);
	keys.clear();
	keys.push_back(mapVirtKeyToButton(VK_SCROLL));
	addModifier(KeyModifierScrollLock, keys);

	BYTE keyState[256];
	GetKeyboardState(keyState);
	for (UINT i = 1; i < 256; ++i) {
		// skip mouse button virtual keys
		switch (i) {
		case VK_LBUTTON:
		case VK_RBUTTON:
		case VK_MBUTTON:
		case VK_XBUTTON1:
		case VK_XBUTTON2:
			continue;

		default:
			break;
		}

		// map to a scancode and back to a virtual key
		KeyButton button2;
		KeyButton button = mapVirtKeyToButton(i, button2);
		if (button == 0) {
			continue;
		}

		// okay, now we have the scan code for the virtual key.  the
		// numpad causes some confusion.  buttons on the numpad are
		// used for two virtual keys (one for num lock off and one
		// for num lock on).  keep a separate map for virtual keys
		// used when num lock is on.
		if ((i >= VK_NUMPAD0 && i <= VK_NUMPAD9) ||
			i == VK_SEPARATOR || i == VK_DECIMAL) {
			m_scanCodeToVirtKeyNumLock[button]  = i;
			m_scanCodeToVirtKeyNumLock[button2] = i;
		}
		else {
			m_scanCodeToVirtKey[button]  = i;
			m_scanCodeToVirtKey[button2] = i;
		}
		m_virtKeyToScanCode[i] = button;

		// if the virtual key is VK_DELETE then use the extended
		// scan code.  this is important for simulating ctrl+alt+del
		// which only works with the extended key.
		if (i == VK_DELETE) {
			m_virtKeyToScanCode[i] |= 0x100u;
		}

		// save the key state
		if ((keyState[i] & 0x80) != 0) {
			setKeyDown(button, true);
		}

		// toggle state applies to all keys but we only want it for
		// the modifier keys with corresponding lights.
		if ((keyState[i] & 0x01) != 0) {
			switch (i) {
			case VK_CAPITAL:
				setToggled(KeyModifierCapsLock);
				break;

			case VK_NUMLOCK:
				setToggled(KeyModifierNumLock);
				break;

			case VK_SCROLL:
				setToggled(KeyModifierScrollLock);
				break;
			}
		}
	}
}

void
CMSWindowsKeyState::doFakeKeyEvent(KeyButton button,
							bool press, bool isAutoRepeat)
{
	UINT vk = 0;
	if ((getActiveModifiers() & KeyModifierNumLock) != 0) {
		vk = m_scanCodeToVirtKeyNumLock[button];
	}
	if (vk == 0) {
		vk = m_scanCodeToVirtKey[button];
	}
	m_desks->fakeKeyEvent(button, vk, press, isAutoRepeat);
}

KeyButton
CMSWindowsKeyState::mapKey(Keystrokes& keys, KeyID id,
				KeyModifierMask mask, bool isAutoRepeat) const
{
	UINT extVirtualKey = 0;

	// check for special keys
	if ((id & 0xfffff000u) == 0xe000u) {
		if ((id & 0xff00u) == 0xe000u) {
			extVirtualKey = s_mapE000[id & 0xffu];
		}
		else if ((id & 0xff00) == 0xee00) {
			extVirtualKey = s_mapEE00[id & 0xffu];
		}
		else if ((id & 0xff00) == 0xef00) {
			extVirtualKey = s_mapEF00[id & 0xffu];
		}
		if (extVirtualKey == 0) {
			LOG((CLOG_DEBUG2 "unknown special key"));
			return 0;
		}
	}

	// special handling of VK_SNAPSHOT
	if (extVirtualKey == VK_SNAPSHOT) {
		// ignore key repeats on print screen
		if (!isAutoRepeat) {
			// active window (with alt) or fullscreen (without alt)?
			BYTE scan = 0;
			if ((mask & KeyModifierAlt) != 0) {
				scan = 1;
			}

			// send events
			keybd_event(VK_SNAPSHOT, scan, 0, 0);
			keybd_event(VK_SNAPSHOT, scan, KEYEVENTF_KEYUP, 0);
		}
		return 0;
	}

	// handle other special keys
	if (extVirtualKey != 0) {
		// compute required modifiers
		KeyModifierMask requiredMask = 0;
		KeyModifierMask outMask      = 0;

		// check numeric keypad.  note that virtual keys do not distinguish
		// between the keypad and non-keypad movement keys.  however, the
		// virtual keys do distinguish between keypad numbers and operators
		// (e.g. add, multiply) and their main keyboard counterparts.
		// therefore, we can ignore the num-lock state for movement virtual
		// keys but not for numeric keys.
		UINT virtualKey = (extVirtualKey & 0xffu);
		if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_DIVIDE) {
			requiredMask |= KeyModifierNumLock;
			if ((getActiveModifiers() & KeyModifierNumLock) != 0) {
				LOG((CLOG_DEBUG2 "turn on num lock for keypad key"));
				outMask |= KeyModifierNumLock;
			}
		}

		// check for left tab.  that requires the shift key.
		if (id == kKeyLeftTab) {
			requiredMask |= KeyModifierShift;
			outMask      |= KeyModifierShift;
		}

		// now generate the keystrokes and return the resulting modifier mask
		KeyButton button = m_virtKeyToScanCode[virtualKey];
		if ((extVirtualKey & 0x100u) != 0) {
			button |= 0x100u;
		}
		LOG((CLOG_DEBUG2 "KeyID 0x%08x to virtual key %d scan code 0x%03x mask 0x%04x", id, virtualKey, button, outMask));
		return mapToKeystrokes(keys, button,
								outMask, requiredMask, isAutoRepeat);
	}

	// determine the thread that'll receive this event
	// FIXME -- we can't be sure we'll get the right thread here
	HWND  targetWindow = GetForegroundWindow();
	DWORD targetThread = GetWindowThreadProcessId(targetWindow, NULL);

	// figure out the code page for the target thread.  i'm just
	// guessing here.  get the target thread's keyboard layout,
	// extract the language id from that, and choose the code page
	// based on that language.
	HKL hkl       = GetKeyboardLayout(targetThread);
	LANGID langID = static_cast<LANGID>(LOWORD(hkl));
	UINT codePage = getCodePageFromLangID(langID);
	LOG((CLOG_DEBUG2 "using code page %d and language id 0x%04x for thread 0x%08x", codePage, langID, targetThread));

	// regular characters are complicated by dead keys.  it may not be
	// possible to generate a desired character directly.  we may need
	// to generate a dead key first then some other character.  the
	// app receiving the events will compose these two characters into
	// a single precomposed character.
	//
	// as best as i can tell this is the simplest way to convert a
	// character into its uncomposed version.  along the way we'll
	// discover if the key cannot be handled at all.  we convert
	// from wide char to multibyte, then from multibyte to wide char
	// forcing conversion to composite characters, then from wide
	// char back to multibyte without making precomposed characters.
	//
	// after the first conversion to multibyte we see if we can map
	// the key.  if so then we don't bother trying to decompose dead
	// keys.
	BOOL error;
	char multiByte[2 * MB_LEN_MAX];
	wchar_t unicode[2];
	unicode[0] = static_cast<wchar_t>(id & 0x0000ffffu);
	int nChars = WideCharToMultiByte(codePage,
								WC_COMPOSITECHECK | WC_DEFAULTCHAR,
								unicode, 1,
								multiByte, sizeof(multiByte),
								NULL, &error);
	if (nChars == 0 || error) {
		LOG((CLOG_DEBUG2 "KeyID 0x%08x not in code page", id));
		return 0;
	}
	KeyButton button = mapCharacter(keys, multiByte[0], hkl, isAutoRepeat);
	if (button != 0) {
		LOG((CLOG_DEBUG2 "KeyID 0x%08x maps to character %u", id, (unsigned char)multiByte[0]));
		if (isDeadChar(multiByte[0], hkl, false)) {
			// character mapped to a dead key but we want the
			// character for real so send a space key afterwards.
			LOG((CLOG_DEBUG2 "character mapped to dead key"));
			Keystroke keystroke;
			keystroke.m_key    = m_virtKeyToScanCode[VK_SPACE];
			keystroke.m_press  = true;
			keystroke.m_repeat = false;
			keys.push_back(keystroke);
			keystroke.m_press  = false;
			keys.push_back(keystroke);

			// ignore the release of this key since we already
			// handled it.
			button = 0;
		}
		return button;
	}
	nChars = MultiByteToWideChar(codePage,
							MB_COMPOSITE | MB_ERR_INVALID_CHARS,
							multiByte, nChars,
							unicode, 2);
	if (nChars == 0) {
		LOG((CLOG_DEBUG2 "KeyID 0x%08x mb->wc mapping failed", id));
		return 0;
	}
	nChars = WideCharToMultiByte(codePage,
							0,
							unicode, nChars,
							multiByte, sizeof(multiByte),
							NULL, &error);
	if (nChars == 0 || error) {
		LOG((CLOG_DEBUG2 "KeyID 0x%08x wc->mb mapping failed", id));
		return 0;
	}

	// we expect one or two characters in multiByte.  if there are two
	// then the *second* is a dead key.  process the dead key if there.
	// FIXME -- we assume each character is one byte here
	if (nChars > 2) {
		LOG((CLOG_DEBUG2 "multibyte characters not supported for character 0x%04x", id));
		return 0;
	}
	if (nChars == 2) {
		LOG((CLOG_DEBUG2 "KeyID 0x%08x needs dead key %u", id, (unsigned char)multiByte[1]));
		mapCharacter(keys, multiByte[1], hkl, isAutoRepeat);
	}

	// process character
	LOG((CLOG_DEBUG2 "KeyID 0x%08x maps to character %u", id, (unsigned char)multiByte[0]));
	return mapCharacter(keys, multiByte[0], hkl, isAutoRepeat);
}

UINT
CMSWindowsKeyState::getCodePageFromLangID(LANGID langid) const
{
	// construct a locale id from the language id
	LCID lcid = MAKELCID(langid, SORT_DEFAULT);

	// get the ANSI code page for this locale
	char data[6];
	if (GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, data, 6) == 0) {
		// can't get code page
		LOG((CLOG_DEBUG1 "can't find code page for langid 0x%04x", langid));
		return CP_ACP;
	}

	// convert stringified code page into a number
	UINT codePage = static_cast<UINT>(atoi(data));
	if (codePage == 0) {
		// parse failed
		LOG((CLOG_DEBUG1 "can't parse code page %s for langid 0x%04x", data, langid));
		return CP_ACP;
	}

	return codePage;
}

KeyButton
CMSWindowsKeyState::mapVirtKeyToButton(UINT virtualKey,
				KeyButton& extended) const
{
	// this method does what MapVirtualKey(virtualKey, 0) should do.
	// we have to explicitly set the extended key flag for some
	// modifiers because the win32 API is inadequate.  we also find
	// the unextended and the extended scancodes for those virtual
	// keys that have both except for VK_SHIFT, VK_CONTROL, and VK_MENU.
	//
	// the windows 95 family doesn't map the side distinguishing virtual
	// keys.  but we know that VK_CONTROL maps to VK_LCONTROL and
	// that VK_RCONTROL is the same scan code | 0x100.  similarly for
	// VK_MENU.  but VK_RSHIFT cannot be determined that way so we
	// search for it.
	extended = 0;
	KeyButton button;
	if (m_is95Family) {
		UINT scancode;
		switch (virtualKey) {
		case VK_LSHIFT:
			button = (KeyButton)MapVirtualKey(VK_SHIFT, 0);
			break;

		case VK_RSHIFT:
			// we have to search
			scancode = MapVirtualKey(VK_SHIFT, 0);
			for (UINT i = 1; i < 256; ++i) {
				if (i != scancode && MapVirtualKey(i, 1) == VK_SHIFT) {
					return (KeyButton)(i);
				}
			}
			return 0;

		case VK_LCONTROL:
		case VK_RCONTROL:
			button = (KeyButton)MapVirtualKey(VK_CONTROL, 0);
			break;

		case VK_LMENU:
		case VK_RMENU:
			button = (KeyButton)MapVirtualKey(VK_MENU, 0);
			break;

		case VK_PAUSE:
			// mapped to 0.  i hope this works on all keyboards.
			button = (KeyButton)0x45u;
			break;

		case VK_DIVIDE:
			// mapped to 0.  i hope this works on all keyboards.
			button = (KeyButton)0x35u;
			break;

		default:
			button = (KeyButton)MapVirtualKey(virtualKey, 0);

			// okay, now we have the scan code for the virtual key.  windows
			// may map different virtual keys to the same button.  for example,
			// windows 95/98/me maps virtual keys 220 and 226 to scan code 86
			// in the british english keyboard map.  why?  who knows.  it
			// doesn't make any sense since a button can't actually generate
			// more than one virtual key.  to avoid this stupidity, we map the
			// button back to a virtual key to see if it matches the starting
			// point.  we don't do this for number pad keys since we expect
			// each key to generate one of two virtual keys, depending on the
			// state of NumLock, a state we can't pass to MapVirtualKey.
			if ((virtualKey < VK_NUMPAD0 || virtualKey > VK_NUMPAD9) &&
				virtualKey != VK_SEPARATOR && virtualKey != VK_DECIMAL) {
				if (button == 0 || MapVirtualKey(button, 1) != virtualKey) {
					return 0;
				}
			}
			break;
		}
	}
	else {
		switch (virtualKey) {
		case VK_PAUSE:
			// mapped to 0.  i hope this works on all keyboards.
			button = (KeyButton)0x45u;
			break;

		default:
			button = (KeyButton)MapVirtualKey(virtualKey, 0);
			break;
		}
	}

	// map extended keys
	switch (virtualKey) {
	case VK_RETURN:		// Return/numpad Enter
	case VK_PRIOR:		// numpad PageUp/PageUp
	case VK_NEXT:		// numpad PageDown/PageDown
	case VK_END:		// numpad End/End
	case VK_HOME:		// numpad Home/Home
	case VK_LEFT:		// numpad Left/Left
	case VK_UP:			// numpad Up/Up
	case VK_RIGHT:		// numpad Right/Right
	case VK_DOWN:		// numpad Down/Down
	case VK_INSERT:		// numpad Insert/Insert
	case VK_DELETE:		// numpad Delete/Delete
//	case VK_SELECT:
//	case VK_EXECUTE:
//	case VK_HELP:
		extended = (KeyButton)(button | 0x100u);
		break;
	}

	// see if the win32 API can help us determine an extended key.
	// if the remapped virtual key doesn't match the starting
	// point then there's a really good chance that that virtual
	// key is mapped to an extended key.  however, this is not
	// the case for modifiers that don't distinguish between left
	// and right.
	UINT virtualKey2 = MapVirtualKey(button, 3);
	if (virtualKey2 != 0 && virtualKey2 != virtualKey) {
		switch (virtualKey) {
		case VK_SHIFT:
		case VK_CONTROL:
		case VK_MENU:
			break;

		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
		case VK_MULTIPLY:
		case VK_ADD:
		case VK_SEPARATOR:
		case VK_SUBTRACT:
		case VK_DECIMAL:
			break;

		case VK_PAUSE:
			break;

		default:
			button  |= 0x100u;
			extended = 0;
			break;
		}
		return button;
	}

	// note other extended keys that the win32 API won't help us with.
	// on the windows 95 family this is the only way to find extended
	// keys since MapVirtualKey(N, 3) is unimplemented.
	switch (virtualKey) {
	case VK_CANCEL:
	case VK_KANJI:
	case VK_LWIN:
	case VK_RWIN:
	case VK_APPS:
//	case VK_SEPARATOR:
	case VK_DIVIDE:
	case VK_F13:
	case VK_F14:
	case VK_F15:
	case VK_F16:
	case VK_F17:
	case VK_F18:
	case VK_F19:
	case VK_F20:
	case VK_F21:
	case VK_F22:
	case VK_F23:
	case VK_F24:
	case VK_NUMLOCK:
	case VK_RSHIFT:
	case VK_RCONTROL:
	case VK_RMENU:
	case VK_BROWSER_BACK:
	case VK_BROWSER_FORWARD:
	case VK_BROWSER_REFRESH:
	case VK_BROWSER_STOP:
	case VK_BROWSER_SEARCH:
	case VK_BROWSER_FAVORITES:
	case VK_BROWSER_HOME:
	case VK_VOLUME_MUTE:
	case VK_VOLUME_DOWN:
	case VK_VOLUME_UP:
	case VK_MEDIA_NEXT_TRACK:
	case VK_MEDIA_PREV_TRACK:
	case VK_MEDIA_STOP:
	case VK_MEDIA_PLAY_PAUSE:
	case VK_LAUNCH_MAIL:
	case VK_LAUNCH_MEDIA_SELECT:
	case VK_LAUNCH_APP1:
	case VK_LAUNCH_APP2:
		button  |= 0x100u;
		extended = 0;
		break;
	}

	return button;
}

KeyButton
CMSWindowsKeyState::mapVirtKeyToButton(UINT virtualKey) const
{
	KeyButton dummy;
	return mapVirtKeyToButton(virtualKey, dummy);
}

KeyButton
CMSWindowsKeyState::mapCharacter(Keystrokes& keys,
				char c, HKL hkl, bool isAutoRepeat) const
{
	KeyModifierMask activeMask = getActiveModifiers();

	// translate the character into its virtual key and its required
	// modifier state.
	SHORT virtualKeyAndModifierState = VkKeyScanEx(c, hkl);

	// get virtual key
	UINT virtualKey = LOBYTE(virtualKeyAndModifierState);
	if (virtualKey == 0xffu) {
		LOG((CLOG_DEBUG2 "cannot map character %d", static_cast<unsigned char>(c)));
		return 0;
	}

	// get the required modifier state
	BYTE modifierState = HIBYTE(virtualKeyAndModifierState);

	// see what modifiers are needed.  we only check for shift and
	// AltGr.  we must always match the desired shift state but only
	// the desired AltGr state if AltGr is required.  AltGr is actually
	// ctrl + alt so we can't require that ctrl and alt not be pressed
	// otherwise users couldn't do, say, ctrl+z.
	//
	// the space character (ascii 32) is special in that it's unaffected
	// by shift and should match our stored shift state.
	KeyModifierMask desiredMask  = 0;
	KeyModifierMask requiredMask = KeyModifierShift;
	if (c == 32) {
		desiredMask |= (activeMask & KeyModifierShift);
	}
	else if ((modifierState & 0x01u) == 1) {
		desiredMask |= KeyModifierShift;
	}
	if ((modifierState & 0x06u) == 6) {
		// add ctrl and alt, which must be matched.  match alt via
		// mode-switch, which uses the right alt key rather than
		// the left.  windows doesn't care which alt key so long
		// as ctrl is also down but some apps do their own mapping
		// and they do care.  Emacs and PuTTY, for example.
		desiredMask  |= KeyModifierControl | KeyModifierModeSwitch;
		requiredMask |= KeyModifierControl | KeyModifierModeSwitch;
	}

	// handle combination of caps-lock and shift.  if caps-lock is
	// off locally then use shift as necessary.  if caps-lock is on
	// locally then it reverses the meaning of shift for keys that
	// are subject to case conversion.
	if ((activeMask & KeyModifierCapsLock) != 0) {
		// there doesn't seem to be a simple way to test if a
		// character respects the caps lock key.  for normal
		// characters it's easy enough but CharLower() and
		// CharUpper() don't map dead keys even though they
		// do respect caps lock for some unfathomable reason.
		// first check the easy way.  if that doesn't work
		// then see if it's a dead key.
		unsigned char uc = static_cast<unsigned char>(c);
		if (CharLower(reinterpret_cast<LPTSTR>(uc)) !=
			CharUpper(reinterpret_cast<LPTSTR>(uc)) ||
			(MapVirtualKeyEx(virtualKey, 2, hkl) & 0x80000000lu) != 0) {
			LOG((CLOG_DEBUG2 "flip shift"));
			desiredMask ^= KeyModifierShift;
		}
	}

	// now generate the keystrokes.  ignore the resulting modifier
	// mask since it can't have changed (because we don't call this
	// method for modifier keys).
	KeyButton scanCode = m_virtKeyToScanCode[virtualKey];
	LOG((CLOG_DEBUG2 "character %d to virtual key %d scan code 0x%04x mask 0x%08x", (unsigned char)c, virtualKey, scanCode, desiredMask));
	return mapToKeystrokes(keys, scanCode,
								desiredMask, requiredMask, isAutoRepeat);
}

KeyButton
CMSWindowsKeyState::mapToKeystrokes(Keystrokes& keys, KeyButton button,
				KeyModifierMask desiredMask, KeyModifierMask requiredMask,
				bool isAutoRepeat) const
{
	// adjust the modifiers to match the desired modifiers
	Keystrokes undo;
	if (!adjustModifiers(keys, undo, desiredMask, requiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		keys.clear();
		return 0;
	}

	// add the key event
	Keystroke keystroke;
	keystroke.m_key    = button;
	keystroke.m_press  = true;
	keystroke.m_repeat = isAutoRepeat;
	keys.push_back(keystroke);

	// put undo keystrokes at end of keystrokes in reverse order
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	return button;
}

bool
CMSWindowsKeyState::adjustModifiers(Keystrokes& keys,
				Keystrokes& undo,
				KeyModifierMask desiredMask,
				KeyModifierMask requiredMask) const
{
	// for each modifier in requiredMask make sure the current state
	// of that modifier matches the bit in desiredMask.
	for (KeyModifierMask mask = 1u; requiredMask != 0; mask <<= 1) {
		if ((mask & requiredMask) != 0) {
			bool active = ((desiredMask & mask) != 0);
			if (!mapModifier(keys, undo, mask, active)) {
				return false;
			}
			requiredMask ^= mask;
		}
	}
	return true;
}

int
CMSWindowsKeyState::toAscii(TCHAR c, HKL hkl, bool menu, WORD* chars) const
{
	// ignore bogus character
	if (c == 0) {
		return 0;
	}

	// translate the character into its virtual key and its required
	// modifier state.
	SHORT virtualKeyAndModifierState = VkKeyScanEx(c, hkl);

	// get virtual key
	BYTE virtualKey = LOBYTE(virtualKeyAndModifierState);
	if (virtualKey == 0xffu) {
		return 0;
	}

	// get the required modifier state
	BYTE modifierState = HIBYTE(virtualKeyAndModifierState);

	// set shift state required to generate key
	BYTE keys[256];
	memset(keys, 0, sizeof(keys));
	if (modifierState & 0x01u) {
		keys[VK_SHIFT]   = 0x80u;
	}
	if (modifierState & 0x02u) {
		keys[VK_CONTROL] = 0x80u;
	}
	if (modifierState & 0x04u) {
		keys[VK_MENU]    = 0x80u;
	}

	// get the scan code for the key
	UINT scanCode = MapVirtualKeyEx(virtualKey, 0, hkl);

	// discard characters if chars is NULL
	WORD dummy;
	if (chars == NULL) {
		chars = &dummy;
	}

	// put it back
	return ToAsciiEx(virtualKey, scanCode, keys, chars, menu ? 1 : 0, hkl);
}

bool
CMSWindowsKeyState::isDeadChar(TCHAR c, HKL hkl, bool menu) const
{
	// first clear out ToAsciiEx()'s internal buffer by sending it
	// a space.
	WORD ascii;
	int old = toAscii(' ', hkl, 0, &ascii);

	// now pass the character of interest
	WORD dummy;
	bool isDead = (toAscii(c, hkl, menu, &dummy) < 0);

	// clear out internal buffer again
	toAscii(' ', hkl, 0, &dummy);

	// put old dead key back if there was one
	if (old == 1 && ascii != ' ') {
		toAscii(static_cast<TCHAR>(ascii & 0xffu), hkl, menu, &dummy);
	}

	return isDead;
}
