/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileDragTests.h"

#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragGeometry.h"
#include "deskflow/FileDragPolicy.h"
#include "deskflow/FileDragProtocol.h"
#include "deskflow/FileDragSender.h"
#include "deskflow/FileDragStaging.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"

#include <QTemporaryDir>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

namespace fs = std::filesystem;
using namespace deskflow::filedrag;

namespace {

//! In-memory stream: writes append, reads consume
class MemoryStream : public deskflow::IStream
{
public:
  void close() override
  {
  }
  uint32_t read(void *buffer, uint32_t n) override
  {
    const auto available = static_cast<uint32_t>(m_data.size() - m_pos);
    n = std::min(n, available);
    if (buffer != nullptr && n > 0) {
      memcpy(buffer, m_data.data() + m_pos, n);
    }
    m_pos += n;
    return n;
  }
  void write(const void *buffer, uint32_t n) override
  {
    m_data.append(static_cast<const char *>(buffer), n);
  }
  void flush() override
  {
  }
  void shutdownInput() override
  {
  }
  void shutdownOutput() override
  {
  }
  void *getEventTarget() const override
  {
    return const_cast<MemoryStream *>(this);
  }
  bool isReady() const override
  {
    return m_pos < m_data.size();
  }
  uint32_t getSize() const override
  {
    return static_cast<uint32_t>(m_data.size() - m_pos);
  }

