/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define TEST_ENV

#include "server/Server.h"

#include <gmock/gmock.h>

class IEventQueue;

class MockServer : public Server
{
public:
  MockServer() : Server()
  {
  }
};
