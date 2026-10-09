/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StatusBar.h"
#include "common/Constants.h"
#include "common/Settings.h"

#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar{parent},
      m_btnFingerprint{new QPushButton(this)},
      m_lblSecurityIcon{new QLabel(this)},
      m_lblStatus{new QLabel(this)},
      m_lblClipboardSending{new QLabel(this)},
      m_lblClipboardReceiving{new QLabel(this)},
      m_lblClipboardOverLimit{new QLabel(this)},
      m_btnUpdate{new QPushButton(this)},
      m_retryTimer{new QTimer(this)},
      m_clipboardSendingTimer{new QTimer(this)},
      m_clipboardReceivingTimer{new QTimer(this)},
      m_clipboardOverLimitTimer{new QTimer(this)}
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

  int index = 2;
  for (auto *label : {m_lblClipboardSending, m_lblClipboardReceiving, m_lblClipboardOverLimit}) {
    label->setVisible(false);
    label->setFixedSize(iconSize);
    label->setScaledContents(true);
    insertPermanentWidget(index++, label);
  }
  for (auto *timer : {m_clipboardSendingTimer, m_clipboardReceivingTimer, m_clipboardOverLimitTimer})
    timer->setSingleShot(true);
  connect(m_clipboardSendingTimer, &QTimer::timeout, m_lblClipboardSending, &QLabel::show);
  connect(m_clipboardReceivingTimer, &QTimer::timeout, m_lblClipboardReceiving, &QLabel::show);
  connect(m_clipboardOverLimitTimer, &QTimer::timeout, m_lblClipboardOverLimit, &QLabel::hide);

  m_lblStatus->setText(tr("%1 is not running").arg(kAppName));
  insertPermanentWidget(5, m_lblStatus, 1);

  m_btnUpdate->setVisible(false);
  m_btnUpdate->setFlat(true);
  m_btnUpdate->setLayoutDirection(Qt::RightToLeft);
  m_btnUpdate->setIcon(QIcon::fromTheme(QStringLiteral("software-updates-release")));
  m_btnUpdate->setFixedHeight(btnHeight);
  m_btnUpdate->setIconSize(iconSize);
  insertPermanentWidget(6, m_btnUpdate);
  connect(m_btnUpdate, &QPushButton::clicked, this, &StatusBar::requestUpdateVersion);

  m_retryTimer->setInterval(1000);
  m_retryTimer->setSingleShot(false);
  connect(m_retryTimer, &QTimer::timeout, this, &StatusBar::updateTimerLabel);

  updateText();
  adjustSize();
}

