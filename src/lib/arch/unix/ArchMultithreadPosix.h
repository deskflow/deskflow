/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"

#include <list>
#include <mutex>
#include <pthread.h>

#define ARCH_MULTITHREAD ArchMultithreadPosix

class ArchCondImpl
{
public:
  pthread_cond_t m_cond;
};

class ArchMutexImpl
{
public:
  pthread_mutex_t m_mutex;
};

//! Posix implementation of IArchMultithread
class ArchMultithreadPosix : public IArchMultithread
{
public:
  ArchMultithreadPosix();
  ArchMultithreadPosix(ArchMultithreadPosix const &) = delete;
  ArchMultithreadPosix(ArchMultithreadPosix &&) = delete;
  ~ArchMultithreadPosix() override;

  ArchMultithreadPosix &operator=(ArchMultithreadPosix const &) = delete;
  ArchMultithreadPosix &operator=(ArchMultithreadPosix &&) = delete;

  //! @name manipulators
  //@{

  void setNetworkDataForCurrentThread(void *);

  //@}
  //! @name accessors
  //@{

  void *getNetworkDataForThread(ArchThread);

  static ArchMultithreadPosix *getInstance();

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
  ArchThread newThread(ThreadFunc, void *) final;
  ArchThread newCurrentThread() override;
  ArchThread copyThread(ArchThread) override;
  void closeThread(ArchThread) final;
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
  void startSignalHandler();

  ArchThreadImpl *find(pthread_t thread);
  ArchThreadImpl *findNoRef(pthread_t thread);
  void insert(ArchThreadImpl *thread);
  void erase(const ArchThreadImpl *thread);

  void refThread(ArchThreadImpl *rep);
  void testCancelThreadImpl(ArchThreadImpl *rep);

  void doThreadFunc(ArchThread thread);
  static void *threadFunc(void *vrep);
  static void threadCancel(int);
  static void *threadSignalHandler(void *vrep);

private:
  using ThreadList = std::list<ArchThread>;

  static ArchMultithreadPosix *s_instance;

  bool m_newThreadCalled = false;

  std::mutex m_threadMutex;
  ArchThread m_mainThread;
  ThreadList m_threadList;
  ThreadID m_nextID = 0;

  pthread_t m_signalThread;
  SignalFunc m_signalFunc[kNUM_SIGNALS];
  void *m_signalUserData[kNUM_SIGNALS];
};
