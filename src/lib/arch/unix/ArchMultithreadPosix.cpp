/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchMultithreadPosix.h"

#include "arch/Arch.h"
#include "arch/ArchException.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <list>
#include <signal.h>
#include <sys/time.h>
#include <system_error>
#include <thread>
#include <time.h>

#define SIGWAKEUP SIGUSR1

static void setSignalSet(sigset_t *sigset)
{
  sigemptyset(sigset);
  sigaddset(sigset, SIGHUP);
  sigaddset(sigset, SIGINT);
  sigaddset(sigset, SIGTERM);
  sigaddset(sigset, SIGUSR2);
}

//
// ArchThreadImpl
//

class ArchThreadImpl
{
public:
  explicit ArchThreadImpl(std::shared_ptr<ArchThreadState> state) : m_state(std::move(state))
  {
  }

public:
  int m_refCount = 1;
  IArchMultithread::ThreadID m_id = 0;
  pthread_t m_thread;
  IArchMultithread::ThreadFunc m_func = nullptr;
  void *m_userData = nullptr;
  bool m_cancel = false;
  bool m_cancelling = false;
  bool m_exited = false;
  bool m_active = false;
  void *m_result = nullptr;
  std::shared_ptr<void> m_networkData;
  std::shared_ptr<ArchThreadState> m_state;
};

namespace {
void currentThreadExited(void *);
}

struct ArchThreadState
{
  ArchThreadState()
  {
    const int status = pthread_key_create(&currentThread, &currentThreadExited);
    if (status != 0) {
      throw std::system_error(status, std::generic_category(), "pthread_key_create");
    }
  }

  ~ArchThreadState()
  {
    pthread_key_delete(currentThread);
  }

  std::mutex mutex;
  std::list<ArchThread> threads;
  IArchMultithread::ThreadID nextID = 0;
  pthread_key_t currentThread{};
  bool stopping = false;
};

namespace {
struct ThreadRegistration
{
  ArchThread thread;
};

// The caller holds the registry mutex. The TLS reference keeps both the
// representation and its registry alive independently of the Arch object.
void registerCurrentThread(ArchThread thread)
{
  auto registration = std::make_unique<ThreadRegistration>(thread);
  const int status = pthread_setspecific(thread->m_state->currentThread, registration.get());
  if (status != 0) {
    throw std::system_error(status, std::generic_category(), "pthread_setspecific");
  }
  ++thread->m_refCount;
  registration.release();
}

bool releaseThreadLocked(ArchThread thread)
{
  assert(thread->m_refCount > 0);
  if (--thread->m_refCount != 0) {
    return false;
  }
  assert(!thread->m_active);
  return true;
}

void releaseThread(ArchThread thread)
{
  const auto state = thread->m_state;
  bool destroy = false;
  {
    std::scoped_lock lock{state->mutex};
    destroy = releaseThreadLocked(thread);
  }
  if (destroy) {
    delete thread;
  }
}

void currentThreadExited(void *data)
{
  const std::unique_ptr<ThreadRegistration> registration(static_cast<ThreadRegistration *>(data));
  ArchThread thread = registration->thread;
  const auto state = thread->m_state;
  std::shared_ptr<void> networkData;
  bool destroy = false;
  {
    std::scoped_lock lock{state->mutex};
    state->threads.remove(thread);
    thread->m_active = false;
    thread->m_exited = true;
    networkData = std::move(thread->m_networkData);
    destroy = releaseThreadLocked(thread);
  }
  if (destroy) {
    delete thread;
  }
}
} // namespace

//
// ArchMultithreadPosix
//

ArchMultithreadPosix *ArchMultithreadPosix::s_instance = nullptr;