  const std::string &bytes() const
  {
    return m_data;
  }
  size_t remaining() const
  {
    return m_data.size() - m_pos;
  }

private:
  std::string m_data;
  size_t m_pos = 0;
};

fs::path u8path(const char *s)
{
  return fs::path(std::u8string(reinterpret_cast<const char8_t *>(s)));
}

std::string readFile(const fs::path &path)
{
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void writeFile(const fs::path &path, const std::string &data)
{
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string randomBytes(size_t n, uint32_t seed)
{
  std::mt19937 gen(seed);
  std::string s(n, '\0');
  for (auto &c : s) {
    c = static_cast<char>(gen() & 0xff);
  }
  return s;
}

Message roundTrip(const Message &m)
{
  MemoryStream stream;
  writeMessage(&stream, m);
  uint8_t code[4];
  stream.read(code, 4);
  Message out;
  std::string error;
  if (!readMessage(&stream, code, out, &error)) {
    qWarning("readMessage failed: %s", error.c_str());
  }
  return out;
}

//! Feeds a sender's messages into a staging, acking each chunk like a client would
struct Loopback
{
  FileDragStaging *staging = nullptr;
  FileDragSender *sender = nullptr;
  std::vector<Message> pending;
  bool failed = false;

  void deliver()
  {
    while (!pending.empty()) {
      auto m = std::move(pending.front());
      pending.erase(pending.begin());
      // wire round trip on the way, like the real connection
      m = roundTrip(m);
      bool ok = true;
      switch (m.type) {
      case Message::Type::FileStart:
        ok = staging->fileStart(m.index, m.size);
        break;
      case Message::Type::Chunk:
        ok = staging->write(m.index, m.data);
        if (ok) {
          sender->onAck(m.seq);
        }
        break;
      case Message::Type::FileEnd:
        ok = staging->fileEnd(m.index);
        break;
      default:
        break;
      }
      failed = failed || !ok;
    }
  }
};

} // namespace

void FileDragTests::initTestCase()
{
  // nothing to set up
}

//
// item list codec
//

void FileDragTests::itemsRoundTrip()
{
  const ItemList items = {
      {"Photos", 0, true},
      {"Photos/2026", 0, true},
      {"Photos/2026/beach.jpg", 5ull * 1024 * 1024 * 1024, false}, // over 4 GB
      {"Photos/empty.txt", 0, false},
      {"\xc3\xa9t\xc3\xa9 \xe6\x97\xa5\xe6\x9c\xac.txt", 42, false}, // été 日本.txt
  };
  ItemList decoded;
  std::string error;
  QVERIFY2(decodeItems(encodeItems(items), decoded, &error), error.c_str());
  QCOMPARE(decoded.size(), items.size());
  QVERIFY(decoded == items);
}

void FileDragTests::itemsRejectUnsafe_data()
{
  QTest::addColumn<QString>("path");
  QTest::newRow("parent") << "../evil.txt";
  QTest::newRow("inner parent") << "a/../../evil.txt";
  QTest::newRow("backslash parent") << "a\\..\\..\\evil.txt";
  QTest::newRow("dot") << "./a.txt";
  QTest::newRow("absolute") << "/etc/passwd";
  QTest::newRow("backslash absolute") << "\\Windows\\evil.dll";
  QTest::newRow("drive") << "C:/Windows/evil.dll";
  QTest::newRow("drive relative") << "C:evil.dll";
  QTest::newRow("unc") << "//server/share/evil";
  QTest::newRow("empty component") << "a//b.txt";
  QTest::newRow("trailing slash") << "a/";
  QTest::newRow("empty") << "";
  QTest::newRow("control") << "a\nb.txt";
}

void FileDragTests::itemsRejectUnsafe()
{
  QFETCH(QString, path);
  const ItemList items = {{path.toStdString(), 1, false}};
  ItemList decoded;
  QVERIFY(!decodeItems(encodeItems(items), decoded));
  QVERIFY(decoded.empty());
  QVERIFY(!validateItems(items));
}

void FileDragTests::itemsRejectBadStructure()
{
  ItemList decoded;
  // child before its folder
  QVERIFY(!decodeItems(encodeItems({{"a/b.txt", 1, false}, {"a", 0, true}}), decoded));
  // child of a folder that is not listed
  QVERIFY(!decodeItems(encodeItems({{"a/b.txt", 1, false}}), decoded));
  // duplicate
  QVERIFY(!decodeItems(encodeItems({{"a.txt", 1, false}, {"a.txt", 2, false}}), decoded));
  // folder with a size (never encoded, but a list built in memory is checked)
  QVERIFY(!validateItems({{"a", 5, true}}));
  // empty list
  QVERIFY(!decodeItems(encodeItems({}), decoded));
  // too many items
  ItemList many;
  for (uint32_t i = 0; i <= kMaxItems; ++i) {
    many.push_back({"f" + std::to_string(i), 0, false});
  }
  QVERIFY(!validateItems(many));
  // total size overflow
  QVERIFY(!validateItems({{"a", UINT64_MAX, false}, {"b", 2, false}}));
}

void FileDragTests::itemsRejectTruncated()
{
  const auto encoded = encodeItems({{"Folder", 0, true}, {"Folder/file.bin", 1234, false}});
  ItemList decoded;
  for (size_t n = 0; n < encoded.size(); ++n) {
    QVERIFY2(!decodeItems(encoded.substr(0, n), decoded), std::to_string(n).c_str());
  }
  QVERIFY(!decodeItems(encoded + "x", decoded));
  // a count much larger than the data
  ByteWriter w;
  w.u32(kMaxItems);
  QVERIFY(!decodeItems(w.str(), decoded));
  // unknown flags
  ByteWriter f;
  f.u32(1);
  f.u8(0x80);
  f.u64(0);
  f.bytes16("a");
  QVERIFY(!decodeItems(f.str(), decoded));
}

void FileDragTests::safePaths_data()
{
  QTest::addColumn<QString>("path");
  QTest::newRow("file") << "a.txt";
  QTest::newRow("nested") << "Folder/Sub/file.txt";
  QTest::newRow("dots in name") << "a..b/c...d";
  QTest::newRow("hidden") << ".config/.hidden";
  QTest::newRow("spaces") << "My Files/report final.docx";
}

void FileDragTests::safePaths()
{
  QFETCH(QString, path);
  QVERIFY(isSafeRelativePath(path.toStdString()));
}

void FileDragTests::sanitizeComponent_data()
{
  QTest::addColumn<QString>("input");
  QTest::addColumn<QString>("expected");
  QTest::newRow("plain") << "report.pdf" << "report.pdf";
  QTest::newRow("colon") << "12:30 notes.txt" << "12_30 notes.txt";
  QTest::newRow("windows chars") << "a<b>c\"d|e?f*g" << "a_b_c_d_e_f_g";
  QTest::newRow("backslash") << "a\\b" << "a_b";
  QTest::newRow("trailing dot") << "name." << "name";
  QTest::newRow("trailing space") << "name  " << "name";
  QTest::newRow("only dots") << "..." << "_";
  QTest::newRow("device") << "CON" << "_CON";
  QTest::newRow("device ext") << "nul.txt" << "_nul.txt";
  QTest::newRow("com port") << "com1.log" << "_com1.log";
  QTest::newRow("not a device") << "console.txt" << "console.txt";
}

void FileDragTests::sanitizeComponent()
{
  QFETCH(QString, input);
  QFETCH(QString, expected);
  QCOMPARE(QString::fromStdString(deskflow::filedrag::sanitizeComponent(input.toStdString())), expected);
}

void FileDragTests::uniqueNameNeverReturnsTaken()
{
  std::set<std::string> taken = {"a.txt", "a (2).txt", "dir.v1", ".gitignore"};
  auto isTaken = [&](const std::string &n) { return taken.contains(n); };
  QCOMPARE(QString::fromStdString(uniqueName("b.txt", false, isTaken)), QString("b.txt"));
  QCOMPARE(QString::fromStdString(uniqueName("a.txt", false, isTaken)), QString("a (3).txt"));
  QCOMPARE(QString::fromStdString(uniqueName("dir.v1", true, isTaken)), QString("dir.v1 (2)"));
  QCOMPARE(QString::fromStdString(uniqueName(".gitignore", false, isTaken)), QString(".gitignore (2)"));
  // keep asking: every answer is new
  for (int i = 0; i < 50; ++i) {
    const auto name = uniqueName("a.txt", false, isTaken);
    QVERIFY(!taken.contains(name));
    taken.insert(name);
  }
}

//
// wire messages
//

void FileDragTests::chunkWireFormat()
{
  MemoryStream stream;
  Message m;
  m.type = Message::Type::Chunk;
  m.dragId = 7;
  m.index = 2;
  m.seq = 5;
  m.data = "abc";
  writeMessage(&stream, m);
  const std::string expected(
      "DFDK\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x05\x00\x00\x00\x03"
      "abc",
      23
  );
  QCOMPARE(QByteArray::fromStdString(stream.bytes()), QByteArray::fromStdString(expected));

  MemoryStream start;
  Message s;
  s.type = Message::Type::FileStart;
  s.dragId = 1;
  s.index = 3;
  s.size = 0x100000002ull;
  writeMessage(&start, s);
  const std::string expectedStart("DFDS\x00\x00\x00\x01\x00\x00\x00\x03\x00\x00\x00\x01\x00\x00\x00\x02", 20);
  QCOMPARE(QByteArray::fromStdString(start.bytes()), QByteArray::fromStdString(expectedStart));
}

void FileDragTests::messagesRoundTrip_data()
{
  QTest::addColumn<int>("type");
  for (int t = 1; t <= static_cast<int>(Message::Type::HandOff); ++t) {
    QTest::newRow(messageTypeName(static_cast<Message::Type>(t))) << t;
  }
}

void FileDragTests::messagesRoundTrip()
{
  QFETCH(int, type);
  Message m;
  m.type = static_cast<Message::Type>(type);
  m.dragId = 0xdeadbeef;
  switch (m.type) {
    using enum Message::Type;
  case Caps:
    m.dragId = 0;
    m.flags = kCapSource | kCapDestination;
    break;
  case Icon:
    m.icon.width = 3;
    m.icon.height = 2;
    m.icon.rgba = randomBytes(3 * 2 * 4, 1);
    break;
  case Begin:
    m.items = {{"Docs", 0, true}, {"Docs/a.txt", 10, false}};
    break;
  case FileStart:
    m.index = 1;
    m.size = 6ull * 1024 * 1024 * 1024;
    break;
  case Chunk:
    m.index = 1;
    m.seq = 99;
    m.data = randomBytes(kChunkSize, 2);
    break;
  case FileEnd:
    m.index = 1;
    break;
  case Ack:
    m.seq = 1234;
    break;
  case Cancel:
    m.reason = CancelReason::CursorReturned;
    break;
  case Dropped:
  case Finished:
  case HandOff:
    break;
  case Watch:
    m.dragId = 0;
    m.flags = static_cast<uint32_t>(DirectionMask::RightMask) | static_cast<uint32_t>(DirectionMask::TopMask);
    break;
  case Offer:
    m.dragId = 0;
    m.flags = 1;
    break;
  }
  const auto out = roundTrip(m);
  QVERIFY(out == m);
  // the core and the helper carry every type too
  Message local;
  QVERIFY(decodeMessage(encodeMessage(m), local));
  QVERIFY(local == m);
}

void FileDragTests::readRejectsOversizedChunk()
{
  MemoryStream stream;
  const std::string big(kMaxChunkSize + 1, 'x');
  ProtocolUtil::writef(&stream, kMsgDFileDragChunk, 1, 0, 1, &big);
  uint8_t code[4];
  stream.read(code, 4);
  Message m;
  QVERIFY(!readMessage(&stream, code, m));
}

void FileDragTests::readRejectsBadIcon()
{
  MemoryStream stream;
  const std::string pixels(10, 'x'); // not 2*2*4
  ProtocolUtil::writef(&stream, kMsgDFileDragIcon, 1, 2, 2, &pixels);
  uint8_t code[4];
  stream.read(code, 4);
  Message m;
  QVERIFY(!readMessage(&stream, code, m));

  MemoryStream huge;
  const std::string none;
  ProtocolUtil::writef(&huge, kMsgDFileDragIcon, 1, 1000, 1, &none);
  huge.read(code, 4);
  QVERIFY(!readMessage(&huge, code, m));
}

void FileDragTests::readRejectsTraversalInBegin()
{
  MemoryStream stream;
  const auto items = encodeItems({{"../../evil.exe", 4, false}});
  ProtocolUtil::writef(&stream, kMsgDFileDragBegin, 9, &items);
  uint8_t code[4];
  stream.read(code, 4);
  Message m;
  std::string error;
  QVERIFY(!readMessage(&stream, code, m, &error));
  // the proxy can still name the drag it refuses
  QCOMPARE(m.type, Message::Type::Begin);
  QCOMPARE(m.dragId, 9u);
  QVERIFY(m.items.empty());
}

void FileDragTests::fileDragCodesAreNew()
{
  for (const char *code :
       {kMsgDFileDragCaps, kMsgDFileDragIcon, kMsgDFileDragBegin, kMsgDFileDragStart, kMsgDFileDragChunk,
        kMsgDFileDragEnd, kMsgDFileDragAck, kMsgDFileDragCancel, kMsgDFileDragDropped, kMsgDFileDragFinished,
        kMsgDFileDragWatch, kMsgDFileDragOffer, kMsgDFileDragHandOff}) {
    QVERIFY(isFileDragCode(reinterpret_cast<const uint8_t *>(code)));
    // never the deprecated drag codes or anything an older peer knows
    for (const char *old :
         {kMsgDFileTransfer, kMsgDDragInfo, kMsgDMouseMove, kMsgDClipboard, kMsgCNoop, kMsgCKeepAlive, kMsgDInfo,
          kMsgDSetOptions, kMsgDLanguageSynchronisation, kMsgDSecureInputNotification}) {
      QVERIFY(memcmp(code, old, 4) != 0);
    }
  }
  QVERIFY(!isFileDragCode(reinterpret_cast<const uint8_t *>(kMsgDFileTransfer)));
  QVERIFY(!isFileDragCode(reinterpret_cast<const uint8_t *>(kMsgDDragInfo)));
  // the client to server direction fits protocol 1.9: no new version, so a stock 1.27.0
  // peer (1.8) and the first file drag build (1.9) negotiate exactly as before
  QCOMPARE(kProtocolMinorVersion, static_cast<int16_t>(9));
  // the three new codes are distinct from each other and from the first ten
  const char *added[] = {kMsgDFileDragWatch, kMsgDFileDragOffer, kMsgDFileDragHandOff};
  for (const char *a : added) {
    for (const char *b :
         {kMsgDFileDragCaps, kMsgDFileDragIcon, kMsgDFileDragBegin, kMsgDFileDragStart, kMsgDFileDragChunk,
          kMsgDFileDragEnd, kMsgDFileDragAck, kMsgDFileDragCancel, kMsgDFileDragDropped, kMsgDFileDragFinished}) {
      QVERIFY(memcmp(a, b, 4) != 0);
    }
  }
  QVERIFY(
      memcmp(added[0], added[1], 4) != 0 && memcmp(added[1], added[2], 4) != 0 && memcmp(added[0], added[2], 4) != 0
  );
}

void FileDragTests::readRejectsBadWatchAndOffer()
{
  uint8_t code[4];
  Message m;

  MemoryStream watch;
  ProtocolUtil::writef(&watch, kMsgDFileDragWatch, 1u); // bit 0 is no side (NoDirection)
  watch.read(code, 4);
  QVERIFY(!readMessage(&watch, code, m));

  MemoryStream offer;
  ProtocolUtil::writef(&offer, kMsgDFileDragOffer, 2u);
  offer.read(code, 4);
  QVERIFY(!readMessage(&offer, code, m));

  MemoryStream good;
  ProtocolUtil::writef(&good, kMsgDFileDragWatch, static_cast<uint32_t>(DirectionMask::RightMask));
  good.read(code, 4);
  QVERIFY(readMessage(&good, code, m));
  QCOMPARE(m.type, Message::Type::Watch);
  QCOMPARE(m.flags, static_cast<uint32_t>(DirectionMask::RightMask));
}

void FileDragTests::reverseNeedsBothPeersNew_data()
{
  QTest::addColumn<uint>("server");
  QTest::addColumn<uint>("client");
  QTest::addColumn<bool>("available");

  const uint both = kCapSource | kCapDestination | kCapReverse;
  // what the first file drag build (1.27.0.10000) sent: the server only kCapSource, the
  // client only kCapDestination; a stock 1.27.0 peer (protocol 1.8) sends nothing (0)
  QTest::newRow("both new") << both << both << true;
  QTest::newRow("first build server") << uint(kCapSource) << both << false;
  QTest::newRow("first build client") << both << uint(kCapDestination) << false;
  QTest::newRow("stock server") << 0u << both << false;
  QTest::newRow("stock client") << both << 0u << false;
  QTest::newRow("source and destination but not the new messages")
      << uint(kCapSource | kCapDestination) << uint(kCapSource | kCapDestination) << false;
  QTest::newRow("server cannot show drags") << uint(kCapSource | kCapReverse) << both << false;
  QTest::newRow("client cannot send drags") << both << uint(kCapDestination | kCapReverse) << false;
}

void FileDragTests::reverseNeedsBothPeersNew()
{
  QFETCH(uint, server);
  QFETCH(uint, client);
  QFETCH(bool, available);
  QCOMPARE(reverseDragAvailable(server, client), available);
}

void FileDragTests::localTransportRoundTrip()
{
  Message m;
  m.type = Message::Type::Begin;
  m.dragId = 3;
  m.items = {{"x", 0, true}, {"x/y.bin", 77, false}};
  m.icon.width = 1;
  m.icon.height = 1;
  m.icon.rgba = "\x01\x02\x03\x04";
  Message out;
  QVERIFY(decodeMessage(encodeMessage(m), out));
  QVERIFY(out == m);

  auto bad = encodeMessage(m);
  bad.pop_back();
  QVERIFY(!decodeMessage(bad, out));
}

//
// staging
//

void FileDragTests::stagingWritesAndReads()
{
  QTemporaryDir temp;
  const fs::path root = temp.path().toStdWString();
  FileDragStaging staging(root);
  const auto data = randomBytes(100000, 3);
  const ItemList items = {{"Folder", 0, true}, {"Folder/data.bin", data.size(), false}, {"empty.txt", 0, false}};
  CancelReason reason;
  QVERIFY(staging.begin(5, items, reason));
  QVERIFY(fs::is_directory(staging.stagingDir()));
  QCOMPARE(staging.stagingDir().parent_path(), root);
  // files exist (empty) before any data arrives
  QVERIFY(fs::exists(staging.pathOf(1)));
  QVERIFY(fs::exists(staging.pathOf(2)));

  QVERIFY(staging.fileStart(1, data.size()));
  QVERIFY(staging.write(1, data.substr(0, 60000)));
  uint64_t available = 0;
  QCOMPARE(staging.waitForData(1, 0, available, std::chrono::milliseconds(0)), FileDragStaging::WaitResult::Data);
  QCOMPARE(available, 60000ull);
  QCOMPARE(
      staging.waitForData(1, 60000, available, std::chrono::milliseconds(10)), FileDragStaging::WaitResult::Timeout
  );
  QVERIFY(staging.write(1, data.substr(60000)));
  QVERIFY(staging.fileEnd(1));
  QCOMPARE(
      staging.waitForData(1, data.size(), available, std::chrono::milliseconds(0)), FileDragStaging::WaitResult::Eof
  );
  QVERIFY(!staging.isComplete());
  QVERIFY(staging.fileStart(2, 0));
  QVERIFY(staging.fileEnd(2));
  QVERIFY(staging.isComplete());
  QCOMPARE(readFile(staging.pathOf(1)), data);
  QCOMPARE(staging.receivedBytes(), static_cast<uint64_t>(data.size()));

  const auto dir = staging.stagingDir();
  QVERIFY(staging.removeStaged());
  QVERIFY(!fs::exists(dir));
}

void FileDragTests::stagingRejectsTraversal()
{
  QTemporaryDir temp;
  const fs::path root = fs::path(temp.path().toStdWString()) / "staging";
  for (const char *path : {"../outside.txt", "/abs.txt", "C:/x.txt", "a/../../b.txt", "..\\up.txt"}) {
    FileDragStaging staging(root);
    CancelReason reason = CancelReason::Unknown;
    QVERIFY2(!staging.begin(1, {{path, 3, false}}, reason), path);
    QCOMPARE(reason, CancelReason::Invalid);
  }
  // nothing was created next to or above the staging root
  QVERIFY(!fs::exists(fs::path(temp.path().toStdWString()) / "outside.txt"));
  QVERIFY(!fs::exists(fs::path(temp.path().toStdWString()) / "b.txt"));
  QVERIFY(!fs::exists(fs::path(temp.path().toStdWString()) / "up.txt"));
}

void FileDragTests::stagingSanitizesAndDeduplicates()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  CancelReason reason;
  // "ab:c.txt" and "ab?c.txt" both sanitize to "ab_c.txt"; "AB_C.TXT" collides by case
  const ItemList items = {{"ab:c.txt", 1, false}, {"ab?c.txt", 1, false}, {"AB_C.TXT", 1, false}, {"CON", 0, true}};
  QVERIFY(staging.begin(1, items, reason));
  const auto &entries = staging.entries();
  QCOMPARE(QString::fromStdString(entries[0].localPath), QString("ab_c.txt"));
  QCOMPARE(QString::fromStdString(entries[1].localPath), QString("ab_c (2).txt"));
  QCOMPARE(QString::fromStdString(entries[2].localPath), QString("AB_C (3).TXT"));
  QCOMPARE(QString::fromStdString(entries[3].localPath), QString("_CON"));
  for (const auto &entry : entries) {
    QVERIFY(fs::exists(staging.stagingDir() / u8path(entry.localPath.c_str())));
  }
}

void FileDragTests::stagingChecksFreeSpace()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  staging.setFreeSpaceOverride(kStagingSpareBytes + 100);
  CancelReason reason = CancelReason::Unknown;
  QVERIFY(!staging.begin(1, {{"big.iso", 101, false}}, reason));
  QCOMPARE(reason, CancelReason::NoSpace);
  // nothing staged
  QVERIFY(fs::is_empty(fs::path(temp.path().toStdWString())));

