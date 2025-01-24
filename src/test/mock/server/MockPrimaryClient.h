/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define TEST_ENV

#include "server/PrimaryClient.h"

#include <gmock/gmock.h>

class MockPrimaryClient : public PrimaryClient
{
public:
  MOCK_METHOD(void *, getEventTarget, (), (const, override));
  MOCK_METHOD(void, getCursorPos, (int32_t &, int32_t &), (const, override));
  MOCK_METHOD(void, setJumpCursorPos, (int32_t, int32_t), (const));
  MOCK_METHOD(void, reconfigure, (uint32_t), (override));
  MOCK_METHOD(void, resetOptions, (), (override));
  MOCK_METHOD(void, setOptions, (const OptionsList &), (override));
  MOCK_METHOD(void, enable, (), (override));
  MOCK_METHOD(void, disable, (), (override));
  MOCK_METHOD(uint32_t, registerHotKey, (KeyID, KeyModifierMask), (override));
  MOCK_METHOD(KeyModifierMask, getToggleMask, (), (const, override));
  MOCK_METHOD(void, unregisterHotKey, (uint32_t), (override));
};
