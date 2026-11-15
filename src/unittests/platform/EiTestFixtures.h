/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"

namespace ei_test {

class TestAppUtil : public AppUtil
{
public:
  int run() override
  {
    return 0;
  }

  std::vector<std::string> getKeyboardLayoutList() override
  {
    return {"en"};
  }

  std::string getCurrentLanguageCode() override
  {
    return "en";
  }
};

inline constexpr char CaptureKeymap[] = R"XKB(xkb_keymap {
xkb_keycodes "capture" {
    minimum = 8; maximum = 255;
    <AD01> = 24; <AD03> = 26; <LFSH> = 50; <NMLK> = 90;
    <KP1> = 91; <GRP> = 92; <MULT> = 93; <UNSP> = 94;
    <FK01> = 67;
    <FK02> = 68;
    <FK03> = 69;
    <FK04> = 70;
    <FK05> = 71;
    <FK06> = 72;
    <FK07> = 73;
    <FK08> = 74;
    <FK09> = 75;
    <FK10> = 76;
    <FK11> = 77;
    <FK12> = 78;
};
xkb_types "capture" {
    type "ONE_LEVEL" { modifiers = none; };
    type "ALPHABETIC" {
        modifiers = Shift+Lock;
        map[Shift] = Level2; map[Lock] = Level2;
        map[Shift+Lock] = Level1;
    };
    type "LEVEL3" { modifiers = Mod5; map[Mod5] = Level2; };
    type "KEYPAD" { modifiers = Mod2; map[Mod2] = Level2; };
    type "FUNCTION" { modifiers = Control+Mod1; map[Control+Mod1] = Level2; };
};
xkb_compat "capture" {
    interpret Shift_L+AnyOfOrNone(all) { action = SetMods(modifiers=Shift); };
    interpret Num_Lock+AnyOfOrNone(all) { action = LockMods(modifiers=Mod2); };
    interpret ISO_Next_Group+AnyOfOrNone(all) { action = LockGroup(group=+1); };
};
xkb_symbols "capture" {
    name[Group1] = "English"; name[Group2] = "Russian";
    key <AD01> { type="ALPHABETIC", symbols[Group1]=[ q, Q ],
                                   symbols[Group2]=[ Cyrillic_shorti, Cyrillic_SHORTI ] };
    key <AD03> { type="LEVEL3", [ e, EuroSign ] };
    key <LFSH> { [ Shift_L ] };
    key <NMLK> { [ Num_Lock ] };
    key <KP1> { type="KEYPAD", [ KP_End, KP_1 ] };
    key <GRP> { [ ISO_Next_Group ] };
    key <MULT> { [ { a, b } ] };
    key <UNSP> { [ 0x1008ffff ] };
    modifier_map Shift { <LFSH> }; modifier_map Mod2 { <NMLK> };
    key <FK01> { type="FUNCTION", [ F1, XF86Switch_VT_1 ] };
    key <FK02> { type="FUNCTION", [ F2, XF86Switch_VT_2 ] };
    key <FK03> { type="FUNCTION", [ F3, XF86Switch_VT_3 ] };
    key <FK04> { type="FUNCTION", [ F4, XF86Switch_VT_4 ] };
    key <FK05> { type="FUNCTION", [ F5, XF86Switch_VT_5 ] };
    key <FK06> { type="FUNCTION", [ F6, XF86Switch_VT_6 ] };
    key <FK07> { type="FUNCTION", [ F7, XF86Switch_VT_7 ] };
    key <FK08> { type="FUNCTION", [ F8, XF86Switch_VT_8 ] };
    key <FK09> { type="FUNCTION", [ F9, XF86Switch_VT_9 ] };
    key <FK10> { type="FUNCTION", [ F10, XF86Switch_VT_10 ] };
    key <FK11> { type="FUNCTION", [ F11, XF86Switch_VT_11 ] };
    key <FK12> { type="FUNCTION", [ F12, XF86Switch_VT_12 ] };
};
};)XKB";

} // namespace ei_test
