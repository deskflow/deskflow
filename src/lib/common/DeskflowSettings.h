/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QSettings>

#include "common/constants.h"
#include "common/settings.h"

class DeskflowSettings : public QObject
{
  Q_OBJECT
public:
  static DeskflowSettings *instance();
  static void setSettingFile(const QString &settingsFile = QString());
  static void setValue(const QString &key = QString(), const QVariant &value = QVariant());
  static QVariant value(const QString &key = QString());
  static void restoreDefaultSettings();
  static QVariant defaultValue(const QString &key);
  static bool isWritable();
  static bool isSystemScope();
  static void setScope(bool systemScope);
  static const QString settingsFile();

signals:
  void scopeChanged(bool isSystemScope);
  void writableChanged(bool canWrite);
  void settingsChanged(const QString key);

private:
  explicit DeskflowSettings(QObject *parent = nullptr);
  DeskflowSettings *operator=(DeskflowSettings &other) = delete;
  DeskflowSettings(const DeskflowSettings &other) = delete;
  ~DeskflowSettings() = default;
  static bool portableSettings();
  void cleanSettings();
  void initSettings();

  // Vars
  QSettings *m_settings = nullptr;
  QString m_portableSettingsFile = QStringLiteral("settings.ini");
};