  FileDragStaging fits(fs::path(temp.path().toStdWString()));
  fits.setFreeSpaceOverride(kStagingSpareBytes + 100);
  QVERIFY(fits.begin(2, {{"small.iso", 100, false}}, reason));
}

void FileDragTests::stagingRejectsSizeMismatch()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  CancelReason reason;
  QVERIFY(staging.begin(1, {{"a.bin", 4, false}, {"b.bin", 4, false}}, reason));
  QVERIFY(!staging.fileStart(0, 5)); // announced size differs
  QVERIFY(staging.fileStart(0, 4));
  QVERIFY(!staging.fileStart(1, 4));   // one file at a time
  QVERIFY(!staging.write(1, "abcd"));  // not the open file
  QVERIFY(!staging.write(0, "abcde")); // past the end
  QVERIFY(staging.write(0, "ab"));
  QVERIFY(!staging.fileEnd(0)); // short
  QVERIFY(staging.write(0, "cd"));
  QVERIFY(staging.fileEnd(0));
}

void FileDragTests::stagingCancelRemovesEverything()
{
  QTemporaryDir temp;
  const fs::path root = temp.path().toStdWString();
  FileDragStaging staging(root);
  CancelReason reason;
  QVERIFY(staging.begin(1, {{"d", 0, true}, {"d/part.bin", 1000, false}}, reason));
  QVERIFY(staging.fileStart(1, 1000));
  QVERIFY(staging.write(1, std::string(500, 'x')));
  const auto dir = staging.stagingDir();
  staging.cancel();
  QVERIFY(staging.isCancelled());
  QVERIFY(!fs::exists(dir));
  QVERIFY(!staging.write(1, std::string(10, 'x')));
  uint64_t available = 0;
  QCOMPARE(staging.waitForData(1, 0, available, std::chrono::milliseconds(0)), FileDragStaging::WaitResult::Failed);
  staging.cancel(); // twice is fine
  QVERIFY(fs::is_empty(root));
}

void FileDragTests::stagingDestructorRemovesPartialFiles()
{
  QTemporaryDir temp;
  const fs::path root = temp.path().toStdWString();
  fs::path dir;
  {
    FileDragStaging staging(root);
    CancelReason reason;
    QVERIFY(staging.begin(1, {{"part.bin", 1000, false}}, reason));
    QVERIFY(staging.fileStart(0, 1000));
    QVERIFY(staging.write(0, std::string(10, 'x')));
    dir = staging.stagingDir();
    QVERIFY(fs::exists(dir));
  }
  QVERIFY(!fs::exists(dir));
}

