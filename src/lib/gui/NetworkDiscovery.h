/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>
#include <QSet>
#include <QString>

class QUdpSocket;
class QTimer;

namespace deskflow::gui {

//! LAN auto-discovery of other Deskflow computers, by name (no IPs).
/*!
A tiny UDP protocol: a scanner broadcasts a "who is there" probe and every
peer (this class, running in any open GUI, plus the coordinator in running
auto-mode cores) replies with its computer name. Names are enough -- the
coordination layer resolves a bare name via mDNS (\c name.local) / DNS, so
nothing ever has to type or exchange an IP address.

The same instance both answers probes (so this machine is discoverable) and
runs scans, sharing one socket bound with address/port reuse.
*/
class NetworkDiscovery : public QObject
{
  Q_OBJECT
public:
  //! \p selfName is this machine's computer name (announced in replies and
  //! filtered out of its own results).
  explicit NetworkDiscovery(QString selfName, QObject *parent = nullptr);
  ~NetworkDiscovery() override;

  //! Begin answering probes so this machine shows up in others' scans.
  bool start();

  //! Broadcast a probe and collect replies for \p durationMs.
  void scan(int durationMs = 1500);

Q_SIGNALS:
  //! A peer answered (or announced itself). Emitted once per unique name.
  void discovered(const QString &name);
  //! The scan window closed.
  void scanFinished();

private:
  void onDatagram();
  void sendProbe();

  QString m_selfName;
  QUdpSocket *m_socket = nullptr;
  QTimer *m_scanTimer = nullptr;
  QSet<QString> m_seenThisScan;
};

} // namespace deskflow::gui
