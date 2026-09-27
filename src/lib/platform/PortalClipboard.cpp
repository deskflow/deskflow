/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"

#include "base/Log.h"
#include "platform/ClipboardImage.h"
#include "platform/EiClipboard.h"

#include <poll.h>
#include <unistd.h>

#include <QByteArrayList>
#include <QFile>
#include <QList>
#include <QPair>
#include <QSet>
#include <QVarLengthArray>

namespace deskflow {

QByteArray PortalClipboard::formatMimeTypes(const char *const *mimeTypes)
{
  if (!mimeTypes || !mimeTypes[0])
    return QByteArrayLiteral("(none)");

  QByteArrayList parts;
  for (int i = 0; mimeTypes[i]; ++i)
    parts.append(mimeTypes[i]);
  return parts.join(", ");
}

const PortalClipboard::SupportedMime *PortalClipboard::findSupportedMime(const char *mime)
{
  if (!mime)
    return nullptr;

  for (const auto &entry : kSupportedMimes) {
    if (g_strcmp0(mime, entry.mime) == 0)
      return &entry;
  }

  return nullptr;
}

const PortalClipboard::SupportedMime *PortalClipboard::pickSupportedMime(const char *const *available)
{
  if (!available)
    return nullptr;

  for (const auto &entry : kSupportedMimes) {
    if (g_strv_contains(available, entry.mime))
      return &entry;
  }

  return nullptr;
}

QByteArray PortalClipboard::encodeFormat(IClipboard::Format format, const QByteArray &data)
{
  if (data.isEmpty())
    return {};

  if (format == IClipboard::Format::Bitmap)
    return ClipboardImage::dibToImage(data, "PNG");
  return data;
}

QByteArray PortalClipboard::decodeFormat(IClipboard::Format format, const QByteArray &bytes)
{
  if (bytes.isEmpty())
    return {};

  if (format == IClipboard::Format::Bitmap)
    return ClipboardImage::imageToDib(bytes, "PNG");
  return bytes;
}

QByteArray PortalClipboard::readSelectionBytes(XdpSession *session, const char *mime, qint64 maxBytes)
{
  const int fd = xdp_session_selection_read(session, mime);
  if (fd < 0) {
    LOG_ERR("failed to read clipboard selection: invalid fd");
    return {};
  }

  QFile pipe;
  if (!pipe.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
    LOG_WARN("failed to wrap clipboard pipe");
    ::close(fd);
    return {};
  }

  QByteArray contents;
  contents.reserve(std::min<qint64>(maxBytes, kChunkBytes));
  while (contents.size() < maxBytes) {
    pollfd pfd{fd, POLLIN, 0};
    if (poll(&pfd, 1, kReadTimeoutMs) <= 0)
      break;

    const auto chunk = pipe.read(std::min<qint64>(kChunkBytes, maxBytes - contents.size()));
    if (chunk.isEmpty())
      break;

    contents.append(chunk);
  }
  return contents;
}

void PortalClipboard::claimOwnership(EiClipboard *cache, XdpSession *session)
{
  if (!cache || !session)
    return;

  cache->open(0);
  QVarLengthArray<const char *, std::size(kSupportedMimes) + 1> mimeTypes;
  for (const auto &entry : kSupportedMimes) {
    if (cache->has(entry.format))
      mimeTypes.append(entry.mime);
  }
  cache->close();

  if (mimeTypes.isEmpty()) {
    LOG_DEBUG("clipboard cache empty, nothing to claim");
    return;
  }
  mimeTypes.append(nullptr);

  LOG_DEBUG("claiming clipboard, mimes: %s", formatMimeTypes(mimeTypes.data()).constData());
  xdp_session_set_selection(session, mimeTypes.data());
}

void PortalClipboard::serveSelectionTransfer(EiClipboard *cache, XdpSession *session, const char *mime, uint32_t serial)
{
  LOG_DEBUG("clipboard selection transfer requested, mime: %s, serial: %u", mime, serial);

  const auto *requested = findSupportedMime(mime);
  if (!requested || !cache) {
    LOG_DEBUG("rejecting clipboard selection, unsupported mime: %s", mime);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  cache->open(0);
  QByteArray raw;
  const bool hasFormat = cache->has(requested->format);
  if (hasFormat)
    raw = QByteArray::fromStdString(cache->get(requested->format));
  cache->close();

  // the portal can't withdraw an offer once made, so answer with nothing rather than make the app time out
  if (!hasFormat) {
    LOG_DEBUG("clipboard has no data for mime, serving nothing: %s", mime);
    const int fd = xdp_session_selection_write(session, serial);
    if (fd < 0)
      LOG_WARN("failed to open clipboard selection write fd");
    else
      ::close(fd);
    xdp_session_selection_write_done(session, serial, fd >= 0);
    return;
  }

  const auto data = encodeFormat(requested->format, raw);
  if (data.isEmpty()) {
    LOG_DEBUG("clipboard has no data for mime: %s", mime);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  const int fd = xdp_session_selection_write(session, serial);
  if (fd < 0) {
    LOG_WARN("failed to open clipboard selection write fd");
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  QFile pipe;
  if (!pipe.open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle)) {
    LOG_WARN("failed to wrap clipboard pipe");
    ::close(fd);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  const char *buf = data.constData();
  qint64 total = data.size();
  qint64 written = 0;
  while (written < total) {
    pollfd pfd{fd, POLLOUT, 0};
    if (poll(&pfd, 1, kWriteTimeoutMs) <= 0) {
      LOG_ERR("timed out writing clipboard selection");
      xdp_session_selection_write_done(session, serial, false);
      return;
    }

    qint64 n = pipe.write(buf + written, total - written);
    if (n <= 0) {
      LOG_ERR("clipboard pipe write returned %lld", static_cast<long long>(n));
      xdp_session_selection_write_done(session, serial, false);
      return;
    }
    written += n;
  }

  xdp_session_selection_write_done(session, serial, true);
  const auto size = IClipboard::formatSize(static_cast<size_t>(written));
  LOG_DEBUG("clipboard selection transfer complete, size: %s", size.constData());
}

bool PortalClipboard::readSelectionIntoCache(
    EiClipboard *cache, XdpSession *session, const char *const *mimeTypes, qint64 maxBytes
)
{
  if (!cache || !session || !mimeTypes || !mimeTypes[0])
    return false;

  if (!pickSupportedMime(mimeTypes)) {
    LOG_DEBUG("clipboard no supported mime types: %s", formatMimeTypes(mimeTypes).constData());
    return false;
  }

  QList<QPair<IClipboard::Format, QByteArray>> reads;
  QSet<IClipboard::Format> seen;
  for (const auto &entry : kSupportedMimes) {
    if (seen.contains(entry.format))
      continue;
    if (!g_strv_contains(mimeTypes, entry.mime))
      continue;

    auto bytes = readSelectionBytes(session, entry.mime, maxBytes);
    if (bytes.isEmpty()) {
      LOG_DEBUG("clipboard read returned no data for mime: %s", entry.mime);
      continue;
    }

    if (entry.format == IClipboard::Format::Text || entry.format == IClipboard::Format::HTML) {
      while (bytes.endsWith('\0'))
        bytes.chop(1);
      bytes.replace("\r\n", "\n");
    }

    auto data = decodeFormat(entry.format, bytes);
    if (data.isEmpty())
      continue;

    reads.append({entry.format, std::move(data)});
    seen.insert(entry.format);
  }

  if (reads.isEmpty()) {
    LOG_DEBUG("clipboard read produced no data, leaving existing clipboard intact");
    return false;
  }

  cache->open(0);
  cache->empty();
  for (const auto &[format, data] : reads)
    cache->add(format, data.toStdString());
  cache->close();

  LOG_DEBUG("clipboard read local selection, formats: %lld", static_cast<long long>(reads.size()));
  return true;
}

} // namespace deskflow
