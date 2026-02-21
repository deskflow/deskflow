/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboard.h"

#include "base/Log.h"
#include "platform/ClipboardImageConverter.h"

#include <chrono>
#include <common/Settings.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <QDateTime>
#include <QProcess>
#include <QStandardPaths>

namespace {

inline static const auto s_copyApp = QStringLiteral("wl-copy");
inline static const auto s_pasteApp = QStringLiteral("wl-paste");

// wl-clipboard args
inline static const auto s_listTypes = QStringLiteral("--list-types");
inline static const auto s_isPrimary = QStringLiteral("--primary");
inline static const auto s_noNewLine = QStringLiteral("-n");
inline static const auto s_readType = QStringLiteral("-t%1");

// MIME types for different clipboard formats
inline static const auto s_mimeTypeText = QStringLiteral("text/plain;charset=utf-8");
inline static const auto s_mimeTypeHtml = QStringLiteral("text/html");
inline static const auto s_mimeTypeBmp = QStringLiteral("image/bmp");
inline static const auto s_mimeTypeBmpX = QStringLiteral("image/x-bmp");
inline static const auto s_mimeTypeBmpMS = QStringLiteral("image/x-MS-bmp");
inline static const auto s_mimeTypeBmpWin = QStringLiteral("image/x-win-bitmap");
inline static const auto s_mimeTypePng = QStringLiteral("image/png");
inline static const auto s_mimeTypeTiff = QStringLiteral("image/tiff");

// Additional HTML MIME type variants
const char *const s_mimeTypeHtmlUtf8 = "text/html;charset=UTF-8";
const char *const s_mimeTypeHtmlWindows = "HTML Format";

// Command timeout (milliseconds)
const int kCacheValidityMs = 100;
const int kMonitorIntervalMs = 1000;
const int kMaxConsecutiveErrors = 5;
const int kClipboardSettleChecks = 3;
const int kClipboardSettleDelayMs = 75;
const int kBitmapRetryChecks = 3;
const int kBitmapRetryDelayMs = 35;
const int kBitmapSettleChecks = 60;
const int kBitmapSettleDelayMs = 50;

QString sanitizeClipboardText(QString text)
{
  // Screenshot tools may prefix filenames with invisible formatting chars.
  QString sanitized;
  sanitized.reserve(text.size());
  for (const auto ch : text) {
    const auto category = ch.category();
    if (category == QChar::Other_Format || category == QChar::Other_Control) {
      continue;
    }
    sanitized.append(ch);
  }
  return sanitized.trimmed();
}

bool looksLikeImageFileName(const QString &text)
{
  const auto sanitized = sanitizeClipboardText(text);
  if (sanitized.isEmpty()) {
    return false;
  }

  static const QStringList imageExtensions = {QStringLiteral(".png"),  QStringLiteral(".jpg"),  QStringLiteral(".jpeg"),
                                              QStringLiteral(".bmp"),  QStringLiteral(".gif"),  QStringLiteral(".tif"),
                                              QStringLiteral(".tiff"), QStringLiteral(".webp"), QStringLiteral(".heic"),
                                              QStringLiteral(".heif"), QStringLiteral(".avif")};

  auto lines = sanitized.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
  if (lines.isEmpty()) {
    lines.push_back(sanitized);
  }

  for (const auto &line : lines) {
    auto lower = sanitizeClipboardText(line).toLower();
    if (lower.isEmpty() || lower.startsWith(QLatin1Char('#'))) {
      continue;
    }

    if ((lower.startsWith(QLatin1Char('"')) && lower.endsWith(QLatin1Char('"'))) ||
        (lower.startsWith(QLatin1Char('\'')) && lower.endsWith(QLatin1Char('\'')))) {
      lower = lower.mid(1, lower.size() - 2).trimmed();
    }

    bool hasImageExtension = false;
    for (const auto &ext : imageExtensions) {
      if (lower.endsWith(ext) || lower.contains(ext + QLatin1Char('"')) || lower.contains(ext + QLatin1Char('\'')) ||
          lower.contains(ext + QLatin1Char('<'))) {
        hasImageExtension = true;
        break;
      }
    }

    if (lower.startsWith(QStringLiteral("screenshot")) || lower.contains(QStringLiteral("screenshot from "))) {
      return true;
    }
    if ((lower.startsWith(QStringLiteral("file://")) || lower.contains(QStringLiteral("file://"))) &&
        (hasImageExtension || lower.contains(QStringLiteral("screenshot")))) {
      return true;
    }
    if ((lower.startsWith(QLatin1Char('/')) || lower.startsWith(QLatin1String("./")) ||
         lower.startsWith(QLatin1String("../"))) &&
        hasImageExtension) {
      return true;
    }
  }

  return false;
}
} // namespace

