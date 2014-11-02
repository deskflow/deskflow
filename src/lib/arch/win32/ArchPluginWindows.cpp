/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "arch/win32/ArchPluginWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/Event.h"
#include "synergy/Screen.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>

typedef int (*initFunc)(void (*sendEvent)(const char*, void*), void (*log)(const char*));

void* g_eventTarget = NULL;
IEventQueue* g_events = NULL;

ArchPluginWindows::ArchPluginWindows()
{
}

ArchPluginWindows::~ArchPluginWindows()
{
}

void
ArchPluginWindows::init(void* eventTarget, IEventQueue* events)
{
	g_eventTarget = eventTarget;
	g_events = events;
	
	String dir = getPluginsDir();
	LOG((CLOG_DEBUG "plugins dir: %s", dir.c_str()));

	String pattern = String(dir).append("\\*.dll");
	std::vector<String> plugins;
	getFilenames(pattern, plugins);

	std::vector<String>::iterator it;
	for (it = plugins.begin(); it != plugins.end(); ++it)
		load(*it);
}

void
ArchPluginWindows::load(const String& dllFilename)
{
	LOG((CLOG_DEBUG "loading plugin: %s", dllFilename.c_str()));
	String path = String(getPluginsDir()).append("\\").append(dllFilename);
	HINSTANCE library = LoadLibrary(path.c_str());
	if (library == NULL)
		throw XArch(new XArchEvalWindows);

	initFunc initPlugin = (initFunc)GetProcAddress(library, "init");
	initPlugin(&sendEvent, &log);
}

String
ArchPluginWindows::getModuleDir()
{
	TCHAR c_modulePath[MAX_PATH];
	if (GetModuleFileName(NULL, c_modulePath, MAX_PATH) == 0) {
		throw XArch(new XArchEvalWindows);
	}

	String modulePath(c_modulePath);
	size_t lastSlash = modulePath.find_last_of("\\");

	if (lastSlash != String::npos) {
		return modulePath.substr(0, lastSlash);
	}

	throw XArch("could not get module path.");
}

void
ArchPluginWindows::getFilenames(const String& pattern, std::vector<String>& filenames)
{
	WIN32_FIND_DATA data;
	HANDLE find = FindFirstFile(pattern.c_str(), &data);
	if (find == INVALID_HANDLE_VALUE) {
		FindClose(find);
		LOG((CLOG_DEBUG "plugins dir is empty: %s", pattern.c_str()));
		return;
	}

	do {
		filenames.push_back(data.cFileName);
	} while (FindNextFile(find, &data));

	FindClose(find);
}

String ArchPluginWindows::getPluginsDir()
{
	return getModuleDir().append("\\").append(PLUGINS_DIR);
}

void
sendEvent(const char* eventName, void* data)
{
	LOG((CLOG_DEBUG5 "plugin sending event"));
	Event::Type type = g_events->getRegisteredType(eventName);
	g_events->addEvent(Event(type, g_eventTarget, data));
}

void
log(const char* text)
{
	LOG((CLOG_DEBUG "plugin: %s", text)); 
}
