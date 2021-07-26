/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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
#include "XWindowsPowerManager.h"
#include "arch/Arch.h"
#include "base/Log.h"

namespace{

bool sleepInhibitCall(bool state, ArchSystemUnix::InhibitScreenServices serviceID)
{
    std::string error;

    if (!ArchSystemUnix::DBusInhibitScreenCall(serviceID, state, error))
    {
        LOG((CLOG_DEBUG "DBus inhibit error %s", error.c_str()));
        return false;
    }

    return true;
}

}

XWindowsPowerManager::~XWindowsPowerManager()
{
    enableSleep();
}

void XWindowsPowerManager::disableSleep() const
{
    if (!sleepInhibitCall(true, ArchSystemUnix::InhibitScreenServices::kScreenSaver) &&
        !sleepInhibitCall(true, ArchSystemUnix::InhibitScreenServices::kSessionManager))
    {
        LOG((CLOG_INFO "Failed to prevent system from going to sleep"));
    }
}

void XWindowsPowerManager::enableSleep() const
{
    if (!sleepInhibitCall(false, ArchSystemUnix::InhibitScreenServices::kScreenSaver) &&
        !sleepInhibitCall(false, ArchSystemUnix::InhibitScreenServices::kSessionManager))
    {
        LOG((CLOG_INFO "Failed to enable system idle sleep"));
    }
}