ArchMultithreadPosix::ArchMultithreadPosix(ThreadCreate createThread)
    : m_state(std::make_shared<ArchThreadState>()),
      m_createThread(createThread)
{
  assert(s_instance == nullptr);
  assert(m_createThread != nullptr);

  // create thread for calling (main) thread and add it to our
  // list.  no need to lock the mutex since we're the only thread.
  m_mainThread = new ArchThreadImpl(m_state);
  m_mainThread->m_thread = pthread_self();
  try {
    insert(m_mainThread);
    registerCurrentThread(m_mainThread);
  } catch (...) {
    m_state->threads.remove(m_mainThread);
    delete m_mainThread;
    throw;
  }

  // install SIGWAKEUP handler.  this causes SIGWAKEUP to interrupt
  // system calls.  we use that when cancelling a thread to force it
  // to wake up immediately if it's blocked in a system call.  we
  // won't need this until another thread is created but it's fine
  // to install it now.
  struct sigaction act;
  sigemptyset(&act.sa_mask);
#if defined(SA_INTERRUPT)
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  act.sa_handler = &threadCancel;
  sigaction(SIGWAKEUP, &act, nullptr);

  // set desired signal dispositions.  let SIGWAKEUP through but
  // ignore SIGPIPE (we'll handle EPIPE).
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGWAKEUP);
  pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

  s_instance = this;
}

ArchMultithreadPosix::~ArchMultithreadPosix()
{
  shutdown();
  assert(s_instance == this);
  s_instance = nullptr;
}

void ArchMultithreadPosix::shutdown()
{
  std::scoped_lock startupLock{m_startupMutex};
  if (m_shutdown) {
    return;
  }
  m_shutdown = true;
  {
    std::scoped_lock lock{m_state->mutex};
    m_state->stopping = true;
    m_signalStop = true;
  }

  if (m_signalStarted) {
    // SIGUSR2 is already in the signal worker's wait set. The stop flag
    // prevents this private wakeup from being dispatched as a user signal.
    pthread_kill(m_signalThread, SIGUSR2);
    const int status = pthread_join(m_signalThread, nullptr);
    (void)status;
    assert(status == 0);
    m_signalStarted = false;
    if (pthread_equal(m_signalMaskOwner, pthread_self())) {
      pthread_sigmask(SIG_SETMASK, &m_previousSignalMask, nullptr);
    }
  }

  auto *registration = static_cast<ThreadRegistration *>(pthread_getspecific(m_state->currentThread));
  if (registration != nullptr) {
    pthread_setspecific(m_state->currentThread, nullptr);
    currentThreadExited(registration);
  }
  releaseThread(m_mainThread);
  m_mainThread = nullptr;
}

std::shared_ptr<void> ArchMultithreadPosix::installNetworkDataForThread(ArchThread thread, std::shared_ptr<void> data)
{
  std::scoped_lock lock{m_state->mutex};
  // TLS destructors can first poll after the thread callback has returned.
  if (m_state->stopping || !thread->m_active) {
    return {};
  }
  if (!thread->m_networkData) {
    thread->m_networkData = std::move(data);
  }
  return thread->m_networkData;
}

std::shared_ptr<void> ArchMultithreadPosix::getNetworkDataForThread(ArchThread thread)
{
  std::scoped_lock lock{m_state->mutex};
  if (m_state->stopping) {
    return {};
  }
  return thread->m_networkData;
}

ArchMultithreadPosix *ArchMultithreadPosix::getInstance()
{
  return s_instance;
}

ArchCond ArchMultithreadPosix::newCondVar()
{
  auto *cond = new ArchCondImpl;
  int status = pthread_cond_init(&cond->m_cond, nullptr);
  (void)status;
  assert(status == 0);
  return cond;
}

void ArchMultithreadPosix::closeCondVar(ArchCond cond)
{
  int status = pthread_cond_destroy(&cond->m_cond);
  (void)status;
  assert(status == 0);
  delete cond;
}

void ArchMultithreadPosix::signalCondVar(ArchCond cond)
{
  int status = pthread_cond_signal(&cond->m_cond);
  (void)status;
  assert(status == 0);
}

void ArchMultithreadPosix::broadcastCondVar(ArchCond cond)
{
  int status = pthread_cond_broadcast(&cond->m_cond);
  (void)status;
  assert(status == 0);
}

