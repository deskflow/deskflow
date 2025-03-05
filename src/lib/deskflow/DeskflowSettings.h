/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/constants.h"

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QSettings>
#include <QVariant>

/**
 * @brief The Settings struct contains all the static path info for the various keys and setting files
 */
struct Settings
{
#if defined(Q_OS_WIN)
  inline const static auto UserSettingPath =
      QStringLiteral("%1/AppData/Local/%2/%3.ini").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("C:/ProgramData/%2/%3.ini").arg(kAppName, kAppName);
#elif defined(Q_OS_MAC)
  inline const static auto UserSettingPath =
      QStringLiteral("%1/Library/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("/Libaray/%2/%3.conf").arg(kAppName, kAppName);
#else
  inline const static auto UserSettingPath =
      QStringLiteral("%1/.config/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("/etc/%2/%3.conf").arg(kAppName, kAppName);
#endif
  struct Core
  {
    inline static const auto Scope = QStringLiteral("core/loadFromSystemScope");
  };
  struct Gui
  {
    inline static const auto Autohide = QStringLiteral("gui/autoHide");
    inline static const auto LogExpanded = QStringLiteral("gui/logExpanded");
  };
};

class DeskflowSettings : public QObject
{
  Q_OBJECT
public:
  static DeskflowSettings *instance();
  static void restoreDefaultSettings();
  static QVariant defaultValue(const QString &key);
  static bool isWritable();
  static bool isSystemScope();
  static void setScope(bool systemScope);
  static void setValue(const QString &key = QString(), const QVariant &value = QVariant());
  static QVariant value(const QString &key = QString());

signals:
  void scopeChanged(bool isSystemScope);
  void writableChanged(bool canWrite);
  void settingsChanged();

private:
  explicit DeskflowSettings(QObject *parent = nullptr);
  DeskflowSettings *operator=(DeskflowSettings &other) = delete;
  DeskflowSettings(const DeskflowSettings &other) = delete;
  ~DeskflowSettings() = default;
  void cleanSettings();
  void initSettings();

  // Vars
  QSettings *systemSettings = nullptr;
  QSettings *userSettings = nullptr;
  QSettings *m_settings = nullptr;

  inline static const QStringList validKeys = {
      Settings::Core::Scope, Settings::Gui::Autohide, Settings::Gui::LogExpanded
  };
};
