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

#include "CMSWindowsKeyMapper.h"
#include "CLog.h"

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

//
// CMSWindowsKeyMapper
//

// table of modifier keys.  note that VK_RMENU shows up under the Alt
// key and ModeSwitch.  when simulating AltGr we need to use the right
// alt key so we use KeyModifierModeSwitch to get it.
const CMSWindowsKeyMapper::CModifierKeys
						CMSWindowsKeyMapper::s_modifiers[] =
{
	KeyModifierShift,      { VK_LSHIFT,          VK_RSHIFT           },
	KeyModifierControl,    { VK_LCONTROL,        VK_RCONTROL | 0x100 },
	KeyModifierAlt,        { VK_LMENU,           VK_RMENU    | 0x100 },
	KeyModifierSuper,      { VK_LWIN    | 0x100, VK_RWIN     | 0x100 },
	KeyModifierModeSwitch, { VK_RMENU   | 0x100, 0                   },
	KeyModifierCapsLock,   { VK_CAPITAL,         0                   },
	KeyModifierNumLock,    { VK_NUMLOCK | 0x100, 0                   },
	KeyModifierScrollLock, { VK_SCROLL,          0                   }
};

const char*				CMSWindowsKeyMapper::s_vkToName[] =
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
	"vk 0x5f",
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
const KeyID				CMSWindowsKeyMapper::s_virtualKey[][2] =
{
	/* 0x00 */ kKeyNone,		kKeyNone,		// reserved
	/* 0x01 */ kKeyNone,		kKeyNone,		// VK_LBUTTON
	/* 0x02 */ kKeyNone,		kKeyNone,		// VK_RBUTTON
	/* 0x03 */ kKeyNone,		kKeyBreak,		// VK_CANCEL
	/* 0x04 */ kKeyNone,		kKeyNone,		// VK_MBUTTON
	/* 0x05 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x06 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x07 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x08 */ kKeyBackSpace,	kKeyNone,		// VK_BACK
	/* 0x09 */ kKeyTab,			kKeyNone,		// VK_TAB
	/* 0x0a */ kKeyNone,		kKeyNone,		// undefined
	/* 0x0b */ kKeyNone,		kKeyNone,		// undefined
	/* 0x0c */ kKeyClear,		kKeyClear,		// VK_CLEAR
	/* 0x0d */ kKeyReturn,		kKeyKP_Enter,	// VK_RETURN
	/* 0x0e */ kKeyNone,		kKeyNone,		// undefined
	/* 0x0f */ kKeyNone,		kKeyNone,		// undefined
	/* 0x10 */ kKeyShift_L,		kKeyShift_R,	// VK_SHIFT
	/* 0x11 */ kKeyControl_L,	kKeyControl_R,	// VK_CONTROL
	/* 0x12 */ kKeyAlt_L,		kKeyAlt_R,		// VK_MENU
	/* 0x13 */ kKeyPause,		kKeyNone,		// VK_PAUSE
	/* 0x14 */ kKeyCapsLock,	kKeyNone,		// VK_CAPITAL
	/* 0x15 */ kKeyNone,		kKeyNone,		// VK_KANA			
	/* 0x16 */ kKeyNone,		kKeyNone,		// VK_HANGUL		
	/* 0x17 */ kKeyNone,		kKeyNone,		// VK_JUNJA			
	/* 0x18 */ kKeyNone,		kKeyNone,		// VK_FINAL			
	/* 0x19 */ kKeyZenkaku,		kKeyNone,		// VK_KANJI			
	/* 0x1a */ kKeyNone,		kKeyNone,		// undefined
	/* 0x1b */ kKeyEscape,		kKeyNone,		// VK_ESCAPE
	/* 0x1c */ kKeyNone,		kKeyNone,		// VK_CONVERT		
	/* 0x1d */ kKeyNone,		kKeyNone,		// VK_NONCONVERT	
	/* 0x1e */ kKeyNone,		kKeyNone,		// VK_ACCEPT		
	/* 0x1f */ kKeyNone,		kKeyNone,		// VK_MODECHANGE	
	/* 0x20 */ kKeyNone,		kKeyNone,		// VK_SPACE
	/* 0x21 */ kKeyKP_PageUp,	kKeyPageUp,		// VK_PRIOR
	/* 0x22 */ kKeyKP_PageDown,	kKeyPageDown,	// VK_NEXT
	/* 0x23 */ kKeyKP_End,		kKeyEnd,		// VK_END
	/* 0x24 */ kKeyKP_Home,		kKeyHome,		// VK_HOME
	/* 0x25 */ kKeyKP_Left,		kKeyLeft,		// VK_LEFT
	/* 0x26 */ kKeyKP_Up,		kKeyUp,			// VK_UP
	/* 0x27 */ kKeyKP_Right,	kKeyRight,		// VK_RIGHT
	/* 0x28 */ kKeyKP_Down,		kKeyDown,		// VK_DOWN
	/* 0x29 */ kKeySelect,		kKeySelect,		// VK_SELECT
	/* 0x2a */ kKeyNone,		kKeyNone,		// VK_PRINT
	/* 0x2b */ kKeyExecute,		kKeyExecute,	// VK_EXECUTE
	/* 0x2c */ kKeyPrint,		kKeyPrint,		// VK_SNAPSHOT
	/* 0x2d */ kKeyKP_Insert,	kKeyInsert,		// VK_INSERT
	/* 0x2e */ kKeyKP_Delete,	kKeyDelete,		// VK_DELETE
	/* 0x2f */ kKeyHelp,		kKeyHelp,		// VK_HELP
	/* 0x30 */ kKeyNone,		kKeyNone,		// VK_0
	/* 0x31 */ kKeyNone,		kKeyNone,		// VK_1
	/* 0x32 */ kKeyNone,		kKeyNone,		// VK_2
	/* 0x33 */ kKeyNone,		kKeyNone,		// VK_3
	/* 0x34 */ kKeyNone,		kKeyNone,		// VK_4
	/* 0x35 */ kKeyNone,		kKeyNone,		// VK_5
	/* 0x36 */ kKeyNone,		kKeyNone,		// VK_6
	/* 0x37 */ kKeyNone,		kKeyNone,		// VK_7
	/* 0x38 */ kKeyNone,		kKeyNone,		// VK_8
	/* 0x39 */ kKeyNone,		kKeyNone,		// VK_9
	/* 0x3a */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3b */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3c */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3d */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3e */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3f */ kKeyNone,		kKeyNone,		// undefined
	/* 0x40 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x41 */ kKeyNone,		kKeyNone,		// VK_A
	/* 0x42 */ kKeyNone,		kKeyNone,		// VK_B
	/* 0x43 */ kKeyNone,		kKeyNone,		// VK_C
	/* 0x44 */ kKeyNone,		kKeyNone,		// VK_D
	/* 0x45 */ kKeyNone,		kKeyNone,		// VK_E
	/* 0x46 */ kKeyNone,		kKeyNone,		// VK_F
	/* 0x47 */ kKeyNone,		kKeyNone,		// VK_G
	/* 0x48 */ kKeyNone,		kKeyNone,		// VK_H
	/* 0x49 */ kKeyNone,		kKeyNone,		// VK_I
	/* 0x4a */ kKeyNone,		kKeyNone,		// VK_J
	/* 0x4b */ kKeyNone,		kKeyNone,		// VK_K
	/* 0x4c */ kKeyNone,		kKeyNone,		// VK_L
	/* 0x4d */ kKeyNone,		kKeyNone,		// VK_M
	/* 0x4e */ kKeyNone,		kKeyNone,		// VK_N
	/* 0x4f */ kKeyNone,		kKeyNone,		// VK_O
	/* 0x50 */ kKeyNone,		kKeyNone,		// VK_P
	/* 0x51 */ kKeyNone,		kKeyNone,		// VK_Q
	/* 0x52 */ kKeyNone,		kKeyNone,		// VK_R
	/* 0x53 */ kKeyNone,		kKeyNone,		// VK_S
	/* 0x54 */ kKeyNone,		kKeyNone,		// VK_T
	/* 0x55 */ kKeyNone,		kKeyNone,		// VK_U
	/* 0x56 */ kKeyNone,		kKeyNone,		// VK_V
	/* 0x57 */ kKeyNone,		kKeyNone,		// VK_W
	/* 0x58 */ kKeyNone,		kKeyNone,		// VK_X
	/* 0x59 */ kKeyNone,		kKeyNone,		// VK_Y
	/* 0x5a */ kKeyNone,		kKeyNone,		// VK_Z
	/* 0x5b */ kKeyNone,		kKeySuper_L,	// VK_LWIN
	/* 0x5c */ kKeyNone,		kKeySuper_R,	// VK_RWIN
	/* 0x5d */ kKeyMenu,		kKeyMenu,		// VK_APPS
	/* 0x5e */ kKeyNone,		kKeyNone,		// undefined
	/* 0x5f */ kKeyNone,		kKeyNone,		// undefined
	/* 0x60 */ kKeyKP_0,		kKeyNone,		// VK_NUMPAD0
	/* 0x61 */ kKeyKP_1,		kKeyNone,		// VK_NUMPAD1
	/* 0x62 */ kKeyKP_2,		kKeyNone,		// VK_NUMPAD2
	/* 0x63 */ kKeyKP_3,		kKeyNone,		// VK_NUMPAD3
	/* 0x64 */ kKeyKP_4,		kKeyNone,		// VK_NUMPAD4
	/* 0x65 */ kKeyKP_5,		kKeyNone,		// VK_NUMPAD5
	/* 0x66 */ kKeyKP_6,		kKeyNone,		// VK_NUMPAD6
	/* 0x67 */ kKeyKP_7,		kKeyNone,		// VK_NUMPAD7
	/* 0x68 */ kKeyKP_8,		kKeyNone,		// VK_NUMPAD8
	/* 0x69 */ kKeyKP_9,		kKeyNone,		// VK_NUMPAD9
	/* 0x6a */ kKeyKP_Multiply,	kKeyNone,		// VK_MULTIPLY
	/* 0x6b */ kKeyKP_Add,		kKeyNone,		// VK_ADD
	/* 0x6c */ kKeyKP_Separator,kKeyKP_Separator,// VK_SEPARATOR
	/* 0x6d */ kKeyKP_Subtract,	kKeyNone,		// VK_SUBTRACT
	/* 0x6e */ kKeyKP_Decimal,	kKeyNone,		// VK_DECIMAL
	/* 0x6f */ kKeyNone,		kKeyKP_Divide,	// VK_DIVIDE
	/* 0x70 */ kKeyF1,			kKeyNone,		// VK_F1
	/* 0x71 */ kKeyF2,			kKeyNone,		// VK_F2
	/* 0x72 */ kKeyF3,			kKeyNone,		// VK_F3
	/* 0x73 */ kKeyF4,			kKeyNone,		// VK_F4
	/* 0x74 */ kKeyF5,			kKeyNone,		// VK_F5
	/* 0x75 */ kKeyF6,			kKeyNone,		// VK_F6
	/* 0x76 */ kKeyF7,			kKeyNone,		// VK_F7
	/* 0x77 */ kKeyF8,			kKeyNone,		// VK_F8
	/* 0x78 */ kKeyF9,			kKeyNone,		// VK_F9
	/* 0x79 */ kKeyF10,			kKeyNone,		// VK_F10
	/* 0x7a */ kKeyF11,			kKeyNone,		// VK_F11
	/* 0x7b */ kKeyF12,			kKeyNone,		// VK_F12
	/* 0x7c */ kKeyF13,			kKeyF13,		// VK_F13
	/* 0x7d */ kKeyF14,			kKeyF14,		// VK_F14
	/* 0x7e */ kKeyF15,			kKeyF15,		// VK_F15
	/* 0x7f */ kKeyF16,			kKeyF16,		// VK_F16
	/* 0x80 */ kKeyF17,			kKeyF17,		// VK_F17
	/* 0x81 */ kKeyF18,			kKeyF18,		// VK_F18
	/* 0x82 */ kKeyF19,			kKeyF19,		// VK_F19
	/* 0x83 */ kKeyF20,			kKeyF20,		// VK_F20
	/* 0x84 */ kKeyF21,			kKeyF21,		// VK_F21
	/* 0x85 */ kKeyF22,			kKeyF22,		// VK_F22
	/* 0x86 */ kKeyF23,			kKeyF23,		// VK_F23
	/* 0x87 */ kKeyF24,			kKeyF24,		// VK_F24
	/* 0x88 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x89 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8a */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8b */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8c */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8d */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8e */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8f */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x90 */ kKeyNumLock,		kKeyNumLock,	// VK_NUMLOCK
	/* 0x91 */ kKeyScrollLock,	kKeyNone,		// VK_SCROLL
	/* 0x92 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x93 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x94 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x95 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x96 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x97 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x98 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x99 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9a */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9b */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9c */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9d */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9e */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9f */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xa0 */ kKeyShift_L,		kKeyShift_L,	// VK_LSHIFT
	/* 0xa1 */ kKeyShift_R,		kKeyShift_R,	// VK_RSHIFT
	/* 0xa2 */ kKeyControl_L,	kKeyControl_L,	// VK_LCONTROL
	/* 0xa3 */ kKeyControl_R,	kKeyControl_R,	// VK_RCONTROL
	/* 0xa4 */ kKeyAlt_L,		kKeyAlt_L,		// VK_LMENU
	/* 0xa5 */ kKeyAlt_R,		kKeyAlt_R,		// VK_RMENU
	/* 0xa6 */ kKeyNone,		kKeyWWWBack,	// VK_BROWSER_BACK
	/* 0xa7 */ kKeyNone,		kKeyWWWForward,	// VK_BROWSER_FORWARD
	/* 0xa8 */ kKeyNone,		kKeyWWWRefresh,	// VK_BROWSER_REFRESH
	/* 0xa9 */ kKeyNone,		kKeyWWWStop,	// VK_BROWSER_STOP
	/* 0xaa */ kKeyNone,		kKeyWWWSearch,	// VK_BROWSER_SEARCH
	/* 0xab */ kKeyNone,		kKeyWWWFavorites,	// VK_BROWSER_FAVORITES
	/* 0xac */ kKeyNone,		kKeyWWWHome,	// VK_BROWSER_HOME
	/* 0xad */ kKeyNone,		kKeyAudioMute,	// VK_VOLUME_MUTE
	/* 0xae */ kKeyNone,		kKeyAudioDown,	// VK_VOLUME_DOWN
	/* 0xaf */ kKeyNone,		kKeyAudioUp,	// VK_VOLUME_UP
	/* 0xb0 */ kKeyNone,		kKeyAudioNext,	// VK_MEDIA_NEXT_TRACK
	/* 0xb1 */ kKeyNone,		kKeyAudioPrev,	// VK_MEDIA_PREV_TRACK
	/* 0xb2 */ kKeyNone,		kKeyAudioStop,	// VK_MEDIA_STOP
	/* 0xb3 */ kKeyNone,		kKeyAudioPlay,	// VK_MEDIA_PLAY_PAUSE
	/* 0xb4 */ kKeyNone,		kKeyAppMail,	// VK_LAUNCH_MAIL
	/* 0xb5 */ kKeyNone,		kKeyAppMedia,	// VK_LAUNCH_MEDIA_SELECT
	/* 0xb6 */ kKeyNone,		kKeyAppUser1,	// VK_LAUNCH_APP1
	/* 0xb7 */ kKeyNone,		kKeyAppUser2,	// VK_LAUNCH_APP2
	/* 0xb8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xba */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbb */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbc */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbd */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbe */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbf */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xc0 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xc1 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc2 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc3 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc4 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc6 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xca */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcb */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcc */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcd */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xce */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcf */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd0 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd1 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd2 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd3 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd4 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd6 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xda */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xdb */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xdc */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xdd */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xde */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xdf */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe0 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe1 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe2 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe3 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe4 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xe6 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xe8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xe9 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xea */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xeb */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xec */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xed */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xee */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xef */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf0 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf1 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf2 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf3 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf4 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf5 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf6 */ kKeyNone,		kKeyNone,		// VK_ATTN			
	/* 0xf7 */ kKeyNone,		kKeyNone,		// VK_CRSEL			
	/* 0xf8 */ kKeyNone,		kKeyNone,		// VK_EXSEL			
	/* 0xf9 */ kKeyNone,		kKeyNone,		// VK_EREOF			
	/* 0xfa */ kKeyNone,		kKeyNone,		// VK_PLAY			
	/* 0xfb */ kKeyNone,		kKeyNone,		// VK_ZOOM			
	/* 0xfc */ kKeyNone,		kKeyNone,		// reserved
	/* 0xfd */ kKeyNone,		kKeyNone,		// VK_PA1			
	/* 0xfe */ kKeyNone,		kKeyNone,		// VK_OEM_CLEAR		
	/* 0xff */ kKeyNone,		kKeyNone		// reserved
};

