/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboard.h"

#include "base/Log.h"
#include "common/Settings.h"

#include <algorithm>

#include <QDateTime>
#include <QElapsedTimer>
#include <QProcess>
#include <QStandardPaths>

namespace {

inline static const auto s_copyApp = QStringLiteral("wl-copy");
inline static const auto s_pasteApp = QStringLiteral("wl-paste");

// wl-clipboard args
inline static const auto s_listTypes = QStringLiteral("--list-types");
inline static const auto s_clear = QStringLiteral("--clear");
inline static const auto s_isPrimary = QStringLiteral("--primary");
inline static const auto s_noNewLine = QStringLiteral("-n");
inline static const auto s_readType = QStringLiteral("-t%1");

// MIME types for different clipboard formats
inline static const auto s_mimeTypeText = QStringLiteral("text/plain;charset=utf-8");
inline static const auto s_mimeTypeHtml = QStringLiteral("text/html");
inline static const auto s_mimeTypeBmp = QStringLiteral("image/bmp");

const int kCacheValidityMs = 100;
const int kListTypesTimeoutMs = 2000;
const int kPasteTimeoutMs = 3000;
const int kCopyTimeoutMs = 5000;

// reading this much of the content is enough to tell two clipboards apart
const qsizetype kFingerprintMaxBytes = 256 * 1024;

size_t contentHash(const QByteArray &data)
{
  return qHash(data);
}

} // namespace

WlClipboard::WlClipboard(ClipboardID id) : m_id(id), m_useClipboard(id == kClipboardClipboard)
{
  // do nothing
}

ClipboardID WlClipboard::getID() const
{
  return m_id;
}

bool WlClipboard::isAvailable()
{
  return !QStandardPaths::findExecutable(s_copyApp).isEmpty() && !QStandardPaths::findExecutable(s_pasteApp).isEmpty();
}

bool WlClipboard::isEnabled()
{
  return Settings::value(Settings::Core::UseWlClipboard).toBool();
}

bool WlClipboard::hasChanged() const
{
  const auto types = getAvailableMimeTypes();

  // hash the content of the first offered type, capped, so copies that
  // keep the same mime types are still detected
  size_t hash = 0;
  if (!types.isEmpty()) {
    hash =
        contentHash(readClipboard({s_noNewLine, s_readType.arg(types.first())}, kFingerprintMaxBytes, kPasteTimeoutMs));
  }

  if (m_haveFingerprint && types == m_lastTypes && hash == m_lastContentHash) {
    return false;
  }

  const bool isBaseline = !m_haveFingerprint;
  const bool isOwnWrite = m_ownWriteHash.has_value() && hash == *m_ownWriteHash;

  m_haveFingerprint = true;
  m_lastTypes = types;
  m_lastContentHash = hash;

  if (isBaseline || isOwnWrite) {
    // the first observation, or our own wl-copy, is not an external change
    return false;
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  invalidateCache();
  return true;
}

void WlClipboard::resetChanged()
{
  // Clear cache to force fresh data retrieval
  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  invalidateCache();
}

bool WlClipboard::empty()
{
  if (!m_open) {
    return false;
  }

  m_pendingWrite = true;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_pendingData[i].clear();
    m_pendingAvailable[i] = false;
  }

  return true;
}

void WlClipboard::add(Format format, const std::string &data)
{
  if (!m_open) {
    return;
  }

  const auto index = static_cast<int>(format);
  if (index < 0 || index >= static_cast<int>(Format::TotalFormats)) {
    LOG_WARN("unsupported clipboard format: %d", format);
    return;
  }

  m_pendingWrite = true;
  m_pendingData[index] = data;
  m_pendingAvailable[index] = true;
}

bool WlClipboard::open(Time time) const
{
  if (m_open) {
    LOG_DEBUG("failed to open clipboard: already opened");
    return false;
  }

  m_open = true;
  m_time = time;

  return true;
}

void WlClipboard::close() const
{
  if (!m_open) {
    return;
  }

  LOG_DEBUG("close clipboard");

  if (m_pendingWrite) {
    commitPendingData();
  }
  m_open = false;
}

void WlClipboard::commitPendingData() const
{
  // wl-copy offers a single mime type per invocation, so pick the most
  // valuable pending format
  static constexpr Format formatPriority[] = {Format::Bitmap, Format::Text, Format::HTML};
  int chosen = -1;
  for (const auto format : formatPriority) {
    if (m_pendingAvailable[static_cast<int>(format)]) {
      chosen = static_cast<int>(format);
      break;
    }
  }

  QProcess cmd;
  cmd.setProgram(s_copyApp);

  QStringList args;
  if (!m_useClipboard) {
    args.append(s_isPrimary);
  }

  if (chosen == -1) {
    // emptied with nothing added, clear the system clipboard
    args.append(s_clear);
    cmd.setArguments(args);
    cmd.start();
  } else {
    args.append(s_noNewLine);
    args.append(s_readType.arg(formatToMimeType(static_cast<Format>(chosen))));
    cmd.setArguments(args);
    cmd.start();
    if (!cmd.waitForStarted(1000)) {
      LOG_WARN("failed to start %s", qPrintable(s_copyApp));
      m_pendingWrite = false;
      return;
    }
    // pass the data via stdin, argv cannot carry binary data
    const auto &data = m_pendingData[chosen];
    cmd.write(data.data(), static_cast<qint64>(data.size()));
    cmd.closeWriteChannel();
  }

  // wl-copy forks a child to serve the selection and exits once stdin is consumed
  if (!cmd.waitForFinished(kCopyTimeoutMs)) {
    LOG_WARN("%s did not finish in time", qPrintable(s_copyApp));
    cmd.kill();
    cmd.waitForFinished(100);
  }

  // remember what we wrote so hasChanged() does not report it as a change
  if (chosen == -1) {
    m_ownWriteHash = 0; // an empty clipboard hashes to 0
  } else {
    const auto &written = m_pendingData[chosen];
    const auto capped = static_cast<qsizetype>(std::min<size_t>(written.size(), kFingerprintMaxBytes));
    m_ownWriteHash = contentHash(QByteArray(written.data(), capped));
  }

  {
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    invalidateCache();
    if (chosen != -1) {
      m_cachedData[chosen] = m_pendingData[chosen];
      m_cachedAvailable[chosen] = true;
    }
    m_cached = true;
    m_cachedTime = getCurrentTime();
  }

  m_pendingWrite = false;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_pendingData[i].clear();
    m_pendingAvailable[i] = false;
  }
}

