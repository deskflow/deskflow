/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "net/SocketMultiplexer.h"

#include "net/ISocketMultiplexerJob.h"
#include "mt/CondVar.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "common/stdvector.h"

//
// SocketMultiplexer
//

class CursorMultiplexerJob : public ISocketMultiplexerJob {
public:
    MultiplexerJobStatus run(bool readable, bool writable, bool error) override
    {
        return {false, {}};
    }

    ArchSocket getSocket() const override { return {}; }
    bool isReadable() const override { return false; }
    bool isWritable() const override { return false; }
    bool isCursor() const override { return true; }
};


SocketMultiplexer::SocketMultiplexer() :
    m_mutex(new Mutex),
    m_thread(NULL),
    m_update(false),
    m_jobsReady(new CondVar<bool>(m_mutex, false)),
    m_jobListLock(new CondVar<bool>(m_mutex, false)),
    m_jobListLockLocked(new CondVar<bool>(m_mutex, false)),
    m_jobListLocker(NULL),
    m_jobListLockLocker(NULL)
{
    // start thread
    m_thread = new Thread(new TMethodJob<SocketMultiplexer>(
                                this, &SocketMultiplexer::serviceThread));
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

void SocketMultiplexer::addSocket(ISocket* socket, std::unique_ptr<ISocketMultiplexerJob>&& job)
{
    assert(socket != NULL);
    assert(job    != NULL);

    // prevent other threads from locking the job list
    lockJobListLock();

    // break thread out of poll
    m_thread->unblockPollSocket();

    // lock the job list
    lockJobList();

    // insert/replace job
    SocketJobMap::iterator i = m_socketJobMap.find(socket);
    if (i == m_socketJobMap.end()) {
        // we *must* put the job at the end so the order of jobs in
        // the list continue to match the order of jobs in pfds in
        // serviceThread().
        JobCursor j = m_socketJobs.insert(m_socketJobs.end(), std::move(job));
        m_update     = true;
        m_socketJobMap.insert(std::make_pair(socket, j));
    }
    else {
        *(i->second) = std::move(job);
        m_update = true;
    }

    // unlock the job list
    unlockJobList();
}

void
SocketMultiplexer::removeSocket(ISocket* socket)
{
    assert(socket != NULL);

    // prevent other threads from locking the job list
    lockJobListLock();

    // break thread out of poll
    m_thread->unblockPollSocket();

    // lock the job list
    lockJobList();

    // remove job.  rather than removing it from the map we put NULL
    // in the list instead so the order of jobs in the list continues
    // to match the order of jobs in pfds in serviceThread().
    SocketJobMap::iterator i = m_socketJobMap.find(socket);
    if (i != m_socketJobMap.end()) {
        if (*(i->second)) {
            i->second->reset();
            m_update = true;
        }
    }

    // unlock the job list
    unlockJobList();
}

void
SocketMultiplexer::serviceThread(void*)
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

            JobCursor cursor    = newCursor();
            JobCursor jobCursor = nextCursor(cursor);
            while (jobCursor != m_socketJobs.end()) {
                if (*jobCursor) {
                    pfd.m_socket = (*jobCursor)->getSocket();
                    pfd.m_events = 0;
                    if ((*jobCursor)->isReadable()) {
                        pfd.m_events |= IArchNetwork::kPOLLIN;
                    }
                    if ((*jobCursor)->isWritable()) {
                        pfd.m_events |= IArchNetwork::kPOLLOUT;
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
            }
            else {
                status = 0;
            }
        }
        catch (XArchNetwork& e) {
            LOG((CLOG_WARN "error in socket multiplexer: %s", e.what()));
            status = 0;
        }

        if (status != 0) {
            // iterate over socket jobs, invoking each and saving the
            // new job.
            UInt32 i             = 0;
            JobCursor cursor    = newCursor();
            JobCursor jobCursor = nextCursor(cursor);
            while (i < pfds.size() && jobCursor != m_socketJobs.end()) {
                if (*jobCursor != NULL) {
                    // get poll state
                    unsigned short revents = pfds[i].m_revents;
                    bool read  = ((revents & IArchNetwork::kPOLLIN) != 0);
                    bool write = ((revents & IArchNetwork::kPOLLOUT) != 0);
                    bool error = ((revents & (IArchNetwork::kPOLLERR |
                                              IArchNetwork::kPOLLNVAL)) != 0);

                    // run job
                    MultiplexerJobStatus status = (*jobCursor)->run(read, write, error);

                    if (!status.continue_servicing) {
                        Lock lock(m_mutex);
                        jobCursor->reset();
                        m_update = true;
                    } else if (status.new_job) {
                        Lock lock(m_mutex);
                        *jobCursor = std::move(status.new_job);
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
        for (SocketJobMap::iterator i = m_socketJobMap.begin();
                            i != m_socketJobMap.end();) {
            if (*(i->second) == NULL) {
                m_socketJobs.erase(i->second);
                m_socketJobMap.erase(i++);
                m_update = true;
            }
            else {
                ++i;
            }
        }

        // unlock the job list
        unlockJobList();
    }
}

SocketMultiplexer::JobCursor
SocketMultiplexer::newCursor()
{
    Lock lock(m_mutex);
    return m_socketJobs.insert(m_socketJobs.begin(), std::make_unique<CursorMultiplexerJob>());
}

SocketMultiplexer::JobCursor
SocketMultiplexer::nextCursor(JobCursor cursor)
{
    Lock lock(m_mutex);
    JobCursor j = m_socketJobs.end();
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

void
SocketMultiplexer::deleteCursor(JobCursor cursor)
{
    Lock lock(m_mutex);
    m_socketJobs.erase(cursor);
}

void
SocketMultiplexer::lockJobListLock()
{
    Lock lock(m_mutex);

    // wait for the lock on the lock
    while (*m_jobListLockLocked) {
        m_jobListLockLocked->wait();
    }

    // take ownership of the lock on the lock
    *m_jobListLockLocked = true;
    m_jobListLockLocker  = new Thread(Thread::getCurrentThread());
}

void
SocketMultiplexer::lockJobList()
{
    Lock lock(m_mutex);

    // make sure we're the one that called lockJobListLock()
    assert(*m_jobListLockLocker == Thread::getCurrentThread());

    // wait for the job list lock
    while (*m_jobListLock) {
        m_jobListLock->wait();
    }

    // take ownership of the lock
    *m_jobListLock      = true;
    m_jobListLocker     = m_jobListLockLocker;
    m_jobListLockLocker = NULL;

    // release the lock on the lock
    *m_jobListLockLocked = false;
    m_jobListLockLocked->broadcast();
}

void
SocketMultiplexer::unlockJobList()
{
    Lock lock(m_mutex);

    // make sure we're the one that called lockJobList()
    assert(*m_jobListLocker == Thread::getCurrentThread());

    // release the lock
    delete m_jobListLocker;
    m_jobListLocker = NULL;
    *m_jobListLock  = false;
    m_jobListLock->signal();

    // set new jobs ready state
    bool isReady = !m_socketJobMap.empty();
    if (*m_jobsReady != isReady) {
        *m_jobsReady = isReady;
        m_jobsReady->signal();
    }
}