// map special KeyID keys to virtual key codes. if the key is an
// extended key then the entry is the virtual key code | 0x100.
// unmapped keys have a 0 entry.
const KeyButton			CMSWindowsKeyMapper::s_mapE000[] =
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
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0,
	/* 0xa4 */ 0, 0, VK_BROWSER_BACK|0x100, VK_BROWSER_FORWARD|0x100,
	/* 0xa8 */ VK_BROWSER_REFRESH|0x100, VK_BROWSER_STOP|0x100,
	/* 0xaa */ VK_BROWSER_SEARCH|0x100, VK_BROWSER_FAVORITES|0x100,
	/* 0xac */ VK_BROWSER_HOME|0x100, VK_VOLUME_MUTE|0x100,
	/* 0xae */ VK_VOLUME_DOWN|0x100, VK_VOLUME_UP|0x100,
	/* 0xb0 */ VK_MEDIA_NEXT_TRACK|0x100, VK_MEDIA_PREV_TRACK|0x100,
	/* 0xb2 */ VK_MEDIA_STOP|0x100, VK_MEDIA_PLAY_PAUSE|0x100,
	/* 0xb4 */ VK_LAUNCH_MAIL|0x100, VK_LAUNCH_MEDIA_SELECT|0x100,
	/* 0xb6 */ VK_LAUNCH_APP1|0x100, VK_LAUNCH_APP2|0x100,
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
const KeyButton			CMSWindowsKeyMapper::s_mapEE00[] =
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
const KeyButton			CMSWindowsKeyMapper::s_mapEF00[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,
	/* 0x10 */ 0, 0, 0, VK_PAUSE, VK_SCROLL, 0/*sys-req*/, 0, 0,
	/* 0x18 */ 0, 0, 0, VK_ESCAPE, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, VK_KANJI, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ VK_HOME|0x100, VK_LEFT|0x100, VK_UP|0x100, VK_RIGHT|0x100,
	/* 0x54 */ VK_DOWN|0x100, VK_PRIOR|0x100, VK_NEXT|0x100, VK_END|0x100,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ VK_SELECT|0x100, VK_SNAPSHOT|0x100, VK_EXECUTE|0x100, VK_INSERT|0x100,
	/* 0x64 */ 0, 0, 0, VK_APPS|0x100,
	/* 0x68 */ 0, 0, VK_HELP|0x100, VK_CANCEL|0x100, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, VK_NUMLOCK|0x100,
	/* 0x80 */ VK_SPACE, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, VK_TAB, 0, 0, 0, VK_RETURN|0x100, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,
	/* 0x98 */ VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT,
	/* 0x9c */ VK_END, 0, VK_INSERT, VK_DELETE,
	/* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa8 */ 0, 0, VK_MULTIPLY, VK_ADD,
	/* 0xac */ VK_DECIMAL, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE|0x100,
	/* 0xb0 */ VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,
	/* 0xb4 */ VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
	/* 0xb8 */ VK_NUMPAD8, VK_NUMPAD9, 0, 0, 0, 0, VK_F1, VK_F2,
	/* 0xc0 */ VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
	/* 0xc8 */ VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18,
	/* 0xd0 */ VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL,
	/* 0xe4 */ VK_RCONTROL|0x100, VK_CAPITAL, 0, 0,
	/* 0xe8 */ 0, VK_LMENU, VK_RMENU|0x100, VK_LWIN|0x100,
	/* 0xec */ VK_RWIN|0x100, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, VK_DELETE|0x100
};

