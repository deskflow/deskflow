/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, inc.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/ArgParser.h"

#include "synergy/App.h"
#include "synergy/ServerArgs.h"
#include "synergy/ClientArgs.h"
#include "synergy/ArgsBase.h"
#include "base/Log.h"

CArgsBase* CArgParser::m_argsBase = NULL;

CArgParser::CArgParser(CApp* app) :
	m_app(app)
{
}

bool
CArgParser::parseServerArgs(CServerArgs& args, int argc, const char* const* argv)
{
	setArgsBase(args);
	updateCommonArgs(argv);

	for (int i = 1; i < argc; ++i) {
		if (parsePlatformArg(args, argc, argv, i)) {
			continue;
		}
		else if (parseGenericArgs(argc, argv, i)) {
			continue;
		}
		else if (isArg(i, argc, argv, "-a", "--address", 1)) {
			// save listen address
			args.m_synergyAddress = argv[++i];
		}
		else if (isArg(i, argc, argv, "-c", "--config", 1)) {
			// save configuration file path
			args.m_configFile = argv[++i];
		}
		else {
			LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE, args.m_pname, argv[i], args.m_pname));
			return false;
		}
	}

	if (checkUnexpectedArgs()) {
		return false;
	}

	return true;
}

bool
CArgParser::parseClientArgs(CClientArgs& args, int argc, const char* const* argv)
{
	setArgsBase(args);
	updateCommonArgs(argv);

	int i;
	for (i = 1; i < argc; ++i) {
		if (parsePlatformArg(args, argc, argv, i)) {
			continue;
		}
		else if (parseGenericArgs(argc, argv, i)) {
			continue;
		}
		else if (isArg(i, argc, argv, NULL, "--camp")) {
			// ignore -- included for backwards compatibility
		}
		else if (isArg(i, argc, argv, NULL, "--no-camp")) {
			// ignore -- included for backwards compatibility
		}
		else if (isArg(i, argc, argv, NULL, "--yscroll", 1)) {
			// define scroll 
			args.m_yscroll = atoi(argv[++i]);
		}
		else {
			if (i + 1 == argc) {
				args.m_synergyAddress = argv[i];
				return true;
			}

			LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE, args.m_pname, argv[i], args.m_pname));
			return false;
		}
	}

	// exactly one non-option argument (server-address)
	if (i == argc) {
		LOG((CLOG_PRINT "%s: a server address or name is required" BYE,
			args.m_pname, args.m_pname));
		return false;
	}

	if (checkUnexpectedArgs()) {
		return false;
	}

	return true;
}

bool
CArgParser::parsePlatformArg(CArgsBase& argsBase, const int& argc, const char* const* argv, int& i)
{
#if WINAPI_MSWINDOWS
	if (isArg(i, argc, argv, NULL, "--service")) {
		LOG((CLOG_WARN "obsolete argument --service, use synergyd instead."));
		argsBase.m_shouldExit = true;
	}
	else if (isArg(i, argc, argv, NULL, "--exit-pause")) {
		argsBase.m_pauseOnExit = true;
	}
	else if (isArg(i, argc, argv, NULL, "--stop-on-desk-switch")) {
		argsBase.m_stopOnDeskSwitch = true;
	}
	else {
		// option not supported here
		return false;
	}

	return true;
#elif WINAPI_XWINDOWS
	if (CArgumentParser::isArg(i, argc, argv, "-display", "--display", 1)) {
		// use alternative display
		argsBase.m_display = argv[++i];
	}

	else if (CArgumentParser::isArg(i, argc, argv, NULL, "--no-xinitthreads")) {
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

bool
CArgParser::parseGenericArgs(int argc, const char* const* argv, int& i)
{
	if (isArg(i, argc, argv, "-d", "--debug", 1)) {
		// change logging level
		argsBase().m_logFilter = argv[++i];
	}
	else if (isArg(i, argc, argv, "-l", "--log", 1)) {
		argsBase().m_logFile = argv[++i];
	}
	else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
		// not a daemon
		argsBase().m_daemon = false;
	}
	else if (isArg(i, argc, argv, NULL, "--daemon")) {
		// daemonize
		argsBase().m_daemon = true;
	}
	else if (isArg(i, argc, argv, "-n", "--name", 1)) {
		// save screen name
		argsBase().m_name = argv[++i];
	}
	else if (isArg(i, argc, argv, "-1", "--no-restart")) {
		// don't try to restart
		argsBase().m_restartable = false;
	}
	else if (isArg(i, argc, argv, NULL, "--restart")) {
		// try to restart
		argsBase().m_restartable = true;
	}
	else if (isArg(i, argc, argv, "-z", NULL)) {
		argsBase().m_backend = true;
	}
	else if (isArg(i, argc, argv, NULL, "--no-hooks")) {
		argsBase().m_noHooks = true;
	}
	else if (isArg(i, argc, argv, "-h", "--help")) {
		if (m_app) {
			m_app->help();
		}
		argsBase().m_shouldExit = true;
	}
	else if (isArg(i, argc, argv, NULL, "--version")) {
		if (m_app) {
			m_app->version();
		}
		argsBase().m_shouldExit = true;
	}
	else if (isArg(i, argc, argv, NULL, "--no-tray")) {
		argsBase().m_disableTray = true;
	}
	else if (isArg(i, argc, argv, NULL, "--ipc")) {
		argsBase().m_enableIpc = true;
	}
	else if (isArg(i, argc, argv, NULL, "--server")) {
		// HACK: stop error happening when using portable (synergyp) 
	}
	else if (isArg(i, argc, argv, NULL, "--client")) {
		// HACK: stop error happening when using portable (synergyp) 
	}
	else if (isArg(i, argc, argv, NULL, "--crypto-pass")) {
		argsBase().m_crypto.m_pass = argv[++i];
		argsBase().m_crypto.setMode("cfb");
	}
	else if (isArg(i, argc, argv, NULL, "--enable-drag-drop")) {
		bool useDragDrop = true;

#ifdef WINAPI_XWINDOWS

		useDragDrop = false;
		LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported on linux."));

#endif

#ifdef WINAPI_MSWINDOWS

		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);

		if (osvi.dwMajorVersion < 6) {
			useDragDrop = false;
			LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported below vista."));
		}
#endif

		if (useDragDrop) {
			argsBase().m_enableDragDrop = true;
		}
	}
	else {
		// option not supported here
		return false;
	}

	return true;
}

