/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QSettings>

#include <QDir>

#include "common/constants.h"

class Settings : public QObject
{
  Q_OBJECT
public:
#if defined(Q_OS_WIN)
  inline const static auto UserSettingFile =
      QStringLiteral("%1/AppData/Local/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingFile = QStringLiteral("C:/ProgramData/%2/%3.conf").arg(kAppName, kAppName);
#elif defined(Q_OS_MAC)
  inline const static auto UserSettingFile =
      QStringLiteral("%1/Library/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingFile = QStringLiteral("/Libaray/%2/%3.conf").arg(kAppName, kAppName);
#else
  inline const static auto UserSettingFile =
      QStringLiteral("%1/.config/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingFile = QStringLiteral("/etc/%2/%3.conf").arg(kAppName, kAppName);
#endif

  struct Core
  {
    inline static const auto Scope = QStringLiteral("core/loadFromSystemScope");
  };
  struct Gui
  {
    inline static const auto Autohide = QStringLiteral("gui/autoHide");
    inline static const auto AutoUpdateCheck = QStringLiteral("gui/enableUpdateCheck");
    inline static const auto CloseToTray = QStringLiteral("gui/closeToTray");
    inline static const auto LogExpanded = QStringLiteral("gui/logExpanded");
    inline static const auto SymbolicTrayIcon = QStringLiteral("gui/symbolicTrayIcon");
  };

  static Settings *instance();
  static void setSettingFile(const QString &settingsFile = QString());
  static void setValue(const QString &key = QString(), const QVariant &value = QVariant());
  static QVariant value(const QString &key = QString());
  static void restoreDefaultSettings();
  static QVariant defaultValue(const QString &key);
  static bool isWritable();
  static bool isSystemScope();
  static void setScope(bool systemScope);
  static const QString settingsFile();
  static const QString settingsPath();

signals:
  void scopeChanged(bool isSystemScope);
  void writableChanged(bool canWrite);
  void settingsChanged(const QString key);

private:
  explicit Settings(QObject *parent = nullptr);
  Settings *operator=(Settings &other) = delete;
  Settings(const Settings &other) = delete;
  ~Settings() = default;
  static bool isPortableSettings();
  void cleanSettings();
  void initSettings();

  QSettings *m_settings = nullptr;
  QString m_portableSettingsFile = QStringLiteral("%1.conf").arg(kAppName);
  // clang-format off
  inline static const QStringList m_validKeys = {
      Core::Scope
    , Gui::Autohide
    , Gui::AutoUpdateCheck
    , Gui::CloseToTray
    , Gui::LogExpanded
    , Gui::SymbolicTrayIcon
  };
  // clang-format on
};