CMSWindowsKeyMapper::CMSWindowsKeyMapper() : m_deadKey(0)
{
	// do nothing
}

CMSWindowsKeyMapper::~CMSWindowsKeyMapper()
{
	// do nothing
}

void
CMSWindowsKeyMapper::update(IKeyState* keyState)
{
	static const size_t numModifiers = sizeof(s_modifiers) /
										sizeof(s_modifiers[0]);

	// clear shadow state
	memset(m_keys, 0, sizeof(m_keys));

	// add modifiers
	if (keyState != NULL) {
		for (size_t i = 0; i < numModifiers; ++i) {
			IKeyState::KeyButtons keys;
			for (size_t j = 0; j < CModifierKeys::s_maxKeys; ++j) {
				if (s_modifiers[i].m_keys[j] != 0) {
					keys.push_back(s_modifiers[i].m_keys[j]);
				}
			}
			keyState->addModifier(s_modifiers[i].m_mask, keys);
		}
	}

	// save current state of modifiers
	for (size_t i = 0; i < numModifiers; ++i) {
		for (size_t j = 0; j < CModifierKeys::s_maxKeys; ++j) {
			if (s_modifiers[i].m_keys[j] != 0) {
				SHORT s = GetKeyState(s_modifiers[i].m_keys[j]);
				m_keys[s_modifiers[i].m_keys[j]] = static_cast<BYTE>(s);
				if (keyState != NULL) {
					if ((s & 0x01) != 0) {
						keyState->setToggled(s_modifiers[i].m_mask);
					}
					if ((s & 0x80) != 0) {
						keyState->setKeyDown(s_modifiers[i].m_keys[j]);
					}
				}
			}
		}
	}
}

