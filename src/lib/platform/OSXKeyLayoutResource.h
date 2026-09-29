/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/IOSXKeyResource.h"

#include <Carbon/Carbon.h>

//! Key resource for a keyboard layout, backed by UCKeyTranslate
/*!
Builds the key map for a keyboard layout using UCKeyTranslate.  Each modifier
combination is treated as its own table.
*/
class OSXKeyLayoutResource : public IOSXKeyResource
{
public:
  OSXKeyLayoutResource(const void *resource, uint32_t keyboardType);

  // IOSXKeyResource overrides
  bool isValid() const override;
  uint32_t getNumModifierCombinations() const override;
  uint32_t getNumTables() const override;
  uint32_t getNumButtons() const override;
  uint32_t getTableForModifier(uint32_t mask) const override;
  KeyID getKey(uint32_t table, uint32_t button) const override;
  KeyID getDeadKeyOutput(uint32_t table, uint32_t button) const override;

private:
  KeyID resolveDeadKeyOutput(UInt32 deadKeyState) const;

  const UCKeyboardLayout *m_layout;
  uint32_t m_keyboardType;
};