bool ArchMultithreadPosix::waitCondVar(ArchCond cond, ArchMutex mutex, double timeout)
{
  // we can't wait on a condition variable and also wake it up for
  // cancellation since we don't use posix cancellation.  so we
  // must wake up periodically to check for cancellation.  we
  // can't simply go back to waiting after the check since the
  // condition may have changed and we'll have lost the signal.
  // so we have to return to the caller.  since the caller will
  // always check for spurious wakeups the only drawback here is
  // performance:  we're waking up a lot more than desired.
  if (static const double maxCancellationLatency = 0.1; timeout < 0.0 || timeout > maxCancellationLatency) {
    timeout = maxCancellationLatency;
  }

  // see if we should cancel this thread
  testCancelThread();

  // get final time
  struct timeval now;
  gettimeofday(&now, nullptr);
  struct timespec finalTime;
  finalTime.tv_sec = now.tv_sec;
  finalTime.tv_nsec = now.tv_usec * 1000;
  auto timeout_sec = (long)timeout;
  auto timeout_nsec = (long)(1.0e+9 * (timeout - timeout_sec));
  finalTime.tv_sec += timeout_sec;
  finalTime.tv_nsec += timeout_nsec;
  if (finalTime.tv_nsec >= 1000000000) {
    finalTime.tv_nsec -= 1000000000;
    finalTime.tv_sec += 1;
  }

  // wait
  int status = pthread_cond_timedwait(&cond->m_cond, &mutex->m_mutex, &finalTime);

  // check for cancel again
  testCancelThread();

  switch (status) {
  case 0:
    // success
    return true;

  case ETIMEDOUT:
    return false;

  default:
    assert(0 && "condition variable wait error");
    return false;
  }
}

ArchMutex ArchMultithreadPosix::newMutex()
{
  pthread_mutexattr_t attr;
  int status = pthread_mutexattr_init(&attr);
  assert(status == 0);
  auto *mutex = new ArchMutexImpl;
  status = pthread_mutex_init(&mutex->m_mutex, &attr);
  assert(status == 0);
  return mutex;
}

void ArchMultithreadPosix::closeMutex(ArchMutex mutex)
{
  int status = pthread_mutex_destroy(&mutex->m_mutex);
  (void)status;
  assert(status == 0);
  delete mutex;
}

void ArchMultithreadPosix::lockMutex(ArchMutex mutex)
{
  int status = pthread_mutex_lock(&mutex->m_mutex);

  switch (status) {
  case 0:
    // success
    return;

  case EDEADLK:
    assert(0 && "lock already owned");
    break;

  case EAGAIN:
    assert(0 && "too many recursive locks");
    break;

  default:
    assert(0 && "unexpected error");
    break;
  }
}

void ArchMultithreadPosix::unlockMutex(ArchMutex mutex)
{
  // TODO: S1-1767, we should use raii c++17 mutex instead of archaeic pthread
  // to solve possible lock order reversal.
  int status = pthread_mutex_unlock(&mutex->m_mutex);

  switch (status) {
  case 0:
    // success
    return;

  case EPERM:
    assert(0 && "thread doesn't own a lock");
    break;

  default:
    assert(0 && "unexpected error");
    break;
  }
}

ArchThread ArchMultithreadPosix::newThread(ThreadFunc func, void *data)
{
  assert(func != nullptr);

  std::scoped_lock startupLock{m_startupMutex};
  if (m_shutdown) {
    return nullptr;
  }

  // initialize signal handler.  we do this here instead of the
  // constructor so we can avoid daemonizing (using fork())
  // when there are multiple threads.  clients can safely
  // use condition variables and mutexes before creating a
  // new thread and they can safely use the only thread
  // they have access to, the main thread, so they really
  // can't tell the difference.
  if (!m_signalStarted && !startSignalHandler()) {
    return nullptr;
  }

  // note that the child thread will wait until we release this mutex
  std::scoped_lock lock{m_state->mutex};

  // create thread impl for new thread
  auto thread = std::make_unique<ArchThreadImpl>(m_state);
  thread->m_func = func;
  thread->m_userData = data;
  // Reserve the list node before initializing native resources. The child
  // cannot inspect its representation until this mutex is released.
  m_state->threads.push_back(thread.get());

  // create the thread.  pthread_create() on RedHat 7.2 smp fails
  // if passed a nullptr attr so use a default attr.
  pthread_attr_t attr;
  int status = pthread_attr_init(&attr);
  if (status == 0) {
    status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (status == 0) {
      status = m_createThread(&thread->m_thread, &attr, &ArchMultithreadPosix::threadFunc, thread.get());
    }
    pthread_attr_destroy(&attr);
  }

  // check if thread was started
  if (status != 0) {
    // failed to start thread so clean up
    m_state->threads.pop_back();
    return nullptr;
  }

  thread->m_active = true;
  thread->m_id = ++m_state->nextID;
  // The execution reference is separate from caller handles and TLS ownership.
  refThread(thread.get());
  return thread.release();
}

