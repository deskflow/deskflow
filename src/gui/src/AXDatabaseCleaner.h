/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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

#include <AvailabilityInternal.h>

// HACK: ideally this file should not be included in project,
// if it is below marvericks, but it seems that .pro can't
// specify mac version
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

class AXDatabaseCleaner {
public:
	AXDatabaseCleaner();
	~AXDatabaseCleaner();

	bool loadPrivilegeHelper();
	bool xpcConnect();
	bool privilegeCommand(const char* command);

private:
	class Private;
	Private* m_private;
	bool m_waitForResponse;
};

#endif
