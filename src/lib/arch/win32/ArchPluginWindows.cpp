/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "arch/win32/ArchPluginWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/Event.h"
#include "synergy/Screen.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>

typedef void (*initFunc)(void*, void*);
typedef int (*initEventFunc)(void (*sendEvent)(const char*, void*));
typedef void* (*invokeFunc)(const char*, void**);
typedef void (*cleanupFunc)();

void* g_eventTarget = NULL;
IEventQueue* g_events = NULL;

ArchPluginWindows::ArchPluginWindows()
{
}

ArchPluginWindows::~ArchPluginWindows()
{
}

void
ArchPluginWindows::load()
{
	String dir = getPluginsDir();
	LOG((CLOG_DEBUG "plugins dir: %s", dir.c_str()));

	String pattern = String(dir).append("\\*.dll");
	std::vector<String> plugins;
	getFilenames(pattern, plugins);

	std::vector<String>::iterator it;
	for (it = plugins.begin(); it != plugins.end(); ++it) {
		LOG((CLOG_DEBUG "loading plugin: %s", (*it).c_str()));
		String path = String(dir).append("\\").append(*it);
		HINSTANCE library = LoadLibrary(path.c_str());

		if (library == NULL) {
			LOG((CLOG_ERR "failed to load plugin: %s %d", (*it).c_str(), GetLastError()));
			throw XArch(new XArchEvalWindows);
		}

		void* lib = reinterpret_cast<void*>(library);
		String filename = synergy::string::removeFileExt(*it);
		m_pluginTable.insert(std::make_pair(filename, lib));

		LOG((CLOG_DEBUG "loaded plugin: %s", (*it).c_str()));
	}
}

void
ArchPluginWindows::unload()
{
	PluginTable::iterator it;
	HINSTANCE lib;
	for (it = m_pluginTable.begin(); it != m_pluginTable.end(); it++) {
		lib = reinterpret_cast<HINSTANCE>(it->second);
		cleanupFunc cleanup = (cleanupFunc)GetProcAddress(lib, "cleanup");
		if (cleanup != NULL) {
			cleanup();
		}
		else {
			LOG((CLOG_DEBUG "no cleanup function in %s", it->first.c_str()));
		}

		LOG((CLOG_DEBUG "unloading plugin: %s", it->first.c_str()));
		FreeLibrary(lib);
	}
}

void
ArchPluginWindows::init(void* log, void* arch)
{
	PluginTable::iterator it;
	HINSTANCE lib;
	for (it = m_pluginTable.begin(); it != m_pluginTable.end(); it++) {
		lib = reinterpret_cast<HINSTANCE>(it->second);
		initFunc initPlugin = (initFunc)GetProcAddress(lib, "init");
		if (initPlugin != NULL) {
			initPlugin(log, arch);
		}
		else {
			LOG((CLOG_DEBUG "no init function in %s", it->first.c_str()));
		}
	}
}

void
ArchPluginWindows::initEvent(void* eventTarget, IEventQueue* events)
{
	g_eventTarget = eventTarget;
	g_events = events;

	PluginTable::iterator it;
	HINSTANCE lib;
	for (it = m_pluginTable.begin(); it != m_pluginTable.end(); it++) {
		lib = reinterpret_cast<HINSTANCE>(it->second);
		initEventFunc initEventPlugin = (initEventFunc)GetProcAddress(lib, "initEvent");
		if (initEventPlugin != NULL) {
			initEventPlugin(&sendEvent);
		}
		else {
			LOG((CLOG_DEBUG "no init event function in %s", it->first.c_str()));
		}
	}
}


bool
ArchPluginWindows::exists(const char* name)
{
	PluginTable::iterator it;
	it = m_pluginTable.find(name);
	return it != m_pluginTable.end() ? true : false;
}

void*
ArchPluginWindows::invoke(
	const char* plugin,
	const char* command,
	void** args)
{
	PluginTable::iterator it;
	it = m_pluginTable.find(plugin);
	if (it != m_pluginTable.end()) {
		HINSTANCE lib = reinterpret_cast<HINSTANCE>(it->second);
		invokeFunc invokePlugin = (invokeFunc)GetProcAddress(lib, "invoke");
		void* result = NULL;
		if (invokePlugin != NULL) {
			 result = invokePlugin(command, args);
		}
		else {
			LOG((CLOG_DEBUG "no invoke function in %s", it->first.c_str()));
		}

		return result;
	}
	else {
		LOG((CLOG_DEBUG "invoke command failed, plugin: %s command: %s",
				plugin, command));
		return NULL;
	}
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
	return ARCH->getPluginDirectory();
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
