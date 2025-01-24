/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/stdvector.h"

#include <string>

namespace deskflow {
class ArgsBase;
class ServerArgs;
class ClientArgs;
} // namespace deskflow

class App;

class ArgParser
{

public:
  ArgParser(App *app);

  bool parseServerArgs(deskflow::ServerArgs &args, int argc, const char *const *argv);
  bool parseClientArgs(deskflow::ClientArgs &args, int argc, const char *const *argv);
  bool parsePlatformArgs(deskflow::ArgsBase &argsBase, const int &argc, const char *const *argv, int &i, bool isServer);
  bool parseGenericArgs(int argc, const char *const *argv, int &i);
  bool parseDeprecatedArgs(int argc, const char *const *argv, int &i);
  void setArgsBase(deskflow::ArgsBase &argsBase)
  {
    m_argsBase = &argsBase;
  }

  static bool isArg(
      int argi, int argc, const char *const *argv, const char *name1, const char *name2, int minRequiredParameters = 0
  );
  static void splitCommandString(std::string &command, std::vector<std::string> &argv);
  static bool searchDoubleQuotes(std::string &command, size_t &left, size_t &right, size_t startPos = 0);
  static void removeDoubleQuotes(std::string &arg);
  static const char **getArgv(std::vector<std::string> &argsArray);
  static std::string
  assembleCommand(std::vector<std::string> &argsArray, std::string ignoreArg = "", int parametersRequired = 0);

  static deskflow::ArgsBase &argsBase()
  {
    return *m_argsBase;
  }

private:
  void updateCommonArgs(const char *const *argv);
  bool checkUnexpectedArgs();

private:
  App *m_app;

  static deskflow::ArgsBase *m_argsBase;
};
