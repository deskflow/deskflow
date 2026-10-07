/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "arch/ArchException.h"

#include <QElapsedTimer>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {
using AddressFamily = IArchNetwork::AddressFamily;
using PollEntry = IArchNetwork::PollEntry;
using PollEventMask = IArchNetwork::PollEventMask;
constexpr int kTimeoutMs = 2000;

struct SocketDeleter
{
  void operator()(ArchSocket socket) const noexcept
  {
    try {
      ARCH->closeSocket(socket);
    } catch (const std::exception &error) {
      qFatal("Cannot close test socket: %s", error.what());
    }
  }
};

struct AddressDeleter
{
  void operator()(ArchNetAddress address) const
  {
    ARCH->closeAddr(address);
  }
};

using Socket = std::unique_ptr<ArchSocketImpl, SocketDeleter>;
using Address = std::unique_ptr<ArchNetAddressImpl, AddressDeleter>;

class NumericAddresses
{
public:
  explicit NumericAddresses(const std::string &literal) : m_addresses(ARCH->nameToAddr(literal))
  {
  }

  ~NumericAddresses()
  {
    for (auto address : m_addresses) {
      ARCH->closeAddr(address);
    }
  }

  NumericAddresses(const NumericAddresses &) = delete;
  NumericAddresses &operator=(const NumericAddresses &) = delete;

  const std::vector<ArchNetAddress> &get() const
  {
    return m_addresses;
  }

private:
  std::vector<ArchNetAddress> m_addresses;
};

class LoopbackConnection
{
public:
  bool open()
  {
    server.setProxy(QNetworkProxy::NoProxy);
    if (!server.listen(QHostAddress::LocalHostIPv6, 0)) {
      error = server.errorString();
      return false;
    }

    try {
      NumericAddresses addresses("::1");
      if (addresses.get().empty()) {
        error = "No address returned for IPv6 loopback";
        return false;
      }
      auto address = addresses.get().front();
      ARCH->setAddrPort(address, server.serverPort());
      socket.reset(ARCH->newSocket(AddressFamily::INet6, IArchNetwork::SocketType::Stream));
      if (!ARCH->connectSocket(socket.get(), address)) {
        PollEntry entry{socket.get(), PollEventMask::Out, 0};
        if (ARCH->pollSocket(&entry, 1, kTimeoutMs / 1000.0) != 1 || (entry.m_revents & PollEventMask::Out) == 0) {
          error = "Loopback connection did not become writable";
          return false;
        }
      }
      ARCH->throwErrorOnSocket(socket.get());
    } catch (const ArchNetworkException &exception) {
      error = QString::fromUtf8(exception.what());
      return false;
    }

    if (!QTest::qWaitFor([this] { return server.hasPendingConnections(); }, kTimeoutMs)) {
      error = "Loopback server did not receive a connection";
      return false;
    }
    peer.reset(server.nextPendingConnection());
    if (!peer) {
      error = "Loopback server returned no pending socket";
      return false;
    }
    peer->setProxy(QNetworkProxy::NoProxy);
    return true;
  }

  QTcpServer server;
  Socket socket;
  std::unique_ptr<QTcpSocket> peer;
  QString error;
};

QByteArray payload()
{
  QByteArray data(257, Qt::Uninitialized);
  for (qsizetype i = 0; i < data.size(); ++i) {
    data[i] = static_cast<char>(i % 251);
  }
  return data;
}

qsizetype writeAll(ArchSocket socket, const QByteArray &data)
{
  QElapsedTimer timer;
  timer.start();
  qsizetype written = 0;
  while (written < data.size() && timer.elapsed() < kTimeoutMs) {
    PollEntry entry{socket, PollEventMask::Out, 0};
    if (ARCH->pollSocket(&entry, 1, 0.05) == 0) {
      continue;
    }
    ARCH->throwErrorOnSocket(socket);
    if ((entry.m_revents & PollEventMask::Out) != 0) {
      const auto chunk = static_cast<size_t>(std::min<qsizetype>(7, data.size() - written));
      written += static_cast<qsizetype>(ARCH->writeSocket(socket, data.constData() + written, chunk));
    }
  }
  return written;
}

