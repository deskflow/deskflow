/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"

#include <list>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define ARCH_MULTITHREAD ArchMultithreadWindows

class ArchCondImpl
{
public:
  enum
  {
    kSignal = 0,
    kBroadcast
  };

  HANDLE m_events[2];
  mutable int m_waitCount;
  ArchMutex m_waitCountMutex;
};

class ArchMutexImpl
{
public:
  CRITICAL_SECTION m_mutex;
};

//! Win32 implementation of IArchMultithread
class ArchMultithreadWindows : public IArchMultithread
{
public:
  ArchMultithreadWindows();
  ~ArchMultithreadWindows() override;

  //! @name manipulators
  //@{

  void setNetworkDataForCurrentThread(void *);

  //@}
  //! @name accessors
  //@{

  HANDLE getCancelEventForCurrentThread();

  void *getNetworkDataForThread(ArchThread);

  static ArchMultithreadWindows *getInstance();

  //@}

  // IArchMultithread overrides
  ArchCond newCondVar() override;
  void closeCondVar(ArchCond) override;
  void signalCondVar(ArchCond) override;
  void broadcastCondVar(ArchCond) override;
  bool waitCondVar(ArchCond, ArchMutex, double timeout) override;
  ArchMutex newMutex() override;
  void closeMutex(ArchMutex) override;
  void lockMutex(ArchMutex) override;
  void unlockMutex(ArchMutex) override;
  ArchThread newThread(ThreadFunc, void *) override;
  ArchThread newCurrentThread() override;
  ArchThread copyThread(ArchThread) override;
  void closeThread(ArchThread) override;
  void cancelThread(ArchThread) override;
  void setPriorityOfThread(ArchThread, int n) override;
  void testCancelThread() override;
  bool wait(ArchThread, double timeout) override;
  bool isSameThread(ArchThread, ArchThread) override;
  bool isExitedThread(ArchThread) override;
  void *getResultOfThread(ArchThread) override;
  ThreadID getIDOfThread(ArchThread) override;
  void setSignalHandler(ESignal, SignalFunc, void *) override;
  void raiseSignal(ESignal) override;

private:
  ArchThreadImpl *find(DWORD id);
  ArchThreadImpl *findNoRef(DWORD id);
  ArchThreadImpl *findNoRefOrCreate(DWORD id);
  void insert(ArchThreadImpl *thread);
  void erase(ArchThreadImpl *thread);

  void refThread(ArchThreadImpl *rep);
  void testCancelThreadImpl(ArchThreadImpl *rep);

  void doThreadFunc(ArchThread thread);
  static unsigned int __stdcall threadFunc(void *vrep);

private:
  using ThreadList = std::list<ArchThread>;

  static ArchMultithreadWindows *s_instance;

  ArchMutex m_threadMutex;

  ThreadList m_threadList;
  ArchThread m_mainThread;

  SignalFunc m_signalFunc[kNUM_SIGNALS];
  void *m_signalUserData[kNUM_SIGNALS];
};