void FileDragTests::stagingReaderWaitsForData()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  const auto data = randomBytes(5 * kChunkSize + 17, 4);
  CancelReason reason;
  QVERIFY(staging.begin(1, {{"f.bin", data.size(), false}}, reason));

  // a reader like the Windows IStream: waits, then reads what is there with its own handle
  std::string got;
  std::atomic<bool> readerOk{true};
  std::thread reader([&] {
    std::ifstream in;
    uint64_t offset = 0;
    while (true) {
      uint64_t available = 0;
      const auto r = staging.waitForData(0, offset, available, std::chrono::seconds(5));
      if (r == FileDragStaging::WaitResult::Eof) {
        break;
      }
      if (r != FileDragStaging::WaitResult::Data) {
        readerOk = false;
        break;
      }
      if (!in.is_open()) {
        in.open(staging.pathOf(0), std::ios::binary);
      }
      std::string buffer(available - offset, '\0');
      in.seekg(static_cast<std::streamoff>(offset));
      in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      got.append(buffer.data(), static_cast<size_t>(in.gcount()));
      offset += static_cast<uint64_t>(in.gcount());
      in.clear();
    }
  });

  QVERIFY(staging.fileStart(0, data.size()));
  for (size_t pos = 0; pos < data.size(); pos += kChunkSize) {
    QVERIFY(staging.write(0, data.substr(pos, kChunkSize)));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  QVERIFY(staging.fileEnd(0));
  reader.join();
  QVERIFY(readerOk);
  QCOMPARE(got.size(), data.size());
  QVERIFY(got == data);

  // a cancel wakes a waiting reader at once
  FileDragStaging second(fs::path(temp.path().toStdWString()));
  QVERIFY(second.begin(2, {{"g.bin", 10, false}}, reason));
  std::atomic<int> result{-1};
  std::thread waiter([&] {
    uint64_t available = 0;
    result = static_cast<int>(second.waitForData(0, 0, available, std::chrono::seconds(10)));
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  second.cancel();
  waiter.join();
  QCOMPARE(result.load(), static_cast<int>(FileDragStaging::WaitResult::Failed));
}

void FileDragTests::stagingSweepsDeadFolders()
{
  QTemporaryDir temp;
  const fs::path root = temp.path().toStdWString();
  // pid 0 is never a live user process: its folder is stale
  fs::create_directories(root / "drag-0-1-1" / "sub");
  writeFile(root / "drag-0-1-1" / "sub" / "x.bin", "x");
  // our own folder survives, so does anything not named drag-*
  FileDragStaging live(root);
  CancelReason reason;
  QVERIFY(live.begin(1, {{"a", 0, true}}, reason));
  fs::create_directories(root / "keep-me");

  FileDragStaging::sweepStale(root);
  QVERIFY(!fs::exists(root / "drag-0-1-1"));
  QVERIFY(fs::exists(live.stagingDir()));
  QVERIFY(fs::exists(root / "keep-me"));
}

//
// moving
//

void FileDragTests::moveIntoPlacesFilesAndFolders()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  const fs::path target = base / "target";
  fs::create_directories(target);
  FileDragStaging staging(base / "staging");
  CancelReason reason;
  const ItemList items = {
      {"Album", 0, true}, {"Album/Sub", 0, true}, {"Album/Sub/p.jpg", 3, false}, {"note.txt", 2, false}
  };
  QVERIFY(staging.begin(1, items, reason));
  QVERIFY(staging.fileStart(2, 3) && staging.write(2, "jpg") && staging.fileEnd(2));
  // not complete yet: refuses
  std::vector<fs::path> placed;
  QVERIFY(!staging.moveInto(target, &placed));
  QVERIFY(staging.fileStart(3, 2) && staging.write(3, "hi") && staging.fileEnd(3));
  QVERIFY(staging.moveInto(target, &placed));
  QCOMPARE(placed.size(), 2u);
  QCOMPARE(readFile(target / "Album" / "Sub" / "p.jpg"), std::string("jpg"));
  QCOMPARE(readFile(target / "note.txt"), std::string("hi"));
}

void FileDragTests::moveIntoNeverOverwrites()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  const fs::path target = base / "target";
  writeFile(target / "note.txt", "original");
  writeFile(target / "note (2).txt", "second");
  writeFile(target / "Album" / "old.txt", "old");
  FileDragStaging staging(base / "staging");
  CancelReason reason;
  QVERIFY(staging.begin(1, {{"Album", 0, true}, {"note.txt", 3, false}}, reason));
  QVERIFY(staging.fileStart(1, 3) && staging.write(1, "new") && staging.fileEnd(1));
  std::vector<fs::path> placed;
  QVERIFY(staging.moveInto(target, &placed));
  // existing files untouched, new ones beside them
  QCOMPARE(readFile(target / "note.txt"), std::string("original"));
  QCOMPARE(readFile(target / "note (2).txt"), std::string("second"));
  QCOMPARE(readFile(target / "note (3).txt"), std::string("new"));
  QCOMPARE(readFile(target / "Album" / "old.txt"), std::string("old"));
  QVERIFY(fs::is_directory(target / "Album (2)"));
}

void FileDragTests::moveNoReplacePicksFreeName()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  writeFile(base / "src" / "x.txt", "mine");
  writeFile(base / "dst" / "x.txt", "theirs");
  fs::path placed;
  QVERIFY(FileDragStaging::moveNoReplace(base / "src" / "x.txt", base / "dst", "x.txt", false, placed));
  QCOMPARE(placed.filename().string(), std::string("x (2).txt"));
  QCOMPARE(readFile(base / "dst" / "x.txt"), std::string("theirs"));
  QCOMPARE(readFile(placed), std::string("mine"));
  QVERIFY(!fs::exists(base / "src" / "x.txt"));
}

//
// sending
//

void FileDragTests::scanBuildsParentsFirst()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  writeFile(base / "Project" / "a.txt", "a");
  writeFile(base / "Project" / "src" / "deep" / "b.txt", "bb");
  fs::create_directories(base / "Project" / "emptydir");
  writeFile(base / "single.bin", "123");
  writeFile(base / "other" / "single.bin", "4567"); // same name as a root: gets a new name

  ItemList items;
  std::vector<fs::path> sources;
  std::string error;
  QVERIFY2(
      scanItems({base / "Project", base / "single.bin", base / "other" / "single.bin"}, items, sources, &error),
      error.c_str()
  );
  QVERIFY(validateItems(items));
  QCOMPARE(items.size(), sources.size());
  QCOMPARE(QString::fromStdString(items.front().path), QString("Project"));
  QVERIFY(items.front().isDirectory);

  std::set<std::string> paths;
  for (const auto &item : items) {
    paths.insert(item.path);
  }
  QVERIFY(paths.contains("Project/src/deep/b.txt"));
  QVERIFY(paths.contains("Project/emptydir"));
  QVERIFY(paths.contains("single.bin"));
  QVERIFY(paths.contains("single (2).bin"));
  for (size_t i = 0; i < items.size(); ++i) {
    if (items[i].path == "Project/src/deep/b.txt") {
      QCOMPARE(items[i].size, 2ull);
      QCOMPARE(readFile(sources[i]), std::string("bb"));
    }
  }
}

void FileDragTests::senderToStagingIsByteIdentical()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  const auto big = randomBytes(3 * kChunkSize + 1234, 5);
  writeFile(base / "src" / "Folder" / "big.bin", big);
  writeFile(base / "src" / "Folder" / "Nested" / "small.txt", "small");
  writeFile(base / "src" / "Folder" / "zero.dat", "");

  ItemList items;
  std::vector<fs::path> sources;
  QVERIFY(scanItems({base / "src" / "Folder"}, items, sources));

  FileDragStaging staging(base / "staging");
  CancelReason reason;
  QVERIFY(staging.begin(
      1,
      roundTrip([&] {
        Message b;
        b.type = Message::Type::Begin;
        b.dragId = 1;
        b.items = items;
        return b;
      }())
          .items,
      reason
  ));

  Loopback loop;
  FileDragSender sender(1, items, sources, [&](Message &&m) { loop.pending.push_back(std::move(m)); });
  loop.staging = &staging;
  loop.sender = &sender;
  sender.pump();
  for (int i = 0; i < 1000 && !sender.isFinished(); ++i) {
    loop.deliver();
  }
  QVERIFY(!loop.failed);
  QVERIFY(sender.isFinished());
  QVERIFY(staging.isComplete());
  QCOMPARE(sender.bytesSent(), static_cast<uint64_t>(big.size() + 5));

  const fs::path target = base / "target";
  fs::create_directories(target);
  QVERIFY(staging.moveInto(target));
  QVERIFY(readFile(target / "Folder" / "big.bin") == big);
  QCOMPARE(readFile(target / "Folder" / "Nested" / "small.txt"), std::string("small"));
  QVERIFY(fs::exists(target / "Folder" / "zero.dat"));
  QCOMPARE(fs::file_size(target / "Folder" / "zero.dat"), 0ull);
}

