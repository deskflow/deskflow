/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <vector>

namespace deskflow {
class ServerArgs;
} // namespace deskflow

class App;

class ArgParser
{

public:
  explicit ArgParser(App *app);

  bool parseGenericArgs(int argc, const char *const *argv, int &i) const;
  bool parseDeprecatedArgs(int argc, const char *const *argv, int &i) const;

  static bool isArg(
      int argi, int argc, const char *const *argv, const char *name1, const char *name2, int minRequiredParameters = 0
  );
  static void splitCommandString(const std::string_view &command, std::vector<std::string> &argv);
  static bool searchDoubleQuotes(const std::string_view &command, size_t &left, size_t &right, size_t startPos = 0);
  static void removeDoubleQuotes(std::string_view &arg);
  static const char **getArgv(std::vector<std::string> &argsArray);
  static std::string assembleCommand(
      std::vector<std::string> &argsArray, const std::string_view &ignoreArg = std::string_view(),
      int parametersRequired = 0
  );

private:
  bool checkUnexpectedArgs() const;

private:
  App *m_app;
};