IClipboard::Time WlClipboard::getTime() const
{
  return m_time;
}

bool WlClipboard::has(Format format) const
{
  if (!m_open) {
    return false;
  }

  const auto index = static_cast<int>(format);

  // an uncommitted write transaction is answered from the pending data
  if (m_pendingWrite) {
    return m_pendingAvailable[index];
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Check cache validity
  Time currentTime = getCurrentTime();
  if (m_cached && (currentTime - m_cachedTime) < kCacheValidityMs) {
    return m_cachedAvailable[index];
  }

  if (const auto availableTypes = getAvailableMimeTypes(); availableTypes.isEmpty()) {
    // No types available - mark all formats as unavailable
    for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
      m_cachedAvailable[i] = false;
      m_cachedData[i].clear();
    }
  } else {
    using enum IClipboard::Format;
    // Check each format against available types
    for (int i = 0; i < static_cast<int>(TotalFormats); ++i) {
      auto currentFormat = static_cast<Format>(i);
      const auto mimeType = formatToMimeType(currentFormat);

      m_cachedAvailable[i] = false;
      if (!mimeType.isEmpty()) {
        for (const auto &available : availableTypes) {
          if (available == mimeType || (currentFormat == Text && available == QStringLiteral("text/plain")) ||
              (currentFormat == HTML && available.startsWith(QStringLiteral("text/html")))) {
            m_cachedAvailable[i] = true;
            break;
          }
        }
      }
    }
  }

  m_cached = true;
  m_cachedTime = currentTime;

  return m_cachedAvailable[index];
}

std::string WlClipboard::get(Format format) const
{
  if (!m_open) {
    return std::string();
  }

  const auto index = static_cast<int>(format);

  // an uncommitted write transaction is answered from the pending data
  if (m_pendingWrite) {
    return m_pendingData[index];
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Return cached data if available and valid
  if (m_cached && m_cachedAvailable[index] && !m_cachedData[index].empty()) {
    return m_cachedData[index];
  }

  auto mimeType = formatToMimeType(format);
  if (mimeType.isEmpty()) {
    return std::string();
  }

  const auto data = readClipboard({s_noNewLine, s_readType.arg(mimeType)}, -1, kPasteTimeoutMs).toStdString();

  // Update cache
  m_cachedData[index] = data;
  m_cachedAvailable[index] = !data.empty();
  m_cached = true;
  m_cachedTime = getCurrentTime();

  return data;
}

QString WlClipboard::formatToMimeType(Format format) const
{
  switch (format) {
    using enum IClipboard::Format;
  case Text:
    return s_mimeTypeText;
  case HTML:
    return s_mimeTypeHtml;
  case Bitmap:
    return s_mimeTypeBmp;
  default:
    return {};
  }
}

QStringList WlClipboard::getAvailableMimeTypes() const
{
  const auto data = readClipboard({s_listTypes}, 64 * 1024, kListTypesTimeoutMs);
  return QString::fromUtf8(data).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
}

QByteArray WlClipboard::readClipboard(const QStringList &args, qsizetype maxBytes, int timeoutMs) const
{
  QProcess cmd;
  cmd.setProgram(s_pasteApp);

  QStringList allArgs = args;
  if (!m_useClipboard) {
    allArgs.append(s_isPrimary);
  }
  cmd.setArguments(allArgs);
  cmd.start();

  QElapsedTimer timer;
  timer.start();

  QByteArray out;
  while (cmd.state() != QProcess::NotRunning && timer.elapsed() < timeoutMs && (maxBytes < 0 || out.size() < maxBytes)
  ) {
    cmd.waitForReadyRead(50);
    out.append(cmd.readAllStandardOutput());
  }
  out.append(cmd.readAllStandardOutput());

  if (cmd.state() != QProcess::NotRunning) {
    cmd.kill();
    cmd.waitForFinished(100);
  }

  if (maxBytes >= 0 && out.size() > maxBytes) {
    out.truncate(maxBytes);
  }
  return out;
}

IClipboard::Time WlClipboard::getCurrentTime() const
{
  return static_cast<Time>(QDateTime::currentMSecsSinceEpoch());
}

void WlClipboard::invalidateCache() const
{
  m_cached = false;
  m_cachedTime = 0;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedData[i].clear();
    m_cachedAvailable[i] = false;
  }
}
