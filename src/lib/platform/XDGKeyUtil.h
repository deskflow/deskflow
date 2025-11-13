/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <X11/X.h>

#include <cstdint>
#include <map>

//! XKB utility functions
class XDGKeyUtil
{
public:
  //! Convert KeySym to KeyID
  /*!
  Converts a KeySym to the equivalent KeyID.  Returns kKeyNone if the
  KeySym cannot be mapped.
  */
  static std::uint32_t mapKeySymToKeyID(KeySym);

  //! Convert KeySym to corresponding KeyModifierMask
  /*!
  Converts a KeySym to the corresponding KeyModifierMask, or 0 if the
  KeySym is not a modifier.
  */
  static std::uint32_t getModifierBitForKeySym(KeySym keysym);

private:
  static void initKeyMaps();

  typedef std::map<KeySym, std::uint32_t> KeySymMap;

  static KeySymMap s_keySymToUCS4;
};