void FileDragTests::senderRespectsWindow()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  writeFile(base / "f.bin", randomBytes(40 * kChunkSize, 6));
  ItemList items;
  std::vector<fs::path> sources;
  QVERIFY(scanItems({base / "f.bin"}, items, sources));

  uint32_t chunks = 0;
  uint32_t lastSeq = 0;
  FileDragSender sender(1, items, sources, [&](Message &&m) {
    if (m.type == Message::Type::Chunk) {
      ++chunks;
      lastSeq = m.seq;
    }
  });
  sender.pump();
  // nothing acked: only the window went out
  QCOMPARE(chunks, sender.flow().window());
  sender.pump();
  QCOMPARE(chunks, sender.flow().window());
  QVERIFY(sender.flow().inFlight() <= sender.flow().window());
  // an ack opens the window again
  const auto before = chunks;
  sender.onAck(lastSeq);
  QVERIFY(chunks > before);
  QVERIFY(sender.flow().inFlight() <= sender.flow().window());
  sender.stop();
  const auto stopped = chunks;
  sender.onAck(lastSeq);
  QCOMPARE(chunks, stopped);
}

void FileDragTests::senderFailsWhenFileChanges()
{
  QTemporaryDir temp;
  const fs::path base = temp.path().toStdWString();
  writeFile(base / "f.bin", std::string(1000, 'a'));
  ItemList items;
  std::vector<fs::path> sources;
  QVERIFY(scanItems({base / "f.bin"}, items, sources));
  // the file shrinks after the list was built
  writeFile(base / "f.bin", std::string(10, 'a'));
  FileDragSender shorter(1, items, sources, [](Message &&) {});
  shorter.pump();
  QVERIFY(shorter.hasFailed());
  QCOMPARE(shorter.failReason(), CancelReason::ReadFailed);

  // or grows
  writeFile(base / "f.bin", std::string(5000, 'a'));
  FileDragSender longer(2, items, sources, [](Message &&) {});
  longer.pump();
  QVERIFY(longer.hasFailed());

  // or vanishes
  fs::remove(base / "f.bin");
  FileDragSender gone(3, items, sources, [](Message &&) {});
  gone.pump();
  QVERIFY(gone.hasFailed());
}

void FileDragTests::flowControlShrinksWhenQueueBuilds()
{
  using namespace std::chrono;
  FlowControl flow(8, 1, 16, milliseconds(4));
  auto t = FlowControl::Clock::now();
  uint32_t seq = 0;
  // a quick round trip first
  flow.onSent(++seq, t);
  flow.onAck(seq, t + milliseconds(1));
  const auto start = flow.window();
  // then chunks start queueing: round trips grow well past the target
  for (int round = 0; round < 20; ++round) {
    t += milliseconds(50);
    const uint32_t first = seq + 1;
    for (uint32_t i = 0; i < flow.window(); ++i) {
      flow.onSent(++seq, t);
    }
    (void)first;
    flow.onAck(seq, t + milliseconds(30));
  }
  QVERIFY(flow.window() < start);
  QCOMPARE(flow.window(), 1u);
}

void FileDragTests::flowControlGrowsWhenQuick()
{
  using namespace std::chrono;
  FlowControl flow(2, 1, 16, milliseconds(4));
  auto t = FlowControl::Clock::now();
  uint32_t seq = 0;
  for (int round = 0; round < 40; ++round) {
    t += milliseconds(10);
    for (uint32_t i = 0; i < flow.window(); ++i) {
      flow.onSent(++seq, t);
    }
    flow.onAck(seq, t + microseconds(500));
  }
  QCOMPARE(flow.window(), 16u);
  QCOMPARE(flow.inFlight(), 0u);
}

//
// crossing the switch edge with a held button
//

namespace {

constexpr uint32_t buttonBit(ButtonID id)
{
  return 1u << id;
}

//! The cursor is on the server and heads for a client that can receive file drags
SwitchCheck leavingServer(DragPhase phase, uint32_t buttons)
{
  SwitchCheck c;
  c.buttonsDown = buttons;
  c.phase = phase;
  c.fromPrimary = true;
  c.toPrimary = false;
  c.toTakesFileDrags = true;
  return c;
}

} // namespace

void FileDragTests::switchWithHeldButton_data()
{
  QTest::addColumn<bool>("lockedByUser");
  QTest::addColumn<uint>("buttons");
  QTest::addColumn<int>("phase");
  QTest::addColumn<bool>("fromPrimary");
  QTest::addColumn<bool>("fromDragTarget");
  QTest::addColumn<bool>("toPrimary");
  QTest::addColumn<bool>("toTakesFileDrags");
  QTest::addColumn<bool>("crosses");

  const uint left = buttonBit(kButtonLeft);
  const uint right = buttonBit(kButtonRight);
  const uint middle = buttonBit(kButtonMiddle);
  const int idle = static_cast<int>(DragPhase::Idle);
  const int offered = static_cast<int>(DragPhase::Offered);
  const int remote = static_cast<int>(DragPhase::Remote);
  const int released = static_cast<int>(DragPhase::Released);
  const int dropped = static_cast<int>(DragPhase::Dropped);

  // a drag of files the edge window saw: the left button takes it across
  QTest::newRow("file drag crosses to the client")
      << false << left << offered << true << false << false << true << true;

  // a held button with no files at the edge keeps today's lock (window drag, Aero Snap at the
  // edge, text selection, scroll bar): the edge window never reports these, so the phase is Idle
  QTest::newRow("window drag or snap stays") << false << left << idle << true << false << false << true << false;
  QTest::newRow("text selection stays") << false << left << idle << true << false << false << true << false;
  QTest::newRow("right button drag stays") << false << right << idle << true << false << false << true << false;
  QTest::newRow("middle button drag stays") << false << middle << idle << true << false << false << true << false;

  // a file drag that is not only the left button, or where it cannot go
  QTest::newRow("files with right button too stay")
      << false << (left | right) << offered << true << false << false << true << false;
  QTest::newRow("files with right button only stay")
      << false << right << offered << true << false << false << true << false;
  QTest::newRow("files to a client without file drag stay")
      << false << left << offered << true << false << false << false << false;
  QTest::newRow("files with scroll lock stay") << true << left << offered << true << false << false << true << false;
  QTest::newRow("files offered while on a client stay")
      << false << left << offered << false << false << false << true << false;

  // after crossing: back to the server before the drop cancels it there; nowhere else
  QTest::newRow("carried drag comes back") << false << left << remote << false << true << true << false << true;
  QTest::newRow("carried drag to a third computer stays")
      << false << left << remote << false << true << false << true << false;
  QTest::newRow("carried drag from another client stays")
      << false << left << remote << false << false << true << false << false;
  QTest::newRow("carried drag with scroll lock stays")
      << true << left << remote << false << true << true << false << false;

  // once let go the button lock no longer applies; nothing here may bypass it
  QTest::newRow("released drag gives no bypass")
      << false << left << released << true << false << false << true << false;
  QTest::newRow("dropped drag gives no bypass") << false << left << dropped << true << false << false << true << false;
  QTest::newRow("no button gives no bypass") << false << 0u << offered << true << false << false << true << false;
}

void FileDragTests::switchWithHeldButton()
{
  QFETCH(bool, lockedByUser);
  QFETCH(uint, buttons);
  QFETCH(int, phase);
  QFETCH(bool, fromPrimary);
  QFETCH(bool, fromDragTarget);
  QFETCH(bool, toPrimary);
  QFETCH(bool, toTakesFileDrags);
  QFETCH(bool, crosses);

  SwitchCheck c;
  c.lockedByUser = lockedByUser;
  c.buttonsDown = buttons;
  c.phase = static_cast<DragPhase>(phase);
  c.fromPrimary = fromPrimary;
  c.fromDragTarget = fromDragTarget;
  c.toPrimary = toPrimary;
  c.toTakesFileDrags = toTakesFileDrags;
  QCOMPARE(fileDragAllowsSwitch(c), crosses);
}

void FileDragTests::offerNeedsOnlyTheLeftButton()
{
  const uint32_t left = buttonBit(kButtonLeft);
  QVERIFY(fileDragAcceptsOffer(DragPhase::Idle, true, left));
  QVERIFY(!fileDragAcceptsOffer(DragPhase::Idle, true, left | buttonBit(kButtonRight)));
  QVERIFY(!fileDragAcceptsOffer(DragPhase::Idle, true, buttonBit(kButtonRight)));
  QVERIFY(!fileDragAcceptsOffer(DragPhase::Idle, true, 0));
  QVERIFY(!fileDragAcceptsOffer(DragPhase::Idle, false, left));
  QVERIFY(!fileDragAcceptsOffer(DragPhase::Remote, true, left));
  QVERIFY(!fileDragAcceptsOffer(DragPhase::Released, true, left));

  // the offer is what lets the held button cross: before it the lock holds, after it it crosses
  QVERIFY(!fileDragAllowsSwitch(leavingServer(DragPhase::Idle, left)));
  QVERIFY(fileDragAllowsSwitch(leavingServer(DragPhase::Offered, left)));
}

