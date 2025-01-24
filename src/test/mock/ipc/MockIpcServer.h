/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcServer.h"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Invoke;

class IEventQueue;

class MockIpcServer : public IpcServer
{
public:
  MockIpcServer() : m_sendCond(ARCH->newCondVar()), m_sendMutex(ARCH->newMutex())
  {
  }

  ~MockIpcServer()
  {
    if (m_sendCond != NULL) {
      ARCH->closeCondVar(m_sendCond);
    }

    if (m_sendMutex != NULL) {
      ARCH->closeMutex(m_sendMutex);
    }
  }

  MOCK_METHOD(void, listen, (), (override));
  MOCK_METHOD(void, send, (const IpcMessage &, IpcClientType), (override));
  MOCK_METHOD(bool, hasClients, (IpcClientType), (const, override));

  void delegateToFake()
  {
    ON_CALL(*this, send(_, _)).WillByDefault(Invoke(this, &MockIpcServer::mockSend));
  }

  void waitForSend()
  {
    ARCH->waitCondVar(m_sendCond, m_sendMutex, 5);
  }

private:
  void mockSend(const IpcMessage &, IpcClientType)
  {
    ArchMutexLock lock(m_sendMutex);
    ARCH->broadcastCondVar(m_sendCond);
  }

  ArchCond m_sendCond;
  ArchMutex m_sendMutex;
};
