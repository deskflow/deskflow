/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WlClipboardBridgeTests.h"

#include "deskflow/ClipboardTypes.h"
#include "platform/EiClipboard.h"
#include "platform/WlClipboardBridge.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include <mutex>
#include <string>
#include <vector>

using deskflow::EiClipboard;
using deskflow::WlClipboardBridge;
using Format = IClipboard::Format;

namespace {

// Fake wl-clipboard commands backed by a directory: one file per mime plus a "types" listing.
const QString kFileForMime = QStringLiteral("\"$0/$(printf %s \"$1\" | tr '/;=' '___')\"");

WlClipboardBridge::Commands fakeCommands(const QString &dir)
{
  WlClipboardBridge::Commands commands;
  commands.list = {QStringLiteral("sh"), QStringLiteral("-c"), QStringLiteral("cat \"$0/types\""), dir};
  commands.paste = {QStringLiteral("sh"), QStringLiteral("-c"), QStringLiteral("cat ") + kFileForMime, dir};
  commands.watch = {QStringLiteral("sh"), QStringLiteral("-c"), QStringLiteral("while sleep 0.05; do echo; done")};
  commands.copy = {
      QStringLiteral("sh"), QStringLiteral("-c"),
      QStringLiteral("printf '%s\\n' \"$1\" > \"$0/types\"; cat > ") + kFileForMime, dir
  };
  return commands;
}

void writeFile(const QString &path, const QByteArray &data)
{
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(data);
}

QString fileForMime(const QString &dir, QString mime)
{
  return dir + '/' + mime.replace('/', '_').replace(';', '_').replace('=', '_');
}

// Simulates a local copy offering the given formats.
void copyLocally(const QString &dir, const std::vector<std::pair<QString, QByteArray>> &formats)
{
  QByteArray types;
  for (const auto &[mime, data] : formats) {
    writeFile(fileForMime(dir, mime), data);
    types += mime.toUtf8() + '\n';
  }
  writeFile(dir + QStringLiteral("/types"), types);
}

QByteArray pngImage()
{
  QImage image(2, 2, QImage::Format_RGB32);
  image.fill(Qt::red);
  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return png;
}

struct Received
{
  std::mutex mutex;
  std::vector<WlClipboardBridge::MimeData> data;

  size_t count()
  {
    std::scoped_lock lock{mutex};
    return data.size();
  }

  WlClipboardBridge::ChangedCallback callback()
  {
    return [this](const WlClipboardBridge::MimeData &received) {
      std::scoped_lock lock{mutex};
      data.push_back(received);
    };
  }
};

} // namespace

void WlClipboardBridgeTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void WlClipboardBridgeTests::readData_noClipboard_returnsEmpty()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  WlClipboardBridge bridge([](const WlClipboardBridge::MimeData &) {}, fakeCommands(dir.path()));

  QVERIFY(bridge.readData().empty());
}

void WlClipboardBridgeTests::localChange_htmlAndText_callbackReceivesBothOnce()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  copyLocally(dir.path(), {{QStringLiteral("text/plain;charset=utf-8"), "initial"}});

  Received received;
  WlClipboardBridge bridge(received.callback(), fakeCommands(dir.path()));
  bridge.start();

  // Content present before start is not a local grab.
  QTest::qWait(300);
  QCOMPARE(received.count(), size_t{0});

  copyLocally(
      dir.path(), {{QStringLiteral("text/html"), "<b>hi</b>"}, {QStringLiteral("text/plain;charset=utf-8"), "hi"}}
  );
  QTRY_COMPARE_WITH_TIMEOUT(received.count(), size_t{1}, 3000);

  // Further watch events with unchanged content are ignored.
  QTest::qWait(300);
  bridge.stop();
  QCOMPARE(received.count(), size_t{1});
  const auto &data = received.data.front();
  QCOMPARE(data.size(), size_t{2});
  QCOMPARE(data.at(WlClipboardBridge::kMimeHtml), std::string{"<b>hi</b>"});
  QCOMPARE(data.at(WlClipboardBridge::kMimeText), std::string{"hi"});
}

