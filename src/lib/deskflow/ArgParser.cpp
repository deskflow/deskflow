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
#include "deskflow/ServerArgs.h"

#ifdef WINAPI_MSWINDOWS
#include <VersionHelpers.h>
#endif

deskflow::ArgsBase *ArgParser::m_argsBase = nullptr;

ArgParser::ArgParser(App *app) : m_app(app)
{
}

bool ArgParser::parseServerArgs(deskflow::ServerArgs &args, int argc, const char *const *argv)
{
  setArgsBase(args);
  updateCommonArgs(argv);
  int i = 1;
  while (i < argc) {
    if (parsePlatformArgs(args, argc, argv, i, true)) {
      ++i;
      continue;
    } else if (parseGenericArgs(argc, argv, i)) {
      ++i;
      continue;
    } else if (parseDeprecatedArgs(argc, argv, i)) {
      ++i;
      continue;
    } else if (isArg(i, argc, argv, "-c", "--config", 1)) {
      // save configuration file path
      args.m_configFile = argv[++i];
    } else if (isArg(i, argc, argv, nullptr, "server")) {
      ++i;
      continue;
    } else if (isArg(i, argc, argv, nullptr, "--disable-client-cert-check")) {
      args.m_chkPeerCert = false;
    } else {
      LOG((CLOG_CRIT "%s: unrecognized option `%s'" BYE, args.m_pname, argv[i], args.m_pname));
      return false;
    }
    ++i;
  }

  if (checkUnexpectedArgs()) {
    return false;
  }

  return true;
}

bool ArgParser::parseClientArgs(deskflow::ClientArgs &args, int argc, const char *const *argv)
{
  setArgsBase(args);
  updateCommonArgs(argv);

  int i{1};
  while (i < argc) {
    if (parsePlatformArgs(args, argc, argv, i, false)) {
      ++i;
      continue;
    } else if (parseGenericArgs(argc, argv, i)) {
      ++i;
      continue;
    } else if (parseDeprecatedArgs(argc, argv, i)) {
      ++i;
      continue;
    } else if (isArg(i, argc, argv, nullptr, "--camp")) {
      // ignore -- included for backwards compatibility
    } else if (isArg(i, argc, argv, nullptr, "--no-camp")) {
      // ignore -- included for backwards compatibility
    } else if (isArg(i, argc, argv, nullptr, "--yscroll", 1)) {
      // define scroll
      args.m_yscroll = atoi(argv[++i]);
    } else if (isArg(i, argc, argv, nullptr, "--sync-language")) {
      args.m_enableLangSync = true;
    } else if (isArg(i, argc, argv, nullptr, "--invert-scroll")) {
      args.m_clientScrollDirection = deskflow::ClientScrollDirection::INVERT_SERVER;
    } else if (isArg(i, argc, argv, nullptr, "--host")) {
      args.m_hostMode = true;
    } else if (isArg(i, argc, argv, nullptr, "client")) {
      ++i;
      continue;
    } else {
      if (i + 1 == argc) {
        args.m_serverAddress = argv[i];
        return true;
      }

      LOG((CLOG_CRIT "%s: unrecognized option `%s'" BYE, args.m_pname, argv[i], args.m_pname));
      return false;
    }
    ++i;
  }

  // exactly one non-option argument (server-address)
  if (i == argc && !args.m_shouldExitFail && !args.m_shouldExitOk) {
    LOG((CLOG_CRIT "%s: a server address or name is required" BYE, args.m_pname, args.m_pname));
    return false;
  }

  if (checkUnexpectedArgs()) {
    return false;
  }

  return true;
}

bool ArgParser::parsePlatformArgs(
    deskflow::ArgsBase &argsBase, const int &argc, const char *const *argv, int &i, bool isServer
)
{
#if WINAPI_MSWINDOWS
  if (isArg(i, argc, argv, nullptr, "--stop-on-desk-switch")) {
    argsBase.m_stopOnDeskSwitch = true;
  } else {
    // option not supported here
    return false;
  }

  return true;
#elif WINAPI_XWINDOWS

  if (isArg(i, argc, argv, "-display", "--display", 1)) {
    // use alternative display
    argsBase.m_display = argv[++i];
  }

  else if (isArg(i, argc, argv, nullptr, "--no-xinitthreads")) {
    argsBase.m_disableXInitThreads = true;
  }

  else {
    // option not supported here
    return false;
  }

  return true;
#elif WINAPI_CARBON
  // no options for carbon
  return false;
#endif
}

