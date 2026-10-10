/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "arch/ArchException.h"

#include <QThread>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <tuple>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

namespace {
using namespace std::chrono_literals;
constexpr int kCycles = 16;
using Fds = std::map<int, std::tuple<dev_t, ino_t, mode_t>>;

void check(bool condition, const char *message)
{
  if (!condition)
    throw std::runtime_error(message);
}

Fds openFds()
{
#if defined(__APPLE__)
  const char *path = "/dev/fd";
#else
  const char *path = "/proc/self/fd";
#endif
  std::unique_ptr<DIR, decltype(&closedir)> directory(opendir(path), closedir);
  check(directory != nullptr, "Cannot enumerate this process's descriptors");
  Fds result;
  while (const auto *entry = readdir(directory.get())) {
    char *end = nullptr;
    const long fd = strtol(entry->d_name, &end, 10);
    if (end == entry->d_name || *end != '\0' || fd == dirfd(directory.get()))
      continue;
    struct stat status;
    if (fstat(static_cast<int>(fd), &status) != 0) {
      if (errno == EBADF)
        continue; // A detached worker can close a descriptor during enumeration.
      throw std::runtime_error("Cannot identify an open descriptor");
    }
    result.emplace(static_cast<int>(fd), std::make_tuple(status.st_dev, status.st_ino, status.st_mode));
  }
  return result;
}

size_t nativeThreadCount()
{
#if defined(__APPLE__)
  thread_act_array_t threads = nullptr;
  mach_msg_type_number_t count = 0;
  check(task_threads(mach_task_self(), &threads, &count) == KERN_SUCCESS, "Cannot enumerate native threads");
  for (mach_msg_type_number_t i = 0; i < count; ++i)
    mach_port_deallocate(mach_task_self(), threads[i]);
  vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(threads), count * sizeof(thread_t));
  return count;
#else
  std::unique_ptr<DIR, decltype(&closedir)> directory(opendir("/proc/self/task"), closedir);
  check(directory != nullptr, "Cannot enumerate native threads");
  size_t count = 0;
  while (const auto *entry = readdir(directory.get()))
    if (entry->d_name[0] != '.')
      ++count;
  return count;
#endif
}

void checkFds(const Fds &expected, const char *phase)
{
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  auto actual = openFds();
  while (actual != expected && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(1ms);
    actual = openFds();
  }
  std::printf("%s: descriptors=%zu expected=%zu\n", phase, actual.size(), expected.size());
  check(actual == expected, "Descriptor identities did not return to their baseline");
}