void WlClipboardBridgeTests::setData_remoteHtml_writesClipboardWithoutCallback()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  copyLocally(dir.path(), {{QStringLiteral("text/plain;charset=utf-8"), "initial"}});

  Received received;
  WlClipboardBridge bridge(received.callback(), fakeCommands(dir.path()));
  bridge.start();

  bridge.setData(WlClipboardBridge::kMimeHtml, "<i>from windows</i>");

  const auto data = bridge.readData();
  QCOMPARE(data.size(), size_t{1});
  QCOMPARE(data.at(WlClipboardBridge::kMimeHtml), std::string{"<i>from windows</i>"});

  // The watch event caused by our own write must not be echoed back.
  QTest::qWait(300);
  bridge.stop();
  QCOMPARE(received.count(), size_t{0});
}

void WlClipboardBridgeTests::toClipboard_textHtmlPng_addsAllFormats()
{
  EiClipboard clipboard(kClipboardClipboard);
  const WlClipboardBridge::MimeData data = {
      {WlClipboardBridge::kMimeText, "hi"},
      {WlClipboardBridge::kMimeHtml, "<b>hi</b>"},
      {WlClipboardBridge::kMimePng, pngImage().toStdString()},
  };

  clipboard.open(0);
  clipboard.empty();
  QVERIFY(WlClipboardBridge::toClipboard(data, &clipboard));

  QCOMPARE(clipboard.get(Format::Text), std::string{"hi"});
  QCOMPARE(clipboard.get(Format::HTML), std::string{"<b>hi</b>"});
  QVERIFY(clipboard.has(Format::Bitmap));

  // The bitmap round-trips back to an equivalent png.
  const auto [mime, png] = WlClipboardBridge::fromClipboard(&clipboard);
  clipboard.close();
  QCOMPARE(mime, std::string{WlClipboardBridge::kMimePng});
  QImage image;
  QVERIFY(image.loadFromData(QByteArray::fromStdString(png), "PNG"));
  QCOMPARE(image.size(), QSize(2, 2));
  QCOMPARE(image.pixelColor(0, 0), QColor(Qt::red));
}

void WlClipboardBridgeTests::toClipboard_utf16Html_convertsToUtf8()
{
  EiClipboard clipboard(kClipboardClipboard);
  const auto html = QStringLiteral("<b>ñ</b>");
  const auto utf16 = QByteArray(reinterpret_cast<const char *>(html.utf16()), html.size() * 2);

  clipboard.open(0);
  clipboard.empty();
  QVERIFY(WlClipboardBridge::toClipboard({{WlClipboardBridge::kMimeHtml, utf16.toStdString()}}, &clipboard));

  QCOMPARE(clipboard.get(Format::HTML), html.toStdString());
  clipboard.close();
}

void WlClipboardBridgeTests::fromClipboard_bitmapAndText_prefersPng()
{
  EiClipboard clipboard(kClipboardClipboard);
  clipboard.open(0);
  clipboard.empty();
  WlClipboardBridge::toClipboard(
      {{WlClipboardBridge::kMimeText, "hi"}, {WlClipboardBridge::kMimePng, pngImage().toStdString()}}, &clipboard
  );

  const auto [mime, data] = WlClipboardBridge::fromClipboard(&clipboard);
  clipboard.close();

  QCOMPARE(mime, std::string{WlClipboardBridge::kMimePng});
  QVERIFY(!data.empty());
}

void WlClipboardBridgeTests::fromClipboard_textAndHtml_prefersText()
{
  EiClipboard clipboard(kClipboardClipboard);
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(Format::Text, "hi");
  clipboard.add(Format::HTML, "<b>hi</b>");

  const auto [mime, data] = WlClipboardBridge::fromClipboard(&clipboard);
  clipboard.close();

  QCOMPARE(mime, std::string{WlClipboardBridge::kMimeText});
  QCOMPARE(data, std::string{"hi"});
}

void WlClipboardBridgeTests::fromClipboard_htmlOnly_returnsHtml()
{
  EiClipboard clipboard(kClipboardClipboard);
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(Format::HTML, "<b>hi</b>");

  const auto [mime, data] = WlClipboardBridge::fromClipboard(&clipboard);
  clipboard.close();

  QCOMPARE(mime, std::string{WlClipboardBridge::kMimeHtml});
  QCOMPARE(data, std::string{"<b>hi</b>"});
}

QTEST_MAIN(WlClipboardBridgeTests)
