/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/PortalSelectionRead.h"
#include "platform/PortalClipboard.h"
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <glib-unix.h>
#include <unistd.h>

namespace deskflow {

void PortalSelectionRead::clearSource(GSource *&source)
{
  if (source) {
    g_source_destroy(source);
    g_source_unref(source);
    source = nullptr;
  }
}

void PortalSelectionRead::closePipe()
{
  clearSource(m_read);
  if (m_fd >= 0) {
    ::close(m_fd);
    m_fd = -1;
  }
}

void PortalSelectionRead::cancel()
{
  closePipe();
  clearSource(m_timeout);
  m_complete = {};
  m_offers.clear();
  m_values.clear();
  m_bytes.clear();
}

void PortalSelectionRead::start(
    std::vector<Offer> offers, qint64 maxBytes, std::function<void(Values)> complete, unsigned timeoutMs
)
{
  cancel();
  m_offers = std::move(offers);
  m_complete = std::move(complete);
  m_remaining = std::max<qint64>(0, maxBytes);
  m_index = 0;
  m_timeout = g_timeout_source_new(timeoutMs);
  g_source_set_callback(
      m_timeout,
      [](gpointer data) -> gboolean {
        static_cast<PortalSelectionRead *>(data)->finish(false);
        return G_SOURCE_REMOVE;
      },
      this, nullptr
  );
  g_source_attach(m_timeout, m_context);
  next();
}

void PortalSelectionRead::next()
{
  closePipe();
  m_bytes.clear();
  if (m_index == m_offers.size()) {
    finish(true);
    return;
  }
  m_fd = m_offers[m_index].open();
  if (m_fd < 0) {
    ++m_index;
    next();
    return;
  }
  const int flags = fcntl(m_fd, F_GETFL);
  if (flags < 0 || fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    finish(false);
    return;
  }
  m_read = g_unix_fd_source_new(m_fd, static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR));
  g_source_set_callback(
      m_read, G_SOURCE_FUNC(+[](gint, GIOCondition condition, gpointer data) -> gboolean {
        return static_cast<PortalSelectionRead *>(data)->readable(condition);
      }),
      this, nullptr
  );
  g_source_attach(m_read, m_context);
}

gboolean PortalSelectionRead::readable(GIOCondition condition)
{
  if (condition & (G_IO_ERR | G_IO_NVAL)) {
    finish(false);
    return G_SOURCE_REMOVE;
  }
  char buffer[64 * 1024];
  const auto count = ::read(m_fd, buffer, sizeof(buffer));
  if (count < 0) {
    if (errno == EAGAIN || errno == EINTR)
      return G_SOURCE_CONTINUE;
    finish(false);
    return G_SOURCE_REMOVE;
  }
  if (count > m_remaining) {
    finish(false);
    return G_SOURCE_REMOVE;
  }
  if (count > 0) {
    m_remaining -= count;
    m_bytes.append(buffer, count);
    return G_SOURCE_CONTINUE;
  }
  const auto format = m_offers[m_index].format;
  if (format != IClipboard::Format::Bitmap) {
    while (m_bytes.endsWith('\0'))
      m_bytes.chop(1);
    m_bytes.replace("\r\n", "\n");
  }
  auto decoded = PortalClipboard::decodeFormat(format, m_bytes);
  if (!decoded.isEmpty())
    m_values.emplaceBack(format, std::move(decoded));
  ++m_index;
  next();
  return G_SOURCE_REMOVE;
}

void PortalSelectionRead::finish(bool success)
{
  auto complete = std::move(m_complete);
  auto values = success ? std::move(m_values) : Values{};
  cancel();
  if (complete)
    complete(std::move(values));
}

} // namespace deskflow