QByteArray readAll(ArchSocket socket, qsizetype size)
{
  QElapsedTimer timer;
  timer.start();
  QByteArray received;
  std::array<char, 7> buffer;
  while (received.size() < size && timer.elapsed() < kTimeoutMs) {
    // Reading also rearms Winsock's FD_READ notification after an earlier poll.
    const auto count = ARCH->readSocket(socket, buffer.data(), buffer.size());
    if (count != 0) {
      received.append(buffer.data(), static_cast<qsizetype>(count));
      continue;
    }
    PollEntry entry{socket, PollEventMask::In, 0};
    ARCH->pollSocket(&entry, 1, 0.05);
    ARCH->throwErrorOnSocket(socket);
  }
  return received;
}

class PollWorker
{
public:
  explicit PollWorker(ArchSocket socket) : m_socket(socket), m_thread(ARCH->newThread(run, this))
  {
  }

  ~PollWorker()
  {
    // The worker's poll has a finite timeout even if unblocking is broken.
    if (!join()) {
      qFatal("Polling worker did not exit within its cleanup deadline");
    }
  }

  PollWorker(const PollWorker &) = delete;
  PollWorker &operator=(const PollWorker &) = delete;

  ArchThread thread() const
  {
    return m_thread;
  }

  bool waitReady()
  {
    std::unique_lock lock(m_mutex);
    m_condition.wait_for(lock, std::chrono::milliseconds(kTimeoutMs), [this] { return m_ready || m_finished; });
    return m_ready;
  }

  bool waitFinished(std::chrono::milliseconds timeout)
  {
    std::unique_lock lock(m_mutex);
    return m_condition.wait_for(lock, timeout, [this] { return m_finished; });
  }

  bool join()
  {
    if (m_thread == nullptr) {
      return true;
    }
    if (!ARCH->wait(m_thread, 7.0)) {
      return false;
    }
    ARCH->closeThread(m_thread);
    m_thread = nullptr;
    return true;
  }

  int result = -1;
  unsigned short events = 0;
  std::exception_ptr error;

private:
  static void *run(void *context)
  {
    auto &worker = *static_cast<PollWorker *>(context);
    try {
      PollEntry entry{worker.m_socket, PollEventMask::In, 0};
      // Create this thread's wakeup pipe/event before allowing another thread to signal it.
      if (ARCH->pollSocket(&entry, 1, 0.0) != 0 || entry.m_revents != 0) {
        throw std::runtime_error("Polling worker requires a quiet connected socket");
      }
      {
        std::scoped_lock lock(worker.m_mutex);
        worker.m_ready = true;
      }
      worker.m_condition.notify_all();
      worker.result = ARCH->pollSocket(&entry, 1, 5.0);
      worker.events = entry.m_revents;
    } catch (const ThreadException &) {
      throw;
    } catch (...) {
      worker.error = std::current_exception();
    }
    {
      std::scoped_lock lock(worker.m_mutex);
      worker.m_finished = true;
    }
    worker.m_condition.notify_all();
    return nullptr;
  }

  ArchSocket m_socket;
  std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_ready = false;
  bool m_finished = false;
  ArchThread m_thread;
};
} // namespace

class ArchNetworkTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase()
  {
    m_arch.init();
  }

  void newAnyAddr_preservesFamilyAndZeroPort_data()
  {
    QTest::addColumn<int>("family");
    QTest::addColumn<QString>("literal");
    QTest::newRow("IPv4") << static_cast<int>(AddressFamily::INet) << QString("0.0.0.0");
    QTest::newRow("IPv6") << static_cast<int>(AddressFamily::INet6) << QString("::");
  }

  void newAnyAddr_preservesFamilyAndZeroPort()
  {
    QFETCH(int, family);
    QFETCH(QString, literal);
    Address address(ARCH->newAnyAddr(static_cast<AddressFamily>(family)));

    QVERIFY(address);
    QCOMPARE(ARCH->getAddrFamily(address.get()), static_cast<AddressFamily>(family));
    QCOMPARE(ARCH->getAddrPort(address.get()), 0);
    QCOMPARE(QString::fromStdString(ARCH->addrToString(address.get())), literal);
    if (static_cast<AddressFamily>(family) == AddressFamily::INet) {
      QVERIFY(ARCH->isAnyAddr(address.get()));
    }
  }

  void nameToAddr_numericLoopbackIsNotAny_data()
  {
    QTest::addColumn<int>("family");
    QTest::addColumn<QString>("literal");
    QTest::newRow("IPv4") << static_cast<int>(AddressFamily::INet) << QString("127.0.0.1");
    QTest::newRow("IPv6") << static_cast<int>(AddressFamily::INet6) << QString("::1");
  }

  void nameToAddr_numericLoopbackIsNotAny()
  {
    QFETCH(int, family);
    QFETCH(QString, literal);
    NumericAddresses addresses(literal.toStdString());

    QVERIFY(!addresses.get().empty());
    for (auto address : addresses.get()) {
      QCOMPARE(ARCH->getAddrFamily(address), static_cast<AddressFamily>(family));
      QCOMPARE(QString::fromStdString(ARCH->addrToString(address)), literal);
      QVERIFY(!ARCH->isAnyAddr(address));
    }
  }

  void copyAddr_preservesValueAndHasIndependentPort_data()
  {
    QTest::addColumn<int>("family");
    QTest::addColumn<QString>("literal");
    QTest::newRow("IPv4") << static_cast<int>(AddressFamily::INet) << QString("127.0.0.1");
    QTest::newRow("IPv6") << static_cast<int>(AddressFamily::INet6) << QString("::1");
  }

  void copyAddr_preservesValueAndHasIndependentPort()
  {
    QFETCH(int, family);
    QFETCH(QString, literal);
    NumericAddresses addresses(literal.toStdString());
    QVERIFY(!addresses.get().empty());
    const auto original = addresses.get().front();
    ARCH->setAddrPort(original, 1);
    Address copy(ARCH->copyAddr(original));

    QVERIFY(copy.get() != original);
    QVERIFY(ARCH->isEqualAddr(original, copy.get()));
    QCOMPARE(ARCH->getAddrFamily(copy.get()), static_cast<AddressFamily>(family));
    QCOMPARE(QString::fromStdString(ARCH->addrToString(copy.get())), literal);
    QCOMPARE(ARCH->getAddrPort(copy.get()), 1);
    ARCH->setAddrPort(copy.get(), 65535);
    QCOMPARE(ARCH->getAddrPort(copy.get()), 65535);
    QCOMPARE(ARCH->getAddrPort(original), 1);
    QVERIFY(!ARCH->isEqualAddr(original, copy.get()));
  }

  void pollSocket_zeroEntriesAndZeroTimeout_returnsZero()
  {
    QCOMPARE(ARCH->pollSocket(nullptr, 0, 0.0), 0);
  }

  void pollSocket_noData_timesOutAndClearsEvents()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));
    PollEntry entry{connection.socket.get(), PollEventMask::In, PollEventMask::In | PollEventMask::Out};
    QElapsedTimer timer;
    timer.start();

    QCOMPARE(ARCH->pollSocket(&entry, 1, 0.025), 0);
    QCOMPARE(entry.m_revents, 0);
    QVERIFY(timer.elapsed() >= 10);
  }

  void pollSocket_connectedSocket_remainsWritable()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));
    PollEntry entry{connection.socket.get(), PollEventMask::Out, PollEventMask::Invalid};

    for (int attempt = 0; attempt < 2; ++attempt) {
      QCOMPARE(ARCH->pollSocket(&entry, 1, kTimeoutMs / 1000.0), 1);
      QCOMPARE(entry.m_revents, PollEventMask::Out);
      ARCH->throwErrorOnSocket(connection.socket.get());
    }
  }

  void readSocket_receivesExactPayloadInSmallChunks()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));
    const auto expected = payload();
    QCOMPARE(connection.peer->write(expected), expected.size());
    QTRY_COMPARE_WITH_TIMEOUT(connection.peer->bytesToWrite(), 0, kTimeoutMs);
    PollEntry entry{connection.socket.get(), PollEventMask::In, PollEventMask::Invalid};

    QCOMPARE(ARCH->pollSocket(&entry, 1, kTimeoutMs / 1000.0), 1);
    QCOMPARE(entry.m_revents, PollEventMask::In);
    QCOMPARE(readAll(connection.socket.get(), expected.size()), expected);
    QCOMPARE(ARCH->pollSocket(&entry, 1, 0.0), 0);
    QCOMPARE(entry.m_revents, 0);
    char byte;
    QCOMPARE(ARCH->readSocket(connection.socket.get(), &byte, 1), size_t(0));
  }

  void writeSocket_sendsExactPayloadInSmallChunks()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));
    const auto expected = payload();

    QCOMPARE(writeAll(connection.socket.get(), expected), expected.size());
    QTRY_COMPARE_WITH_TIMEOUT(connection.peer->bytesAvailable(), expected.size(), kTimeoutMs);
    QCOMPARE(connection.peer->readAll(), expected);
  }

  void pollSocket_multipleReadableSockets_countsSockets()
  {
    LoopbackConnection first;
    LoopbackConnection second;
    QVERIFY2(first.open(), qPrintable(first.error));
    QVERIFY2(second.open(), qPrintable(second.error));
    QCOMPARE(first.peer->write("a", 1), qint64(1));
    QCOMPARE(second.peer->write("b", 1), qint64(1));
    QTRY_COMPARE_WITH_TIMEOUT(first.peer->bytesToWrite(), 0, kTimeoutMs);
    QTRY_COMPARE_WITH_TIMEOUT(second.peer->bytesToWrite(), 0, kTimeoutMs);
    std::array<PollEntry, 2> entries{
        {{first.socket.get(), PollEventMask::In | PollEventMask::Out, 0},
         {second.socket.get(), PollEventMask::In | PollEventMask::Out, 0}}
    };

    std::array<unsigned short, 2> observedEvents{};
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < kTimeoutMs) {
      QCOMPARE(ARCH->pollSocket(entries.data(), static_cast<int>(entries.size()), 0.0), 2);
      for (size_t i = 0; i < entries.size(); ++i) {
        QCOMPARE(entries[i].m_revents & PollEventMask::Out, PollEventMask::Out);
        observedEvents[i] |= entries[i].m_revents;
      }
      if ((observedEvents[0] & PollEventMask::In) != 0 && (observedEvents[1] & PollEventMask::In) != 0) {
        break;
      }
      QTest::qWait(1);
    }
    for (const auto events : observedEvents) {
      QCOMPARE(events, PollEventMask::In | PollEventMask::Out);
    }
    QCOMPARE(readAll(first.socket.get(), 1), QByteArray("a"));
    QCOMPARE(readAll(second.socket.get(), 1), QByteArray("b"));
  }

  void copySocket_keepsConnectionAliveAfterOriginalClose()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));
    Socket copy(ARCH->copySocket(connection.socket.get()));
    connection.socket.reset();
    const QByteArray expected("The copied socket still owns the connection.");

    QCOMPARE(writeAll(copy.get(), expected), expected.size());
    QTRY_COMPARE_WITH_TIMEOUT(connection.peer->bytesAvailable(), expected.size(), kTimeoutMs);
    QCOMPARE(connection.peer->readAll(), expected);
  }

  void setNoDelayOnSocket_returnsPreviousValue()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));

    ARCH->setNoDelayOnSocket(connection.socket.get(), true);
    QVERIFY(ARCH->setNoDelayOnSocket(connection.socket.get(), false));
    QVERIFY(!ARCH->setNoDelayOnSocket(connection.socket.get(), true));
    ARCH->setKeepAliveOnSocket(connection.socket.get(), true);
    ARCH->setKeepAliveOnSocket(connection.socket.get(), false);
  }

  void setKeepAliveOnSocket_nullSocket_throwsSupportError()
  {
    QVERIFY_THROWS_EXCEPTION(ArchNetworkSupportException, ARCH->setKeepAliveOnSocket(nullptr, true));
  }

  void unblockPollSocket_wakesPollingThread()
  {
    LoopbackConnection connection;
    QVERIFY2(connection.open(), qPrintable(connection.error));
    PollWorker worker(connection.socket.get());
    QVERIFY(worker.thread() != nullptr);
    const bool ready = worker.waitReady();
    const bool finishedEarly = ready && worker.waitFinished(std::chrono::milliseconds(30));
    bool unblocked = false;
    if (ready && !finishedEarly) {
      QElapsedTimer timer;
      timer.start();
      // Readiness precedes the blocking call; repeat signals to cover that scheduling gap.
      while (timer.elapsed() < kTimeoutMs) {
        ARCH->unblockPollSocket(worker.thread());
        if (worker.waitFinished(std::chrono::milliseconds(20))) {
          unblocked = true;
          break;
        }
      }
    }
    const bool joined = worker.join();

    QVERIFY(joined);
    QVERIFY(ready);
    QVERIFY(!finishedEarly);
    QVERIFY(unblocked);
    QVERIFY(!worker.error);
    QCOMPARE(worker.result, 0);
    QCOMPARE(worker.events, 0);
  }

private:
  Arch m_arch;
};

QTEST_GUILESS_MAIN(ArchNetworkTests)
#include "ArchNetworkTests.moc"
