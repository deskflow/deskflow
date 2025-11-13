/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <array>
#include <cstdint>
#include <map>

using KeySym = unsigned long;
using KeySymMap = std::map<KeySym, uint32_t>;

//! XKB utility functions
class XDGKeyUtil
{

public:
  //! Convert KeySym to KeyID
  /*!
  Converts a KeySym to the equivalent KeyID.  Returns kKeyNone if the
  KeySym cannot be mapped.
  */
  static uint32_t mapKeySymToKeyID(KeySym);

  //! Convert KeySym to corresponding KeyModifierMask
  /*!
  Converts a KeySym to the corresponding KeyModifierMask, or 0 if the
  KeySym is not a modifier.
  */
  static uint32_t getModifierBitForKeySym(KeySym keysym);

private:
  static const KeySymMap s_keySymToUCS4;
  // map "Internet" keys to KeyIDs
  static const std::array<KeySym, 256> s_map1008FF;
};
