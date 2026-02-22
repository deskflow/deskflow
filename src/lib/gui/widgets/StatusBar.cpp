/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StatusBar.h"
#include "common/Constants.h"
#include "common/Settings.h"

#include <QEvent>
#include <QLabel>
#include <QPushButton>

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar{parent},
      m_btnFingerprint{new QPushButton(this)},
      m_lblSecurityIcon{new QLabel(this)},
      m_lblStatus{new QLabel(this)},
      m_btnUpdate{new QPushButton(this)}
{
  static const auto btnHeight = height() - 2;
  static const auto btnSize = QSize(btnHeight, btnHeight);
  static const auto iconSize = QSize(fontMetrics().height() + 2, fontMetrics().height() + 2);

  m_btnFingerprint->setFlat(true);
  m_btnFingerprint->setIcon(QIcon::fromTheme(QStringLiteral("fingerprint")));
  m_btnFingerprint->setFixedSize(btnSize);
  m_btnFingerprint->setIconSize(iconSize);
  insertPermanentWidget(0, m_btnFingerprint);
  connect(m_btnFingerprint, &QPushButton::clicked, this, &StatusBar::requestShowMyFingerprints);

  m_lblSecurityIcon->setVisible(false);
  m_lblSecurityIcon->setFixedSize(iconSize);
  m_lblSecurityIcon->setScaledContents(true);
  insertPermanentWidget(1, m_lblSecurityIcon);

  m_lblStatus->setText(tr("%1 is not running").arg(kAppName));
  insertPermanentWidget(2, m_lblStatus, 1);

  m_btnUpdate->setVisible(false);
  m_btnUpdate->setFlat(true);
  m_btnUpdate->setLayoutDirection(Qt::RightToLeft);
  m_btnUpdate->setIcon(QIcon::fromTheme(QStringLiteral("software-updates-release")));
  m_btnUpdate->setFixedHeight(btnHeight);
  m_btnUpdate->setIconSize(iconSize);
  insertPermanentWidget(3, m_btnUpdate);
  connect(m_btnUpdate, &QPushButton::clicked, this, &StatusBar::requestUpdateVersion);

  updateText();
  adjustSize();
}

// clang-format off
void StatusBar::setStatus(ConnectionState connectionState, ProcessState processState, bool isServer)
{
  setSecurityIconVisible(false);
  switch (processState) {
    using enum ProcessState;
    case Starting:
      m_lblStatus->setText(tr("%1 is starting...").arg(kAppName));
      break;

    case RetryPending:
      m_lblStatus->setText(tr("%1 will retry in a moment...").arg(kAppName));
      break;

    case Stopping:
      m_lblStatus->setText(tr("%1 is stopping...").arg(kAppName));
      break;

    case Stopped:
      m_lblStatus->setText(tr("%1 is not running").arg(kAppName));
      break;

    case Started: {
      switch (connectionState) {
        using enum ConnectionState;

        case Listening: {
          if (isServer) {
            setSecurityIconVisible(true);
            m_lblStatus->setText(tr("%1 is waiting for clients").arg(kAppName));
          }
          break;
        }

        case Connecting:
          m_lblStatus->setText(tr("%1 is connecting...").arg(kAppName));
          break;

        case Connected: {
          setSecurityIconVisible(true);
          if (!isServer) {
            m_lblStatus->setText(tr("%1 is connected as client of %2")
                                     .arg(kAppName, Settings::value(Settings::Client::RemoteHost).toString()));
          }
          break;
        }

        case Disconnected:
          m_lblStatus->setText(tr("%1 is disconnected").arg(kAppName));
          break;
      }
    }
  }
}
// clang-format on
void StatusBar::setServerClients(const QStringList &clients)
{
  if (clients.isEmpty()) {
    m_lblStatus->setText(tr("%1 is waiting for clients").arg(kAppName));
    m_lblStatus->setToolTip("");
    return;
  }
  const auto clientCount = static_cast<int>(clients.size());
  static const auto comma = QStringLiteral(", ");
  static const auto newLine = QStringLiteral("\n");
  //: Shown when in server mode and at least 1 client is connected
  //: %1 is replaced by the app name
  //: %2 will be a list of at least one client
  //: %n will be replaced by the number of clients (n is >=1), it is not requried to be in the translation
  const auto text = tr("%1 is connected, with %n client(s): %2", "", clientCount).arg(kAppName, clients.join(comma));
  m_lblStatus->setText(text);

  const auto toolTipString = clientCount == 1 ? "" : tr("Clients:\n %1").arg(clients.join(newLine));
  m_lblStatus->setToolTip(toolTipString);
}

void StatusBar::setSecurityIconVisible(bool visible)
{
  m_lblSecurityIcon->setVisible(visible);
}

bool StatusBar::securityIconVisible() const
{
  return m_lblSecurityIcon->isVisible();
}

void StatusBar::setBtnFingerprintVisible(bool visible)
{
  m_btnFingerprint->setVisible(visible);
}

void StatusBar::updateFound(const QString &version)
{
  m_btnUpdate->setVisible(true);
  m_btnUpdate->setToolTip(tr("A new version v%1 is available").arg(version));
}

void StatusBar::changeEvent(QEvent *e)
{
  QStatusBar::changeEvent(e);
  if (e->type() == QEvent::LanguageChange)
    updateText();
}

void StatusBar::updateText()
{
  m_btnFingerprint->setToolTip(tr("View local fingerprint"));
  m_btnUpdate->setText(tr("Update available"));
  setSecurityLevel(m_securityLevel);
}

void StatusBar::setSecurityIcon(bool encrypted)
{
  const auto icon = QIcon::fromTheme(encrypted ? QIcon::ThemeIcon::SecurityHigh : QIcon::ThemeIcon::SecurityLow);
  m_lblSecurityIcon->setPixmap(icon.pixmap(QSize(32, 32)));
  m_encrypted = encrypted;
  setSecurityLevel(m_securityLevel);
}

void StatusBar::setSecurityLevel(const QString &securityLevel)
{
  m_securityLevel = securityLevel;
  const auto txt = m_encrypted ? tr("%1 Encryption Enabled").arg(m_securityLevel) : tr("Encryption Disabled");
  m_lblSecurityIcon->setToolTip(txt);
}
