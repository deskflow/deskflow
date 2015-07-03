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

#include "Plugin.h"

#include "CoreInterface.h"

static const char kBaseUrl[] = "http://synergy-project.org/files";
static const char kDefaultVersion[] = "1.1";
static const char kWinPackagePlatform32[] = "Windows-x86";
static const char kWinPackagePlatform64[] = "Windows-x64";
static const char kMacPackagePlatform[] = "MacOSX%1-i386";
static const char kLinuxPackagePlatformDeb32[] = "Linux-i686-deb";
static const char kLinuxPackagePlatformDeb64[] = "Linux-x86_64-deb";
static const char kLinuxPackagePlatformRpm32[] = "Linux-i686-rpm";
static const char kLinuxPackagePlatformRpm64[] = "Linux-x86_64-rpm";

#if defined(Q_OS_WIN)
static const char kWinPluginExt[] = ".dll";
static const char kInstallerPluginLocation[] = "Plugins";
#elif defined(Q_OS_MAC)
static const char kMacPluginPrefix[] = "lib";
static const char kMacPluginExt[] = ".dylib";
static const char kInstallerPluginLocation[] = "plugins"; // TODO: Fix for mac
#else
static const char kLinuxPluginPrefix[] = "lib";
static const char kLinuxPluginExt[] = ".so";
// /usr/bin becomes /usr/bin/../lib/syn...
static const char kInstallerPluginLocation[] = "../lib/synergy/plugins";
#endif

QString Plugin::getOsSpecificExt()
{

#if defined(Q_OS_WIN)
	return kWinPluginExt;
#elif defined(Q_OS_MAC)
	return kMacPluginExt;
#else
	return kLinuxPluginExt;
#endif
}

QString Plugin::getOsSpecificName(const QString& pluginName)
{
	QString result = pluginName;
#if defined(Q_OS_WIN)
	result.append(getOsSpecificExt());
#elif defined(Q_OS_MAC)
	result = kMacPluginPrefix + pluginName + getOsSpecificExt();
#else
	result = kLinuxPluginPrefix + pluginName + getOsSpecificExt();
#endif
	return result;
}

QString Plugin::getOsSpecificInstallerLocation() {
	return kInstallerPluginLocation;
}