// clang-format off
void StatusBar::setStatus(ConnectionState connectionState, ProcessState processState, bool isServer)
{
  if (m_retryTimer->isActive())
    m_retryTimer->stop();
  setSecurityIconVisible(false);
  if (processState != ProcessState::Started || connectionState != ConnectionState::Connected) {
    m_clipboardSendingTo.clear();
    m_clipboardReceivingFrom.clear();
    updateClipboardIcons();
  }
  switch (processState) {
    using enum ProcessState;
    case Starting:
      m_connectionInterval = -1;
      m_lblStatus->setText(tr("%1 is starting...").arg(kAppName));
      break;

    case RetryPending:
      m_connectionInterval = -1;
      m_lblStatus->setText(tr("%1 will retry in a moment...").arg(kAppName));
      break;

    case Stopping:
        m_connectionInterval = -1;
      m_lblStatus->setText(tr("%1 is stopping...").arg(kAppName));
      break;

    case Stopped:
      m_connectionInterval = -1;
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
          updateTimerLabel();
          if (Settings::value(Settings::Client::DynamicConnectionRetry).toBool())
            m_retryTimer->start();
          break;

        case Connected: {
          setSecurityIconVisible(true);
          m_connectionInterval = -1;
          if (!isServer) {
            m_lblStatus->setText(tr("%1 is connected as client of %2")
                                     .arg(kAppName, Settings::value(Settings::Client::RemoteHost).toString()));
          }
          break;
        }

        case Disconnected:
          m_lblStatus->setText(tr("%1 is disconnected").arg(kAppName));
          m_connectionInterval = -1;
          break;
      }
    }
  }
}
// clang-format on
void StatusBar::setServerClients(const QStringList &clients)
{
  const auto disconnected = [&clients](std::pair<const QString &, qint64 &> transfer) {
    return !clients.contains(transfer.first);
  };
  m_clipboardSendingTo.removeIf(disconnected);
  m_clipboardReceivingFrom.removeIf(disconnected);
  updateClipboardIcons();

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

void StatusBar::setConnectionInterval(int newInterval)
{
  m_connectionInterval = newInterval;
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

void StatusBar::showClipboardSending(qint64 bytes, const QString &peer)
{
  m_clipboardSendingTo.insert(peer, bytes);
  updateClipboardIcons();
}

void StatusBar::showClipboardSent(const QString &peer)
{
  m_clipboardSendingTo.remove(peer);
  updateClipboardIcons();
}

void StatusBar::showClipboardReceiving(qint64 bytes, const QString &peer)
{
  m_clipboardReceivingFrom.insert(peer, bytes);
  updateClipboardIcons();
}

void StatusBar::showClipboardReceived(const QString &peer)
{
  m_clipboardReceivingFrom.remove(peer);
  updateClipboardIcons();
}

void StatusBar::showClipboardOverLimit(qint64 bytes, qint64 limit)
{
  const auto size = locale().formattedDataSize(bytes, 0, QLocale::DataSizeTraditionalFormat);
  const auto maximum = locale().formattedDataSize(limit, 0, QLocale::DataSizeTraditionalFormat);
  const auto icon = QIcon::fromTheme(QIcon::ThemeIcon::DialogWarning);
  m_lblClipboardOverLimit->setPixmap(icon.pixmap(QSize(32, 32)));
  m_lblClipboardOverLimit->setToolTip(tr("Clipboard not shared, %1 is over the %2 limit").arg(size, maximum));
  m_lblClipboardOverLimit->setVisible(true);
  m_clipboardOverLimitTimer->start(kClipboardNoticeTimeoutMs);
}

void StatusBar::updateClipboardIcons()
{
  const auto formatSize = [this](qint64 bytes) {
    return locale().formattedDataSize(bytes, 0, QLocale::DataSizeTraditionalFormat);
  };

  QStringList sending;
  for (const auto &[peer, bytes] : m_clipboardSendingTo.asKeyValueRange())
    sending.append(tr("Sending clipboard to %1 (%2)...").arg(clipboardPeerName(peer), formatSize(bytes)));
  m_lblClipboardSending->setPixmap(QIcon::fromTheme(QStringLiteral("cloud-upload")).pixmap(QSize(32, 32)));
  m_lblClipboardSending->setToolTip(sending.join(QLatin1Char('\n')));
  showClipboardIconAfterDelay(m_lblClipboardSending, m_clipboardSendingTimer, !sending.isEmpty());

  QStringList receiving;
  for (const auto &[peer, bytes] : m_clipboardReceivingFrom.asKeyValueRange())
    receiving.append(tr("Receiving clipboard from %1 (%2)...").arg(clipboardPeerName(peer), formatSize(bytes)));
  m_lblClipboardReceiving->setPixmap(QIcon::fromTheme(QStringLiteral("cloud-download")).pixmap(QSize(32, 32)));
  m_lblClipboardReceiving->setToolTip(receiving.join(QLatin1Char('\n')));
  showClipboardIconAfterDelay(m_lblClipboardReceiving, m_clipboardReceivingTimer, !receiving.isEmpty());
}

void StatusBar::showClipboardIconAfterDelay(QLabel *label, QTimer *delay, bool active)
{
  if (!active) {
    delay->stop();
    label->hide();
  } else if (label->isHidden() && !delay->isActive()) {
    delay->start(kClipboardIconDelayMs);
  }
}

QString StatusBar::clipboardPeerName(const QString &peer)
{
  // an empty peer is the server, which the client's core can't name
  return peer.isEmpty() ? Settings::value(Settings::Client::RemoteHost).toString() : peer;
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

void StatusBar::updateTimerLabel()
{
  QString text;
  if (m_connectionInterval < 2 || !Settings::value(Settings::Client::DynamicConnectionRetry).toBool()) {
    text = tr("%1 is connecting...").arg(kAppName);
  } else {
    text = tr("%1 is waiting %2 seconds before the next retry").arg(kAppName, QString::number(m_connectionInterval));
    m_connectionInterval--;
  }
  m_lblStatus->setText(text);
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
