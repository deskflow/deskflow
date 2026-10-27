/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboardBridge.h"

#include "base/Log.h"
#include "base/Unicode.h"

#ifdef HAVE_LIBPORTAL_CLIPBOARD
#include "platform/PortalClipboard.h"
#endif

#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <stdexcept>

namespace deskflow {

namespace {

constexpr int kProcessTimeoutMs = 2000;
constexpr int kWatchPollMs = 200;

QStringList withMime(QStringList command, const QString &mime)
{
  command.append(mime);
  return command;
}

} // namespace

WlClipboardBridge::WlClipboardBridge(ChangedCallback onChanged, Commands commands)
    : m_onChanged{std::move(onChanged)},
      m_commands{std::move(commands)}
{
  // Default-constructed commands are never empty; guard against bad overrides.
  if (m_commands.list.isEmpty() || m_commands.paste.isEmpty() || m_commands.watch.isEmpty() ||
      m_commands.copy.isEmpty()) {
    throw std::invalid_argument("wl clipboard bridge commands must not be empty");
  }
}

WlClipboardBridge::~WlClipboardBridge()
{
  stop();
}

bool WlClipboardBridge::isAvailable()
{
  const auto env = QProcessEnvironment::systemEnvironment();
  const auto setting = env.value(QStringLiteral("DESKFLOW_WL_CLIPBOARD"));
  if (setting == QStringLiteral("0")) {
    return false;
  }

  if (env.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty()) {
    return false;
  }

  if (QStandardPaths::findExecutable(QStringLiteral("wl-paste")).isEmpty() ||
      QStandardPaths::findExecutable(QStringLiteral("wl-copy")).isEmpty()) {
    LOG_DEBUG("wl-clipboard not found, wayland clipboard bridge disabled");
    return false;
  }

  if (setting == QStringLiteral("1")) {
    return true;
  }

  // These desktops implement the Clipboard portal, which deskflow already uses.
  const auto desktop = env.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
  return !desktop.contains(QStringLiteral("GNOME"), Qt::CaseInsensitive) &&
         !desktop.contains(QStringLiteral("KDE"), Qt::CaseInsensitive);
}

void WlClipboardBridge::start()
{
  if (m_thread.joinable()) {
    return;
  }

  // Seed with the current content so the initial watch event is not treated as a grab.
  const auto current = readData();
  {
    std::scoped_lock lock{m_mutex};
    m_lastRead = current;
  }

  m_stopping = false;
  m_thread = std::thread([this] { watchLoop(); });
  LOG_INFO("wayland clipboard bridge started");
}

void WlClipboardBridge::stop()
{
  m_stopping = true;
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void WlClipboardBridge::setData(const std::string &mime, const std::string &data)
{
  {
    std::scoped_lock lock{m_mutex};
    if (mime == m_lastWrittenMime && data == m_lastWrittenData) {
      return;
    }
    // Remember it first so the watch event caused by wl-copy is ignored.
    m_lastWrittenMime = mime;
    m_lastWrittenData = data;
  }

  const auto command = withMime(m_commands.copy, QString::fromStdString(mime));
  QProcess process;
  // wl-copy forks a server that keeps serving the selection; don't keep pipes open to it.
  process.setStandardOutputFile(QProcess::nullDevice());
  process.setStandardErrorFile(QProcess::nullDevice());
  process.start(command.first(), command.mid(1));
  if (!process.waitForStarted(kProcessTimeoutMs)) {
    LOG_WARN("failed to start wl-copy: %s", qPrintable(process.errorString()));
    return;
  }

  process.write(data.data(), static_cast<qint64>(data.size()));
  process.closeWriteChannel();
  if (!process.waitForFinished(kProcessTimeoutMs) || process.exitCode() != 0) {
    LOG_WARN("wl-copy did not finish successfully");
    return;
  }

  LOG_DEBUG("wayland clipboard set, %s, %zu bytes", mime.c_str(), data.size());
}

QStringList WlClipboardBridge::listTypes() const
{
  QProcess process;
  process.setStandardErrorFile(QProcess::nullDevice());
  process.start(m_commands.list.first(), m_commands.list.mid(1));
  if (!process.waitForFinished(kProcessTimeoutMs)) {
    process.kill();
    process.waitForFinished();
    return {};
  }

  // wl-paste exits non-zero when the clipboard is empty.
  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    return {};
  }

  return QString::fromUtf8(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
}

std::string WlClipboardBridge::readMime(const QString &mime) const
{
  const auto command = withMime(m_commands.paste, mime);
  QProcess process;
  process.setStandardErrorFile(QProcess::nullDevice());
  process.start(command.first(), command.mid(1));
  if (!process.waitForFinished(kProcessTimeoutMs)) {
    process.kill();
    process.waitForFinished();
    return {};
  }

  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    return {};
  }

  return process.readAllStandardOutput().toStdString();
}

WlClipboardBridge::MimeData WlClipboardBridge::readData() const
{
  const auto types = listTypes();
  MimeData data;

  const auto read = [this, &data](const QString &mime, const char *key) {
    if (auto bytes = readMime(mime); !bytes.empty()) {
      data[key] = std::move(bytes);
    }
  };

  if (types.contains(QLatin1String(kMimePng))) {
    read(QLatin1String(kMimePng), kMimePng);
  }

  if (types.contains(QLatin1String(kMimeHtml))) {
    read(QLatin1String(kMimeHtml), kMimeHtml);
  }

  for (const auto &mime :
       {QStringLiteral("text/plain;charset=utf-8"), QStringLiteral("UTF8_STRING"), QStringLiteral("text/plain")}) {
    if (types.contains(mime)) {
      read(mime, kMimeText);
      break;
    }
  }

  return data;
}

bool WlClipboardBridge::toClipboard(const MimeData &data, IClipboard *clipboard)
{
  using enum IClipboard::Format;
  bool added = false;

  if (const auto it = data.find(kMimeText); it != data.end()) {
    clipboard->add(Text, it->second);
    added = true;
  }

  if (const auto it = data.find(kMimeHtml); it != data.end()) {
    // Some apps (e.g. Firefox) serve text/html as UTF-16.
    clipboard->add(HTML, Unicode::isUTF8(it->second) ? it->second : Unicode::UTF16ToUTF8(it->second));
    added = true;
  }

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (const auto it = data.find(kMimePng); it != data.end()) {
    const auto png = QByteArray::fromStdString(it->second);
    if (const auto dib = PortalClipboard::decodeFormat(Bitmap, png); !dib.isEmpty()) {
      clipboard->add(Bitmap, dib.toStdString());
      added = true;
    }
  }
#endif

  return added;
}

std::pair<std::string, std::string> WlClipboardBridge::fromClipboard(const IClipboard *clipboard)
{
  using enum IClipboard::Format;

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (clipboard->has(Bitmap)) {
    const auto dib = QByteArray::fromStdString(clipboard->get(Bitmap));
    if (const auto png = PortalClipboard::encodeFormat(Bitmap, dib); !png.isEmpty()) {
      return {kMimePng, png.toStdString()};
    }
  }
#endif

  if (clipboard->has(Text)) {
    return {kMimeText, clipboard->get(Text)};
  }

  if (clipboard->has(HTML)) {
    return {kMimeHtml, clipboard->get(HTML)};
  }

  return {};
}

bool WlClipboardBridge::isOwnWrite(const MimeData &data) const
{
  // wl-copy may add aliases (e.g. text/plain for text/html), so only compare what we wrote.
  const auto it = data.find(m_lastWrittenMime);
  return it != data.end() && it->second == m_lastWrittenData;
}

void WlClipboardBridge::handleChange()
{
  const auto data = readData();
  if (data.empty()) {
    return;
  }

  {
    std::scoped_lock lock{m_mutex};
    if (data == m_lastRead) {
      return;
    }
    m_lastRead = data;
    if (isOwnWrite(data)) {
      return;
    }
  }

  LOG_DEBUG("wayland clipboard changed, %zu formats", data.size());
  m_onChanged(data);
}

void WlClipboardBridge::watchLoop()
{
  QProcess watcher;
  watcher.setStandardErrorFile(QProcess::nullDevice());
  watcher.start(m_commands.watch.first(), m_commands.watch.mid(1));
  if (!watcher.waitForStarted(kProcessTimeoutMs)) {
    LOG_ERR("failed to start wl-paste watcher: %s", qPrintable(watcher.errorString()));
    return;
  }

  while (!m_stopping) {
    if (!watcher.waitForReadyRead(kWatchPollMs)) {
      if (watcher.state() == QProcess::NotRunning) {
        LOG_ERR("wl-paste watcher exited, wayland clipboard bridge stopped");
        return;
      }
      continue;
    }

    // Each line is one clipboard change notification.
    bool changed = false;
    while (watcher.canReadLine()) {
      watcher.readLine();
      changed = true;
    }
    if (changed) {
      handleChange();
    }
  }

  watcher.kill();
  watcher.waitForFinished();
}

} // namespace deskflow
