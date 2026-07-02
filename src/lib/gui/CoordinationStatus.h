/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class QTimer;

namespace deskflow::gui {

//! Formats the mesh v2 \c fleet object from a status reply for display.
QString formatFleetGraph(const QJsonObject &fleet);

//! Extra status-bar note for mesh version mismatch or missing topology.
QString formatStatusAnnotation(const QJsonObject &status);

//! Polls the local coordination mesh for the fleet's live role state.
/*!
In auto (coordinated) mode a \c deskflow-core process answers a
``{"t":"status"}`` query on 127.0.0.1:<coordination port> with the node's
role and the address of the server it follows. This class polls that on a
timer so the GUI can show "this computer is in control" / "following X"
without owning the core -- it works whether the core was launched by this
GUI or by a background agent.

When mesh v2 is active the status reply may include a read-only \c fleet
object; \c fleetGraph summarizes screens and edges for the status bar.
*/
class CoordinationStatus : public QObject
{
  Q_OBJECT
public:
  explicit CoordinationStatus(QObject *parent = nullptr);

  void start(quint16 port, int intervalMs = 2500);
  void stop();

Q_SIGNALS:
  //! \p role is "server" / "client" / "init"; \p serverName is the short
  //! name of the followed server (empty when this node is the server).
  //! \p fleetGraph is empty when topology is unknown or mesh v1 is active.
  void online(const QString &role, const QString &serverName, const QString &fleetGraph);
  //! No coordinator is answering locally (auto mode not running here).
  void offline();

private:
  void poll();

  quint16 m_port = 24851;
  QTimer *m_timer = nullptr;
};

} // namespace deskflow::gui
