/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <QCommandLineParser>

/**
 * @brief The CoreArgParser class
 * This class processes the argments for the "core" app
 */
class CoreArgParser
{
public:
  /**
   * @brief CoreArgParser calling this funciton will parse apps and set the setting and version options
   * For any other settings to be set you must call parse();
   * @sa parse();
   * @param args List of args to parse
   */
  explicit CoreArgParser(const QStringList &args = {});
  /**
   * @brief parse
   * This method will parse all options other then help and version
   */
  void parse();
  QString helpText() const;
  QString versionText() const;
  QString errorText() const;
  bool help() const;
  bool version() const;
  bool serverMode() const;
  bool clientMode() const;
  bool singleInstanceOnly() const;

private:
  [[noreturn]] void showHelpText() const;
  QCommandLineParser m_parser;
  QString m_helpText;
  bool m_clientMode = false;
  bool m_serverMode = false;
  bool m_singleInstance = true;
  static const QString s_headerText;
};
