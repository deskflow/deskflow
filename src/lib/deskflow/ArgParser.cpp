/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ArgParser.h"

#include "base/Log.h"
#include "deskflow/App.h"
#include "deskflow/ArgsBase.h"
#include "deskflow/ClientArgs.h"

#ifdef WINAPI_MSWINDOWS
#include <VersionHelpers.h>
#endif

#include <QFileInfo>
#include <QSysInfo>

deskflow::ArgsBase *ArgParser::m_argsBase = nullptr;

ArgParser::ArgParser(App *app) : m_app(app)
{
}

bool ArgParser::parseClientArgs(deskflow::ClientArgs &args, int argc, const char *const *argv) const
{
  setArgsBase(args);

  int i{1};
  while (i < argc) {
    if (parseGenericArgs(argc, argv, i) || parseDeprecatedArgs(argc, argv, i) ||
        isArg(i, argc, argv, nullptr, "client")) {
      ++i;
      continue;
    } else if (isArg(i, argc, argv, nullptr, "--camp") || isArg(i, argc, argv, nullptr, "--no-camp")) {
      // ignore -- included for backwards compatibility
    } else if (isArg(i, argc, argv, nullptr, "--invert-scroll")) {
      args.m_clientScrollDirection = deskflow::ClientScrollDirection::Inverted;
    } else {
      if (i + 1 == argc) {
        args.m_serverAddress = argv[i];
        return true;
      }

      LOG_CRIT("%s: unrecognized option `%s'" BYE, "deskflow-core", argv[i], "deskflow-core");
      return false;
    }
    ++i;
  }

  // exactly one non-option argument (server-address)
  if (i == argc && !args.m_shouldExitFail && !args.m_shouldExitOk) {
    LOG_CRIT("%s: a server address or name is required" BYE, "deskflow-core", "deskflow-core");
    return false;
  }

  if (checkUnexpectedArgs()) {
    return false;
  }

  return true;
}

bool ArgParser::parseGenericArgs(int argc, const char *const *argv, int &i) const
{
  if (isArg(i, argc, argv, "-h", "--help")) {
    if (m_app) {
      m_app->help();
    }
    argsBase().m_shouldExitOk = true;
  } else {
    // option not supported here
    return false;
  }

  return true;
}

bool ArgParser::parseDeprecatedArgs(int argc, const char *const *argv, int &i) const
{
  static const std::vector<const char *> deprecatedArgs = {"--crypto-pass", "--res-w",  "--res-h",
                                                           "--prm-wc",      "--prm-hc", "--log"};

  for (auto &arg : deprecatedArgs) {
    if (isArg(i, argc, argv, nullptr, arg)) {
      LOG_NOTE("%s is deprecated", arg);
      i++;
      return true;
    }
  }

  return false;
}

bool ArgParser::isArg(
    int argi, int argc, const char *const *argv, const char *name1, const char *name2, int minRequiredParameters
)
{
  if ((name1 != nullptr && strcmp(argv[argi], name1) == 0) || (name2 != nullptr && strcmp(argv[argi], name2) == 0)) {
    // match.  check args left.
    if (argi + minRequiredParameters >= argc) {
      LOG_PRINT("%s: missing arguments for `%s'" BYE, "deskflow-core", argv[argi], "deskflow-core");
      argsBase().m_shouldExitFail = true;
      return false;
    }
    return true;
  }

  // no match
  return false;
}

void ArgParser::splitCommandString(const std::string_view &command, std::vector<std::string> &argv)
{
  if (command.empty()) {
    return;
  }

  size_t leftDoubleQuote = 0;
  size_t rightDoubleQuote = 0;
  searchDoubleQuotes(command, leftDoubleQuote, rightDoubleQuote);

  size_t startPos = 0;
  size_t space = command.find(" ", startPos);

  while (space != std::string::npos) {
    bool ignoreThisSpace = false;

    // check if the space is between two double quotes
    if (space > leftDoubleQuote && space < rightDoubleQuote) {
      ignoreThisSpace = true;
    } else if (space > rightDoubleQuote) {
      searchDoubleQuotes(command, leftDoubleQuote, rightDoubleQuote, rightDoubleQuote + 1);
    }

    if (!ignoreThisSpace) {
      auto subString = command.substr(startPos, space - startPos);

      removeDoubleQuotes(subString);
      argv.emplace_back(subString);
    }

    // find next space
    if (ignoreThisSpace) {
      space = command.find(" ", rightDoubleQuote + 1);
    } else {
      startPos = space + 1;
      space = command.find(" ", startPos);
    }
  }

  auto subString = command.substr(startPos, command.size());
  removeDoubleQuotes(subString);
  argv.emplace_back(subString);
}

bool ArgParser::searchDoubleQuotes(const std::string_view &command, size_t &left, size_t &right, size_t startPos)
{
  bool result = false;
  left = std::string::npos;
  right = std::string::npos;

  left = command.find("\"", startPos);
  if (left != std::string::npos) {
    right = command.find("\"", left + 1);
    if (right != std::string::npos) {
      result = true;
    }
  }

  if (!result) {
    left = 0;
    right = 0;
  }

  return result;
}

void ArgParser::removeDoubleQuotes(std::string_view &arg)
{
  // if string is surrounded by double quotes, remove them
  if (arg[0] == '\"' && arg[arg.size() - 1] == '\"') {
    arg = arg.substr(1, arg.size() - 2);
  }
}

const char **ArgParser::getArgv(std::vector<std::string> &argsArray)
{
  size_t argc = argsArray.size();

  // caller is responsible for deleting the outer array only
  // we use the c string pointers from argsArray and assign
  // them to the inner array. So caller only need to use
  // delete[] to delete the outer array
  const auto **argv = new const char *[argc];

  for (size_t i = 0; i < argc; i++) {
    argv[i] = argsArray[i].c_str();
  }

  return argv;
}

std::string ArgParser::assembleCommand(
    std::vector<std::string> &argsArray, const std::string_view &ignoreArg, int parametersRequired
)
{
  std::string result;

  for (auto it = argsArray.begin(); it != argsArray.end(); ++it) {
    if (it->compare(ignoreArg) == 0) {
      it = it + parametersRequired;
      continue;
    }

    // if there is a space in this arg, use double quotes surround it
    if ((*it).find(" ") != std::string::npos) {
      (*it).insert(0, "\"");
      (*it).push_back('\"');
    }

    result.append(*it);
    // add space to saperate args
    result.append(" ");
  }

  if (!result.empty()) {
    // remove the tail space
    result = result.substr(0, result.size() - 1);
  }

  return result;
}

bool ArgParser::checkUnexpectedArgs() const
{
  return false;
}