bool
CArgParser::isArg(
	int argi, int argc, const char* const* argv,
	const char* name1, const char* name2,
	int minRequiredParameters)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
			// match.  check args left.
			if (argi + minRequiredParameters >= argc) {
				LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
					argsBase().m_pname, argv[argi], argsBase().m_pname));
				argsBase().m_shouldExit = true;
				return false;
			}
			return true;
	}

	// no match
	return false;
}

void
CArgParser::splitCommandString(CString& command, std::vector<CString>& argv)
{
	if (command.empty()) {
		return ;
	}

	size_t leftDoubleQuote = 0;
	size_t rightDoubleQuote = 0;
	searchDoubleQuotes(command, leftDoubleQuote, rightDoubleQuote);

	size_t startPos = 0;
	size_t space = command.find(" ", startPos);

	while (space != CString::npos) {
		bool ignoreThisSpace = false;

		// check if the space is between two double quotes
		if (space > leftDoubleQuote && space < rightDoubleQuote) {
			ignoreThisSpace = true;
		}
		else if (space > rightDoubleQuote){
			searchDoubleQuotes(command, leftDoubleQuote, rightDoubleQuote, rightDoubleQuote + 1);
		}
		
		if (!ignoreThisSpace) {
			CString subString = command.substr(startPos, space - startPos);

			removeDoubleQuotes(subString);
			argv.push_back(subString);
		}

		// find next space
		if (ignoreThisSpace) {
			space = command.find(" ", rightDoubleQuote + 1);
		}
		else {
			startPos = space + 1;
			space = command.find(" ", startPos);
		}
	}

	CString subString = command.substr(startPos, command.size());
	removeDoubleQuotes(subString);
	argv.push_back(subString);
}

bool
CArgParser::searchDoubleQuotes(CString& command, size_t& left, size_t& right, size_t startPos)
{
	bool result = false;
	left = CString::npos;
	right = CString::npos;

	left = command.find("\"", startPos);
	if (left != CString::npos) {
		right = command.find("\"", left + 1);
		if (right != CString::npos) {
			result = true;
		}
	}

	if (!result) {
		left = 0;
		right = 0;
	}

	return result;
}

void
CArgParser::removeDoubleQuotes(CString& arg)
{
	// if string is surrounded by double quotes, remove them
	if (arg[0] == '\"' &&
		arg[arg.size() - 1] == '\"') {
		arg = arg.substr(1, arg.size() - 2);
	}
}

const char**
CArgParser::getArgv(std::vector<CString>& argsArray)
{
	size_t argc = argsArray.size();

	// caller is responsible for deleting the outer array only
	// we use the c string pointers from argsArray and assign
	// them to the inner array. So caller only need to use
	// delete[] to delete the outer array
	const char** argv = new const char*[argc];
	
	for (size_t i = 0; i < argc; i++) {
		argv[i] = argsArray[i].c_str();
	}

	return argv;
}

CString
CArgParser::assembleCommand(std::vector<CString>& argsArray,  CString ignoreArg, int parametersRequired)
{
	CString result;

	for (std::vector<CString>::iterator it = argsArray.begin(); it != argsArray.end(); ++it) {
		if (it->compare(ignoreArg) == 0) {
			it = it + parametersRequired;
			continue;
		}

		// if there is a space in this arg, use double quotes surround it
		if ((*it).find(" ") != CString::npos) {
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

void
CArgParser::updateCommonArgs(const char* const* argv)
{
	argsBase().m_name = ARCH->getHostName();
	argsBase().m_pname = ARCH->getBasename(argv[0]);
}

bool
CArgParser::checkUnexpectedArgs()
{
#if SYSAPI_WIN32
	// suggest that user installs as a windows service. when launched as 
	// service, process should automatically detect that it should run in
	// daemon mode.
	if (argsBase().m_daemon) {
		LOG((CLOG_ERR 
			"the --daemon argument is not supported on windows. "
			"instead, install %s as a service (--service install)", 
			argsBase().m_pname));
		return true;
	}
#endif

	return false;
}
