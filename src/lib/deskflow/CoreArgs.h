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
  inline static const auto options = {helpOption, versionOption, configOption, interfaceOption, portOption};
};