WlClipboard::WlClipboard(ClipboardID id) : m_id(id), m_useClipboard(id == kClipboardClipboard)
{
  // Initialize cached data
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedAvailable[i] = false;
  }
}

WlClipboard::~WlClipboard()
{
  stopMonitoring();
  for (auto &cmd : m_runningWlCopies) {
    cmd->close();
    cmd->waitForFinished(100);
  }
  m_runningWlCopies.clear();
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

void WlClipboard::startMonitoring()
{
  if (m_monitoring) {
    return;
  }
  m_stopMonitoring = false;
  m_monitoring = true;
  m_monitorThread = std::make_unique<std::thread>(&WlClipboard::monitorClipboard, this);
}

void WlClipboard::stopMonitoring()
{
  if (!m_monitoring) {
    return;
  }

  m_stopMonitoring = true;
  m_monitoring = false;

  if (m_monitorThread && m_monitorThread->joinable()) {
    m_monitorThread->join();
  }
  m_monitorThread.reset();
}

bool WlClipboard::hasChanged() const
{
  return m_hasChanged.load();
}

bool WlClipboard::empty()
{
  if (!m_open) {
    return false;
  }
  auto cmd = new QProcess(this);
  cmd->setProgram(s_copyApp);
  m_runningWlCopies.append(cmd);
  connect(cmd, &QProcess::finished, this, [this, cmd] { m_runningWlCopies.removeAll(cmd); });

  QStringList args = {s_noNewLine, ""};
  if (!m_useClipboard)
    args.prepend(s_isPrimary);

  cmd->setArguments(args);
  cmd->start();
  bool success = cmd->waitForStarted(100);

  if (success) {
    // Update ownership and cache only if command succeeded
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    updateOwnership(true);
    invalidateCache();
  }

  return success;
}

void WlClipboard::add(Format format, const std::string &data)
{
  if (!m_open) {
    return;
  }

  if (format == Format::HTML) {
    return;
  }

  auto mimeType = formatToMimeType(format);
  if (mimeType.isEmpty()) {
    LOG_WARN("unsupported clipboard format: %d", format);
    return;
  }

  std::string payload = data;
  if (format == Format::Bitmap) {
    payload = deskflow::platform::clipboard::encodeBitmapToImage(data, "PNG");
    if (payload.empty()) {
      LOG_WARN("failed to convert bitmap clipboard data to PNG for wayland");
      return;
    }
  }

  auto cmd = new QProcess(this);
  cmd->setProgram(s_copyApp);

  m_runningWlCopies.append(cmd);
  connect(cmd, &QProcess::finished, this, [this, cmd] { m_runningWlCopies.removeAll(cmd); });

  QStringList args = {s_noNewLine, s_readType.arg(mimeType)};
  if (!m_useClipboard)
    args.prepend(s_isPrimary);

  cmd->setArguments(args);
  cmd->start();

  if (!cmd->waitForStarted(100)) {
    return;
  }

  const auto payloadSize = static_cast<qint64>(payload.size());
  qint64 totalWritten = 0;
  while (totalWritten < payloadSize) {
    const auto bytesToWrite = payloadSize - totalWritten;
    const auto written = cmd->write(payload.data() + totalWritten, bytesToWrite);
    if (written <= 0) {
      LOG_WARN("failed writing clipboard payload to wl-copy");
      return;
    }
    totalWritten += written;
    if (!cmd->waitForBytesWritten(1000)) {
      LOG_WARN("timed out writing clipboard payload to wl-copy");
      return;
    }
  }

  cmd->closeWriteChannel();

  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  updateOwnership(true);
  invalidateCache();
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

  m_open = false;
  const_cast<WlClipboard *>(this)->invalidateCache();
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

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  const auto refreshAvailability = [this]() {
    if (const auto availableTypes = getAvailableMimeTypes(); availableTypes.isEmpty()) {
      // No types available - mark all formats as unavailable
      for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
        m_cachedAvailable[i] = false;
        m_cachedData[i].clear();
      }
    } else {
      using enum IClipboard::Format;
      const auto bitmapMime = getBestMimeTypeForRead(Bitmap, availableTypes);
      const bool bitmapFlavorPresent = !bitmapMime.isEmpty();
      const bool bitmapReady = bitmapFlavorPresent && isBitmapDataReady(availableTypes);
      const bool suppressScreenshotLabelText = hasScreenshotPlaceholderText(availableTypes) && bitmapFlavorPresent;
      // Check each format against available types
      for (int i = 0; i < static_cast<int>(TotalFormats); ++i) {
        auto currentFormat = static_cast<Format>(i);
        m_cachedAvailable[i] = false;
        if (currentFormat == Bitmap) {
          m_cachedAvailable[i] = bitmapReady;
          continue;
        }
        if ((currentFormat == Text || currentFormat == HTML) && suppressScreenshotLabelText) {
          m_cachedAvailable[i] = false;
          continue;
        }
        for (const auto &available : availableTypes) {
          if (mimeTypeMatchesFormat(currentFormat, available)) {
            m_cachedAvailable[i] = true;
            break;
          }
        }
      }
    }

    m_cached = true;
    m_cachedTime = getCurrentTime();
  };

  // Check cache validity
  Time currentTime = getCurrentTime();
  const bool cacheValid = m_cached && (currentTime - m_cachedTime) < kCacheValidityMs;
  if (!cacheValid) {
    refreshAvailability();
  } else if (format == Format::Bitmap && !m_cachedAvailable[static_cast<int>(Format::Bitmap)] &&
             m_cachedAvailable[static_cast<int>(Format::Text)]) {
    // Some screenshot tools briefly expose only a text filename/URI before
    // image flavors are ready. Re-check a few times to avoid syncing only text.
    for (int attempt = 0; attempt < kBitmapRetryChecks; ++attempt) {
      std::this_thread::sleep_for(std::chrono::milliseconds(kBitmapRetryDelayMs));
      refreshAvailability();
      if (m_cachedAvailable[static_cast<int>(Format::Bitmap)] || !m_cachedAvailable[static_cast<int>(Format::Text)]) {
        break;
      }
    }
  }

  return m_cachedAvailable[static_cast<int>(format)];
}

