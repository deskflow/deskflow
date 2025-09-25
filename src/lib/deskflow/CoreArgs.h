/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QCommandLineOption>
/**
 * @brief The CoreArgs class
 * This class contains args for the CoreArgParser
 */
struct CoreArgs
{
  inline static const auto displayOption =
      QCommandLineOption("display", "When in X mode, connect to the X server at <display>", "display");

  inline static const auto helpOption = QCommandLineOption({"h", "help"}, "Display Help on the command line");
  inline static const auto versionOption = QCommandLineOption({"v", "version"}, "Display version information");
  inline static const auto configOption = QCommandLineOption(
      {"s", "settings"}, "override configuration file to use", "configFile"
  ); // use -c later, now avoid breaking the serverApp args.
  inline static const auto interfaceOption = QCommandLineOption(
      {"i", "interface"},
      "Use a specific interface for the connection. Instead of any address in client mode or listening on all "
      "addresses in server mode address.\n address must be the IP address or hostname of an interface on the system.",
      "address"
  );
  inline static const auto portOption =
      QCommandLineOption({"p", "port"}, "Port to use in place of default port", "port");
  inline static const auto nameOption =
      QCommandLineOption({"n", "name"}, "use screen-name instead the hostname to identify this screen", "screen-name");

  inline static const auto logLevelOption = QCommandLineOption(
      "log-level",
      "filter out log messages with priority below level.\nlevel may be:\nFATAL, ERROR, WARNING, NOTE, "
      "INFO, DEBUG, DEBUG1, DEBUG2.",
      "level"
  );

  inline static const auto logFileOption = QCommandLineOption({"l", "log"}, "Write messages to file", "file");

  inline static const auto secureOption =
      QCommandLineOption("secure", "Enable TLS encryption (default: true)", "value");

  inline static const auto tlsCertOption =
      QCommandLineOption("tls-cert", "Use file in place of default TLS certificate path", "file");

  inline static const auto preventSleepOption = QCommandLineOption(
      "prevent-sleep", "When true the machine will be prevented from sleeping while the program is running", "value"
  );

  inline static const auto restartOption = QCommandLineOption(
      {"r", "restartOnFailure"}, "Set if the core should automatically restart if it fails", "value"
  );

  inline static const auto useHooksOption =
      QCommandLineOption("useHooks", "Sets if hooks are used for windows desks", "value");

  // Server Options
  inline static const auto peerCheckOption = QCommandLineOption(
      "peerCertCheck", "Server Mode: Enable client SSL certificate checking (default: true)", "value"
  );

  inline static const auto serverConfigOption =
      QCommandLineOption("serverConfig", "Server Mode: File to load as server config", "path");

  // Client Options
  inline static const auto yscrollOption =
      QCommandLineOption("yscroll", "Client Mode: Vertical scrolling delta (default: 120)", "value");

  inline static const auto languageSyncOption =
      QCommandLineOption("languageSync", "Client Mode: Sync language with client (default: false)", "value");

  inline static const auto options = {helpOption,     versionOption,     configOption,       interfaceOption,
                                      portOption,     nameOption,        logLevelOption,     logFileOption,
                                      secureOption,   tlsCertOption,     preventSleepOption, restartOption,
                                      displayOption,  useHooksOption,    peerCheckOption,    serverConfigOption,
                                      yscrollOption,  languageSyncOption};
};
