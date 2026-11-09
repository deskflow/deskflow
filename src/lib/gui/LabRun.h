// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#ifdef DESKFLOW_LAB_BUILD
#include "common/Settings.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QTextStream>
#include <QUuid>
namespace deskflow::gui::lab {
// Before QApplication/I18N/Settings. Existing profiles or symlinks fail closed.
inline QString prepare(int argc, char **argv)
{
  QString path, language = "en";
  for (int i = 1; i < argc; ++i) {
    const auto arg = QString::fromLocal8Bit(argv[i]);
    if (arg == "--lab-root" && i + 1 < argc)
      path = QString::fromLocal8Bit(argv[++i]);
    else if (arg == "--lab-language" && i + 1 < argc)
      language = QString::fromLocal8Bit(argv[++i]);
  }
  QString allowed = QDir::cleanPath(QStringLiteral(DESKFLOW_LAB_RUNS_ROOT));
  // Only a packaged, inert preview can create a fresh sibling LabRuns profile.
  const QFileInfo executable(QString::fromLocal8Bit(argv[0]));
  const auto macos = QFileInfo(executable.canonicalFilePath()).dir();
  QFile marker(macos.filePath("../Resources/DeskflowLabIsolatedPreview"));
  if (marker.open(QIODevice::ReadOnly) && marker.readAll() == "deskflow-lab-offline-v1\n") {
    const auto portable = QDir::cleanPath(macos.absoluteFilePath("../../../LabRuns"));
    if (path.isEmpty())
      path = portable + "/run-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (QFileInfo(path).absolutePath() == portable)
      allowed = portable;
  }
  const QFileInfo requested(path);
  if (path.isEmpty() || !requested.isAbsolute() || requested.exists() || requested.isSymLink() ||
      requested.absolutePath() != allowed ||
      !QRegularExpression("^[A-Za-z0-9][A-Za-z0-9_-]{0,80}$").match(requested.fileName()).hasMatch() ||
      !QStringList{"en", "zh_CN"}.contains(language))
    return {};
  if (!QDir().mkpath(allowed) || QFileInfo(allowed).canonicalFilePath() != allowed)
    return {};
  if (!QDir().mkpath(path + "/config/DeskflowLab") || !QDir().mkpath(path + "/state"))
    return {};
  qputenv("XDG_CONFIG_HOME", (path + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (path + "/state").toUtf8());
  QSettings settings(path + "/config/DeskflowLab/DeskflowLab.conf", QSettings::IniFormat);
  settings.setValue(Settings::Core::ComputerName, "Lab-Mac");
  settings.setValue(Settings::Core::CoreMode, "Client");
  settings.setValue(Settings::Core::ProcessMode, "Desktop");
  settings.setValue(Settings::Core::Language, language);
  settings.setValue(Settings::Core::Port, 34800);
  settings.setValue(Settings::Client::RemoteHost, "demo-device.invalid");
  settings.setValue(Settings::Client::LanguageSync, false);
  settings.setValue(Settings::Gui::AutoStartCore, false);
  settings.setValue(Settings::Gui::AutoUpdateCheck, false);
  settings.setValue(Settings::Gui::Autohide, false);
  settings.setValue(Settings::Gui::LogExpanded, false);
  settings.setValue(Settings::Security::TlsEnabled, true);
  settings.setValue(Settings::Security::CheckPeers, true);
  settings.setValue(Settings::Security::Certificate, path + "/unused-lab-certificate.pem");
  settings.setValue(Settings::Log::ToFile, false);
  settings.setValue(Settings::Log::File, path + "/lab.log");
  settings.sync();
  return settings.status() == QSettings::NoError ? path : QString();
}
inline bool writeEvidence(const QString &root)
{
  QJsonObject evidence{
      {"application", "DeskflowLab"},
      {"settings", Settings::settingsFile()},
      {"state_root", qEnvironmentVariable("XDG_STATE_HOME")},
      {"input_core_disabled", true},
      {"daemon_ipc_disabled", true},
      {"permission_prompt_disabled", true},
      {"update_network_disabled", true},
      {"tls_enabled", Settings::value(Settings::Security::TlsEnabled).toBool()},
      {"peer_checks", Settings::value(Settings::Security::CheckPeers).toBool()}
  };
  QFile out(root + "/isolation.json");
  return out.open(QIODevice::WriteOnly) && out.write(QJsonDocument(evidence).toJson()) > 0;
}
} // namespace deskflow::gui::lab
#endif
