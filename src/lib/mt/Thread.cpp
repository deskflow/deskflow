/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "mt/Thread.h"

#include "arch/Arch.h"
#include "base/IJob.h"
#include "base/Log.h"
#include "mt/MTException.h"
#include "mt/ThreadException.h"
#include <exception>

//
// Thread
//

Thread::Thread(IJob *job)
{
  m_thread = ARCH->newThread(&Thread::threadFunc, job);
  if (m_thread == nullptr) {
    // couldn't create thread
    delete job;
    throw MTThreadUnavailableException();
  }
}

Thread::Thread(const Thread &thread) : m_thread{ARCH->copyThread(thread.m_thread)}
{
  // do nothing
}

Thread::Thread(ArchThread adoptedThread) : m_thread{adoptedThread}
{
  // do nothing
}

Thread::~Thread()
{
  ARCH->closeThread(m_thread);
}

Thread &Thread::operator=(const Thread &thread)
{
  // copy given thread and release ours
  ArchThread copy = ARCH->copyThread(thread.m_thread);
  ARCH->closeThread(m_thread);

  // cut over
  m_thread = copy;

  return *this;
}

[[noreturn]] void Thread::exit(void *result)
{
  throw ThreadExitException(result);
}

void Thread::cancel()
{
  ARCH->cancelThread(m_thread);
}

void Thread::setPriority(int n)
{
  ARCH->setPriorityOfThread(m_thread, n);
}

void Thread::unblockPollSocket()
{
  ARCH->unblockPollSocket(m_thread);
}

Thread Thread::getCurrentThread()
{
  return Thread(ARCH->newCurrentThread());
}

void Thread::testCancel()
{
  ARCH->testCancelThread();
}

bool Thread::wait(double timeout) const
{
  return ARCH->wait(m_thread, timeout);
}

void *Thread::getResult() const
{
  if (wait())
    return ARCH->getResultOfThread(m_thread);
  else
    return nullptr;
}

IArchMultithread::ThreadID Thread::getID() const
{
  return ARCH->getIDOfThread(m_thread);
}

bool Thread::operator==(const Thread &thread) const
{
  return ARCH->isSameThread(m_thread, thread.m_thread);
}

void *Thread::threadFunc(void *vjob)
{
  // get this thread's id for logging
  IArchMultithread::ThreadID id;
  {
    ArchThread thread = ARCH->newCurrentThread();
    id = ARCH->getIDOfThread(thread);
    ARCH->closeThread(thread);
  }

  // get job
  auto *job = static_cast<IJob *>(vjob);

  // run job
  void *result = nullptr;
  try {
    // go
    LOG_DEBUG1("thread 0x%08x entry", id);
    job->run();
    LOG_DEBUG1("thread 0x%08x exit", id);
  } catch (ThreadCancelException &) {
    // client called cancel()
    LOG_DEBUG1("caught cancel on thread 0x%08x", id);
    delete job;
    throw;
  } catch (ThreadExitException &e) {
    // client called exit()
    result = e.m_result;
    LOG_DEBUG1("caught exit on thread 0x%08x, result %p", id, result);
  } catch (BaseException &e) {
    LOG_ERR("exception on thread 0x%08x: %s", id, e.what());
    delete job;
    throw;
  } catch (std::exception &e) {
    LOG_ERR("standard exception on thread 0x%08x: %s", id, e.what());
    delete job;
    throw;
  } catch (...) {
    LOG_ERR("non-exception throw on thread 0x%08x: <unknown>", id);
    delete job;
    throw;
  }

  // done with job
  delete job;

  // return exit result
  return result;
}
