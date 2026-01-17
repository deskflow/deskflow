/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <QMap>
#include <QObject>
#include <QStringList>
#include <mutex>

namespace deskflow {

class PortalClipboardProxy;

/**
 * @brief IClipboard implementation for XDG Desktop Portal
 * 
 * This class bridges Deskflow's IClipboard interface with the portal.
 * It uses PortalClipboardProxy for the low-level communication.
 */
class PortalClipboard : public QObject, public ::IClipboard
{
  Q_OBJECT

public:
  explicit PortalClipboard(PortalClipboardProxy *proxy, PortalClipboardProxy::SelectionType type = PortalClipboardProxy::Standard);
  ~PortalClipboard() override;

  // IClipboard interface
  bool empty() override;
  void add(Format format, const std::string &data) override;
  bool open(Time time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format format) const override;
  std::string get(Format format) const override;

private Q_SLOTS:
  void onSelectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner, PortalClipboardProxy::SelectionType type);
  void onSelectionTransferRequested(const QString &mimeType, quint32 serial, PortalClipboardProxy::SelectionType type);
  void onClipboardError(const QString &error);

private:
  QString formatToMimeType(Format format) const;
  Format mimeTypeToFormat(const QString &mimeType) const;

  PortalClipboardProxy *m_proxy;
  PortalClipboardProxy::SelectionType m_type;
  mutable bool m_isOpen = false;
  mutable Time m_time = 0;
  mutable std::mutex m_cacheMutex;
  mutable QMap<Format, std::string> m_cache;
  mutable QStringList m_portalMimeTypes;
  mutable bool m_portalIsOwner = false;
};

} // namespace deskflow
