/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/SocketMultiplexer.h"

#include "arch/Arch.h"
#include "arch/ArchException.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "mt/CondVar.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"
#include "net/ISocketMultiplexerJob.h"

#include <vector>

class CursorMultiplexerJob : public ISocketMultiplexerJob
{
public:
  MultiplexerJobStatus run(bool readable, bool writable, bool error) override
  {
    return {false, {}};
  }

  ArchSocket getSocket() const override
  {
    return {};
  }

  bool isReadable() const override
  {
    return false;
  }

  bool isWritable() const override
  {
    return false;
  }

  bool isCursor() const override
  {
    return true;
  }
};

//
// SocketMultiplexer
//

SocketMultiplexer::SocketMultiplexer()
    : m_mutex(new Mutex),
      m_jobsReady(new CondVar<bool>(m_mutex, false)),
      m_jobListLock(new CondVar<bool>(m_mutex, false)),
      m_jobListLockLocked(new CondVar<bool>(m_mutex, false))
{
  // start thread
  auto tMethodJob = new TMethodJob<SocketMultiplexer>(this, &SocketMultiplexer::serviceThread);
  m_thread = new Thread(tMethodJob);
}

SocketMultiplexer::~SocketMultiplexer()
{
  m_thread->cancel();
  m_thread->unblockPollSocket();
  m_thread->wait();
  delete m_thread;
  delete m_jobsReady;
  delete m_jobListLock;
  delete m_jobListLockLocked;
  delete m_jobListLocker;
  delete m_jobListLockLocker;
  delete m_mutex;
}

void SocketMultiplexer::addSocket(ISocket *socket, std::unique_ptr<ISocketMultiplexerJob> &&job)
{
  assert(socket != nullptr);
  assert(job != nullptr);

  // prevent other threads from locking the job list
  lockJobListLock();

  // break thread out of poll
  m_thread->unblockPollSocket();

  // lock the job list
  lockJobList();

  // insert/replace job
  if (SocketJobMap::iterator i = m_socketJobMap.find(socket); i == m_socketJobMap.end()) {
    // we *must* put the job at the end so the order of jobs in
    // the list continue to match the order of jobs in pfds in
    // serviceThread().
    JobCursor j = m_socketJobs.insert(m_socketJobs.end(), std::move(job));
    m_update = true;
    m_socketJobMap.insert(std::make_pair(socket, j));
  } else {
    *(i->second) = std::move(job);
    m_update = true;
  }

  // unlock the job list
  unlockJobList();
}

void SocketMultiplexer::removeSocket(ISocket *socket)
{
  assert(socket != nullptr);

  // prevent other threads from locking the job list
  lockJobListLock();

  // break thread out of poll
  m_thread->unblockPollSocket();

  // lock the job list
  lockJobList();

  // remove job.  rather than removing it from the map we put nullptr
  // in the list instead so the order of jobs in the list continues
  // to match the order of jobs in pfds in serviceThread().
  if (SocketJobMap::iterator i = m_socketJobMap.find(socket); i != m_socketJobMap.end()) {
    if (*(i->second)) {
      i->second->reset();
      m_update = true;
    }
  }

  // unlock the job list
  unlockJobList();
}

