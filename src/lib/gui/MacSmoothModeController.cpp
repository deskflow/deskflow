/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MacSmoothModeController.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSaveFile>
#include <QTimer>

#include <utility>

MacSmoothModeController::MacSmoothModeController(QObject *parent, QString controlPath, QString statusPath)
    : QObject(parent),
      m_controlPath(
          controlPath.isEmpty() ? QDir::homePath() + "/Library/Application Support/DeskflowMXAWDL/control"
                                : std::move(controlPath)
      ),
      m_statusPath(statusPath.isEmpty() ? "/Library/Logs/DeskflowMXAWDL/status" : std::move(statusPath))
{
  auto *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &MacSmoothModeController::refresh);
  timer->start(1000);
  refresh();
}

const MacSmoothModeController::State &MacSmoothModeController::state() const
{
  return m_state;
}

bool MacSmoothModeController::setAutomatic(bool enabled)
{
  return writeControl(enabled ? "auto\n" : "disabled\n");
}

bool MacSmoothModeController::allowAirDrop()
{
  return writeControl("pause_until=" + QByteArray::number(QDateTime::currentSecsSinceEpoch() + 600) + '\n');
}

bool MacSmoothModeController::resume()
{
  return writeControl("auto\n");
}

void MacSmoothModeController::refresh()
{
  const QFileInfo statusInfo(m_statusPath);
  const auto age = statusInfo.lastModified().secsTo(QDateTime::currentDateTime());
  QFile status(m_statusPath);
  QMap<QByteArray, QByteArray> fields;
  if (statusInfo.isFile() && !statusInfo.isSymLink() && statusInfo.size() <= 4096 && age >= -5 && age <= 15 &&
      status.open(QIODevice::ReadOnly)) {
    for (const auto &line : status.read(4096).split('\n')) {
      const auto separator = line.indexOf('=');
      if (separator > 0)
        fields.insert(line.left(separator), line.mid(separator + 1));
    }
  }
  QFile control(m_controlPath);
  const QFileInfo controlInfo(control);
  const bool available = fields.value("version") == "1" && controlInfo.isFile() && !controlInfo.isSymLink() &&
                         controlInfo.size() <= 128 && control.open(QIODevice::ReadOnly);
  const auto preference = available ? control.read(128).trimmed() : QByteArray();
  const bool paused =
      preference.startsWith("pause_until=") && preference.mid(12).toLongLong() > QDateTime::currentSecsSinceEpoch();
  const auto mode = fields.value("mode");
  QString summary = tr("Controller offline — run setup");
  if (available) {
    if (mode == "smooth")
      summary = tr("Client active — AWDL suppressed");
    else if (mode == "allowed")
      summary = tr("Server active — AirDrop allowed");
    else if (mode == "waiting")
      summary = tr("Waiting for a Deskflow switch");
    else if (mode == "deskflow-stopped")
      summary = tr("Deskflow stopped — AirDrop allowed");
    else if (mode == "airdrop")
      summary = tr("AirDrop override active");
    else if (mode == "disabled")
      summary = tr("Automatic smoothing is off");
    else if (mode == "stopped")
      summary = tr("Controller stopped");
    else
      summary = tr("Controller error — see log");
  }
  m_state = {available, available && preference != "disabled", paused, summary};
  Q_EMIT stateChanged();
}

bool MacSmoothModeController::writeControl(const QByteArray &value)
{
  // Do not silently create a control file when setup is absent or replace a link.
  const QFileInfo info(m_controlPath);
  if (!info.isFile() || info.isSymLink()) {
    Q_EMIT controlError(tr("The Smooth Mode control file is missing or invalid. Run setup first."));
    refresh();
    return false;
  }
  QSaveFile file(m_controlPath);
  if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
      file.write(value) != value.size() || !file.commit()) {
    Q_EMIT controlError(tr("Could not update Smooth Mode: %1").arg(file.errorString()));
    refresh();
    return false;
  }
  refresh();
  return true;
}