ArchThread ArchMultithreadPosix::newCurrentThread()
{
  std::scoped_lock lock{m_state->mutex};
  ArchThreadImpl *thread = find(pthread_self());
  assert(thread != nullptr);
  return thread;
}

void ArchMultithreadPosix::closeThread(ArchThread thread)
{
  assert(thread != nullptr);

  releaseThread(thread);
}

ArchThread ArchMultithreadPosix::copyThread(ArchThread thread)
{
  std::scoped_lock lock{m_state->mutex};
  refThread(thread);
  return thread;
}

void ArchMultithreadPosix::cancelThread(ArchThread thread)
{
  assert(thread != nullptr);

  // set cancel and wakeup flags if thread can be cancelled
  std::scoped_lock lock{m_state->mutex};
  if (!thread->m_exited && !thread->m_cancelling && thread->m_active) {
    thread->m_cancel = true;
    // Serialize with TLS deregistration so a recycled pthread_t is never used.
    pthread_kill(thread->m_thread, SIGWAKEUP);
  }
}

void ArchMultithreadPosix::setPriorityOfThread(ArchThread thread, int /*n*/)
{
  assert(thread != nullptr);

  // FIXME
}

void ArchMultithreadPosix::testCancelThread()
{
  // find current thread
  ArchThreadImpl *thread = nullptr;
  {
    std::scoped_lock lock{m_state->mutex};
    thread = findNoRefOrInsert(pthread_self());
  }
  // test cancel on thread
  testCancelThreadImpl(thread);
}

bool ArchMultithreadPosix::wait(ArchThread target, double timeout)
{
  assert(target != nullptr);

  ArchThreadImpl *self = nullptr;
  {
    std::scoped_lock lock{m_state->mutex};
    // find current thread
    self = findNoRefOrInsert(pthread_self());
    // ignore wait if trying to wait on ourself
    if (target == self) {
      return false;
    }
    // ref the target so it can't go away while we're watching it
    refThread(target);
  }

  try {
    // do first test regardless of timeout
    testCancelThreadImpl(self);
    if (isExitedThread(target)) {
      closeThread(target);
      return true;
    }

    // wait and repeat test if there's a timeout
    if (timeout != 0.0) {
      const auto start = std::chrono::steady_clock::now();
      const auto duration = std::chrono::duration<double>(timeout);
      do {
        // wait a little
        auto interval = std::chrono::duration<double>(0.05);
        if (timeout > 0.0) {
          const std::chrono::duration<double> remaining = duration - (std::chrono::steady_clock::now() - start);
          if (remaining <= std::chrono::duration<double>::zero()) {
            break;
          }
          interval = std::min(interval, remaining);
        }
        std::this_thread::sleep_for(interval);

        // repeat test
        testCancelThreadImpl(self);
        if (isExitedThread(target)) {
          closeThread(target);
          return true;
        }

        // repeat wait and test until timed out
      } while (timeout < 0.0 || (std::chrono::steady_clock::now() - start) < duration);
    }

    closeThread(target);
    return false;
  } catch (...) {
    closeThread(target);
    throw;
  }
}

bool ArchMultithreadPosix::isSameThread(ArchThread thread1, ArchThread thread2)
{
  return (thread1 == thread2);
}

bool ArchMultithreadPosix::isExitedThread(ArchThread thread)
{
  std::scoped_lock lock{m_state->mutex};
  return thread->m_exited;
}