std::string WlClipboard::get(Format format) const
{
  if (!m_open) {
    return std::string();
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Return cached data if available and valid
  if (m_cached && m_cachedAvailable[static_cast<int>(format)] && !m_cachedData[static_cast<int>(format)].empty()) {
    return m_cachedData[static_cast<int>(format)];
  }

  const bool textLikeFormat = format == Format::Text || format == Format::HTML;
  const int maxAttempts = textLikeFormat ? kBitmapSettleChecks : kBitmapRetryChecks;
  const int retryDelayMs = textLikeFormat ? kBitmapSettleDelayMs : kBitmapRetryDelayMs;

  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    const auto availableTypes = getAvailableMimeTypes();
    const auto bitmapMime = getBestMimeTypeForRead(Format::Bitmap, availableTypes);
    const bool bitmapFlavorPresent = !bitmapMime.isEmpty();
    const bool bitmapReady = bitmapFlavorPresent && isBitmapDataReady(availableTypes);

    if (textLikeFormat && hasScreenshotPlaceholderText(availableTypes) && bitmapFlavorPresent) {
      if (!bitmapReady && attempt + 1 < maxAttempts) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        continue;
      }
      return {};
    }

    auto mimeType = getBestMimeTypeForRead(format, availableTypes);

    // For bitmap reads, wait briefly if flavors are still being populated.
    if (mimeType.isEmpty()) {
      if (format == Format::Bitmap && attempt + 1 < maxAttempts) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        continue;
      }
      return {};
    }

    auto data = readMimeData(mimeType);

    if (format == Format::Bitmap) {
      const char *formatHint = nullptr;
      if (mimeType == s_mimeTypePng) {
        formatHint = "PNG";
      } else if (mimeType == s_mimeTypeTiff) {
        formatHint = "TIFF";
      } else {
        formatHint = "BMP";
      }

      data = deskflow::platform::clipboard::decodeImageToBitmap(data, formatHint);
      if (data.empty()) {
        if (attempt + 1 < maxAttempts) {
          std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
          continue;
        }
        return {};
      }
    }

    // Update cache
    m_cachedData[static_cast<int>(format)] = data;
    m_cachedAvailable[static_cast<int>(format)] = !data.empty();
    m_cached = true;
    m_cachedTime = getCurrentTime();

    return data;
  }

  return {};
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
    return s_mimeTypePng;
  default:
    return {};
  }
}

