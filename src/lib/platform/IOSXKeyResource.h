/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyState.h"

class IOSXKeyResource : public IInterface
{
public:
  virtual bool isValid() const = 0;
  virtual uint32_t getNumModifierCombinations() const = 0;
  virtual uint32_t getNumTables() const = 0;
  virtual uint32_t getNumButtons() const = 0;
  virtual uint32_t getTableForModifier(uint32_t mask) const = 0;
  virtual KeyID getKey(uint32_t table, uint32_t button) const = 0;

  // Convert a character in the current script to the equivalent KeyID
  static KeyID getKeyID(uint8_t);

  // Convert a unicode character to the equivalent KeyID.
  static KeyID unicharToKeyID(UniChar);
};
