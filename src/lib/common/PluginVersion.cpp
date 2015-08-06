/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "PluginVersion.h"

#include <string.h>

static const char kUnknownVersion[] = "unknown";
const char* s_pluginNames[] = { "ns" };
static const char* s_pluginVersions[] = { "1.3" };

const char* getExpectedPluginVersion(const char* name)
{
	for (int i = 0; i < kPluginCount; i++) {
		if (strcmp(name, s_pluginNames[i]) == 0) {
			return s_pluginVersions[i];
			break;
		}
	}

	return kUnknownVersion;
}
