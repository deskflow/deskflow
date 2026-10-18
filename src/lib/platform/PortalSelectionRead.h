/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include "deskflow/IClipboard.h"
#include <QByteArray>
#include <QList>
#include <functional>
#include <glib.h>
#include <vector>

namespace deskflow {

// All methods and completion callbacks run on the supplied context.
class PortalSelectionRead
{
public:
  using Values = QList<std::pair<IClipboard::Format, QByteArray>>;
  struct Offer
  {
    IClipboard::Format format;
    std::function<int()> open;
  };
  explicit PortalSelectionRead(GMainContext *context) : m_context(context)
  {
  }
  ~PortalSelectionRead()
  {
    cancel();
  }
  void
  start(std::vector<Offer> offers, qint64 maxBytes, std::function<void(Values)> complete, unsigned timeoutMs = 2000);
  void cancel();

private:
  void next();
  gboolean readable(GIOCondition condition);
  void finish(bool success);
  void closePipe();
  static void clearSource(GSource *&source);
  GMainContext *m_context;
  GSource *m_read = nullptr;
  GSource *m_timeout = nullptr;
  int m_fd = -1;
  qint64 m_remaining = 0;
  size_t m_index = 0;
  std::vector<Offer> m_offers;
  QByteArray m_bytes;
  Values m_values;
  std::function<void(Values)> m_complete;
};

} // namespace deskflow