void FileDragTests::edgeStripFollowsDirectionMask_data()
{
  QTest::addColumn<int>("side");
  QTest::addColumn<uint>("mask");
  QTest::addColumn<bool>("shown");
  QTest::addColumn<int>("x");
  QTest::addColumn<int>("y");
  QTest::addColumn<int>("w");
  QTest::addColumn<int>("h");

  const int t = kEdgeStripThickness;
  const auto L = static_cast<int>(Direction::Left);
  const auto R = static_cast<int>(Direction::Right);
  const auto T = static_cast<int>(Direction::Top);
  const auto B = static_cast<int>(Direction::Bottom);
  const auto lm = static_cast<uint>(DirectionMask::LeftMask);
  const auto rm = static_cast<uint>(DirectionMask::RightMask);
  const auto tm = static_cast<uint>(DirectionMask::TopMask);
  const auto bm = static_cast<uint>(DirectionMask::BottomMask);

  // screen 3440x1440 at 0,0 (the owner's server), the client on its left
  QTest::newRow("left asked, left strip") << L << lm << true << 0 << 0 << t << 1440;
  QTest::newRow("left asked, no right strip") << R << lm << false << 0 << 0 << 0 << 0;
  QTest::newRow("right asked, right strip") << R << rm << true << 3440 - t << 0 << t << 1440;
  QTest::newRow("right asked, no left strip") << L << rm << false << 0 << 0 << 0 << 0;
  QTest::newRow("top asked, top strip") << T << tm << true << 0 << 0 << 3440 << t;
  QTest::newRow("bottom asked, bottom strip") << B << bm << true << 0 << 1440 - t << 3440 << t;
  QTest::newRow("left and bottom, left strip") << L << (lm | bm) << true << 0 << 0 << t << 1440;
  QTest::newRow("left and bottom, bottom strip") << B << (lm | bm) << true << 0 << 1440 - t << 3440 << t;
  QTest::newRow("nothing asked") << L << 0u << false << 0 << 0 << 0 << 0;
}

void FileDragTests::edgeStripFollowsDirectionMask()
{
  QFETCH(int, side);
  QFETCH(uint, mask);
  QFETCH(bool, shown);
  QFETCH(int, x);
  QFETCH(int, y);
  QFETCH(int, w);
  QFETCH(int, h);

  const EdgeRect screen{0, 0, 3440, 1440};
  const auto strip = edgeStrip(static_cast<Direction>(side), mask, screen);
  QCOMPARE(strip.has_value(), shown);
  if (shown) {
    QCOMPARE(*strip, (EdgeRect{x, y, w, h}));
  }
}

void FileDragTests::edgeStripIgnoresOldLeftValue()
{
  // the first test build's helper read bit 0 as "left" while the server sends DirectionMask
  // (left = bit 1), so the window went up on the right edge and the drag hit the wall
  const auto left = static_cast<uint32_t>(DirectionMask::LeftMask);
  QCOMPARE(left, 2u);
  const EdgeRect screen{-1920, -200, 1920, 1080}; // a monitor left of and above the main one
  const auto strip = edgeStrip(Direction::Left, left, screen);
  QVERIFY(strip.has_value());
  QCOMPARE(*strip, (EdgeRect{-1920, -200, kEdgeStripThickness, 1080}));
  QVERIFY(!edgeStrip(Direction::Right, left, screen).has_value());
  QVERIFY(!edgeStrip(Direction::Left, 1u, screen).has_value());
  QVERIFY(!edgeStrip(Direction::NoDirection, 0xffu, screen).has_value());
}

void FileDragTests::edgeStripsLieOnOneDisplayEach()
{
  // a 1512x982 laptop with a 2560x1440 display to its right, top edges level
  const std::vector<EdgeRect> displays{{0, 0, 1512, 982}, {1512, 0, 2560, 1440}};
  const EdgeRect desktop{0, 0, 4072, 1440};
  const auto all = static_cast<uint32_t>(DirectionMask::LeftMask) | static_cast<uint32_t>(DirectionMask::RightMask) |
                   static_cast<uint32_t>(DirectionMask::TopMask) | static_cast<uint32_t>(DirectionMask::BottomMask);

  const auto left = edgeStripsOnDisplays(Direction::Left, all, desktop, displays);
  QCOMPARE(left.size(), size_t{1});
  QCOMPARE(left[0], (EdgeRect{0, 0, kEdgeStripThickness, 982}));

  const auto right = edgeStripsOnDisplays(Direction::Right, all, desktop, displays);
  QCOMPARE(right.size(), size_t{1});
  QCOMPARE(right[0], (EdgeRect{4072 - kEdgeStripThickness, 0, kEdgeStripThickness, 1440}));

  // both displays share the top edge: one strip on each, never one across both
  const auto top = edgeStripsOnDisplays(Direction::Top, all, desktop, displays);
  QCOMPARE(top.size(), size_t{2});
  QCOMPARE(top[0], (EdgeRect{0, 0, 1512, kEdgeStripThickness}));
  QCOMPARE(top[1], (EdgeRect{1512, 0, 2560, kEdgeStripThickness}));

  // only the taller display reaches the bottom of the desktop
  const auto bottom = edgeStripsOnDisplays(Direction::Bottom, all, desktop, displays);
  QCOMPARE(bottom.size(), size_t{1});
  QCOMPARE(bottom[0], (EdgeRect{1512, 1440 - kEdgeStripThickness, 2560, kEdgeStripThickness}));
}

void FileDragTests::edgeStripsSkipDisplaysAwayFromTheEdge()
{
  // a smaller display above the main one, not reaching the left edge of the desktop
  const std::vector<EdgeRect> displays{{0, 0, 1920, 1080}, {200, -900, 1600, 900}};
  const EdgeRect desktop{0, -900, 1920, 1980};
  const auto left = static_cast<uint32_t>(DirectionMask::LeftMask);
  const auto strips = edgeStripsOnDisplays(Direction::Left, left, desktop, displays);
  QCOMPARE(strips.size(), size_t{1});
  QCOMPARE(strips[0], (EdgeRect{0, 0, kEdgeStripThickness, 1080}));

  // a side the server did not ask for gets nothing
  QVERIFY(edgeStripsOnDisplays(Direction::Right, left, desktop, displays).empty());
  QVERIFY(edgeStripsOnDisplays(Direction::NoDirection, 0xffu, desktop, displays).empty());

  // the top of the desktop is the top of the upper display
  const auto top = static_cast<uint32_t>(DirectionMask::TopMask);
  const auto topStrips = edgeStripsOnDisplays(Direction::Top, top, desktop, displays);
  QCOMPARE(topStrips.size(), size_t{1});
  QCOMPARE(topStrips[0], (EdgeRect{200, -900, 1600, kEdgeStripThickness}));
}

void FileDragTests::edgeStripsWithoutDisplayListUseDesktop()
{
  const EdgeRect desktop{-1920, -200, 1920, 1080};
  const auto left = static_cast<uint32_t>(DirectionMask::LeftMask);
  const auto strips = edgeStripsOnDisplays(Direction::Left, left, desktop, {});
  QCOMPARE(strips.size(), size_t{1});
  QCOMPARE(strips[0], *edgeStrip(Direction::Left, left, desktop));
}

void FileDragTests::squareAroundStaysOnDisplay()
{
  const std::vector<EdgeRect> displays{{0, 0, 1512, 982}, {1512, 0, 2560, 1440}};

  // in the middle: centred on the point
  QCOMPARE(squareAround(700, 500, 64, displayAt(700, 500, displays)), (EdgeRect{668, 468, 64, 64}));

  // pressed against the right edge of the desktop: moved left, still under the cursor
  const auto edge = squareAround(4071, 10, 160, displayAt(4071, 10, displays));
  QCOMPARE(edge, (EdgeRect{4072 - 160, 0, 160, 160}));
  QVERIFY(4071 >= edge.x && 4071 < edge.x + edge.w && 10 >= edge.y && 10 < edge.y + edge.h);

  // on the laptop's corner it stays on the laptop instead of reaching into the big display
  const auto seam = squareAround(1511, 981, 64, displayAt(1511, 981, displays));
  QCOMPARE(seam, (EdgeRect{1512 - 64, 982 - 64, 64, 64}));

  // off every display: only centred
  QVERIFY(!displayAt(-5, -5, displays).has_value());
  QCOMPARE(squareAround(-5, -5, 64, displayAt(-5, -5, displays)), (EdgeRect{-37, -37, 64, 64}));
  // a display smaller than the square is never used to clamp
  QCOMPARE(squareAround(10, 10, 64, EdgeRect{0, 0, 32, 32}), (EdgeRect{-22, -22, 64, 64}));
}

void FileDragTests::flipVerticalIsItsOwnInverse()
{
  // main display 982 high: a strip along its top edge is at the top of the bottom-left space too
  const EdgeRect top{0, 0, 1512, 4};
  QCOMPARE(flipVertical(top, 982), (EdgeRect{0, 978, 1512, 4}));
  // a display above the main one has negative y here and lies above 982 there
  const EdgeRect above{200, -900, 1600, 900};
  QCOMPARE(flipVertical(above, 982), (EdgeRect{200, 982, 1600, 900}));
  QCOMPARE(flipVertical(flipVertical(above, 982), 982), above);
  static_assert(flipVertical(EdgeRect{1, 2, 3, 4}, 100) == EdgeRect{1, 94, 3, 4});
}

