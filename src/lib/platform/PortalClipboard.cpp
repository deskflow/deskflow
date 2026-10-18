/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"

#include "base/Log.h"
#include "platform/EiClipboard.h"

#include <cstring>
#include <memory>
#include <poll.h>
#include <unistd.h>

#include <QBuffer>
#include <QByteArrayList>
#include <QDataStream>
#include <QFile>
#include <QImage>
#include <QList>
#include <QPair>
#include <QSet>
#include <QVarLengthArray>
#include <QtEndian>

namespace deskflow {

static constexpr int kBmpSignatureSize = 2;
static constexpr quint32 kBmpFileHeaderSize = 14;
static constexpr quint32 kMinDibHeaderSize = 12;

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

QByteArray PortalClipboard::dibToBmp(const QByteArray &dib)
{
  if (dib.size() < static_cast<qint64>(sizeof(quint32)))
    return {};

  quint32 headerSize;
  std::memcpy(&headerSize, dib.constData(), sizeof(headerSize));
  headerSize = qFromLittleEndian(headerSize);
  if (headerSize < kMinDibHeaderSize || headerSize > static_cast<quint32>(dib.size()))
    return {};

  const auto fileSize = static_cast<quint32>(kBmpFileHeaderSize + dib.size());
  const quint32 pixelOffset = kBmpFileHeaderSize + headerSize;

  QByteArray bmp;
  QDataStream ds(&bmp, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::LittleEndian);
  ds.writeRawData("BM", kBmpSignatureSize);
  ds << fileSize;
  ds << quint32(0);
  ds << pixelOffset;
  ds.writeRawData(dib.constData(), static_cast<int>(dib.size()));
  return bmp;
}

QByteArray PortalClipboard::bmpToDib(const QByteArray &bmp)
{
  if (bmp.size() < kBmpFileHeaderSize)
    return {};

  return bmp.mid(kBmpFileHeaderSize);
}

QByteArray PortalClipboard::encodeFormat(IClipboard::Format format, const QByteArray &data)
{
  if (data.isEmpty())
    return {};

  if (format == IClipboard::Format::Bitmap) {
    const auto bmpFile = dibToBmp(data);
    if (bmpFile.isEmpty()) {
      LOG_WARN("clipboard bitmap data is malformed");
      return {};
    }

    QImage image;
    if (!image.loadFromData(bmpFile, "BMP")) {
      LOG_WARN("failed to decode clipboard bitmap");
      return {};
    }

    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    if (!image.save(&buf, "PNG")) {
      LOG_WARN("failed to encode clipboard image as png");
      return {};
    }

    return png;
  }
  return data;
}

QByteArray PortalClipboard::decodeFormat(IClipboard::Format format, const QByteArray &bytes)
{
  if (bytes.isEmpty())
    return {};

  if (format == IClipboard::Format::Bitmap) {
    QImage image;
    if (!image.loadFromData(bytes, "PNG")) {
      LOG_WARN("failed to decode clipboard png");
      return {};
    }

    QByteArray bmp;
    QBuffer buf(&bmp);
    buf.open(QIODevice::WriteOnly);
    if (!image.save(&buf, "BMP")) {
      LOG_WARN("failed to encode clipboard image as bmp");
      return {};
    }

    return bmpToDib(bmp);
  }
  return bytes;
}

void PortalClipboard::readSelectionAsync(
    PortalSelectionRead &reader, EiClipboard *cache, XdpSession *session, const char *const *mimeTypes, qint64 maxBytes,
    std::function<void()> complete
)
{
  std::vector<PortalSelectionRead::Offer> offers;
  QSet<IClipboard::Format> seen;
  auto retained =
      std::shared_ptr<XdpSession>(XDP_SESSION(g_object_ref(session)), [](XdpSession *s) { g_object_unref(s); });
  for (const auto &entry : kSupportedMimes) {
    if (!mimeTypes || !g_strv_contains(mimeTypes, entry.mime) || seen.contains(entry.format))
      continue;
    seen.insert(entry.format);
    offers.push_back({entry.format, [retained, mime = entry.mime] {
                        return xdp_session_selection_read(retained.get(), mime);
                      }});
  }
  reader.start(
      std::move(offers), maxBytes,
      [cache, revision = cache->revision(), complete = std::move(complete)](PortalSelectionRead::Values values) {
        if (values.isEmpty())
          return;
        std::vector<std::pair<IClipboard::Format, std::string>> snapshot;
        for (const auto &[format, data] : values)
          snapshot.emplace_back(format, data.toStdString());
        if (cache->replace(snapshot, revision))
          complete();
      }
  );
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
  LOG_DEBUG("clipboard selection transfer complete, bytes: %lld", static_cast<long long>(written));
}

} // namespace deskflow
