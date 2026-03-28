/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include "deskflow/KeyMap.h"

// NOTE: do not mock methods that are not pure virtual. this mock exists only
// to provide an implementation of the KeyMap abstract class.
class MockKeyMap : public deskflow::KeyMap
{
public:
  void swap(KeyMap &) noexcept override
  {
  }

  void finish() override
  {
  }

  void foreachKey(ForeachKeyCallback, void *) override
  {
  }

  void addHalfDuplexModifier(KeyID) override
  {
  }

  bool isHalfDuplex(KeyID, KeyButton) const override
  {
    return false;
  }
};

using KeyID = uint32_t;