void FileDragTests::fileReferencePathsAreRecognised()
{
  QVERIFY(isFileReferencePath("/.file/id=6571367.2773272/"));
  QVERIFY(isFileReferencePath("/.file/id=6571367.2773272"));
  QVERIFY(!isFileReferencePath("/Users/chris/.file/id=1"));
  QVERIFY(!isFileReferencePath("/Users/chris/Desktop/a.txt"));
  QVERIFY(!isFileReferencePath(""));
}

void FileDragTests::stagingListsTopLevelItems()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  CancelReason reason;
  QVERIFY(staging.begin(
      2,
      {{"a.txt", 3, false}, {"Photos", 0, true}, {"Photos/x.jpg", 2, false}, {"Photos/Old", 0, true}, {"b", 0, false}},
      reason
  ));
  QCOMPARE(staging.topLevelIndices(), (std::vector<uint32_t>{0, 1, 4}));
}

void FileDragTests::stagingWaitForItemCoversFolder()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  CancelReason reason;
  const ItemList items{
      {"Photos", 0, true},        {"Photos/2026", 0, true}, {"Photos/2026/a.jpg", 5, false},
      {"Photos/b.jpg", 4, false}, {"Empty", 0, true},       {"c.txt", 0, false},
  };
  QVERIFY(staging.begin(3, items, reason));
  using WaitResult = FileDragStaging::WaitResult;
  const auto quick = std::chrono::milliseconds(20);

  // an empty folder is complete at once; a file waits for its FileEnd even when empty
  QCOMPARE(staging.waitForItem(4, quick), WaitResult::Eof);
  QCOMPARE(staging.waitForItem(5, quick), WaitResult::Timeout);
  QCOMPARE(staging.waitForItem(0, quick), WaitResult::Timeout);

  // a waiter on the folder wakes only once every file under it is in
  std::atomic<int> result{-1};
  std::thread waiter([&] { result = static_cast<int>(staging.waitForItem(0, std::chrono::seconds(10))); });

  QVERIFY(staging.fileStart(2, 5));
  QVERIFY(staging.write(2, "hello"));
  QVERIFY(staging.fileEnd(2));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  QCOMPARE(result.load(), -1); // b.jpg is still missing

  QVERIFY(staging.fileStart(3, 4));
  QVERIFY(staging.write(3, "abcd"));
  QVERIFY(staging.fileEnd(3));
  waiter.join();
  QCOMPARE(result.load(), static_cast<int>(WaitResult::Eof));

  QVERIFY(staging.fileStart(5, 0));
  QVERIFY(staging.fileEnd(5));
  QCOMPARE(staging.waitForItem(5, quick), WaitResult::Eof);
  QVERIFY(staging.isComplete());
}

void FileDragTests::stagingWaitForItemFailsOnCancel()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  CancelReason reason;
  QVERIFY(staging.begin(4, {{"big.bin", 1000, false}}, reason));

  std::atomic<int> result{-1};
  std::thread waiter([&] { result = static_cast<int>(staging.waitForItem(0, std::chrono::seconds(10))); });
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  staging.cancel();
  waiter.join();
  QCOMPARE(result.load(), static_cast<int>(FileDragStaging::WaitResult::Failed));
  QCOMPARE(staging.waitForItem(0, std::chrono::milliseconds(1)), FileDragStaging::WaitResult::Failed);
}

void FileDragTests::stagingWaitForItemRejectsNestedIndex()
{
  QTemporaryDir temp;
  FileDragStaging staging(fs::path(temp.path().toStdWString()));
  using WaitResult = FileDragStaging::WaitResult;
  // nothing begun yet
  QCOMPARE(staging.waitForItem(0, std::chrono::milliseconds(1)), WaitResult::Failed);
  CancelReason reason;
  QVERIFY(staging.begin(5, {{"Docs", 0, true}, {"Docs/a.txt", 1, false}}, reason));
  // only top-level items can be waited for: a promise is made per top-level item
  QCOMPARE(staging.waitForItem(1, std::chrono::milliseconds(1)), WaitResult::Failed);
  QCOMPARE(staging.waitForItem(7, std::chrono::milliseconds(1)), WaitResult::Failed);
}

void FileDragTests::unpremultiplyRestoresStraightAlpha()
{
  // opaque, half transparent, fully transparent, and a premultiplied value above alpha
  const unsigned char pixels[] = {
      0x10, 0x20, 0x30, 0xff, // opaque: unchanged
      0x40, 0x20, 0x00, 0x80, // half: colour doubles
      0x05, 0x06, 0x07, 0x00, // transparent: cleared
      0x90, 0x10, 0x10, 0x80, // invalid premultiplied value: clamped
      0x01, 0x02              // trailing bytes: left alone
  };
  std::string rgba(reinterpret_cast<const char *>(pixels), sizeof(pixels));
  unpremultiplyRgba(rgba);
  const auto px = [&rgba](size_t i) { return static_cast<int>(static_cast<uint8_t>(rgba[i])); };
  QCOMPARE(px(0), 0x10);
  QCOMPARE(px(1), 0x20);
  QCOMPARE(px(2), 0x30);
  QCOMPARE(px(3), 0xff);
  QCOMPARE(px(4), (0x40 * 255 + 0x40) / 0x80);
  QCOMPARE(px(5), (0x20 * 255 + 0x40) / 0x80);
  QCOMPARE(px(6), 0);
  QCOMPARE(px(7), 0x80);
  QCOMPARE(px(8), 0);
  QCOMPARE(px(9), 0);
  QCOMPARE(px(10), 0);
  QCOMPARE(px(11), 0);
  QCOMPARE(px(12), 255);
  QCOMPARE(rgba.size(), sizeof(pixels));
  QCOMPARE(px(16), 1);
  QCOMPARE(px(17), 2);
}

//
// client to server
//

void FileDragTests::switchFromClientWithHeldButton_data()
{
  QTest::addColumn<bool>("lockedByUser");
  QTest::addColumn<uint>("buttons");
  QTest::addColumn<int>("phase");
  QTest::addColumn<bool>("fromPrimary");
  QTest::addColumn<bool>("fromDragSource");
  QTest::addColumn<bool>("toPrimary");
  QTest::addColumn<bool>("toDragSource");
  QTest::addColumn<bool>("primaryTakes");
  QTest::addColumn<bool>("crosses");

  const uint left = buttonBit(kButtonLeft);
  const uint right = buttonBit(kButtonRight);
  const int idle = static_cast<int>(DragPhase::Idle);
  const int offered = static_cast<int>(DragPhase::Offered);
  const int remote = static_cast<int>(DragPhase::Remote);
  const int released = static_cast<int>(DragPhase::Released);
  const int dropped = static_cast<int>(DragPhase::Dropped);

  // files the client's edge window saw: the left button takes them from the laptop to the PC
  QTest::newRow("files cross from the client to the server")
      << false << left << offered << false << true << true << false << true << true;

  // on the client, a held button with no files at its edge keeps today's lock: a window
  // dragged to the edge (Aero Snap), a text selection, a scroll bar. Its edge window never
  // reports these, so the server never leaves Idle for them
  QTest::newRow("window drag or snap on the client stays")
      << false << left << idle << false << false << true << false << true << false;
  QTest::newRow("text selection on the client stays")
      << false << left << idle << false << false << true << false << true << false;
  QTest::newRow("right button drag on the client stays")
      << false << right << idle << false << false << true << false << true << false;

  // files, but not only the left button, or where they cannot go
  QTest::newRow("files with right button too stay")
      << false << (left | right) << offered << false << true << true << false << true << false;
  QTest::newRow("files to a server that cannot show drags stay")
      << false << left << offered << false << true << true << false << false << false;
  QTest::newRow("files from the client to a third computer stay")
      << false << left << offered << false << true << false << false << true << false;
  QTest::newRow("files offered by another client stay")
      << false << left << offered << false << false << true << false << true << false;
  QTest::newRow("files with scroll lock stay")
      << true << left << offered << false << true << true << false << true << false;

  // after crossing: back to the client before the drop (the drag carries on there); nowhere else
  QTest::newRow("carried drag goes back to the client")
      << false << left << remote << true << false << false << true << true << true;
  QTest::newRow("carried drag to another client stays")
      << false << left << remote << true << false << false << false << true << false;
  QTest::newRow("carried drag with scroll lock stays")
      << true << left << remote << true << false << false << true << true << false;
  QTest::newRow("carried drag from the client again stays")
      << false << left << remote << false << true << true << false << true << false;

  // once let go the button lock no longer applies; nothing here may bypass it
  QTest::newRow("released drag gives no bypass")
      << false << left << released << true << false << false << true << true << false;
  QTest::newRow("dropped drag gives no bypass")
      << false << left << dropped << true << false << false << true << true << false;
  QTest::newRow("no button gives no bypass")
      << false << 0u << offered << false << true << true << false << true << false;
}

