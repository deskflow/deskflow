/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventQueue.h"

class EventQueueTimer;

class TestEventQueue : public EventQueue
{
public:
  TestEventQueue() : m_pQuitTimeoutTimer(nullptr)
  {
  }

  void handleQuitTimeout(const Event &, void *vclient);
  void raiseQuitEvent();
  void initQuitTimeout(double timeout);
  void cleanupQuitTimeout();

private:
  void timeoutThread(void *);

private:
  EventQueueTimer *m_pQuitTimeoutTimer;
};
