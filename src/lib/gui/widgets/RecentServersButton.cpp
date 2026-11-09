/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "RecentServersButton.h"

#include <QEvent>
#include <QFontMetrics>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>

RecentServersButton::RecentServersButton(QWidget *parent) : QToolButton(parent)
{
  setObjectName(QStringLiteral("recentServersButton"));
  setPopupMode(QToolButton::MenuButtonPopup);
  setToolButtonStyle(Qt::ToolButtonTextOnly);
  setMaximumWidth(245);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  setMenu(new QMenu(this));
  connect(this, &QToolButton::clicked, this, [this] {
    if (!m_history.entries().empty())
      requestConnection(m_history.entries().front());
  });
  refresh();
}

QSize RecentServersButton::sizeHint() const
{
  // Preserve a usable hit area without a hard minimum that can override the
  // larger height required by the active font, style and popup indicator.
  return QToolButton::sizeHint().expandedTo(QSize(0, 32));
}

QSize RecentServersButton::minimumSizeHint() const
{
  return sizeHint();
}

void RecentServersButton::loadHistory(const QString &json)
{
  std::vector<deskflow::gui::RecentServerHistory::Entry> entries;
  if (json.size() <= 8192) {
    const auto values = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const auto &value : values) {
      const auto item = value.toObject();
      entries.push_back({item.value("address").toString().toStdString(), item.value("port").toInt(0)});
    }
  }
  m_history.load(entries);
  refresh();
}

QString RecentServersButton::history() const
{
  QJsonArray entries;
  for (const auto &entry : m_history.entries())
    entries.append(QJsonObject{{"address", QString::fromStdString(entry.address)}, {"port", entry.port}});
  return QString::fromUtf8(QJsonDocument(entries).toJson(QJsonDocument::Compact));
}

void RecentServersButton::setConnectionAllowed(bool allowed, const QString &reason)
{
  m_connectionAllowed = allowed;
  m_unavailableReason = reason;
  refresh();
}

void RecentServersButton::beginAttempt(const QString &address, int port, bool tlsAndPeerChecksEnabled)
{
  m_history.beginAttempt({address.toStdString(), port}, tlsAndPeerChecksEnabled);
}

void RecentServersButton::cancelAttempt()
{
  m_history.cancelAttempt();
}

void RecentServersButton::connectionStateChanged(deskflow::core::ConnectionState state)
{
  if (state == deskflow::core::ConnectionState::Connected)
    publishChange(m_history.connected());
  else
    m_history.disconnected();
}

void RecentServersButton::secureSocket(bool enabled)
{
  if (enabled)
    publishChange(m_history.tlsEstablished());
  else
    m_history.disconnected();
}

void RecentServersButton::publishChange(bool changed)
{
  if (!changed)
    return;
  refresh();
  Q_EMIT historyChanged(history());
}

void RecentServersButton::requestConnection(const deskflow::gui::RecentServerHistory::Entry &entry)
{
  if (!m_connectionAllowed || !isEnabled())
    return;
  // Copy before emitting: the owner may synchronously update/rebuild the history.
  const auto address = QString::fromStdString(entry.address);
  const int port = entry.port;
  setEnabled(false);
  Q_EMIT reconnectRequested(address, port);
}

void RecentServersButton::refresh()
{
  menu()->clear();
  const bool empty = m_history.entries().empty();
  setText(tr("Quick reconnect"));
  setAccessibleName(tr("Quick reconnect"));
  if (!empty) {
    const auto &first = m_history.entries().front();
    const auto address = QString::fromStdString(first.address);
    // Keep the toolbar stable at narrow widths. The exact target remains in
    // the tooltip, accessible name and history menu; certificate checks remain.
    setAccessibleName(tr("Reconnect to %1 on port %2").arg(address).arg(first.port));
  }
  setEnabled(m_connectionAllowed && !empty);
  if (!m_connectionAllowed)
    setToolTip(m_unavailableReason);
  else if (empty)
    setToolTip(tr("A successful connection with TLS and peer checks enabled will appear here."));
  else {
    const auto &first = m_history.entries().front();
    setToolTip(tr("Reconnect to %1 (port %2). The usual certificate checks still apply.")
                   .arg(QString::fromStdString(first.address))
                   .arg(first.port));
  }
  setAccessibleDescription(toolTip());

  for (const auto &entry : m_history.entries()) {
    auto action = menu()->addAction(tr("%1 (port %2)").arg(QString::fromStdString(entry.address)).arg(entry.port));
    connect(action, &QAction::triggered, this, [this, entry] { requestConnection(entry); });
  }
  if (!empty) {
    menu()->addSeparator();
    auto clear = menu()->addAction(tr("Clear recent servers"));
    connect(clear, &QAction::triggered, this, [this] {
      m_history.clear();
      publishChange(true);
    });
  }
}

void RecentServersButton::changeEvent(QEvent *event)
{
  QToolButton::changeEvent(event);
  if (event->type() == QEvent::LanguageChange)
    refresh();
}
