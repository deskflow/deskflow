/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// keymap.h - this file is shared between SInput.cxx and CKeyboard.cxx
//
// Mapping of X keysyms to and from Windows VK codes.  Ordering here must be
// such that when we look up a Windows VK code we get the preferred X keysym.
// Going the other way there is no problem because an X keysym always maps to
// exactly one Windows VK code.  This map only contain keys which are not the
// normal keys for printable ASCII characters.  For example it does not contain
// VK_SPACE (note that things like VK_ADD are for the plus key on the keypad,
// not on the main keyboard).

struct keymap_t {
  rdr::U32 keysym;
  rdr::U8 vk;
  bool extended;
};

static keymap_t keymap[] = {

  { XK_BackSpace,        VK_BACK, 0 },
  { XK_Tab,              VK_TAB, 0 },
  { XK_Clear,            VK_CLEAR, 0 },
  { XK_Return,           VK_RETURN, 0 },
  { XK_Pause,            VK_PAUSE, 0 },
  { XK_Escape,           VK_ESCAPE, 0 },
  { XK_Delete,           VK_DELETE, 1 },

  // Cursor control & motion

  { XK_Home,             VK_HOME, 1 },
  { XK_Left,             VK_LEFT, 1 },
  { XK_Up,               VK_UP, 1 },
  { XK_Right,            VK_RIGHT, 1 },
  { XK_Down,             VK_DOWN, 1 },
  { XK_Page_Up,          VK_PRIOR, 1 },
  { XK_Page_Down,        VK_NEXT, 1 },
  { XK_End,              VK_END, 1 },

  // Misc functions

  { XK_Select,           VK_SELECT, 0 },
  { XK_Print,            VK_SNAPSHOT, 0 },
  { XK_Execute,          VK_EXECUTE, 0 },
  { XK_Insert,           VK_INSERT, 1 },
  { XK_Help,             VK_HELP, 0 },
  { XK_Break,            VK_CANCEL, 1 },

  // Auxilliary Functions - must come before XK_KP_F1, etc

  { XK_F1,               VK_F1, 0 },
  { XK_F2,               VK_F2, 0 },
  { XK_F3,               VK_F3, 0 },
  { XK_F4,               VK_F4, 0 },
  { XK_F5,               VK_F5, 0 },
  { XK_F6,               VK_F6, 0 },
  { XK_F7,               VK_F7, 0 },
  { XK_F8,               VK_F8, 0 },
  { XK_F9,               VK_F9, 0 },
  { XK_F10,              VK_F10, 0 },
  { XK_F11,              VK_F11, 0 },
  { XK_F12,              VK_F12, 0 },
  { XK_F13,              VK_F13, 0 },
  { XK_F14,              VK_F14, 0 },
  { XK_F15,              VK_F15, 0 },
  { XK_F16,              VK_F16, 0 },
  { XK_F17,              VK_F17, 0 },
  { XK_F18,              VK_F18, 0 },
  { XK_F19,              VK_F19, 0 },
  { XK_F20,              VK_F20, 0 },
  { XK_F21,              VK_F21, 0 },
  { XK_F22,              VK_F22, 0 },
  { XK_F23,              VK_F23, 0 },
  { XK_F24,              VK_F24, 0 },

  // Keypad Functions, keypad numbers

  { XK_KP_Tab,           VK_TAB, 0 },
  { XK_KP_Enter,         VK_RETURN, 1 },
  { XK_KP_F1,            VK_F1, 0 },
  { XK_KP_F2,            VK_F2, 0 },
  { XK_KP_F3,            VK_F3, 0 },
  { XK_KP_F4,            VK_F4, 0 },
  { XK_KP_Home,          VK_HOME, 0 },
  { XK_KP_Left,          VK_LEFT, 0 },
  { XK_KP_Up,            VK_UP, 0 },
  { XK_KP_Right,         VK_RIGHT, 0 },
  { XK_KP_Down,          VK_DOWN, 0 },
  { XK_KP_End,           VK_END, 0 },
  { XK_KP_Page_Up,       VK_PRIOR, 0 },
  { XK_KP_Page_Down,     VK_NEXT, 0 },
  { XK_KP_Begin,         VK_CLEAR, 0 },
  { XK_KP_Insert,        VK_INSERT, 0 },
  { XK_KP_Delete,        VK_DELETE, 0 },
  { XK_KP_Multiply,      VK_MULTIPLY, 0 },
  { XK_KP_Add,           VK_ADD, 0 },
  { XK_KP_Separator,     VK_SEPARATOR, 0 },
  { XK_KP_Subtract,      VK_SUBTRACT, 0 },
  { XK_KP_Decimal,       VK_DECIMAL, 0 },
  { XK_KP_Divide,        VK_DIVIDE, 1 },

  { XK_KP_0,             VK_NUMPAD0, 0 },
  { XK_KP_1,             VK_NUMPAD1, 0 },
  { XK_KP_2,             VK_NUMPAD2, 0 },
  { XK_KP_3,             VK_NUMPAD3, 0 },
  { XK_KP_4,             VK_NUMPAD4, 0 },
  { XK_KP_5,             VK_NUMPAD5, 0 },
  { XK_KP_6,             VK_NUMPAD6, 0 },
  { XK_KP_7,             VK_NUMPAD7, 0 },
  { XK_KP_8,             VK_NUMPAD8, 0 },
  { XK_KP_9,             VK_NUMPAD9, 0 },

  // Modifiers
    
  { XK_Shift_L,          VK_SHIFT, 0 },
  { XK_Shift_R,          VK_SHIFT, 0 },
  { XK_Control_L,        VK_CONTROL, 0 },
  { XK_Control_R,        VK_CONTROL, 1 },
  { XK_Alt_L,            VK_MENU, 0 },
  { XK_Alt_R,            VK_MENU, 1 },

  // Left & Right Windows keys & Windows Menu Key

  { XK_Super_L,          VK_LWIN, 0 },
  { XK_Super_R,          VK_RWIN, 0 },
  { XK_Menu,             VK_APPS, 0 },

  // Japanese stuff - almost certainly wrong...

  { XK_Kanji,            VK_KANJI, 0 },
  { XK_Kana_Shift,       VK_KANA, 0 },

};