[[noreturn]] void SocketMultiplexer::serviceThread(void *)
{
  std::vector<IArchNetwork::PollEntry> pfds;
  IArchNetwork::PollEntry pfd;

  // service the connections
  for (;;) {
    Thread::testCancel();

    // wait until there are jobs to handle
    {
      Lock lock(m_mutex);
      while (!(bool)*m_jobsReady) {
        m_jobsReady->wait();
      }
    }

    // lock the job list
    lockJobListLock();
    lockJobList();

    // collect poll entries
    if (m_update) {
      m_update = false;
      pfds.clear();
      pfds.reserve(m_socketJobMap.size());

      JobCursor cursor = newCursor();
      JobCursor jobCursor = nextCursor(cursor);
      while (jobCursor != m_socketJobs.end()) {
        if (*jobCursor) {
          pfd.m_socket = (*jobCursor)->getSocket();
          pfd.m_events = 0;
          if ((*jobCursor)->isReadable()) {
            pfd.m_events |= IArchNetwork::PollEventMask::In;
          }
          if ((*jobCursor)->isWritable()) {
            pfd.m_events |= IArchNetwork::PollEventMask::Out;
          }
          pfds.push_back(pfd);
        }
        jobCursor = nextCursor(cursor);
      }
      deleteCursor(cursor);
    }

    int status;
    try {
      // check for status
      if (!pfds.empty()) {
        status = ARCH->pollSocket(&pfds[0], (int)pfds.size(), -1);
      } else {
        status = 0;
      }
    } catch (ArchNetworkException &e) {
      LOG_WARN("error in socket multiplexer: %s", e.what());
      status = 0;
    }

    if (status != 0) {
      // iterate over socket jobs, invoking each and saving the
      // new job.
      uint32_t i = 0;
      JobCursor cursor = newCursor();
      JobCursor jobCursor = nextCursor(cursor);
      while (i < pfds.size() && jobCursor != m_socketJobs.end()) {
        if (*jobCursor != nullptr) {
          // get poll state
          unsigned short revents = pfds[i].m_revents;
          bool read = ((revents & int(IArchNetwork::PollEventMask::In)) != 0);
          bool write = ((revents & int(IArchNetwork::PollEventMask::Out)) != 0);
          bool error =
              ((revents & (int(IArchNetwork::PollEventMask::Error) | int(IArchNetwork::PollEventMask::Invalid))) != 0);

          // run job
          MultiplexerJobStatus status = (*jobCursor)->run(read, write, error);

          // save job, if different
          if (!status.continueServicing) {
            Lock lock(m_mutex);
            jobCursor->reset();
            m_update = true;
          } else if (status.newJob) {
            *jobCursor = std::move(status.newJob);
            m_update = true;
          }
          ++i;
        }

        // next job
        jobCursor = nextCursor(cursor);
      }
      deleteCursor(cursor);
    }

    // delete any removed socket jobs
    for (auto i = m_socketJobMap.begin(); i != m_socketJobMap.end();) {
      if (*(i->second) == nullptr) {
        m_socketJobs.erase(i->second);
        m_socketJobMap.erase(i++);
        m_update = true;
      } else {
        ++i;
      }
    }

    // unlock the job list
    unlockJobList();
  }
}

SocketMultiplexer::JobCursor SocketMultiplexer::newCursor()
{
  Lock lock(m_mutex);
  return m_socketJobs.insert(m_socketJobs.begin(), std::make_unique<CursorMultiplexerJob>());
}

SocketMultiplexer::JobCursor SocketMultiplexer::nextCursor(JobCursor cursor)
{
  Lock lock(m_mutex);
  auto j = m_socketJobs.end();
  JobCursor i = cursor;
  while (++i != m_socketJobs.end()) {
    if (*i && !(*i)->isCursor()) {
      // found a real job (as opposed to a cursor)
      j = i;

      // move our cursor just past the job
      m_socketJobs.splice(++i, m_socketJobs, cursor);
      break;
    }
  }
  return j;
}

void SocketMultiplexer::deleteCursor(JobCursor cursor)
{
  Lock lock(m_mutex);
  m_socketJobs.erase(cursor);
}

void SocketMultiplexer::lockJobListLock()
{
  Lock lock(m_mutex);

  // wait for the lock on the lock
  while (*m_jobListLockLocked) {
    m_jobListLockLocked->wait();
  }

  // take ownership of the lock on the lock
  *m_jobListLockLocked = true;
  m_jobListLockLocker = new Thread(Thread::getCurrentThread());
}

void SocketMultiplexer::lockJobList()
{
  Lock lock(m_mutex);

  // make sure we're the one that called lockJobListLock()
  assert(*m_jobListLockLocker == Thread::getCurrentThread());

  // wait for the job list lock
  while (*m_jobListLock) {
    m_jobListLock->wait();
  }

  // take ownership of the lock
  *m_jobListLock = true;
  m_jobListLocker = m_jobListLockLocker;
  m_jobListLockLocker = nullptr;

  // release the lock on the lock
  *m_jobListLockLocked = false;
  m_jobListLockLocked->broadcast();
}

void SocketMultiplexer::unlockJobList()
{
  Lock lock(m_mutex);

  // make sure we're the one that called lockJobList()
  assert(*m_jobListLocker == Thread::getCurrentThread());

  // release the lock
  delete m_jobListLocker;
  m_jobListLocker = nullptr;
  *m_jobListLock = false;
  m_jobListLock->signal();

  // set new jobs ready state
  bool isReady = !m_socketJobMap.empty();
  if (*m_jobsReady != isReady) {
    *m_jobsReady = isReady;
    m_jobsReady->signal();
  }
}