void *ArchMultithreadPosix::getResultOfThread(ArchThread thread)
{
  std::scoped_lock lock{m_state->mutex};
  return thread->m_result;
}

IArchMultithread::ThreadID ArchMultithreadPosix::getIDOfThread(ArchThread thread)
{
  return thread->m_id;
}

void ArchMultithreadPosix::setSignalHandler(ThreadSignal signal, SignalFunc func, void *userData)
{
  std::scoped_lock lock{m_state->mutex};
  const auto index = static_cast<int>(signal);
  m_signalFunc[index] = func;
  m_signalUserData[index] = userData;
}

void ArchMultithreadPosix::raiseSignal(ThreadSignal signal)
{
  using enum ThreadSignal;

  std::scoped_lock lock{m_state->mutex};
  if (m_state->stopping) {
    return;
  }
  const auto index = static_cast<int>(signal);
  if (m_signalFunc[index] != nullptr) {
    m_signalFunc[index](signal, m_signalUserData[index]);
    pthread_kill(m_mainThread->m_thread, SIGWAKEUP);
  } else if (signal == Interrupt || signal == Terminate) {
    if (!m_mainThread->m_cancelling) {
      m_mainThread->m_cancel = true;
      pthread_kill(m_mainThread->m_thread, SIGWAKEUP);
    }
  }
}

bool ArchMultithreadPosix::startSignalHandler()
{
  // set signal mask.  the main thread blocks these signals and
  // the signal handler thread will listen for them.
  sigset_t sigset;
  sigset_t oldsigset;
  setSignalSet(&sigset);
  int status = pthread_sigmask(SIG_BLOCK, &sigset, &oldsigset);
  if (status != 0) {
    return false;
  }

  // fire up the INT and TERM signal handler thread.  we could
  // instead arrange to catch and handle these signals but
  // we'd be unable to cancel the main thread since no pthread
  // calls are allowed in a signal handler.
  pthread_attr_t attr;
  status = pthread_attr_init(&attr);
  if (status == 0) {
    status = m_createThread(&m_signalThread, &attr, &ArchMultithreadPosix::threadSignalHandler, this);
    pthread_attr_destroy(&attr);
  }
  if (status != 0) {
    // can't create thread to wait for signal so don't block
    // the signals.
    pthread_sigmask(SIG_SETMASK, &oldsigset, nullptr);
    return false;
  }
  m_previousSignalMask = oldsigset;
  m_signalMaskOwner = pthread_self();
  m_signalStarted = true;
  return true;
}

ArchThreadImpl *ArchMultithreadPosix::find(pthread_t thread)
{
  ArchThreadImpl *impl = findNoRefOrInsert(thread);
  if (impl != nullptr) {
    refThread(impl);
  }
  return impl;
}

ArchThreadImpl *ArchMultithreadPosix::findNoRefOrInsert(pthread_t thread)
{
  assert(pthread_equal(thread, pthread_self()));
  ArchThreadImpl *impl = findNoRef(thread);
  if (impl == nullptr) {
    // create thread for calling thread which isn't in our list and
    // add it to the list. this can happen when a foreign thread
    // (e.g. a Qt thread) calls into the arch layer.
    auto owner = std::make_unique<ArchThreadImpl>(m_state);
    owner->m_thread = thread;
    insert(owner.get());
    try {
      registerCurrentThread(owner.get());
    } catch (...) {
      m_state->threads.remove(owner.get());
      throw;
    }
    // Foreign threads are owned by their TLS registration, not by the list.
    --owner->m_refCount;
    impl = owner.release();
  }
  return impl;
}

ArchThreadImpl *ArchMultithreadPosix::findNoRef(pthread_t thread)
{
  // linear search
  for (auto *entry : m_state->threads) {
    if (entry->m_active && pthread_equal(entry->m_thread, thread)) {
      return entry;
    }
  }
  return nullptr;
}

