// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QMap>
#include <QObject>
namespace deskflow::gui {
// Address hints only: never a certificate, trusted identity, or connection command.
class DiscoveryCatalog : public QObject
{
  Q_OBJECT
public:
  using QObject::QObject;
  QStringList serviceNames(const QString &host, int port) const;
  static bool validName(const QString &name);
  static bool validEndpoint(const QString &host, int port);
  static QString canonicalHost(QString host);
  bool
  upsert(const QString &source, const QString &name, const QString &host, int port, qint64 nowMs, int leaseMs = 120000);
  void remove(const QString &source);
  void expire(qint64 nowMs);
  void clear();
  int sourceCount() const
  {
    return m_sources.size();
  }
Q_SIGNALS:
  void found(const QString &key, const QString &name, const QString &host, int port);
  void removed(const QString &key);

private:
  struct Entry
  {
    QString name, host;
    int port;
    qint64 expires;
  };
  QMap<QString, Entry> m_sources, m_visible;
  void publish();
};
} // namespace deskflow::gui