QStringList WlClipboard::formatToMimeTypes(Format format) const
{
  switch (format) {
    using enum IClipboard::Format;
  case Text:
    return {s_mimeTypeText, QStringLiteral("text/plain")};
  case HTML:
    return {s_mimeTypeHtml, QString::fromLatin1(s_mimeTypeHtmlUtf8), QString::fromLatin1(s_mimeTypeHtmlWindows)};
  case Bitmap:
    return {s_mimeTypePng, s_mimeTypeBmp, s_mimeTypeBmpX, s_mimeTypeBmpMS, s_mimeTypeBmpWin, s_mimeTypeTiff};
  default:
    return {};
  }
}

bool WlClipboard::mimeTypeMatchesFormat(Format format, const QString &mimeType) const
{
  using enum IClipboard::Format;
  switch (format) {
  case Text:
    return mimeType == s_mimeTypeText || mimeType == QStringLiteral("text/plain");
  case HTML:
    return mimeType == s_mimeTypeHtml || mimeType == s_mimeTypeHtmlUtf8 || mimeType == s_mimeTypeHtmlWindows ||
           mimeType.startsWith(QStringLiteral("text/html"));
  case Bitmap:
    return mimeType == s_mimeTypePng || mimeType == s_mimeTypeBmp || mimeType == s_mimeTypeBmpX ||
           mimeType == s_mimeTypeBmpMS || mimeType == s_mimeTypeBmpWin || mimeType == s_mimeTypeTiff;
  default:
    return false;
  }
}

QString WlClipboard::getBestMimeTypeForRead(Format format, const QStringList &availableTypes) const
{
  const auto preferredTypes = formatToMimeTypes(format);
  for (const auto &preferred : preferredTypes) {
    for (const auto &available : availableTypes) {
      if (preferred == available) {
        return available;
      }
    }
  }

  // As a fallback, match format families like text/html;charset=... variants.
  for (const auto &available : availableTypes) {
    if (mimeTypeMatchesFormat(format, available)) {
      return available;
    }
  }

  return {};
}

