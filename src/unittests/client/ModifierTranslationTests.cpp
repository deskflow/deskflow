/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ModifierTranslation.h"

#include <iostream>

namespace {
int failures = 0;

void check(bool condition, const char *description)
{
  if (!condition) {
    std::cerr << "FAIL: " << description << '\n';
    ++failures;
  }
}

void checkSwap(const ModifierTranslation &mapping)
{
  check(mapping.isAutoSwitchActive(), "automatic mapping is active");
  check(mapping.translateKey(kKeyAlt_L) == kKeySuper_L, "left Alt becomes left Super");
  check(mapping.translateKey(kKeyAlt_R) == kKeySuper_R, "right Alt becomes right Super");
  check(mapping.translateKey(kKeySuper_L) == kKeyAlt_L, "left Super becomes left Alt");
  check(mapping.translateKey(kKeySuper_R) == kKeyAlt_R, "right Super becomes right Alt");
  check(mapping.translateKey(kKeyAltGr) == kKeyAltGr, "AltGr is unchanged");
  check(mapping.translateKey(kKeyControl_L) == kKeyControl_L, "Ctrl is unchanged");
  check(mapping.translateKey('c') == 'c', "character keys are unchanged");
  check(
      mapping.translateMask(KeyModifierAlt | KeyModifierShift | KeyModifierCapsLock) ==
          (KeyModifierSuper | KeyModifierShift | KeyModifierCapsLock),
      "shortcut and toggle masks keep unrelated bits"
  );
  check(
      mapping.translateMask(KeyModifierAlt | KeyModifierSuper) == (KeyModifierAlt | KeyModifierSuper),
      "simultaneously held modifiers remain active"
  );
}

void checkIdentity(const ModifierTranslation &mapping)
{
  check(!mapping.isAutoSwitchActive(), "automatic mapping is inactive");
  check(mapping.translateKey(kKeyAlt_L) == kKeyAlt_L, "Alt remains Alt");
  check(mapping.translateKey(kKeySuper_L) == kKeySuper_L, "Super remains Super");
  check(mapping.translateMask(KeyModifierAlt) == KeyModifierAlt, "Alt mask remains Alt");
}
} // namespace

int main()
{
  ModifierTranslation mac(kKeyboardPlatformMacOS);
  checkIdentity(mac); // An older server does not advertise its platform.
  mac.setOptions({
      kOptionModifierMapForAlt, kKeyModifierIDAlt, kOptionModifierMapForSuper, kKeyModifierIDSuper,
      kOptionAutoModifierSwitch, 1, kOptionServerKeyboardPlatform, kKeyboardPlatformWindows
  });
  checkSwap(mac);

  ModifierTranslation windows(kKeyboardPlatformWindows);
  windows.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformMacOS});
  checkSwap(windows);

  mac.setOptions({kOptionAutoModifierSwitch, 0});
  checkIdentity(mac);
  mac.setOptions({kOptionAutoModifierSwitch, 1});
  checkSwap(mac);

  mac.setOptions({kOptionModifierMapForControl, kKeyModifierIDSuper});
  checkIdentity(mac);
  check(mac.translateKey(kKeyControl_R) == kKeySuper_R, "manual Ctrl mapping takes precedence");
  check(mac.translateMask(KeyModifierControl) == KeyModifierSuper, "manual modifier mask is preserved");
  mac.setOptions({kOptionModifierMapForControl, kKeyModifierIDControl});
  checkSwap(mac);

  mac.setOptions({kOptionModifierMapForAlt, kKeyModifierIDNull});
  check(!mac.isAutoSwitchActive(), "a manual Alt mapping disables automatic switching");
  check(mac.translateKey(kKeyAlt_L) == kKeyNone, "manual None mapping disables the key");
  check(mac.translateMask(KeyModifierAlt | KeyModifierShift) == KeyModifierShift, "manual None clears the mask");
  mac.setOptions({kOptionModifierMapForAlt, kKeyModifierIDAlt});
  checkSwap(mac);

  mac.setOptions({kOptionModifierMapForAltGr, kKeyModifierIDControl});
  check(mac.translateKey(kKeyAltGr) == kKeyControl_R, "mapped AltGr keeps the right side");
  mac.reset();
  checkIdentity(mac);
  mac.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformMacOS});
  checkIdentity(mac);
  mac.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformOther});
  checkIdentity(mac);
  mac.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformWindows});
  checkSwap(mac);
  mac.setOptions({kOptionAutoModifierSwitch}); // Reject incomplete option pairs.
  checkSwap(mac);

  ModifierTranslation current;
#if defined(_WIN32)
  current.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformMacOS});
  checkSwap(current);
#elif defined(__APPLE__)
  current.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformWindows});
  checkSwap(current);
#else
  current.setOptions({kOptionServerKeyboardPlatform, kKeyboardPlatformWindows});
  checkIdentity(current);
#endif

  if (failures != 0) {
    std::cerr << failures << " modifier mapping check(s) failed\n";
    return 1;
  }
  std::cout << "Modifier mapping checks passed\n";
  return 0;
}