void
CMSWindowsKeyMapper::updateKey(KeyButton key, bool pressed)
{
	if (pressed) {
		switch (key) {
		case 0:
		case VK_LBUTTON:
		case VK_MBUTTON:
		case VK_RBUTTON:
			// ignore bogus key
			break;

		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_SHIFT:
			m_keys[key]        |= 0x80;
			m_keys[VK_SHIFT]   |= 0x80;
			break;

		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_CONTROL:
			m_keys[key]        |= 0x80;
			m_keys[VK_CONTROL] |= 0x80;
			break;

		case VK_LMENU:
		case VK_RMENU:
		case VK_MENU:
			m_keys[key]        |= 0x80;
			m_keys[VK_MENU]    |= 0x80;
			break;

		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// toggle keys
			m_keys[key]        |= 0x80;
			break;

		default:
		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
			m_keys[key]        |= 0x80;
			break;
		}

		// special case:  we detect ctrl+alt+del being pressed on some
		// systems but we don't detect the release of those keys.  so
		// if ctrl, alt, and del are down then mark them up.
		if ((m_keys[VK_CONTROL] & 0x80) != 0 &&
			(m_keys[VK_MENU]    & 0x80) != 0 &&
			(m_keys[VK_DELETE]  & 0x80) != 0) {
			m_keys[VK_LCONTROL] &= ~0x80;
			m_keys[VK_RCONTROL] &= ~0x80;
			m_keys[VK_CONTROL]  &= ~0x80;
			m_keys[VK_LMENU]    &= ~0x80;
			m_keys[VK_RMENU]    &= ~0x80;
			m_keys[VK_MENU]     &= ~0x80;
			m_keys[VK_DELETE]   &= ~0x80;
		}
	}
	else {
		switch (key) {
		case 0:
		case VK_LBUTTON:
		case VK_MBUTTON:
		case VK_RBUTTON:
			// ignore bogus key
			break;

		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_SHIFT:
			m_keys[key]            &= ~0x80;
			if (((m_keys[VK_LSHIFT] | m_keys[VK_RSHIFT]) & 0x80) == 0) {
				m_keys[VK_SHIFT]   &= ~0x80;
			}
			break;

		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_CONTROL:
			m_keys[key]            &= ~0x80;
			if (((m_keys[VK_LCONTROL] | m_keys[VK_RCONTROL]) & 0x80) == 0) {
				m_keys[VK_CONTROL] &= ~0x80;
			}
			break;

		case VK_LMENU:
		case VK_RMENU:
		case VK_MENU:
			m_keys[key]            &= ~0x80;
			if (((m_keys[VK_LMENU] | m_keys[VK_RMENU]) & 0x80) == 0) {
				m_keys[VK_MENU]    &= ~0x80;
			}
			break;

		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// toggle keys
			m_keys[key]            &= ~0x80;
			m_keys[key]            ^=  0x01;
			break;

		default:
		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
			m_keys[key]            &= ~0x80;
			break;
		}
	}
}