std::string WlClipboard::readMimeData(const QString &mimeType) const
{
  if (mimeType.isEmpty()) {
    return {};
  }

  QProcess cmd;
  cmd.setProgram(s_pasteApp);

  QStringList args = {s_noNewLine, s_readType.arg(mimeType)};
  if (!m_useClipboard)
    args.append(s_isPrimary);

  cmd.setArguments(args);
  cmd.start();
  cmd.waitForFinished();

  return cmd.readAll().toStdString();
}

bool WlClipboard::isBitmapDataReady(const QStringList &availableTypes) const
{
  const auto mimeType = getBestMimeTypeForRead(Format::Bitmap, availableTypes);
  if (mimeType.isEmpty()) {
    return false;
  }

  auto data = readMimeData(mimeType);
  if (data.empty()) {
    return false;
  }

  const char *formatHint = nullptr;
  if (mimeType == s_mimeTypePng) {
    formatHint = "PNG";
  } else if (mimeType == s_mimeTypeTiff) {
    formatHint = "TIFF";
  } else {
    formatHint = "BMP";
  }

  return !deskflow::platform::clipboard::decodeImageToBitmap(data, formatHint).empty();
}

bool WlClipboard::hasScreenshotPlaceholderText(const QStringList &availableTypes) const
{
  const auto hasPlaceholderInMime = [this](const QString &mimeType) {
    if (mimeType.isEmpty()) {
      return false;
    }
    const auto rawData = readMimeData(mimeType);
    if (rawData.empty()) {
      return false;
    }

    const auto text = QString::fromUtf8(rawData.c_str(), static_cast<int>(rawData.size()));
    return looksLikeImageFileName(text);
  };

  if (hasPlaceholderInMime(getBestMimeTypeForRead(Format::Text, availableTypes))) {
    return true;
  }

  if (hasPlaceholderInMime(getBestMimeTypeForRead(Format::HTML, availableTypes))) {
    return true;
  }

  for (const auto &mimeType : availableTypes) {
    if (mimeType == QStringLiteral("text/uri-list") || mimeType == QStringLiteral("x-special/gnome-copied-files")) {
      if (hasPlaceholderInMime(mimeType)) {
        return true;
      }
    }
  }

  return false;
}

QStringList WlClipboard::getAvailableMimeTypes() const
{
  QProcess cmd;
  cmd.setProgram(s_pasteApp);

  QStringList args = {s_listTypes};
  if (!m_useClipboard)
    args.append(s_isPrimary);

  cmd.setArguments(args);
  cmd.start();
  cmd.waitForFinished();

  const static QChar newLine = QLatin1Char('\n');
  auto types = QString::fromLocal8Bit(cmd.readAll()).split(newLine, Qt::SkipEmptyParts);
  for (auto &type : types) {
    type = type.trimmed();
  }
  types.removeAll(QString{});
  types.removeDuplicates();
  types.sort(Qt::CaseSensitive);
  return types;
}

