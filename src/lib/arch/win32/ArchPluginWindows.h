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

#pragma once

#include "arch/IArchPlugin.h"
#include "base/String.h"

#include <vector>

#define ARCH_PLUGIN ArchPluginWindows

class Screen;
class IEventQueue;

//! Windows implementation of IArchPlugin
class ArchPluginWindows : public IArchPlugin {
public:
	ArchPluginWindows();
	virtual ~ArchPluginWindows();

	// IArchPlugin overrides
	void				init(void* eventTarget, IEventQueue* events);

private:
	String				getModuleDir();
	void				getFilenames(const String& pattern, std::vector<String>& filenames);
	void				load(const String& dllPath);
	String				getPluginsDir();
};

void					sendEvent(const char* text, void* data);
void					log(const char* text);
