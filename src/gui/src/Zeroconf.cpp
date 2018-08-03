/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2018 Symless Ltd.
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

#include "Zeroconf.h"

#include "ZeroconfService.h"
#include "MainWindow.h"

Zeroconf::Zeroconf(MainWindow* mainWindow) :
    m_pMainWindow(mainWindow),
    m_pZeroconfService(nullptr)
{
}

Zeroconf::~Zeroconf()
{
    stopService();
}

void Zeroconf::startService()
{
    stopService();
    m_pZeroconfService = new ZeroconfService(m_pMainWindow);
}

void Zeroconf::stopService()
{
    if (m_pZeroconfService != nullptr) {
        delete m_pZeroconfService;
        m_pZeroconfService = nullptr;
    }
}
