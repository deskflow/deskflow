/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"

#include "base/Log.h"
#include "deskflow/ClipboardChunk.h"
#include "platform/ClipboardImage.h"
#include "platform/EiClipboard.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include <QByteArrayList>
#include <QElapsedTimer>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QPair>
#include <QSet>
#include <QVarLengthArray>

namespace deskflow {

class PortalClipboard::SelectionPipe : public QIODevice
{
public:
  explicit SelectionPipe(int fd) : m_fd(fd)
  {
    // a reader that stops reading without closing the pipe would otherwise block the write forever
    fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL) | O_NONBLOCK);
    open(QIODevice::WriteOnly | QIODevice::Unbuffered);
  }

  ~SelectionPipe() override
  {
    ::close(m_fd);
  }

  qint64 writtenBytes() const
  {
    return m_written;
  }

protected:
  qint64 readData(char *, qint64) override
  {
    return -1;
  }

  qint64 writeData(const char *data, qint64 size) override
  {
    qint64 written = 0;
    bool failed = false;
    while (written < size && !failed) {
      pollfd pfd{m_fd, POLLOUT, 0};
      if (poll(&pfd, 1, kWriteTimeoutMs) <= 0) {
        LOG_ERR("timed out writing clipboard selection");
        failed = true;
      } else if (const auto n = ::write(m_fd, data + written, size - written); n >= 0) {
        written += n;
      } else if (errno != EAGAIN) {
        LOG_ERR("clipboard pipe write failed: %s", std::strerror(errno));
        failed = true;
      }
    }
    m_written += written;
    return failed ? -1 : written;
  }

private:
  int m_fd;
  qint64 m_written = 0;
};

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

bool PortalClipboard::writeFormat(const SupportedMime &entry, const QByteArray &data, QIODevice *device)
{
  if (entry.format == IClipboard::Format::Bitmap)
    return ClipboardImage::writeDibAsImage(data, entry.imageFormat, device);
  return device->write(data) == data.size();
}

QByteArray PortalClipboard::decodeFormat(const SupportedMime &entry, const QByteArray &bytes)
{
  if (bytes.isEmpty())
    return {};

  if (entry.format == IClipboard::Format::Bitmap)
    return ClipboardImage::imageToDib(bytes, entry.imageFormat);
  return bytes;
}

std::optional<QByteArray> PortalClipboard::readSelectionBytes(XdpSession *session, const char *mime, qint64 maxBytes)
{
  const int fd = xdp_session_selection_read(session, mime);
  if (fd < 0) {
    LOG_ERR("failed to read clipboard selection: invalid fd");
    return QByteArray{};
  }

  QFile pipe;
  if (!pipe.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
    LOG_WARN("failed to wrap clipboard pipe");
    ::close(fd);
    return QByteArray{};
  }

  QByteArray contents;
  contents.reserve(std::min<qint64>(maxBytes, kChunkBytes));
  bool timedOut = false;
  while (contents.size() < maxBytes && !timedOut) {
    pollfd pfd{fd, POLLIN, 0};
    timedOut = poll(&pfd, 1, kReadTimeoutMs) <= 0;
    if (!timedOut) {
      const auto chunk = pipe.read(std::min<qint64>(kChunkBytes, maxBytes - contents.size()));
      if (chunk.isEmpty())
        break;

      contents.append(chunk);
    }
  }

  if (timedOut)
    LOG_WARN("clipboard read timed out, mime: %s, waited: %d ms", mime, kReadTimeoutMs);
  return timedOut ? std::nullopt : std::make_optional(std::move(contents));
}

void PortalClipboard::claimOwnership(EiClipboard *cache, XdpSession *session)
{
  if (!cache || !session)
    return;

  cache->open(0);
  QVarLengthArray<const char *, std::size(kSupportedMimes) + 1> mimeTypes;
  for (const auto &entry : kSupportedMimes) {
    if (entry.offered && cache->has(entry.format))
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

  const int fd = xdp_session_selection_write(session, serial);
  if (fd < 0) {
    LOG_WARN("failed to open clipboard selection write fd");
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  QElapsedTimer sinceStart;
  sinceStart.start();
  bool served = false;
  qint64 written = 0;
  {
    // streamed, so a large image reaches the app as it's encoded rather than after, which apps give up waiting for
    SelectionPipe pipe(fd);
    served = writeFormat(*requested, raw, &pipe);
    written = pipe.writtenBytes();
  }

  xdp_session_selection_write_done(session, serial, served);
  const auto transfer = ClipboardChunk::describeTransfer(static_cast<size_t>(written), sinceStart.elapsed());
  if (served)
    LOG_DEBUG("clipboard selection transfer complete: %s", transfer.constData());
  else
    LOG_WARN("clipboard selection transfer failed, sent: %s", transfer.constData());
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

    auto selection = readSelectionBytes(session, entry.mime, maxBytes);

    // the source app is still busy with this request, and each further one would queue behind it
    if (!selection)
      break;

    auto bytes = std::move(*selection);
    if (bytes.isEmpty()) {
      LOG_DEBUG("clipboard read returned no data for mime: %s", entry.mime);
      continue;
    }

    if (entry.format == IClipboard::Format::Text || entry.format == IClipboard::Format::HTML) {
      while (bytes.endsWith('\0'))
        bytes.chop(1);
      bytes.replace("\r\n", "\n");
    }

    auto data = decodeFormat(entry, bytes);
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