void checkThreads(size_t expected)
{
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (nativeThreadCount() != expected && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(1ms);
  check(nativeThreadCount() == expected, "Native threads did not return to their baseline");
}

class Gate
{
public:
  void open()
  {
    std::lock_guard lock(m_mutex);
    m_open = true;
    m_condition.notify_all();
  }

  void wait()
  {
    std::unique_lock lock(m_mutex);
    check(m_condition.wait_for(lock, 10s, [this] { return m_open; }), "Synchronization deadline expired");
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_open = false;
};

class ManagedThread
{
public:
  explicit ManagedThread(std::function<void()> body) : m_body(std::move(body)), m_thread(ARCH->newThread(run, this))
  {
    check(m_thread != nullptr, "Cannot create managed thread");
  }

  ~ManagedThread()
  {
    if (m_thread != nullptr) {
      if (!ARCH->wait(m_thread, 15.0))
        std::abort(); // A live worker must not retain this destroyed context.
      ARCH->closeThread(m_thread);
    }
  }

  ManagedThread(const ManagedThread &) = delete;
  ManagedThread &operator=(const ManagedThread &) = delete;

  ArchThread get() const
  {
    return m_thread;
  }

  void finish(bool cancelled = false)
  {
    check(ARCH->wait(m_thread, 15.0), "Managed thread did not finish");
    check(ARCH->getResultOfThread(m_thread) == (cancelled ? nullptr : this), "Managed thread result changed");
    ARCH->closeThread(m_thread);
    m_thread = nullptr;
    if (m_error)
      std::rethrow_exception(m_error);
  }

private:
  static void *run(void *context)
  {
    auto &self = *static_cast<ManagedThread *>(context);
    try {
      self.m_body();
    } catch (const ThreadException &) {
      throw; // Cooperative cancellation must reach the production thread entry point.
    } catch (...) {
      self.m_error = std::current_exception();
    }
    return context;
  }

  std::function<void()> m_body;
  std::exception_ptr m_error;
  ArchThread m_thread;
};

struct SocketDeleter
{
  void operator()(ArchSocket socket) const
  {
    ARCH->closeSocket(socket);
  }
};

class QuietSocket
{
public:
  QuietSocket() : m_socket(ARCH->newSocket(IArchNetwork::AddressFamily::INet, IArchNetwork::SocketType::Stream))
  {
    const auto addresses = ARCH->nameToAddr("127.0.0.1");
    check(!addresses.empty(), "No numeric loopback address");
    try {
      ARCH->setAddrPort(addresses.front(), 0);
      ARCH->bindSocket(m_socket.get(), addresses.front());
      ARCH->listenOnSocket(m_socket.get());
    } catch (...) {
      for (const auto address : addresses)
        ARCH->closeAddr(address);
      throw;
    }
    for (const auto address : addresses)
      ARCH->closeAddr(address);
  }

  void poll(ArchNetworkBSD &network, double timeout = 0.0) const
  {
    IArchNetwork::PollEntry entry{m_socket.get(), IArchNetwork::PollEventMask::In, 0xffff};
    check(network.pollSocket(&entry, 1, timeout) == 0, "Quiet listener unexpectedly became ready");
    check(entry.m_revents == 0, "Quiet poll did not clear events");
  }

private:
  std::unique_ptr<ArchSocketImpl, SocketDeleter> m_socket;
};

struct ObservedPoll : ArchNetworkBSD::Deps
{
  int poll(struct pollfd *fds, nfds_t size, int timeout) override
  {
    if (entered != nullptr)
      entered->open();
    const int result = Deps::poll(fds, size, timeout);
    lastResult = result;
    return result;
  }
  Gate *entered = nullptr;
  int lastResult = -1;
};

void repeatedWorkers()
{
  const auto threads = nativeThreadCount();
  QuietSocket socket;
  socket.poll(*ARCH);
  const auto baseline = openFds();
  for (int cycle = 0; cycle < kCycles; ++cycle) {
    ManagedThread worker([&] { socket.poll(*ARCH); });
    worker.finish();
    checkThreads(threads + 1); // The lazy signal handler remains until Arch destruction.
    checkFds(baseline, "managed worker released");
  }
}

void cooperativeCancel()
{
  const auto threads = nativeThreadCount();
  QuietSocket socket;
  socket.poll(*ARCH);
  const auto baseline = openFds();
  for (int cycle = 0; cycle < kCycles; ++cycle) {
    Gate ready;
    bool cancelled = false;
    bool returnedNormally = false;
    ManagedThread worker([&] {
      socket.poll(*ARCH);
      ready.open();
      try {
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (std::chrono::steady_clock::now() < deadline) {
          ARCH->testCancelThread();
          std::this_thread::sleep_for(1ms);
        }
        returnedNormally = true;
      } catch (const ThreadCancelException &) {
        cancelled = true;
        throw;
      }
    });
    ready.wait();
    ARCH->cancelThread(worker.get());
    worker.finish(true);
    checkThreads(threads + 1);
    check(cancelled && !returnedNormally, "Worker returned without observing cooperative cancellation");
    checkFds(baseline, "cancelled worker released");
  }
}

void firstUnblock()
{
  const auto threads = nativeThreadCount();
  auto observation = std::make_shared<ObservedPoll>();
  ArchNetworkBSD network(observation);
  QuietSocket socket;
  socket.poll(network); // Keep a live caller pipe which must not be overwritten.
  const auto baseline = openFds();
  for (int cycle = 0; cycle < kCycles; ++cycle) {
    Gate release;
    ManagedThread target([&] { release.wait(); });
    network.unblockPollSocket(target.get()); // The target has never called poll.
    socket.poll(network, 0.005);
    const bool callerTimedOut = observation->lastResult == 0;
    release.open();
    target.finish();
    checkThreads(threads + 1);
    check(callerTimedOut, "Unblocking another thread delivered a wake to the caller");
    checkFds(baseline, "first unblock target released");
  }
}

void concurrentWait()
{
  const auto threads = nativeThreadCount();
  QuietSocket socket;
  socket.poll(*ARCH);
  const auto baseline = openFds();
  for (int cycle = 0; cycle < kCycles; ++cycle) {
    Gate release, firstReady, secondReady;
    ManagedThread target([&] {
      socket.poll(*ARCH);
      release.wait();
    });
    // Each waiter owns its own reference. No raw handle is used after the last close.
    const auto first = ARCH->copyThread(target.get());
    const auto second = ARCH->copyThread(target.get());
    auto wait = [&](ArchThread reference, Gate &ready) {
      ready.open();
      for (int copy = 0; copy < 64; ++copy)
        ARCH->closeThread(ARCH->copyThread(reference));
      const bool finished = ARCH->wait(reference, 15.0);
      const bool hasResult = finished && ARCH->getResultOfThread(reference) == &target;
      ARCH->closeThread(reference);
      check(finished && hasResult, "Concurrent wait lost the target result");
    };
    ManagedThread waiter1([&] { wait(first, firstReady); });
    ManagedThread waiter2([&] { wait(second, secondReady); });
    firstReady.wait();
    secondReady.wait();
    release.open();
    target.finish();
    waiter1.finish();
    waiter2.finish();
    checkThreads(threads + 1);
    checkFds(baseline, "concurrent owners released");
  }
}

struct TlsGate
{
  Gate *entered = nullptr;
  Gate *release = nullptr;
  std::atomic<bool> *completed = nullptr;
  ~TlsGate()
  {
    if (entered != nullptr) {
      entered->open();
      release->wait();
      *completed = true;
    }
  }
};

void waitForTls()
{
  const auto threads = nativeThreadCount();
  Gate entered, release;
  std::atomic<bool> completed = false;
  ManagedThread worker([&] {
    thread_local TlsGate cleanup;
    cleanup.entered = &entered;
    cleanup.release = &release;
    cleanup.completed = &completed;
  });
  entered.wait();
  const bool returnedBeforeTls = ARCH->wait(worker.get(), 0.0);
  release.open();
  worker.finish();
  checkThreads(threads + 1);
  check(returnedBeforeTls, "wait no longer reports callback completion during TLS cleanup");
  check(completed, "TLS destructor did not finish after release");
}

struct TlsWait
{
  Gate *entered = nullptr;
  Gate *release = nullptr;
  ArchThread *target = nullptr;
  std::atomic<bool> *succeeded = nullptr;
  ~TlsWait()
  {
    if (entered != nullptr) {
      entered->open();
      release->wait();
      *succeeded = ARCH->wait(*target, 2.0);
      ARCH->closeThread(*target);
    }
  }
};

void crossWorkerTlsWait()
{
  const auto threads = nativeThreadCount();
  Gate entered, release;
  ArchThread target = nullptr;
  std::atomic<bool> succeeded = false;
  ManagedThread first([&] {
    thread_local TlsWait cleanup;
    cleanup.entered = &entered;
    cleanup.release = &release;
    cleanup.target = &target;
    cleanup.succeeded = &succeeded;
  });
  entered.wait(); // A is already in TLS cleanup before B is created.
  ManagedThread second([] {});
  target = ARCH->copyThread(second.get());
  release.open();
  first.finish();
  second.finish();
  checkThreads(threads + 1);
  check(succeeded, "One worker's TLS cleanup prevented another worker from completing");
}

struct TlsPoll
{
  const QuietSocket *socket = nullptr;
  ArchNetworkBSD *network = nullptr;
  std::exception_ptr *error = nullptr;
  Gate *completed = nullptr;
  ~TlsPoll()
  {
    if (socket != nullptr) {
      try {
        socket->poll(*network, 2.0);
      } catch (...) {
        *error = std::current_exception();
      }
      completed->open();
    }
  }
};

void firstPollInTls()
{
  const auto threads = nativeThreadCount();
  Gate entered, completed;
  auto observation = std::make_shared<ObservedPoll>();
  observation->entered = &entered;
  ArchNetworkBSD network(observation);
  QuietSocket socket;
  std::exception_ptr error;
  ManagedThread worker([&] {
    thread_local TlsPoll cleanup;
    cleanup.socket = &socket;
    cleanup.network = &network;
    cleanup.error = &error;
    cleanup.completed = &completed;
    // The callback never polls; its first wake pipe is created during TLS cleanup.
  });
  entered.wait();
  network.unblockPollSocket(worker.get());
  worker.finish();
  completed.wait();
  checkThreads(threads + 1);
  if (error)
    std::rethrow_exception(error);
  check(observation->lastResult == 1, "The first poll in TLS cleanup timed out instead of receiving its wake");
}

class ForeignQtThread : public QThread
{
public:
  explicit ForeignQtThread(std::function<void()> body) : m_body(std::move(body))
  {
  }
  ~ForeignQtThread() override
  {
    if (!wait(15000))
      std::abort();
  }
  std::exception_ptr error;

private:
  void run() override
  {
    try {
      m_body();
    } catch (...) {
      error = std::current_exception();
    }
  }
  std::function<void()> m_body;
};

void foreignThreads(bool qt)
{
  QuietSocket socket;
  socket.poll(*ARCH);
  const auto baseline = openFds();
  IArchMultithread::ThreadID previous = 0;
  for (int cycle = 0; cycle < kCycles; ++cycle) {
    ArchThread retained = nullptr;
    std::exception_ptr error;
    auto body = [&] {
      try {
        retained = ARCH->newCurrentThread();
        const auto copy = ARCH->newCurrentThread();
        const bool same = ARCH->isSameThread(retained, copy);
        ARCH->closeThread(copy);
        check(same, "One foreign thread acquired different identities");
        socket.poll(*ARCH);
      } catch (...) {
        error = std::current_exception();
      }
    };
    if (qt) {
      ForeignQtThread thread(body);
      thread.start();
      check(thread.wait(15000), "Foreign Qt thread did not exit");
      if (thread.error)
        std::rethrow_exception(thread.error);
    } else {
      std::thread thread(body);
      thread.join();
    }
    check(retained != nullptr, "Foreign thread did not publish a reference");
    const auto id = ARCH->getIDOfThread(retained);
    const bool exited = ARCH->isExitedThread(retained);
    ARCH->closeThread(retained);
    if (error)
      std::rethrow_exception(error);
    check(exited, "Native foreign thread exit was not recorded");
    check(id != previous, "A new foreign thread reused dead metadata");
    previous = id;
    checkFds(baseline, "foreign thread released");
  }
}

void foreignOutlivesArch()
{
  Gate ready, release;
  std::exception_ptr error;
  std::thread foreign;
  {
    Arch arch;
    foreign = std::thread([&] {
      try {
        const auto reference = ARCH->newCurrentThread();
        ARCH->closeThread(reference);
        {
          QuietSocket socket;
          socket.poll(*ARCH);
        } // The socket closes now; the thread's pipe survives until its later TLS cleanup.
        ready.open();
        release.wait(); // Its TLS cleanup runs after Arch has been destroyed.
      } catch (...) {
        error = std::current_exception();
        ready.open();
      }
    });
    ready.wait();
  }
  release.open();
  foreign.join();
  if (error)
    std::rethrow_exception(error);
}

#ifndef POSIX_LIFECYCLE_BASELINE
// Only these fault cases require the new production syscall seams.
struct PipeRace
{
  Gate first, second, releaseCandidates;
  std::atomic<int> arrivals = 0;
  bool armed = false;
};

struct RacingPipe : ObservedPoll
{
  explicit RacingPipe(PipeRace &race) : race(race)
  {
  }
  int createPipe(int fds[2]) override
  {
    const int result = Deps::createPipe(fds);
    if (result == 0 && race.armed) {
      if (race.arrivals++ == 0)
        race.first.open();
      else
        race.second.open();
      race.releaseCandidates.wait();
    }
    return result;
  }
  PipeRace &race;
};

void concurrentFirstUnblock()
{
  const auto threads = nativeThreadCount();
  QuietSocket socket;
  socket.poll(*ARCH);
  const auto baseline = openFds();
  for (int cycle = 0; cycle < kCycles; ++cycle) {
    PipeRace race;
    Gate ready[2], start, releaseTarget, polling, pollCompleted, releaseTargetExit;
    std::exception_ptr errors[2];
    auto observation = std::make_shared<ObservedPoll>();
    observation->entered = &polling;
    ArchNetworkBSD network(observation);
    ManagedThread target([&] {
      releaseTarget.wait();
      try {
        socket.poll(network, 2.0);
      } catch (...) {
        pollCompleted.open();
        throw;
      }
      pollCompleted.open();
      releaseTargetExit.wait(); // Both installers must finish while the target is still active.
    });
    auto caller = [&](int index) {
      try {
        auto close = [](ArchThread reference) { ARCH->closeThread(reference); };
        std::unique_ptr<ArchThreadImpl, decltype(close)> reference(ARCH->copyThread(target.get()), close);
        auto dependency = std::make_shared<RacingPipe>(race);
        ArchNetworkBSD callerNetwork(dependency);
        socket.poll(callerNetwork); // Each caller has its own pipe before the installation race.
        ready[index].open();
        start.wait();
        callerNetwork.unblockPollSocket(reference.get());
        socket.poll(callerNetwork, 0.005);
        check(dependency->lastResult == 0, "Concurrent first wake was delivered to its caller");
      } catch (...) {
        errors[index] = std::current_exception();
        ready[index].open();
      }
    };
    std::jthread first, second;
    try {
      first = std::jthread(caller, 0);
      second = std::jthread(caller, 1);
      ready[0].wait();
      ready[1].wait();
      race.armed = true;
      start.open();
      race.first.wait();
      race.second.wait();
      // Both candidate allocations are paused before installation. The target's plain
      // dependency now installs a third pipe and captures it in its actual poll query.
      releaseTarget.open();
      polling.wait();
      race.releaseCandidates.open();
      first.join();
      second.join();
      pollCompleted.wait(); // Publishes lastResult independently of native-thread enumeration.
      const bool receivedWake = observation->lastResult == 1;
      const bool onePipe = openFds().size() == baseline.size() + 2;
      releaseTargetExit.open();
      target.finish();
      checkThreads(threads + 1);
      for (const auto &error : errors)
        if (error)
          std::rethrow_exception(error);
      check(race.arrivals == 2 && onePipe, "Concurrent installation did not retain exactly one target pipe");
      check(receivedWake, "Target's captured poll pipe was replaced instead of receiving the callers' wake");
      checkFds(baseline, "concurrent first wake released");
    } catch (...) {
      // Release every worker gate before jthread's destructors join, including when
      // constructing the second caller or waiting for an observation fails.
      start.open();
      race.releaseCandidates.open();
      releaseTarget.open();
      releaseTargetExit.open();
      throw;
    }
  }
}

struct FailedPipe : ArchNetworkBSD::Deps
{
  int failAt = 0;
  int calls = 0;
  bool failCreate = false;
  int createPipe(int fds[2]) override
  {
    if (failCreate) {
      failCreate = false;
      errno = EMFILE;
      return -1;
    }
    return Deps::createPipe(fds);
  }
  int setNonBlocking(int fd) override
  {
    if (++calls == failAt) {
      errno = EIO;
      return -1;
    }
    return Deps::setNonBlocking(fd);
  }
};

void failedPipe(int step)
{
  auto failure = std::make_shared<FailedPipe>();
  failure->failAt = step;
  failure->failCreate = step == 0;
  ArchNetworkBSD network(failure);
  QuietSocket socket;
  const auto baseline = openFds();
  socket.poll(network);
  check(step == 0 ? !failure->failCreate : failure->calls == step, "The requested pipe fault was not reached");
  checkFds(baseline, "failed pipe setup");
  failure->failAt = 0;
  socket.poll(network); // Failure must not publish an unusable pipe or poison a retry.
  check(openFds().size() == baseline.size() + 2, "Retry did not create exactly one pipe");
}

struct WakeObservation : ArchNetworkBSD::Deps
{
  ssize_t read(int fd, void *buffer, size_t size) override
  {
    const auto count = Deps::read(fd, buffer, size);
    if (count > 0)
      drained += static_cast<size_t>(count);
    return count;
  }
  size_t drained = 0;
};

void fullWakePipe()
{
  auto observation = std::make_shared<WakeObservation>();
  ArchNetworkBSD network(observation);
  QuietSocket socket;
  socket.poll(network);
  const auto self = ARCH->newCurrentThread();
  constexpr size_t requests = 262144;
  for (size_t i = 0; i < requests; ++i)
    network.unblockPollSocket(self);
  ARCH->closeThread(self);
  socket.poll(network);
  check(observation->drained > 0 && observation->drained < requests, "The wake burst did not exercise a full pipe");
  const auto drained = observation->drained;
  socket.poll(network);
  check(observation->drained == drained, "The wake pipe did not drain completely");
}

std::atomic<int> createCalls = 0;
int failedCreateCall = 0;

int failThreadCreate(pthread_t *thread, const pthread_attr_t *attr, void *(*entry)(void *), void *context)
{
  if (++createCalls == failedCreateCall)
    return EAGAIN;
  return pthread_create(thread, attr, entry, context);
}

void failedThread(int step)
{
  createCalls = 0;
  failedCreateCall = step;
  ArchMultithreadPosix backend(failThreadCreate);
  sigset_t before, after;
  check(pthread_sigmask(SIG_SETMASK, nullptr, &before) == 0, "Cannot read signal mask");
  auto body = +[](void *context) -> void * { return context; };
  auto thread = backend.newThread(body, &backend);
  check(thread == nullptr, "Injected pthread_create failure did not reject newThread");
  check(createCalls >= step, "The requested startup fault was not reached");
  check(pthread_sigmask(SIG_SETMASK, nullptr, &after) == 0, "Cannot read signal mask after failure");
  if (step == 1) {
    for (const int signal : {SIGHUP, SIGINT, SIGTERM, SIGUSR2, SIGUSR1, SIGPIPE})
      check(sigismember(&before, signal) == sigismember(&after, signal), "Failed signal startup changed the mask");
  }
  failedCreateCall = 0;
  thread = backend.newThread(body, &backend);
  check(thread != nullptr, "Startup did not recover after a one-shot failure");
  const bool finished = backend.wait(thread, 15.0);
  backend.closeThread(thread);
  check(finished, "Retried worker did not finish");
}
#endif

void run(const std::string &name)
{
  if (name == "foreign-outlives-arch") {
    foreignOutlivesArch();
    return;
  }
#ifndef POSIX_LIFECYCLE_BASELINE
  if (name == "startup-1" || name == "startup-2") {
    failedThread(name.back() - '0');
    return;
  }
#endif
  Arch arch;
  if (name == "main-only") {
    // Normal Arch destruction is the memory assertion in this process.
  } else if (name == "main-poll") {
    QuietSocket socket;
    socket.poll(*ARCH);
  } else if (name == "managed-repeat") {
    repeatedWorkers();
  } else if (name == "cooperative-cancel") {
    cooperativeCancel();
  } else if (name == "first-unblock") {
    firstUnblock();
  } else if (name == "concurrent-wait") {
    concurrentWait();
  } else if (name == "wait-tls") {
    waitForTls();
  } else if (name == "cross-worker-tls-wait") {
    crossWorkerTlsWait();
  } else if (name == "tls-first-poll") {
    firstPollInTls();
  } else if (name == "signal-stop") {
    const auto threads = nativeThreadCount();
    ManagedThread worker([] {});
    worker.finish();
    checkThreads(threads + 1);
  } else if (name == "foreign-std" || name == "foreign-qt") {
    foreignThreads(name == "foreign-qt");
#ifndef POSIX_LIFECYCLE_BASELINE
  } else if (name == "concurrent-first-unblock") {
    concurrentFirstUnblock();
  } else if (name == "pipe-create-failure" || name == "pipe-read-flags-failure" || name == "pipe-write-flags-failure") {
    failedPipe(name == "pipe-create-failure" ? 0 : (name == "pipe-read-flags-failure" ? 1 : 2));
  } else if (name == "full-wake-pipe") {
    fullWakePipe();
#endif
  } else {
    throw std::runtime_error("Unknown lifecycle case");
  }
}
} // namespace

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::fprintf(stderr, "Usage: ArchPosixLifecycleTests CASE\n");
    return 2;
  }
  try {
    // Qt can retain a process-global dispatcher FD after its first thread.
    // Initialize it without Arch before measuring Deskflow-owned resources.
    if (std::string(argv[1]) == "foreign-qt") {
      ForeignQtThread warmup([] {});
      warmup.start();
      check(warmup.wait(15000), "Empty Qt warmup thread did not exit");
      if (warmup.error)
        std::rethrow_exception(warmup.error);
    }
    const auto descriptors = openFds();
    const auto threads = nativeThreadCount();
    run(argv[1]); // All production owners have been destroyed before the final snapshots.
    checkFds(descriptors, "Arch destroyed");
    checkThreads(threads);
    std::printf("PASS %s\n", argv[1]);
    return 0;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "FAIL %s: %s\n", argv[1], error.what());
    return 1;
  }
}
