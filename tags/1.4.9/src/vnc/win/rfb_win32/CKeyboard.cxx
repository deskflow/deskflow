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

#include <map>

#define XK_MISCELLANY
#define XK_LATIN1
#define XK_CURRENCY
#include <rfb/keysymdef.h>

#include <rfb_win32/CKeyboard.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/OSVersion.h>
#include "keymap.h"

#pragma warning(disable: 4800)

using namespace rfb;

static LogWriter vlog("CKeyboard");


// Client-side RFB keyboard event sythesis

class CKeymapper {

public:
  CKeymapper()
  {
    for (int i = 0; i < sizeof(keymap) / sizeof(keymap_t); i++) {
      int extendedVkey = keymap[i].vk + (keymap[i].extended ? 256 : 0);
      if (keysymMap.find(extendedVkey) == keysymMap.end()) {
        keysymMap[extendedVkey] = keymap[i].keysym;
      }
    }
  }

  // lookup() tries to find a match for vkey with the extended flag.  We check
  // first for an exact match including the extended flag, then try without the
  // extended flag.
  rdr::U32 lookup(int extendedVkey) {
    if (keysymMap.find(extendedVkey) != keysymMap.end())
      return keysymMap[extendedVkey];
    if (keysymMap.find(extendedVkey ^ 256) != keysymMap.end())
      return keysymMap[extendedVkey ^ 256];
    return 0;
  }

private:
  std::map<int,rdr::U32> keysymMap;
} ckeymapper;


class ModifierKeyReleaser {
public:
  ModifierKeyReleaser(InputHandler* writer_, int vkCode, bool extended)
    : writer(writer_), extendedVkey(vkCode + (extended ? 256 : 0)),
      keysym(0)
  {}
  void release(std::map<int,rdr::U32>* downKeysym) {
    if (downKeysym->find(extendedVkey) != downKeysym->end()) {
      keysym = (*downKeysym)[extendedVkey];
      vlog.debug("fake release extendedVkey 0x%x, keysym 0x%x",
                 extendedVkey, keysym);
      writer->keyEvent(keysym, false);
    }
  }
  ~ModifierKeyReleaser() {
    if (keysym) {
      vlog.debug("fake press extendedVkey 0x%x, keysym 0x%x",
                 extendedVkey, keysym);
      writer->keyEvent(keysym, true);
    }
  }
  InputHandler* writer;
  int extendedVkey;
  rdr::U32 keysym;
};

// IS_PRINTABLE_LATIN1 tests if a character is either a printable latin1
// character, or 128, which is the Euro symbol on Windows.
#define IS_PRINTABLE_LATIN1(c) (((c) >= 32 && (c) <= 126) || (c) == 128 || \
                                ((c) >= 160 && (c) <= 255))