void ArchMultithreadPosix::insert(ArchThreadImpl *thread)
{
  assert(thread != nullptr);

  // thread shouldn't already be on the list
  assert(findNoRef(thread->m_thread) == nullptr);

  // set thread id.  note that we don't worry about nextID
  // wrapping back to 0 and duplicating thread ID's since the
  // likelihood of deskflow running that long is vanishingly
  // small.
  thread->m_id = ++m_state->nextID;

  // append to list
  m_state->threads.push_back(thread);
  thread->m_active = true;
}

void ArchMultithreadPosix::refThread(ArchThreadImpl *thread)
{
  assert(thread != nullptr);
  assert(thread->m_refCount > 0);
  ++thread->m_refCount;
}

void ArchMultithreadPosix::testCancelThreadImpl(ArchThreadImpl *thread)
{
  assert(thread != nullptr);

  // update cancel state
  std::scoped_lock lock{thread->m_state->mutex};
  bool cancel = false;
  if (thread->m_cancel && !thread->m_cancelling) {
    thread->m_cancelling = true;
    thread->m_cancel = false;
    cancel = true;
  }

  // unwind thread's stack if cancelling
  if (cancel) {
    throw ThreadCancelException();
  }
}

void *ArchMultithreadPosix::threadFunc(void *vrep)
{
  // get the thread
  auto *thread = static_cast<ArchThreadImpl *>(vrep);

  // setup pthreads
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

  // run thread
  doThreadFunc(thread);

  // terminate the thread
  return nullptr;
}

void ArchMultithreadPosix::doThreadFunc(ArchThread thread)
{
  const auto state = thread->m_state;
  void *result = nullptr;
  try {
    // Wait for the parent to publish the representation, then give TLS its
    // own reference. Cleanup never needs the Arch instance after this point.
    {
      std::scoped_lock lock{state->mutex};
      registerCurrentThread(thread);
    }
    // go
    result = (*thread->m_func)(thread->m_userData);
  }

  catch (ThreadCancelException &) {
    // client called cancel()
    // set base value
    result = nullptr;
  } catch (...) {
    // note -- don't catch (...) to avoid masking bugs
    {
      std::scoped_lock lock{state->mutex};
      thread->m_exited = true;
      if (pthread_getspecific(state->currentThread) == nullptr) {
        state->threads.remove(thread);
        thread->m_active = false;
      }
    }
    releaseThread(thread);
    throw;
  }

  // Preserve the existing callback-completion semantics of wait(). Native
  // TLS cleanup can still run; its separate owner retains the representation.
  {
    std::scoped_lock lock{state->mutex};
    thread->m_result = result;
    thread->m_exited = true;
  }

  // done with thread
  releaseThread(thread);
}

void ArchMultithreadPosix::threadCancel(int)
{
  // do nothing
}

void *ArchMultithreadPosix::threadSignalHandler(void *data)
{
  auto *self = static_cast<ArchMultithreadPosix *>(data);

  // add signal to mask
  sigset_t sigset;
  setSignalSet(&sigset);

  // also wait on SIGABRT.  on linux (others?) this thread (process)
  // will persist after all the other threads evaporate due to an
  // assert unless we wait on SIGABRT.  that means our resources (like
  // the socket we're listening on) are not released and never will be
  // until the lingering thread is killed.  i don't know why sigwait()
  // should protect the thread from being killed.  note that sigwait()
  // doesn't actually return if we receive SIGABRT and, for some
  // reason, we don't have to block SIGABRT.
  sigaddset(&sigset, SIGABRT);

  // The owner requests shutdown with a thread-directed signal, then joins us.
  for (;;) {
    // wait
    int signal = 0;
    const int status = sigwait(&sigset, &signal);
    {
      std::scoped_lock lock{self->m_state->mutex};
      if (self->m_signalStop) {
        return nullptr;
      }
    }
    if (status != 0) {
      return nullptr;
    }

    // if we get here then the signal was raised
    switch (signal) {
      using enum ThreadSignal;
    case SIGINT:
      self->raiseSignal(Interrupt);
      break;

    case SIGTERM:
      self->raiseSignal(Terminate);
      break;

    case SIGHUP:
      self->raiseSignal(Hangup);
      break;

    case SIGUSR2:
      self->raiseSignal(User);
      break;

    default:
      // ignore
      break;
    }
  }

  return nullptr;
}
