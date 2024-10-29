/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "base/String.h"
#include "common/stdvector.h"

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
  static void splitCommandString(String &command, std::vector<String> &argv);
  static bool searchDoubleQuotes(String &command, size_t &left, size_t &right, size_t startPos = 0);
  static void removeDoubleQuotes(String &arg);
  static const char **getArgv(std::vector<String> &argsArray);
  static String assembleCommand(std::vector<String> &argsArray, String ignoreArg = "", int parametersRequired = 0);

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
