/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"

#include <memory>
#include <mutex>
#include <pthread.h>
#include <signal.h>

#define ARCH_MULTITHREAD ArchMultithreadPosix

struct ArchThreadState;

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
  using ThreadCreate = int (*)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

  explicit ArchMultithreadPosix(ThreadCreate createThread = &pthread_create);
  ArchMultithreadPosix(ArchMultithreadPosix const &) = delete;
  ArchMultithreadPosix(ArchMultithreadPosix &&) = delete;
  ~ArchMultithreadPosix() override;

  ArchMultithreadPosix &operator=(ArchMultithreadPosix const &) = delete;
  ArchMultithreadPosix &operator=(ArchMultithreadPosix &&) = delete;

  //! @name manipulators
  //@{

  void shutdown();
  std::shared_ptr<void> installNetworkDataForThread(ArchThread, std::shared_ptr<void>);

  //@}
  //! @name accessors
  //@{

  std::shared_ptr<void> getNetworkDataForThread(ArchThread);

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
  void setSignalHandler(ThreadSignal, SignalFunc, void *) override;
  void raiseSignal(ThreadSignal) override;

private:
  bool startSignalHandler();

  ArchThreadImpl *find(pthread_t thread);
  ArchThreadImpl *findNoRef(pthread_t thread);
  ArchThreadImpl *findNoRefOrInsert(pthread_t thread);
  void insert(ArchThreadImpl *thread);
  void refThread(ArchThreadImpl *rep);
  static void testCancelThreadImpl(ArchThreadImpl *rep);

  static void doThreadFunc(ArchThread thread);
  static void *threadFunc(void *vrep);
  static void threadCancel(int);
  static void *threadSignalHandler(void *vrep);

  static ArchMultithreadPosix *s_instance;

  std::shared_ptr<ArchThreadState> m_state;
  std::mutex m_startupMutex;
  ThreadCreate m_createThread;
  ArchThread m_mainThread = nullptr;
  bool m_shutdown = false;
  bool m_signalStarted = false;
  bool m_signalStop = false;
  pthread_t m_signalThread{};
  pthread_t m_signalMaskOwner{};
  sigset_t m_previousSignalMask{};
  SignalFunc m_signalFunc[static_cast<int>(ThreadSignal::MaxSignals)]{};
  void *m_signalUserData[static_cast<int>(ThreadSignal::MaxSignals)]{};
};