void WlClipboard::monitorClipboard()
{
  QStringList lastTypes;
  bool hasLastScreenshotState = false;
  bool lastBitmapReady = false;
  bool lastHasPlaceholderText = false;
  int consecutiveErrors = 0;

  while (!m_stopMonitoring) {
    std::this_thread::sleep_for(std::chrono::milliseconds(kMonitorIntervalMs));
    try {
      // Check if clipboard content has changed by comparing available types.
      // Let the type list settle briefly to avoid propagating transient
      // text-only states from screenshot tools.
      auto currentTypes = getAvailableMimeTypes();
      auto settledTypes = currentTypes;
      for (int i = 0; i < kClipboardSettleChecks; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kClipboardSettleDelayMs));
        const auto nextTypes = getAvailableMimeTypes();
        if (nextTypes == settledTypes) {
          break;
        }
        settledTypes = nextTypes;
      }

      auto bitmapMime = getBestMimeTypeForRead(Format::Bitmap, settledTypes);
      bool bitmapReady = !bitmapMime.isEmpty() && isBitmapDataReady(settledTypes);
      bool hasPlaceholderText = hasScreenshotPlaceholderText(settledTypes);
      bool waitForBitmap = hasPlaceholderText && !bitmapReady;

      if (waitForBitmap) {
        for (int i = 0; i < kBitmapSettleChecks; ++i) {
          std::this_thread::sleep_for(std::chrono::milliseconds(kBitmapSettleDelayMs));
          const auto nextTypes = getAvailableMimeTypes();
          if (nextTypes != settledTypes) {
            settledTypes = nextTypes;
          }

          bitmapMime = getBestMimeTypeForRead(Format::Bitmap, settledTypes);
          bitmapReady = !bitmapMime.isEmpty() && isBitmapDataReady(settledTypes);
          if (bitmapReady) {
            break;
          }

          hasPlaceholderText = hasScreenshotPlaceholderText(settledTypes);
          if (!hasPlaceholderText) {
            break;
          }
        }
      }

      bitmapMime = getBestMimeTypeForRead(Format::Bitmap, settledTypes);
      bitmapReady = !bitmapMime.isEmpty() && isBitmapDataReady(settledTypes);
      hasPlaceholderText = hasScreenshotPlaceholderText(settledTypes);
      const bool suppressTextOnlyScreenshot = hasPlaceholderText && !bitmapReady;
      const bool screenshotStateChanged =
          hasLastScreenshotState && (lastBitmapReady != bitmapReady || lastHasPlaceholderText != hasPlaceholderText);

      if (suppressTextOnlyScreenshot) {
        if (!hasLastScreenshotState || screenshotStateChanged) {
          LOG_INFO("deferring clipboard update: screenshot placeholder detected before bitmap is ready");
        }
        lastTypes = settledTypes;
        lastBitmapReady = bitmapReady;
        lastHasPlaceholderText = hasPlaceholderText;
        hasLastScreenshotState = true;
        consecutiveErrors = 0;
        continue;
      }

      // Reset error counter on successful operation
      consecutiveErrors = 0;

      if (settledTypes != lastTypes || screenshotStateChanged || !hasLastScreenshotState) {
        if (screenshotStateChanged && settledTypes == lastTypes) {
          LOG_INFO("forcing clipboard update: screenshot bitmap readiness changed without MIME change");
        }
        m_hasChanged = true;
        lastTypes = settledTypes;
        lastBitmapReady = bitmapReady;
        lastHasPlaceholderText = hasPlaceholderText;
        hasLastScreenshotState = true;

        // Clear cache when clipboard changes
        std::scoped_lock<std::mutex> lock(m_cacheMutex);
        invalidateCache();
        updateOwnership(false);
      } else {
        lastBitmapReady = bitmapReady;
        lastHasPlaceholderText = hasPlaceholderText;
        hasLastScreenshotState = true;
      }
    } catch (const std::exception &e) {
      LOG_WARN("clipboard monitoring error: %s", e.what());
      if (++consecutiveErrors >= kMaxConsecutiveErrors) {
        LOG_ERR("too many consecutive errors in clipboard monitoring, stopping");
        break;
      }
    } catch (...) {
      LOG_WARN("clipboard monitoring unknown error");
      if (++consecutiveErrors >= kMaxConsecutiveErrors) {
        LOG_ERR("too many consecutive errors in clipboard monitoring, stopping");
        break;
      }
    }
  }
}

IClipboard::Time WlClipboard::getCurrentTime() const
{
  return static_cast<Time>(QDateTime::currentMSecsSinceEpoch());
}

bool WlClipboard::isOwned() const
{
  return m_owned;
}

void WlClipboard::resetChanged()
{
  m_hasChanged = false;

  // Clear cache when resetting change flag to force fresh data retrieval
  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  invalidateCache();
}

void WlClipboard::updateOwnership(bool owned)
{
  m_owned = owned;
}

void WlClipboard::invalidateCache()
{
  m_cached = false;
  m_cachedTime = 0;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedData[i].clear();
    m_cachedAvailable[i] = false;
  }
}