void FileDragTests::switchFromClientWithHeldButton()
{
  QFETCH(bool, lockedByUser);
  QFETCH(uint, buttons);
  QFETCH(int, phase);
  QFETCH(bool, fromPrimary);
  QFETCH(bool, fromDragSource);
  QFETCH(bool, toPrimary);
  QFETCH(bool, toDragSource);
  QFETCH(bool, primaryTakes);
  QFETCH(bool, crosses);

  SwitchCheck c;
  c.lockedByUser = lockedByUser;
  c.buttonsDown = buttons;
  c.phase = static_cast<DragPhase>(phase);
  c.fromPrimary = fromPrimary;
  c.toPrimary = toPrimary;
  c.fromSecondary = true;
  c.fromDragSource = fromDragSource;
  c.toDragSource = toDragSource;
  c.primaryTakesFileDrags = primaryTakes;
  // the server to client fields must not matter for a drag from a client
  c.fromDragTarget = true;
  c.toTakesFileDrags = true;
  QCOMPARE(fileDragAllowsSwitch(c), crosses);
}

void FileDragTests::remoteOfferNeedsWatchAndOnlyLeftButton()
{
  const uint32_t left = buttonBit(kButtonLeft);
  const auto rightSide = static_cast<uint32_t>(DirectionMask::RightMask);
  QVERIFY(fileDragAcceptsRemoteOffer(DragPhase::Idle, true, rightSide, left));
  // the server did not ask (a late or stray report): the lock holds
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Idle, true, 0, left));
  // the cursor is not on that client
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Idle, false, rightSide, left));
  // not only the left button, or let go already
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Idle, true, rightSide, left | buttonBit(kButtonRight)));
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Idle, true, rightSide, 0));
  // another drag is running
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Offered, true, rightSide, left));
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Remote, true, rightSide, left));
  QVERIFY(!fileDragAcceptsRemoteOffer(DragPhase::Released, true, rightSide, left));

  // before the offer the held button stays on the client, after it it crosses to the server
  SwitchCheck c;
  c.buttonsDown = left;
  c.fromSecondary = true;
  c.fromDragSource = true;
  c.toPrimary = true;
  c.primaryTakesFileDrags = true;
  c.phase = DragPhase::Idle;
  QVERIFY(!fileDragAllowsSwitch(c));
  c.phase = DragPhase::Offered;
  QVERIFY(fileDragAllowsSwitch(c));
}

void FileDragTests::clientWatchesTheEdgeThatLeadsToTheServer_data()
{
  QTest::addColumn<int>("serverSide"); // side of the CLIENT's screen where the server is
  QTest::addColumn<int>("cursorX");
  QTest::addColumn<int>("cursorY");
  QTest::addColumn<uint>("sides");
  QTest::addColumn<int>("stripX");
  QTest::addColumn<int>("stripY");
  QTest::addColumn<int>("stripW");
  QTest::addColumn<int>("stripH");

  const int t = kEdgeStripThickness;
  const auto L = static_cast<int>(Direction::Left);
  const auto R = static_cast<int>(Direction::Right);
  const auto B = static_cast<int>(Direction::Bottom);
  const auto lm = static_cast<uint>(DirectionMask::LeftMask);
  const auto rm = static_cast<uint>(DirectionMask::RightMask);
  const auto bm = static_cast<uint>(DirectionMask::BottomMask);

  // the owner's layout: the laptop (1920x1080) is left of the PC, so the laptop's RIGHT edge
  // leads back to the PC; the window must go up there and nowhere else
  QTest::newRow("laptop left of the PC, at its right edge") << R << 1919 << 500 << rm << 1920 - t << 0 << t << 1080;
  QTest::newRow("laptop left of the PC, near its right edge") << R << 1900 << 500 << rm << 1920 - t << 0 << t << 1080;
  QTest::newRow("laptop left of the PC, at its left edge") << R << 0 << 500 << 0u << 0 << 0 << 0 << 0;
  QTest::newRow("laptop left of the PC, in the middle") << R << 960 << 500 << 0u << 0 << 0 << 0 << 0;
  // mirrored and other layouts
  QTest::newRow("laptop right of the PC, at its left edge") << L << 3 << 10 << lm << 0 << 0 << t << 1080;
  QTest::newRow("laptop right of the PC, at its right edge") << L << 1919 << 10 << 0u << 0 << 0 << 0 << 0;
  QTest::newRow("laptop above the PC, at its bottom edge") << B << 700 << 1079 << bm << 0 << 1080 - t << 1920 << t;
}

void FileDragTests::clientWatchesTheEdgeThatLeadsToTheServer()
{
  QFETCH(int, serverSide);
  QFETCH(int, cursorX);
  QFETCH(int, cursorY);
  QFETCH(uint, sides);
  QFETCH(int, stripX);
  QFETCH(int, stripY);
  QFETCH(int, stripW);
  QFETCH(int, stripH);

  // the server works out the sides of the client's screen (as Server::fileDragUpdateRemoteWatch
  // does with mapToNeighbor) and sends them; the client puts its window on them
  const EdgeRect client{0, 0, 1920, 1080};
  const auto server = static_cast<Direction>(serverSide);
  bool pointsOutside = true;
  const uint32_t watched = sidesToWatch(client, cursorX, cursorY, [&](Direction side, int32_t nx, int32_t ny) {
    // the point handed over is just outside that side of the client's screen
    switch (side) {
    case Direction::Left:
      pointsOutside = pointsOutside && nx == client.x - 1;
      break;
    case Direction::Right:
      pointsOutside = pointsOutside && nx == client.x + client.w;
      break;
    case Direction::Top:
      pointsOutside = pointsOutside && ny == client.y - 1;
      break;
    case Direction::Bottom:
      pointsOutside = pointsOutside && ny == client.y + client.h;
      break;
    default:
      pointsOutside = false;
      break;
    }
    return side == server;
  });
  QVERIFY(pointsOutside);
  QCOMPARE(watched, sides);

  int shown = 0;
  for (Direction side : {Direction::Left, Direction::Right, Direction::Top, Direction::Bottom}) {
    const auto strip = edgeStrip(side, watched, client);
    if (strip) {
      ++shown;
      QCOMPARE(side, server);
      QCOMPARE(*strip, (EdgeRect{stripX, stripY, stripW, stripH}));
    }
  }
  QCOMPARE(shown, sides != 0 ? 1 : 0);
}

void FileDragTests::serverWatchesTheEdgeThatLeadsToTheClient()
{
  // the server's own watch goes through the same helper (the earlier fix: the PC's LEFT
  // edge leads to the laptop)
  const EdgeRect pc{0, 0, 3440, 1440};
  auto laptopOnTheLeft = [](Direction side, int32_t, int32_t) { return side == Direction::Left; };
  QCOMPARE(sidesToWatch(pc, 2, 700, laptopOnTheLeft), static_cast<uint32_t>(DirectionMask::LeftMask));
  QCOMPARE(sidesToWatch(pc, 3439, 700, laptopOnTheLeft), 0u);
  QCOMPARE(sidesToWatch(pc, kWatchReach, 700, laptopOnTheLeft), 0u);
  QCOMPARE(sidesToWatch(pc, kWatchReach - 1, 700, laptopOnTheLeft), static_cast<uint32_t>(DirectionMask::LeftMask));
  QCOMPARE(sidesToWatch(EdgeRect{0, 0, 0, 0}, 0, 0, laptopOnTheLeft), 0u);
  // a corner near two sides that both lead somewhere
  auto anywhere = [](Direction, int32_t, int32_t) { return true; };
  QCOMPARE(
      sidesToWatch(pc, 1, 1, anywhere),
      static_cast<uint32_t>(DirectionMask::LeftMask) | static_cast<uint32_t>(DirectionMask::TopMask)
  );
}

void FileDragTests::stepAcrossPushesOverTheWatchedEdge()
{
  const EdgeRect laptop{0, 0, 1920, 1080};
  const auto rm = static_cast<uint32_t>(DirectionMask::RightMask);
  const auto lm = static_cast<uint32_t>(DirectionMask::LeftMask);
  const auto bm = static_cast<uint32_t>(DirectionMask::BottomMask);
  QCOMPARE(stepAcross(laptop, 1919, 400, rm), (Step{1, 0}));
  QCOMPARE(stepAcross(laptop, 1918, 400, rm), (Step{}));
  QCOMPARE(stepAcross(laptop, 0, 400, rm), (Step{}));
  QCOMPARE(stepAcross(laptop, 0, 400, lm), (Step{-1, 0}));
  QCOMPARE(stepAcross(laptop, 50, 1079, bm), (Step{0, 1}));
  QCOMPARE(stepAcross(laptop, 1919, 400, 0), (Step{}));
}

QTEST_MAIN(FileDragTests)