bool ArgParser::parseGenericArgs(int argc, const char *const *argv, int &i)
{
  if (isArg(i, argc, argv, "-a", "--address", 1)) {
    argsBase().m_deskflowAddress = argv[++i];
  } else if (isArg(i, argc, argv, "-d", "--debug", 1)) {
    // change logging level
    argsBase().m_logFilter = argv[++i];
  } else if (isArg(i, argc, argv, "-l", "--log", 1)) {
    argsBase().m_logFile = argv[++i];
  } else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
    // not a daemon
    argsBase().m_daemon = false;
  } else if (isArg(i, argc, argv, nullptr, "--daemon")) {
    // daemonize
    argsBase().m_daemon = true;
  } else if (isArg(i, argc, argv, "-n", "--name", 1)) {
    // save screen name
    argsBase().m_name = argv[++i];
  } else if (isArg(i, argc, argv, "-1", "--no-restart")) {
    // don't try to restart
    argsBase().m_restartable = false;
  } else if (isArg(i, argc, argv, nullptr, "--restart")) {
    // try to restart
    argsBase().m_restartable = true;
  } else if (isArg(i, argc, argv, nullptr, "--no-hooks")) {
    argsBase().m_noHooks = true;
  } else if (isArg(i, argc, argv, "-h", "--help")) {
    if (m_app) {
      m_app->help();
    }
    argsBase().m_shouldExitOk = true;
  } else if (isArg(i, argc, argv, nullptr, "--version")) {
    if (m_app) {
      m_app->version();
    }
    argsBase().m_shouldExitOk = true;
  } else if (isArg(i, argc, argv, nullptr, "--server")) {
    // HACK: stop error happening when using portable (deskflowp)
  } else if (isArg(i, argc, argv, nullptr, "--client")) {
    // HACK: stop error happening when using portable (deskflowp)
  } else if (isArg(i, argc, argv, nullptr, "--enable-drag-drop")) {
    bool useDragDrop = true;

#ifdef WINAPI_XWINDOWS

    useDragDrop = false;
    LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported on linux."));

#endif

#ifdef WINAPI_MSWINDOWS

    if (!IsWindowsVistaOrGreater()) {
      useDragDrop = false;
      LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported below vista."));
    }
#endif

    if (useDragDrop) {
      argsBase().m_enableDragDrop = true;
    }
  } else if (isArg(i, argc, argv, nullptr, "--enable-crypto")) {
    argsBase().m_enableCrypto = true;
  } else if (isArg(i, argc, argv, nullptr, "--profile-dir", 1)) {
    argsBase().m_profileDirectory = argv[++i];
  } else if (isArg(i, argc, argv, nullptr, "--plugin-dir", 1)) {
    argsBase().m_pluginDirectory = argv[++i];
  } else if (isArg(i, argc, argv, nullptr, "--tls-cert", 1)) {
    argsBase().m_tlsCertFile = argv[++i];
  } else if (isArg(i, argc, argv, nullptr, "--prevent-sleep")) {
    argsBase().m_preventSleep = true;
  } else {
    // option not supported here
    return false;
  }

  return true;
}

bool ArgParser::parseDeprecatedArgs(int argc, const char *const *argv, int &i)
{
  static const std::vector<const char *> deprecatedArgs = {
      "--crypto-pass", "--res-w", "--res-h", "--prm-wc", "--prm-hc"
  };

  for (auto &arg : deprecatedArgs) {
    if (isArg(i, argc, argv, nullptr, arg)) {
      LOG((CLOG_NOTE "%s is deprecated", arg));
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
  const auto match1 = (name1 != nullptr && strcmp(argv[argi], name1) == 0);
  const auto match2 = (name2 != nullptr && strcmp(argv[argi], name2) == 0);
  if (match1 || match2) {
    // match.  check args left.
    if (argi + minRequiredParameters >= argc) {
      LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE, argsBase().m_pname, argv[argi], argsBase().m_pname));
      argsBase().m_shouldExitFail = true;
      return false;
    }
    return true;
  }

  // no match
  return false;
}

void ArgParser::splitCommandString(std::string &command, std::vector<std::string> &argv)
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
      std::string subString = command.substr(startPos, space - startPos);

      removeDoubleQuotes(subString);
      argv.push_back(subString);
    }

    // find next space
    if (ignoreThisSpace) {
      space = command.find(" ", rightDoubleQuote + 1);
    } else {
      startPos = space + 1;
      space = command.find(" ", startPos);
    }
  }

  std::string subString = command.substr(startPos, command.size());
  removeDoubleQuotes(subString);
  argv.push_back(subString);
}

bool ArgParser::searchDoubleQuotes(std::string &command, size_t &left, size_t &right, size_t startPos)
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

void ArgParser::removeDoubleQuotes(std::string &arg)
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
  const char **argv = new const char *[argc];

  for (size_t i = 0; i < argc; i++) {
    argv[i] = argsArray[i].c_str();
  }

  return argv;
}

std::string
ArgParser::assembleCommand(std::vector<std::string> &argsArray, std::string ignoreArg, int parametersRequired)
{
  std::string result;

  for (std::vector<std::string>::iterator it = argsArray.begin(); it != argsArray.end(); ++it) {
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

void ArgParser::updateCommonArgs(const char *const *argv)
{
  argsBase().m_name = ARCH->getHostName();
  argsBase().m_pname = ARCH->getBasename(argv[0]);
}

bool ArgParser::checkUnexpectedArgs()
{
#if SYSAPI_WIN32
  // suggest that user installs as a windows service. when launched as
  // service, process should automatically detect that it should run in
  // daemon mode.
  if (argsBase().m_daemon) {
    LOG(
        (CLOG_ERR "the --daemon argument is not supported on windows. "
                  "instead, install %s as a service (--service install)",
         argsBase().m_pname)
    );
    return true;
  }
#endif

  return false;
}