KeyButton
CMSWindowsKeyMapper::mapKey(IKeyState::Keystrokes& keys,
				const IKeyState& keyState, KeyID id,
				KeyModifierMask mask, bool isAutoRepeat) const
{
	KeyButton virtualKey = 0;

	// check for special keys
	if ((id & 0xfffff000u) == 0xe000u) {
		if ((id & 0xff00u) == 0xe000u) {
			virtualKey = s_mapE000[id & 0xffu];
		}
		else if ((id & 0xff00) == 0xee00) {
			virtualKey = s_mapEE00[id & 0xffu];
		}
		else if ((id & 0xff00) == 0xef00) {
			virtualKey = s_mapEF00[id & 0xffu];
		}
		if (virtualKey == 0) {
			LOG((CLOG_DEBUG2 "unknown special key"));
			return virtualKey;
		}
	}

	// special handling of VK_SNAPSHOT
	if ((virtualKey & 0xffu) == VK_SNAPSHOT) {
		// ignore key repeats on print screen
		if (!isAutoRepeat) {
			// get event flags
			DWORD flags = 0;
			if (isExtendedKey(virtualKey)) {
				flags |= KEYEVENTF_EXTENDEDKEY;
			}

			// active window (with alt) or fullscreen (without alt)?
			BYTE scan = 0;
			if ((mask & KeyModifierAlt) != 0) {
				scan = 1;
			}

			// send events
			keybd_event(static_cast<BYTE>(virtualKey & 0xffu), scan, flags, 0);
			flags |= KEYEVENTF_KEYUP;
			keybd_event(static_cast<BYTE>(virtualKey & 0xffu), scan, flags, 0);
		}
		return 0;
	}

	// handle other special keys
	if (virtualKey != 0) {
		// compute required modifiers
		KeyModifierMask requiredMask = 0;
		KeyModifierMask outMask      = 0;

		// strip out extended key flag
		UINT virtualKey2 = (virtualKey & 0xffu);

		// check numeric keypad.  note that virtual keys do not distinguish
		// between the keypad and non-keypad movement keys.  however, the
		// virtual keys do distinguish between keypad numbers and operators
		// (e.g. add, multiply) and their main keyboard counterparts.
		// therefore, we can ignore the num-lock state for movement virtual
		// keys but not for numeric keys.
		if (virtualKey2 >= VK_NUMPAD0 && virtualKey2 <= VK_DIVIDE) {
			requiredMask |= KeyModifierNumLock;
			if (!keyState.isModifierActive(KeyModifierNumLock)) {
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
		LOG((CLOG_DEBUG2 "KeyID 0x%08x to virtual key %d mask 0x%04x", id, virtualKey2, outMask));
		return mapToKeystrokes(keys, keyState, virtualKey,
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
	virtualKey = mapCharacter(keys, keyState, multiByte[0], hkl, isAutoRepeat);
	if (virtualKey != 0) {
		LOG((CLOG_DEBUG2 "KeyID 0x%08x maps to character %u", id, (unsigned char)multiByte[0]));
		if (isDeadChar(multiByte[0], hkl, false)) {
			// character mapped to a dead key but we want the
			// character for real so send a space key afterwards.
			LOG((CLOG_DEBUG2 "character mapped to dead key"));
			IKeyState::Keystroke keystroke;
			keystroke.m_key    = VK_SPACE;
			keystroke.m_press  = true;
			keystroke.m_repeat = false;
			keys.push_back(keystroke);
			keystroke.m_press  = false;
			keys.push_back(keystroke);

			// ignore the release of this key since we already
			// handled it.
			virtualKey = 0;
		}
		return virtualKey;
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
		mapCharacter(keys, keyState, multiByte[1], hkl, isAutoRepeat);
	}

	// process character
	LOG((CLOG_DEBUG2 "KeyID 0x%08x maps to character %u", id, (unsigned char)multiByte[0]));
	virtualKey = mapCharacter(keys, keyState, multiByte[0], hkl, isAutoRepeat);

	return virtualKey;
}

KeyID
CMSWindowsKeyMapper::mapKeyFromEvent(WPARAM vkCode, LPARAM info,
				KeyModifierMask* maskOut, bool* altgr) const
{
	// note:  known microsoft bugs
	//  Q72583 -- MapVirtualKey() maps keypad keys incorrectly
	//    95,98: num pad vk code -> invalid scan code
	//    95,98,NT4: num pad scan code -> bad vk code except
	//      SEPARATOR, MULTIPLY, SUBTRACT, ADD

	HKL hkl = GetKeyboardLayout(0);

	// get the scan code and the extended keyboard flag
	UINT scanCode = static_cast<UINT>((info & 0x00ff0000u) >> 16);
	int extended  = ((info & 0x01000000) == 0) ? 0 : 1;
	bool press    = ((info & 0x80000000) == 0);
	LOG((CLOG_DEBUG1 "key vk=%d info=0x%08x ext=%d scan=%d", vkCode, info, extended, scanCode));

	// handle some keys via table lookup
	char c   = 0;
	KeyID id = s_virtualKey[vkCode][extended];
	if (id == kKeyNone) {
		// not in table

		// save the control state then clear it.  ToAscii() maps ctrl+letter
		// to the corresponding control code and ctrl+backspace to delete.
		// we don't want that translation so we clear the control modifier
		// state.  however, if we want to simulate AltGr (which is ctrl+alt)
		// then we must not clear it.
		BYTE keys[256];
		memcpy(keys, m_keys, sizeof(keys));
		BYTE control = keys[VK_CONTROL];
		BYTE menu    = keys[VK_MENU];
		if ((control & 0x80) == 0 || (menu & 0x80) == 0) {
			keys[VK_LCONTROL] = 0;
			keys[VK_RCONTROL] = 0;
			keys[VK_CONTROL]  = 0;
		}
		else {
			keys[VK_LCONTROL] = 0x80;
			keys[VK_CONTROL]  = 0x80;
			keys[VK_LMENU]    = 0x80;
			keys[VK_MENU]     = 0x80;
		}

		// map to a character
		bool isMenu = ((menu & 0x80) != 0);
		c = mapToCharacter(vkCode, scanCode, keys, press, isMenu, hkl);

		// if mapping failed and ctrl and alt are pressed then try again
		// with both not pressed.  this handles the case where ctrl and
		// alt are being used as individual modifiers rather than AltGr.
		if (c == 0 && (control & 0x80) != 0 && (menu & 0x80) != 0) {
			keys[VK_LCONTROL] = 0;
			keys[VK_RCONTROL] = 0;
			keys[VK_CONTROL]  = 0;
			keys[VK_LMENU]    = 0;
			keys[VK_RMENU]    = 0;
			keys[VK_MENU]     = 0;
			c = mapToCharacter(vkCode, scanCode, keys, press, isMenu, hkl);
		}

		// map character to key id
		if (c != 0) {
			if ((c & 0x80u) != 0) {
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
			else {
				id = static_cast<KeyID>(c) & 0xffu;
			}
		}
	}

	// set mask
	bool needAltGr = false;
	if (id != kKeyNone && id != kKeyMultiKey && c != 0) {
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
		WORD virtualKeyAndModifierState = VkKeyScanEx(c, hkl);
		if (virtualKeyAndModifierState == 0xffff) {
			// there is no mapping.  assume AltGr.
			LOG((CLOG_DEBUG1 "no VkKeyScan() mapping"));
			needAltGr = true;
		}
		else if (LOBYTE(virtualKeyAndModifierState) != vkCode) {
			// we didn't get the key that was actually pressed
			LOG((CLOG_DEBUG1 "VkKeyScan() mismatch"));
			if ((m_keys[VK_CONTROL] & 0x80) != 0 &&
				(m_keys[VK_MENU] & 0x80) != 0) {
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
	if (altgr != NULL) {
		*altgr = needAltGr;
	}

	// map modifier key
	KeyModifierMask mask = 0;
	if (((m_keys[VK_LSHIFT] |
		  m_keys[VK_RSHIFT] |
		  m_keys[VK_SHIFT]) & 0x80) != 0) {
		mask |= KeyModifierShift;
	}
	if (needAltGr) {
		mask |= KeyModifierModeSwitch;
	}
	else {
		if (((m_keys[VK_LCONTROL] |
			  m_keys[VK_RCONTROL] |
			  m_keys[VK_CONTROL]) & 0x80) != 0) {
			mask |= KeyModifierControl;
		}
		if (((m_keys[VK_LMENU] |
			  m_keys[VK_RMENU] |
			  m_keys[VK_MENU]) & 0x80) != 0) {
			mask |= KeyModifierAlt;
		}
	}
	if (((m_keys[VK_LWIN] |
		  m_keys[VK_RWIN]) & 0x80) != 0) {
		mask |= KeyModifierSuper;
	}
	if ((m_keys[VK_CAPITAL] & 0x01) != 0) {
		mask |= KeyModifierCapsLock;
	}
	if ((m_keys[VK_NUMLOCK] & 0x01) != 0) {
		mask |= KeyModifierNumLock;
	}
	if ((m_keys[VK_SCROLL] & 0x01) != 0) {
		mask |= KeyModifierScrollLock;
	}
	if (maskOut != NULL) {
		*maskOut = mask;
	}

	return id;
}

bool
CMSWindowsKeyMapper::isPressed(KeyButton key) const
{
	return ((m_keys[key & 0xffu] & 0x80) != 0);
}

UINT
CMSWindowsKeyMapper::keyToScanCode(KeyButton* virtualKey) const
{
	// try mapping given virtual key
	HKL hkl   = GetKeyboardLayout(0);
	UINT code = MapVirtualKeyEx((*virtualKey) & 0xffu, 0, hkl);
	if (code != 0) {
		return code;
	}

	// no dice.  if the virtual key distinguishes between left/right
	// then try the one that doesn't distinguish sides.  windows (or
	// keyboard drivers) are inconsistent in their treatment of these
	// virtual keys.  the following behaviors have been observed:
	//
	//  win2k (gateway desktop):
	//      MapVirtualKey(vk, 0):
	//        VK_SHIFT == VK_LSHIFT != VK_RSHIFT
	//        VK_CONTROL == VK_LCONTROL == VK_RCONTROL
	//        VK_MENU == VK_LMENU == VK_RMENU
	//      MapVirtualKey(sc, 3):
	//        VK_LSHIFT and VK_RSHIFT mapped independently
	//        VK_LCONTROL is mapped but not VK_RCONTROL
	//        VK_LMENU is mapped but not VK_RMENU
	//
	//  win me (sony vaio laptop):
	//      MapVirtualKey(vk, 0):
	//        VK_SHIFT mapped;  VK_LSHIFT, VK_RSHIFT not mapped
	//        VK_CONTROL mapped;  VK_LCONTROL, VK_RCONTROL not mapped
	//        VK_MENU mapped;  VK_LMENU, VK_RMENU not mapped
	//      MapVirtualKey(sc, 3):
	//        all scan codes unmapped (function apparently unimplemented)
	switch ((*virtualKey) & 0xffu) {
	case VK_LSHIFT:
	case VK_RSHIFT:
		*virtualKey = VK_SHIFT;
		return MapVirtualKeyEx(VK_SHIFT, 0, hkl);

	case VK_LCONTROL:
	case VK_RCONTROL:
		*virtualKey = VK_CONTROL;
		return MapVirtualKeyEx(VK_CONTROL, 0, hkl);

	case VK_LMENU:
	case VK_RMENU:
		*virtualKey = VK_MENU;
		return MapVirtualKeyEx(VK_MENU, 0, hkl);

	default:
		return 0;
	}
}

bool
CMSWindowsKeyMapper::isExtendedKey(KeyButton virtualKey) const
{
	// see if we've already encoded the extended flag
	if ((virtualKey & 0x100u) != 0) {
		return true;
	}

	// check known virtual keys
	switch (virtualKey & 0xffu) {
	case VK_NUMLOCK:
	case VK_RCONTROL:
	case VK_RMENU:
	case VK_LWIN:
	case VK_RWIN:
	case VK_APPS:
		return true;

	default:
		return false;
	}
}

const char*
CMSWindowsKeyMapper::getKeyName(KeyButton key) const
{
	return s_vkToName[key & 0xffu];
}

UINT
CMSWindowsKeyMapper::getCodePageFromLangID(LANGID langid) const
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
CMSWindowsKeyMapper::mapCharacter(IKeyState::Keystrokes& keys,
				const IKeyState& keyState, char c, HKL hkl,
				bool isAutoRepeat) const
{
	// translate the character into its virtual key and its required
	// modifier state.
	SHORT virtualKeyAndModifierState = VkKeyScanEx(c, hkl);

	// get virtual key
	KeyButton virtualKey = LOBYTE(virtualKeyAndModifierState);
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
	// by shift and should match the shift state from keyState.
	KeyModifierMask desiredMask  = 0;
	KeyModifierMask requiredMask = KeyModifierShift;
	if (c == 32) {
		if (keyState.isModifierActive(KeyModifierShift)) {
			desiredMask |= KeyModifierShift;
		}
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
	if (keyState.isModifierActive(KeyModifierCapsLock)) {
		// there doesn't seem to be a simple way to test if a
		// character respects the caps lock key.  for normal
		// characters it's easy enough but CharLower() and
		// CharUpper() don't map dead keys even though they
		// do respect caps lock for some unfathomable reason.
		// first check the easy way.  if that doesn't work
		// then see if it's a dead key.
		unsigned char uc = static_cast<unsigned char>(c);
		if (CharLower((LPTSTR)uc) != CharUpper((LPTSTR)uc) ||
			(MapVirtualKeyEx(virtualKey, 2, hkl) & 0x80000000lu) != 0) {
			LOG((CLOG_DEBUG2 "flip shift"));
			desiredMask ^= KeyModifierShift;
		}
	}

	// now generate the keystrokes.  ignore the resulting modifier
	// mask since it can't have changed (because we don't call this
	// method for modifier keys).
	LOG((CLOG_DEBUG2 "character %d to virtual key %d mask 0x%08x", (unsigned char)c, virtualKey, desiredMask));
	mapToKeystrokes(keys, keyState, virtualKey,
								desiredMask, requiredMask, isAutoRepeat);

	return virtualKey;
}

KeyButton
CMSWindowsKeyMapper::mapToKeystrokes(IKeyState::Keystrokes& keys,
				const IKeyState& keyState, KeyButton virtualKey,
				KeyModifierMask desiredMask, KeyModifierMask requiredMask,
				bool isAutoRepeat) const
{
	// adjust the modifiers to match the desired modifiers
	IKeyState::Keystrokes undo;
	if (!adjustModifiers(keys, undo, keyState, desiredMask, requiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		keys.clear();
		return 0;
	}

	// add the key event
	IKeyState::Keystroke keystroke;
	keystroke.m_key    = virtualKey;
	keystroke.m_press  = true;
	keystroke.m_repeat = isAutoRepeat;
	keys.push_back(keystroke);

	// put undo keystrokes at end of keystrokes in reverse order
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	return virtualKey;
}

bool
CMSWindowsKeyMapper::adjustModifiers(IKeyState::Keystrokes& keys,
				IKeyState::Keystrokes& undo,
				const IKeyState& keyState,
				KeyModifierMask desiredMask,
				KeyModifierMask requiredMask) const
{
	// for each modifier in requiredMask make sure the current state
	// of that modifier matches the bit in desiredMask.
	for (KeyModifierMask mask = 1u; requiredMask != 0; mask <<= 1) {
		if ((mask & requiredMask) != 0) {
			bool active = ((desiredMask & mask) != 0);
			if (!keyState.mapModifier(keys, undo, mask, active)) {
				return false;
			}
			requiredMask ^= mask;
		}
	}

	return true;
}

int
CMSWindowsKeyMapper::toAscii(TCHAR c, HKL hkl, bool menu, WORD* chars) const
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
CMSWindowsKeyMapper::isDeadChar(TCHAR c, HKL hkl, bool menu) const
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
	if (old == 2) {
		toAscii(static_cast<TCHAR>(ascii & 0xffu), hkl, menu, &dummy);
	}

	return isDead;
}

bool
CMSWindowsKeyMapper::putBackDeadChar(TCHAR c, HKL hkl, bool menu) const
{
	return (toAscii(c, hkl, menu, NULL) < 0);
}

TCHAR
CMSWindowsKeyMapper::getSavedDeadChar(HKL hkl) const
{
	WORD old;
	int nOld = toAscii(' ', hkl, false, &old);
	if (nOld == 1 || nOld == 2) {
		TCHAR c = static_cast<TCHAR>(old & 0xffu);
		if (nOld == 2 || isDeadChar(c, hkl, false)) {
			return c;
		}
	}
	return 0;
}

char
CMSWindowsKeyMapper::mapToCharacter(UINT vkCode, UINT scanCode,
				BYTE* keys, bool press, bool isMenu, HKL hkl) const
{
	// get contents of keyboard layout buffer and clear out that
	// buffer.  we don't want anything placed there by some other
	// app interfering and we need to put anything there back in
	// place when we're done.
	TCHAR oldDeadKey = getSavedDeadChar(hkl);

	// put our previous dead key, if any, in the layout buffer
	putBackDeadChar(m_deadKey, hkl, false);
	m_deadKey = 0;

	// process key
	WORD ascii;
	int result = ToAsciiEx(vkCode, scanCode, keys, &ascii,
							isMenu ? 1 : 0, hkl);

	// if result is less than zero then it was a dead key
	char c = 0;
	if (result < 0) {
		// save dead key if a key press.  we catch the dead key
		// release in the result == 2 case below.
		if (press) {
			m_deadKey = static_cast<TCHAR>(ascii & 0xffu);
		}
	}

	// if result is 1 then the key was succesfully converted
	else if (result == 1) {
		c = static_cast<char>(ascii & 0xff);
	}

	// if result is 2 and the two characters are the same and this
	// is a key release then a dead key was released.  save the
	// dead key.  if the two characters are the same and this is
	// not a release then a dead key was pressed twice.  send the
	// dead key.
	else if (result == 2) {
		if (((ascii & 0xff00u) >> 8) == (ascii & 0x00ffu)) {
			if (!press) {
				m_deadKey = static_cast<TCHAR>(ascii & 0xffu);
			}
			else {
				putBackDeadChar(oldDeadKey, hkl, false);
				result = toAscii(' ', hkl, false, &ascii);
				c = static_cast<char>((ascii >> 8) & 0xffu);
			}
		}
	}

	// clear keyboard layout buffer.  this removes any dead key we
	// may have just put there.
	toAscii(' ', hkl, false, NULL);

	// restore keyboard layout buffer so a dead key inserted by
	// another app doesn't disappear mysteriously (from its point
	// of view).
	putBackDeadChar(oldDeadKey, hkl, false);

	return c;
}