void win32::CKeyboard::keyEvent(InputHandler* writer, rdr::U8 vkey,
                                rdr::U32 flags, bool down)
{
  bool extended = (flags & 0x1000000);
  int extendedVkey = vkey + (extended ? 256 : 0);

  // If it's a release, just release whichever keysym corresponded to the same
  // key being pressed, regardless of how it would be interpreted in the
  // current keyboard state.
  if (!down) {
    releaseKey(writer, extendedVkey);
    return;
  }

  // We should always pass every down event to ToAscii() otherwise it can get
  // out of sync.

  // XXX should we pass CapsLock, ScrollLock or NumLock to ToAscii - they
  // actually alter the lock state on the keyboard?

  BYTE keystate[256];
  GetKeyboardState(keystate);
  rdr::U8 chars[2];

  int nchars = ToAscii(vkey, 0, keystate, (WORD*)&chars, 0);

  // See if it's in the Windows VK code -> X keysym map.  We do this before
  // looking at the result of ToAscii so that e.g. we recognise that it's
  // XK_KP_Add rather than '+'.

  rdr::U32 keysym = ckeymapper.lookup(extendedVkey);
  if (keysym) {
    vlog.debug("mapped key: extendedVkey 0x%x", extendedVkey);
    pressKey(writer, extendedVkey, keysym);
    return;
  }

  if (nchars < 0) {
    // Dead key - the next call to ToAscii() will give us either the accented
    // character or two characters.
    vlog.debug("ToAscii dead key (1): extendedVkey 0x%x", extendedVkey);
    return;
  }

  if (nchars > 0 && IS_PRINTABLE_LATIN1(chars[0])) {
    // Got a printable latin1 character.  We must release Control and Alt
    // (AltGr) if they were both pressed, so that the latin1 character is seen
    // without them by the VNC server.
    ModifierKeyReleaser lctrl(writer, VK_CONTROL, 0);
    ModifierKeyReleaser rctrl(writer, VK_CONTROL, 1);
    ModifierKeyReleaser lalt(writer, VK_MENU, 0);
    ModifierKeyReleaser ralt(writer, VK_MENU, 1);

    if ((keystate[VK_CONTROL] & 0x80) && (keystate[VK_MENU] & 0x80)) {
      lctrl.release(&downKeysym);
      rctrl.release(&downKeysym);
      lalt.release(&downKeysym);
      ralt.release(&downKeysym);
    }

    for (int i = 0; i < nchars; i++) {
      vlog.debug("ToAscii key (1): extendedVkey 0x%x", extendedVkey);
      if (chars[i] == 128) { // special hack for euro!
        pressKey(writer, extendedVkey, XK_EuroSign);
      } else {
        pressKey(writer, extendedVkey, chars[i]);
      }
    }
    return;
  }

  // Either no chars were generated, or something outside the printable
  // character range.  Try ToAscii() without the Control and Alt keys down to
  // see if that yields an ordinary character.

  keystate[VK_CONTROL] = keystate[VK_LCONTROL] = keystate[VK_RCONTROL] = 0;
  keystate[VK_MENU] = keystate[VK_LMENU] = keystate[VK_RMENU] = 0;

  nchars = ToAscii(vkey, 0, keystate, (WORD*)&chars, 0);

  if (nchars < 0) {
    // So it would be a dead key if neither control nor alt were pressed.
    // However, we know that at least one of control and alt must be pressed.
    // We can't leave it at this stage otherwise the next call to ToAscii()
    // with a valid character will get wrongly interpreted in the context of
    // this bogus dead key.  Working on the assumption that a dead key followed
    // by space usually returns the dead character itself, try calling ToAscii
    // with VK_SPACE.
    vlog.debug("ToAscii dead key (2): extendedVkey 0x%x", extendedVkey);
    nchars = ToAscii(VK_SPACE, 0, keystate, (WORD*)&chars, 0);
    if (nchars < 0) {
      vlog.debug("ToAscii dead key (3): extendedVkey 0x%x - giving up!",
                 extendedVkey);
      return;
    }
  }

  if (nchars > 0 && IS_PRINTABLE_LATIN1(chars[0])) {
    for (int i = 0; i < nchars; i++) {
      vlog.debug("ToAscii key (2) (no ctrl/alt): extendedVkey 0x%x",
                 extendedVkey);
      if (chars[i] == 128) { // special hack for euro!
        pressKey(writer, extendedVkey, XK_EuroSign);
      } else {
        pressKey(writer, extendedVkey, chars[i]);
      }
    }
    return;
  }

  vlog.debug("no chars regardless of control and alt: extendedVkey 0x%x",
             extendedVkey);
}

// releaseAllKeys() - write key release events to the server for all keys
// that are currently regarded as being down.
void win32::CKeyboard::releaseAllKeys(InputHandler* writer) {
  std::map<int,rdr::U32>::iterator i, next_i;
  for (i=downKeysym.begin(); i!=downKeysym.end(); i=next_i) {
    next_i = i; next_i++;
    writer->keyEvent((*i).second, false);
    downKeysym.erase(i);
  }
}

// releaseKey() - write a key up event to the server, but only if we've
// actually sent a key down event for the given key.  The key up event always
// contains the same keysym we used in the key down event, regardless of what
// it would look up as using the current keyboard state.
void win32::CKeyboard::releaseKey(InputHandler* writer, int extendedVkey)
{
  if (downKeysym.find(extendedVkey) != downKeysym.end()) {
    vlog.debug("release extendedVkey 0x%x, keysym 0x%x",
               extendedVkey, downKeysym[extendedVkey]);
    writer->keyEvent(downKeysym[extendedVkey], false);
    downKeysym.erase(extendedVkey);
  }
}

// pressKey() - write a key down event to the server, and record which keysym
// was sent as corresponding to the given extendedVkey.  The only tricky bit is
// that if we are trying to press an extendedVkey which is already marked as
// down but with a different keysym, then we need to release the old keysym
// first.  This can happen in two cases: (a) when a single key press results in
// more than one character, and (b) when shift is released while another key is
// autorepeating.
void win32::CKeyboard::pressKey(InputHandler* writer, int extendedVkey,
                                rdr::U32 keysym)
{
  if (downKeysym.find(extendedVkey) != downKeysym.end()) {
    if (downKeysym[extendedVkey] != keysym) {
      vlog.debug("release extendedVkey 0x%x, keysym 0x%x",
                 extendedVkey, downKeysym[extendedVkey]);
      writer->keyEvent(downKeysym[extendedVkey], false);
    }
  }
  vlog.debug("press   extendedVkey 0x%x, keysym 0x%x",
             extendedVkey, keysym);
  writer->keyEvent(keysym, true);
  downKeysym[extendedVkey] = keysym;
}
